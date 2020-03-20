#ifndef HTTP_H
#define HTTP_H

#define HTTP_REQUEST 1
#define HTTP_RESPONSE 2

struct http_status {
    int status;
    char *semantic;
};

struct http_start_line {
    int type;
    struct http_status status;
    char *method;
    char *target;
    char *version;
};

struct http_pack {
    struct http_start_line start_line;
    struct fmap *headers;
    char *body;
};

struct http_header_item {
    char *key;
    char *val;
    /* 0x10 request header
     * 0x01 response header
     * 0x11 both request & response */
    int type;

    /* function that parse the value */
    void *(*val_parse_func)(char *val);
};

/****************** API *******************/
void http_init();
struct http_pack *http_pack_create();
struct http_pack *http_parse(char *http_buf);

#endif /* HTTP_H */
