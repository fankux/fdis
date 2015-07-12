//
// Created by fankux on 15-7-12.
//

#ifndef WEBC_CMD_H
#define WEBC_CMD_H


#include <stdint.h>
#include <stdio.h>

struct webc_cmd {
    char *name;
    uint8_t type;
    size_t arity;

    int ( *func)(void *param);
};

struct webc_mutation {
    struct webc_cmd *cmd;
    char *key;
    char *value;
};

extern struct fmap *cmd_dict;

struct webc_mutation *mutation_create(struct webc_cmd *cmd);

#endif //WEBC_CMD_H
