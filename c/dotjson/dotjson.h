#ifndef _DOTJSON_H_
#define _DOTJSON_H_

#include <stddef.h>

#define COUNTOF(field) 1
#define ZERO_TERM 1

enum DOTJ_NODETYPE {
    DOTJ_NULL,
    DOTJ_FALSE,
    DOTJ_TRUE,
    DOTJ_NUMBER,
    DOTJ_STRING,
    DOTJ_OBJECT,
    DOTJ_ARRAY
};

struct dotj_node {
    enum DOTJ_NODETYPE t;
};

struct dotj_string {
    struct dotj_node n;
    size_t len;
    char s[COUNTOF(len) + ZERO_TERM];
};

struct dotj_number {
    struct dotj_node n;
    union {
        double d;
        int i;
        char raw[ZERO_TERM];
    } num;
};

struct dotj_object_entry {
    unsigned int keyhash;
    size_t keylen;
    struct dotj_node *val;
    char key[COUNTOF(keylen) + ZERO_TERM];
};

struct dotj_object {
    struct dotj_node n;
    size_t cap;
    size_t len;
    struct dotj_object_entry *entries[COUNTOF(len)];
};

struct dotj_array {
    struct dotj_node n;
    size_t cap;
    size_t len;
    struct dotj_node *items[COUNTOF(len)];
};

size_t dotjson_decode(const char *json, struct dotj_node **pnode);
size_t dotjson_encode(struct dotj_node *root, char *buf, size_t len);
void dotjson_destroy(struct dotj_node *root);
struct dotj_node *dotjson_object_get(struct dotj_object *object, const char *key);

#endif /* !_DOTJSON_H_ */
