#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define LOG_CLI 1
#include "flog.h"
#include "fmem.h"
#include "http.h"
#include "fstr.h"
#include "fmap.h"

/* TODO.......complete protocol */

//**************** constants ****************/
#define HTTP_BUFSIZE 4096
#define HTTP_ITEMBUFSIZE 2048
#define HTTP_START_LINE_MAX 8000

/**************** type function *****************/
static inline void * _int_parse_func(char * val) {
    return (void *)atoi(val);
}

static inline void _header_item_free(void *item) {
    if (item == NULL) return;
    ffree(((struct http_header_item *) item)->val);
    ffree(item);
}

static inline void _header_item_dup(struct fmap_node *n, void *item) {
    _header_item_free(n->value);

    void *item_target = fmalloc(sizeof(struct http_header_item));
    fmemcpy(item_target, item, sizeof(struct http_header_item));
    n->value = item_target;
}

/****** global variables ******/
/*    request line
    GET /search HTTP/1.1
    Request Header
    Accept	指定客户端能够接收的内容类型	Accept: text/plain, text/html
    Accept-Charset	浏览器可以接受的字符编码集。	Accept-Charset: iso-8859-5
    Accept-Encoding	指定浏览器可以支持的web服务器返回内容压缩编码类型。	Accept-Encoding: compress, gzip
    Accept-Language	浏览器可接受的语言	Accept-Language: en,zh
    Accept-Ranges	可以请求网页实体的一个或者多个子范围字段	Accept-Ranges: bytes
    Authorization	HTTP授权的授权证书	Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
    Cache-Control	指定请求和响应遵循的缓存机制	Cache-Control: no-cache
    Connection	表示是否需要持久连接。（HTTP 1.1默认进行持久连接）	Connection: close
    Cookie	HTTP请求发送时，会把保存在该请求域名下的所有cookie值一起发送给web服务器。	Cookie: $Version=1; Skin=new;
    Content-Length	请求的内容长度	Content-Length: 348
    Content-Type	请求的与实体对应的MIME信息	Content-Type: application/x-www-form-urlencoded
    Date	请求发送的日期和时间	Date: Tue, 15 Nov 2010 08:12:31 GMT
    Expect	请求的特定的服务器行为	Expect: 100-continue
    From	发出请求的用户的Email	From: user@email.com
    Host	指定请求的服务器的域名和端口号	Host: www.zcmhi.com
    If-Match	只有请求内容与实体相匹配才有效	If-Match: “737060cd8c284d8af7ad3082f209582d”
    If-Modified-Since	如果请求的部分在指定时间之后被修改则请求成功，未被修改则返回304代码	If-Modified-Since: Sat, 29 Oct 2010 19:43:31 GMT
    If-None-Match	如果内容未改变返回304代码，参数为服务器先前发送的Etag，与服务器回应的Etag比较判断是否改变	If-None-Match: “737060cd8c284d8af7ad3082f209582d”
    If-Range	如果实体未改变，服务器发送客户端丢失的部分，否则发送整个实体。参数也为Etag	If-Range: “737060cd8c284d8af7ad3082f209582d”
    If-Unmodified-Since	只在实体在指定时间之后未被修改才请求成功	If-Unmodified-Since: Sat, 29 Oct 2010 19:43:31 GMT
    Max-Forwards	限制信息通过代理和网关传送的时间	Max-Forwards: 10
    Pragma	用来包含实现特定的指令	Pragma: no-cache
    Proxy-Authorization	连接到代理的授权证书	Proxy-Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
    Range	只请求实体的一部分，指定范围	Range: bytes=500-999
    Referer	先前网页的地址，当前请求网页紧随其后,即来路	Referer: http://www.zcmhi.com/archives/71.html
    TE	客户端愿意接受的传输编码，并通知服务器接受接受尾加头信息	TE: trailers,deflate;q=0.5
    Upgrade	向服务器指定某种传输协议以便服务器进行转换（如果支持）	Upgrade: HTTP/2.0, SHTTP/1.3, IRC/6.9, RTA/x11
    User-Agent	User-Agent的内容包含发出请求的用户信息	User-Agent: Mozilla/5.0 (Linux; X11)
    Via	通知中间网关或代理服务器地址，通信协议	Via: 1.0 fred, 1.1 nowhere.com (Apache/1.1)
    Warning 关于消息实体的警告信息	Warn: 199 Miscellaneous warning
=================================================================================
    response line
    HTTP/1.1 200 OK
    Response Headers
    Accept-Ranges	表明服务器是否支持指定范围请求及哪种类型的分段请求	Accept-Ranges: bytes
    Age	从原始服务器到代理缓存形成的估算时间（以秒计，非负）	Age: 12
    Allow	对某网络资源的有效的请求行为，不允许则返回405	Allow: GET, HEAD
    Cache-Control	告诉所有的缓存机制是否可以缓存及哪种类型	Cache-Control: no-cache
    Content-Encoding	web服务器支持的返回内容压缩编码类型。	Content-Encoding: gzip
    Content-Language	响应体的语言	Content-Language: en,zh
    Content-Length	响应体的长度	Content-Length: 348
    Content-Location	请求资源可替代的备用的另一地址	Content-Location: /index.htm
    Content-MD5	返回资源的MD5校验值	Content-MD5: Q2hlY2sgSW50ZWdyaXR5IQ==
    Content-Range	在整个返回体中本部分的字节位置	Content-Range: bytes 21010-47021/47022
    Content-Type	返回内容的MIME类型	Content-Type: text/html; charset=utf-8
    Date	原始服务器消息发出的时间	Date: Tue, 15 Nov 2010 08:12:31 GMT
    ETag	请求变量的实体标签的当前值	ETag: “737060cd8c284d8af7ad3082f209582d”
    Expires	响应过期的日期和时间	Expires: Thu, 01 Dec 2010 16:00:00 GMT
    Last-Modified	请求资源的最后修改时间	Last-Modified: Tue, 15 Nov 2010 12:45:26 GMT
    Location	用来重定向接收方到非请求URL的位置来完成请求或标识新的资源	Location: http://www.zcmhi.com/archives/94.html
    Pragma	包括实现特定的指令，它可应用到响应链上的任何接收方	Pragma: no-cache
    Proxy-Authenticate	它指出认证方案和可应用到代理的该URL上的参数	Proxy-Authenticate: Basic
    refresh	应用于重定向或一个新的资源被创造，在5秒之后重定向（由网景提出，被大部分浏览器支持）
    Refresh: 5; url=http://www.zcmhi.com/archives/94.html
    Retry-After	如果实体暂时不可取，通知客户端在指定时间之后再次尝试	Retry-After: 120
    Server	web服务器软件名称	Server: Apache/1.3.27 (Unix) (Red-Hat/Linux)
    Set-Cookie	设置Http Cookie	Set-Cookie: UserID=JohnDoe; Max-Age=3600; Version=1
    Trailer	指出头域在分块传输编码的尾部存在	Trailer: Max-Forwards
    Transfer-Encoding	文件传输编码	Transfer-Encoding:chunked
    Vary	告诉下游代理是使用缓存响应还是从原始服务器请求	Vary: *
    Via	告知代理客户端响应是通过哪里发送的	Via: 1.0 fred, 1.1 nowhere.com (Apache/1.1)
    Warning	警告实体可能存在的问题	Warning: 199 Miscellaneous warning
    WWW-Authenticate	表明客户端请求实体应该使用的授权方案	WWW-Authenticate: Basic
*/
struct fmap *defined_header_map;
struct http_header_item defined_headers[] = {
        {"Accept",              NULL, 0x10, NULL},
        {"Accept-Charset",      NULL, 0x10, NULL},
        {"Accept-Encoding",     NULL, 0x10, NULL},
        {"Accept-Language",     NULL, 0x10, NULL},
        {"Accept-Ranges",       NULL, 0x10, NULL},
        {"Authorization",       NULL, 0x10, NULL},
        {"Cache-Control",       NULL, 0x10, NULL},
        {"Connection",          NULL, 0x10, NULL},
        {"Cookie",              NULL, 0x10, NULL},
        {"Content-Length",      NULL, 0x11, _int_parse_func},
        {"Content-Type",        NULL, 0x11, NULL},
        {"Date",                NULL, 0x10, NULL},
        {"Expect",              NULL, 0x10, NULL},
        {"From",                NULL, 0x10, NULL},
        {"Host",                NULL, 0x10, NULL},
        {"If-Match",            NULL, 0x10, NULL},
        {"If-Modified-Since",   NULL, 0x10, NULL},
        {"If-None-Match",       NULL, 0x10, NULL},
        {"If-Range",            NULL, 0x10, NULL},
        {"If-Unmodified-Since", NULL, 0x10, NULL},
        {"Max-Forwards",        NULL, 0x10, NULL},
        {"Pragma",              NULL, 0x10, NULL},
        {"Proxy-Authorization", NULL, 0x10, NULL},
        {"Range",               NULL, 0x10, NULL},
        {"Referer",             NULL, 0x10, NULL},
        {"TE",                  NULL, 0x10, NULL},
        {"Upgrade",             NULL, 0x10, NULL},
        {"User-Agent",          NULL, 0x10, NULL},
        {"Via",                 NULL, 0x10, NULL},
        {"Warning",             NULL, 0x10, NULL}
};

struct fmap *defined_status_map;
struct http_status defined_status[] = {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {102, "Processing"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {207, "Multi-Status"},
        {208, "Already Reported"},
        {226, "IM Used"},
        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {306, "(Unused)"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Payload Too Large"},
        {414, "URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {425, "Unassigned"},
        {426, "Upgrade Required"},
        {427, "Unassigned"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {430, "Unassigned"},
        {431, "Request Header Fields Too Large"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {509, "Unassigned"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"}
};

/****** inner function list ******/
static struct http_status *_defined_status_get(int status);

static char *_parse_request_line(char *buf, struct http_start_line *header_line);

static char *_parse_header(char *buf, struct fmap *out_map);

static inline void _header_map_add(struct fmap *header_map, char *key, char *val);

hash_type_t http_header_hash_type = {
        str_hash_func, str_cmp_func,
        str_dup_func, NULL,
        str_free_func, NULL
};

void http_init() {
    if (NULL == (defined_status_map = fmap_create_int())) {
        fatal("http status map init faild");
    }
    if (NULL == (defined_header_map = fmap_create())) {
        fatal("http header map init faild");
    }

    int len = sizeof(defined_status) / sizeof(defined_status[0]);
    for (int i = 0; i < len; ++i) {
        fmap_add(defined_status_map, (void *) defined_status[i].status, &defined_status[i]);
    }

    len = sizeof(defined_headers) / sizeof(defined_headers[0]);
    for (int i = 0; i < len; ++i) {
        fmap_add(defined_header_map, defined_headers[i].key, &defined_headers[i]);
    }
}

/* parse status */
#define STAT_BF_METHOD 0
#define STAT_METHOD 1
#define STAT_BF_TARGET 2
#define STAT_TARGET 3
#define STAT_BF_VERSION 4
#define STAT_VERSION 5
#define STAT_TO_END 6
#define STAT_END 7
#define STAT_NORMAL 8

#define STAT_BF_KEY 10
#define STAT_AT_KEY 11
#define STAT_AF_KEY 12
#define STAT_BF_VAL 13
#define STAT_AT_VAL 14
#define STAT_AF_VAL 15

/**
 * read line till \r\n or \0 means buf end
 * return the next postion of buffer after \r\n or -1 error occur;
 * [RFC 7230 3.1.1]
 * TODO..parse status line when got response
 */
static char *_parse_request_line(char *buf, struct http_start_line *header_line) {
    int status = STAT_BF_METHOD;
    int run = 1;
    char *l = buf, *h = buf;
    struct http_status *http_stat = NULL;
    struct fstr *method = NULL, *target = NULL, *version = NULL;

    while (*buf && run) {
        switch (status) {
            case STAT_BF_METHOD:
                if (isspace(*buf)) {
                    http_stat = _defined_status_get(400);
                    run = 0;
                } else {
                    l = h = buf;
                    status = STAT_METHOD;
                }
                break;

            case STAT_METHOD:
                if (isspace(*buf)) {
                    if (h - l + 1 > HTTP_START_LINE_MAX) {
                        http_stat = _defined_status_get(414);
                        run = 0;
                    } else {/* got method */
                        method = fstr_createlen(l, (size_t) (h - l + 1));
                        header_line->method = method->buf;
                        l = ++buf;
                        status = STAT_BF_TARGET;
                    }
                } else {
                    h = buf++;
                }
                break;

            case STAT_BF_TARGET:
                if (isspace(*buf)) {
                    http_stat = _defined_status_get(400);
                    run = 0;
                } else {
                    l = h = buf;
                    status = STAT_TARGET;
                }
                break;

            case STAT_TARGET:
                if (isspace(*buf)) {
                    if (h - l + 1 > HTTP_START_LINE_MAX) {
                        http_stat = _defined_status_get(414);
                        run = 0;
                    } else {/* got http_status */
                        target = fstr_createlen(l, (size_t) (h - l + 1));
                        header_line->target = target->buf;
                        l = ++buf;
                        status = STAT_BF_VERSION;
                    }
                } else {
                    h = buf++;
                }
                break;

            case STAT_BF_VERSION:
                if (isspace(*buf)) {
                    http_stat = _defined_status_get(400);
                    run = 0;
                } else {
                    l = h = buf;
                    status = STAT_VERSION;
                }
                break;

            case STAT_VERSION:
                if (*buf == '\r') {
                    if (h - l + 1 > HTTP_START_LINE_MAX) {
                        http_stat = _defined_status_get(414);
                        run = 0;
                    } else { /* got version */
                        version = fstr_createlen(l, (size_t) (h - l + 1));
                        header_line->version = version->buf;
                        l = ++buf;
                        status = STAT_TO_END;
                    }
                } else {
                    h = buf++;
                }
                break;

            case STAT_TO_END:
                if (*buf == '\n') {
                    status = STAT_END;
                    ++buf;
                } else {
                    http_stat = _defined_status_get(400);
                }
                run = 0;
                break;
            default:
                break;
        }
    }

    if (status != STAT_END) {
        header_line->status = *http_stat;
        fstr_free(method);
        fstr_free(target);
        fstr_free(version);
        return 0;
    }

    header_line->type = HTTP_REQUEST;
    return buf;
}

/* parse http header like
 * key1: content\r\n
 * key2: content\r\n
 * \r\n
 * body
 *
 * key field is predefined
 *
 * note that, at this stage, key and val used in the routine living in stack,
 * so fmap should run with 'bothdup' mode, but this way decrese performance
 * TODO...avoid memory copy, 'buf' usually created by network loop in heap,
 * so point it directly would be memory friendly, security issues may also easy to solv.
*/
static char *_parse_header(char *buf, struct fmap *out_map) {
    size_t buf_idx = 0;
    int stat = STAT_BF_KEY;
    char field[HTTP_ITEMBUFSIZE] = "\0";
    char value[HTTP_ITEMBUFSIZE] = "\0";

    /* status machine for parse config syntax
     * TODO. backslash...
     */
    while (*buf != '\0') {
        switch (stat) {
            case STAT_BF_KEY:
                if (*buf == '\r') { /* handle header endline \r\n */
                    field[0] = '\0';
                    ++buf;
                    stat = STAT_AF_VAL;
                } else if (isspace(*buf)) { /* skip spaces before key */
                    ++buf;
                } else {
                    buf_idx = 0;
                    stat = STAT_AT_KEY;
                }
                break;

            case STAT_AT_KEY:
                if (*buf == ':') {
                    field[buf_idx] = '\0';
                    ++buf;
                    stat = STAT_BF_VAL;
                } else if (isspace(*buf)) {
                    field[buf_idx] = '\0';
                    stat = STAT_AF_KEY;
                } else {
                    field[buf_idx++] = *buf++;
                }
                break;

            case STAT_AF_KEY:  /* content after key must be : or spaces, otherwise syntax error */
                if (*buf == ':') {
                    ++buf;
                    stat = STAT_BF_VAL;
                } else if (isspace(*buf)) {
                    ++buf;
                } else {
                    log("syntax error : error charactors between key and value");
                    goto error;
                }
                break;

            case STAT_BF_VAL:
                if (isspace(*buf)) { /* skip spaces before value */
                    ++buf;
                } else {
                    buf_idx = 0;
                    stat = STAT_AT_VAL;
                }
                break;

            case STAT_AT_VAL:
                if (*buf == '\r') { /* header item endup with \r\n */
                    ++buf;
                    stat = STAT_AF_VAL;
                } else {
                    value[buf_idx++] = *buf++;
                }
                break;

            case STAT_AF_VAL:
                if (*buf == '\n') {
                    stat = STAT_BF_KEY;
                    value[buf_idx] = '\0';
                    if (strcmp(field, "") == 0) {
                        return ++buf;
                    } else {
                        _header_map_add(out_map, field, value);
                    }
                    ++buf;
                } else {
                    value[buf_idx++] = *buf++;
                }
                break;

            default:
                break;
        }
    }

    /* at most condition, last value will follow by '\0'  */
    log("stat:%d", stat);
    if (stat == STAT_AF_VAL) {
        value[buf_idx] = '\0';
        _header_map_add(out_map, field, value);
    } else if (stat != STAT_BF_KEY) {
        log("Syntax error : status error: %d", stat);
        goto error;
    }

    return buf;

    error:
    return NULL;
}

static inline void _header_map_add(struct fmap *header_map, char *key, char *val) {
    struct http_header_item *header_item = fmap_getvalue(defined_header_map, key);
    if (header_item == NULL) return;

    struct http_header_item *item_target = fmalloc(sizeof(struct http_header_item));
    fmemcpy(item_target, header_item, sizeof(struct http_header_item));

    size_t val_len = strlen(val);
    item_target->val_parse_func = header_item->val_parse_func;
    item_target->val = fmalloc(val_len + 1);
    cpystr(item_target->val, val, strlen(val));

    fmap_add(header_map, key, item_target);
}

static int _parse_body(char *http_body, const int len, struct http_pack *http) {
    char *buf = http_body;
    int real_len = 0;
    int run = 1;
    int stat = STAT_NORMAL;

    while (*buf && run) {
        switch (stat) {
            case STAT_NORMAL:
                if (*buf == '\r') {
                    stat = STAT_TO_END;
                } else {
                    ++real_len;
                }
                ++buf;
                break;
            case STAT_TO_END:
                if (*buf == '\n') {
                    run = 0;
                } else {
                    real_len += 2;
                    ++buf;
                    stat = STAT_NORMAL;
                }
                break;
            default:
                break;
        }
    }

    if (real_len != len) {
        http->start_line.status = *_defined_status_get(400);
        return -1;
    }

    struct fstr *body = fstr_createlen(http_body, (size_t const) real_len);
    http->body = body->buf;

    return 0;
}

inline struct http_pack *http_pack_create() {
    struct http_pack *http;
    http = fmalloc(sizeof(struct http_pack));
    if (http == NULL) return NULL;

    memset(http, 0, sizeof(struct http_pack));
    return http;
}

static inline struct http_status *_defined_status_get(int status) {
    return fmap_getvalue(defined_status_map, (void *) status);
}

struct http_pack *http_parse(char *http_buf) {
    struct http_pack *http;
    struct fmap *header_map = NULL;

    http = http_pack_create();
    if (http == NULL) return NULL;

    /* 1.parse start line */
    if ((http_buf = _parse_request_line(http_buf, &http->start_line)) == NULL)
        goto reject;

    /* 2.parse headers */
    if ((header_map = fmap_create_dupkey()) == NULL) return NULL;
    header_map->type = &http_header_hash_type;
    if ((http_buf = _parse_header(http_buf, header_map)) == NULL) goto reject;
    http->headers = header_map;

    /* 3.parse body with Content-Length
     * TODO..to implement Transfer-Encoding may allow not Content-Length */
    struct http_header_item *item = fmap_getvalue(http->headers, "Content-Length");
    if (item == NULL) {
        http->start_line.status = *_defined_status_get(411);
        goto reject;
    }
    if (_parse_body(http_buf, (int) item->val_parse_func(item->val), http) == -1)
        goto reject;

    return http;

    reject:
    http->headers = NULL;
    http->body = NULL;
    fmap_free(header_map);
    fstr_freestr(http->body);
    return http;
}

void http_pack_info(struct http_pack *http) {
    if (http == NULL) {
        printf("http pack is NULL \n");
        return;
    }

    printf("http pack \n");

    struct http_start_line *start_line = &http->start_line;
    printf("%s %s %s\n", start_line->method, start_line->target, start_line->version);
    fmap_info_str(http->headers);
    printf("\r\n%s\n", http->body);
}

#ifndef DEBUG_HTTP

int main(void) {
    char *http_buf = "GET /index HTTP/1.1\r\n"
            "Content-Type:html/text\r\n"
            "Content-Length:5\r\n\r\n"
            "hello\r\n";

    http_init();

    struct http_pack *http = http_parse(http_buf);

    http_pack_info(http);

    return 0;
}

#endif /* DEBUG_HTTP */void struct fdis::Worker::init(){

}
