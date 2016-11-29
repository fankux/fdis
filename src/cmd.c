#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/fstr.h"
#include "common/fmap.h"
#include "common/check.h"
#include "cmd.h"

static struct webc_mutation *mutation_create(struct webc_cmd *cmd) {
    struct webc_mutation *mutation = fmalloc(sizeof(struct webc_mutation));
    if (mutation == NULL) return NULL;

    mutation->cmd = cmd;
    mutation->key = NULL;

    return mutation;
}

/////// Implement //////////////////////////////////////////////////////////////

struct fmap *cmd_dict;

int dummy_command_func(void *args) {
    return (int) args;
}

struct webc_cmd command_list[] = {
        /* list commands */
        {"LPUSH", 0x01, 2, dummy_command_func},
        {"RPUSH", 0x01, 2, dummy_command_func},
        {"LPOP",  0x02, 1, dummy_command_func},
        {"RPOP",  0x02, 1, dummy_command_func},
        {"LSET",  0x03, 3, dummy_command_func},
        {"LLEN",  0x00, 1, dummy_command_func},
        {"LINS",  0x01, 2, dummy_command_func},
        {"LREM",  0x02, 3, dummy_command_func},
        {"LIDX",  0x00, 2, dummy_command_func},
        {"LRNG",  0x00, 3, dummy_command_func},
        /* hash commands */
        {"HSET",  0x03, 3, dummy_command_func},
        {"HSETX", 0x03, 3, dummy_command_func},
        {"HGET",  0x00, 2, dummy_command_func},
        {"HGETM", 0x00, 3, dummy_command_func},
        {"HGETA", 0x00, 1, dummy_command_func},
        {"HDEL",  0x02, 2, dummy_command_func},
        {"HLEN",  0x00, 1, dummy_command_func},
        {"HXST",  0x00, 2, dummy_command_func},
        {"HKEYS", 0x00, 1, dummy_command_func},
        {"HVALS", 0x00, 1, dummy_command_func},
        {"HINCR", 0x03, 3, dummy_command_func},
};

/* classify the command list by type field, it's a mask,
** each mask could combined, the flag field used for local persistence
** flag set 0, query command, never modify the memory using,
** first  2 bits: indicate the memory changing
** 00(0x00), memory won't by change,
** 01(0x01), add command, will increce memory using,
** 10(0x02), remove command, will decrece memory using,
** 11(0x03), set command, may modify memory using,
** bit 3, system command, would modify some system setting */
void cmd_classify() {
    int i, cmd_list_len = sizeof(command_list) /
                          sizeof(command_list[0]);
    struct webc_cmd *p;

    for (i = 0; i < cmd_list_len; ++i) {
        p = command_list + i;
        fmap_add(cmd_dict, p->name, p);
    }
}

/* judge if this command would modify memory use
** if memory sensetive, return 1, else return 0 */
inline int cmd_ismem(struct webc_cmd *cmd) {
    return (cmd->type & 3) != 0;
}

/* decrypt command, and then re-format command
** | ..0.. |s| ..1.. |s| ..2.. |s|.....
** 0, command strings
** s, spaces, unsure length of bits
** 1, argument 1
** 2, argument 2
** ..
** n,  argument n
** end up with \r\n */
struct webc_mutation *cmd_parse(char *buf) {
    /* get cmd */
    char *start, *next, *s = buf;
    char key_buf[128] = "\0";
    size_t sec_len;
    int re;

    re = valuesplit(buf, &sec_len, &start, &next);
    check_cond(re == 1, return NULL, "command parse fiald, re:%d", re);

    cpystr(key_buf, start, sec_len);

    /* get cmd struct */
    struct fmap_node *cmd_node;
    struct webc_mutation *mutation;
    struct webc_cmd *cmd;

    cmd_node = fmap_get(cmd_dict, (void *) key_buf);
    check_null(cmd_node, return NULL, "command not found: %s", key_buf);

    cmd = (struct webc_cmd *) cmd_node->value;
    mutation = mutation_create(cmd);
    check_null_oom(mutation, return NULL, "mutation");
//    check_null_ret((mutation->argv = malloc(sizeof(char *) * cmd->arity)), NULL,
//                   CONST_STR_OOM"mutation argv");

    buf = next;
    /* get key */
    re = valuesplit(buf, &sec_len, &start, &next);
    check_null(re <= 0, return NULL, "command syntax error, %s", s);

    struct fstr *str = fstr_createlen(start, sec_len);
    check_null_oom(str, return NULL, "mutation key");
    mutation->key = str->buf;
    mutation->value = *next ? next + 1 : NULL;

    return mutation;
}

//TODO..temp implement simply, feather first
char *cmd_serialize(struct webc_mutation *mutation) {
    struct fstr *buf = fstr_createlen(NULL, strlen(mutation->cmd->name) + 8 +
                                            fstr_getpt(mutation->key)->len + 8 +
                                            fstr_getpt(mutation->value)->len +
                                            8);
    check_null_oom(buf, return NULL, "command serialize faild");
    sprintf(buf->buf, "%s\r\n%s\r\n%s\r\n",
            mutation->cmd->name, mutation->key, mutation->value);

    return buf->buf;
}

struct webc_mutation *cmd_unserialize(char *buf) {
    struct webc_mutation *mutation = NULL;
    struct fstr *cmd_str = NULL;
    struct fstr *key_str = NULL;
    struct fstr *value_str = NULL;
    char *idx;

    mutation = malloc(sizeof(struct webc_mutation));
    check_null_oom(mutation, goto faild, "command mutation");

    //cmd
    idx = strstr(buf, "\r\n");
    check_null(idx, goto faild, "command not found");
    cmd_str = fstr_createlen(buf, idx - buf);
    check_null_oom(cmd_str, goto faild, "mutation cmd memory");
    struct fmap_node *cmd_node = fmap_get(cmd_dict, cmd_str->buf);
    check_null(cmd_node, goto faild, "command %s unsupport", cmd_str->buf);
    mutation->cmd = cmd_node->value;
    buf = idx;

    //key
    idx = strstr(buf, "\r\n");
    check_null(idx, goto faild, "key not found");
    key_str = fstr_createlen(buf, idx - buf);
    check_null_oom(cmd_str, goto faild, "mutation key");
    mutation->key = key_str->buf;
    buf = idx;

    //value
    idx = strstr(buf, "\r\n");
    check_null(idx, goto faild, "value not found");
    value_str = fstr_createlen(buf, idx - buf);
    check_null_oom(cmd_str, goto faild, "mutation value ");
    mutation->value = value_str->buf;

    return mutation;

    faild:
    fstr_free(cmd_str);
    fstr_free(key_str);
    fstr_free(value_str);
    free(mutation);
    return NULL;
}