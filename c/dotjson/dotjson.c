#include "dotjson.h"
#include <string.h>
#include <malloc.h>

#define IGNORE_SPACE(p) while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') { p++; }

#ifdef DOTJSON_FEATURE_COMMENT
static void ignore_chars(const char **pp)
{
    const char *p = *pp;

_redo_ignore:
    IGNORE_SPACE(p);
    if (*p == '/') {
        switch (p[1]) {
        case '/':
            while (*p) {
                if (*p++ == '\n') {
                    break;
                }
            }
            goto _redo_ignore;
        case '*':
            p += 2;
            while (*p) {
                if (*p == '*' && p[1] == '/') {
                    p += 2;
                    break;
                }
                p++;
            }
            goto _redo_ignore;
        }
    }
    *pp = p;
}

#define IGNORE_CHARS(p) ignore_chars(&p)
#else
#define IGNORE_CHARS(p) IGNORE_SPACE(p)
#endif

static size_t decode_node(const char *s, struct dotj_node **pnode);
static size_t encode_node(struct dotj_node *node, char *buf, size_t len);

static unsigned int hash(const char *s)
{
    const unsigned int seed = 1313;

    unsigned int h = seed;
    while (*s) {
        h = h * seed + *s++;
    }
    return h;
}

static size_t min_bucket(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

static size_t uc_2_utf8(unsigned int uc, char *u8)
{
    if (uc <= 0x7f) {
        if (u8) {
            u8[0] = uc;
        }
        return 1;
    } else if (uc <= 0x7ff) {
        if (u8) {
            u8[0] = 0xc0 | (uc >> 6);
            u8[1] = 0x80 | (uc & 0x3f);
        }
        return 2;
    } else if (uc <= 0xffff) {
        if (u8) {
            u8[0] = 0xe0 | (uc >> 12);
            u8[1] = 0x80 | ((uc >> 6) & 0x3f);
            u8[2] = 0x80 | (uc & 0x3f);
        }
        return 3;
    } else if (uc <= 0x1fffff) {
        if (u8) {
            u8[0] = 0xf0 | (uc >> 18);
            u8[1] = 0x80 | ((uc >> 12) & 0x3f);
            u8[2] = 0x80 | ((uc >> 6) & 0x3f);
            u8[3] = 0x80 | (uc & 0x3f);
        }
        return 4;
    } else if (uc <= 0x3ffffff) {
        if (u8) {
            u8[0] = 0xf8 | (uc >> 24);
            u8[1] = 0x80 | ((uc >> 18) & 0x3f);
            u8[2] = 0x80 | ((uc >> 12) & 0x3f);
            u8[3] = 0x80 | ((uc >> 6) & 0x3f);
            u8[4] = 0x80 | (uc & 0x3f);
        }
        return 5;
    } else if (uc <= 0x7fffffff) {
        if (u8) {
            u8[0] = 0xfc | (uc >> 30);
            u8[1] = 0x80 | ((uc >> 24) & 0x3f);
            u8[2] = 0x80 | ((uc >> 18) & 0x3f);
            u8[3] = 0x80 | ((uc >> 12) & 0x3f);
            u8[4] = 0x80 | ((uc >> 6) & 0x3f);
            u8[5] = 0x80 | (uc & 0x3f);
        }
        return 6;
    }
    return 0;
}

static size_t unescape_str(const char *s, char *buf, size_t *len)
{
    size_t i;
    char c;
    int uc;
    const char *p = s;
    size_t n = 0;
    size_t l = 0;

    if (*p++ != '"') {
        return 0;
    }
    if (buf && len && *len > 0) {
        l = *len;
    }
    while (*p != '"') {
        if (!*p) {
            return 0;
        }
        switch (*p) {
        case '\\':
            p++;
            if (*p == 'u') {
                uc = 0;
                for (i = 1; i <= 4;i++) {
                    uc <<= 4;
                    c = p[i];
                    if (c >= '0' && c <= '9') {
                        uc |= (c & 0x0f);
                    } else if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
                        uc |= ((c & 0x0f) + 9);
                    } else {
                        return 0;
                    }
                }
                i = uc_2_utf8(uc, NULL);
                if (n + i <= l) {
                    uc_2_utf8(uc, buf + n);
                }
                n += i;
                p += 4;
            } else {
                switch (*p) {
                case '"':
                    c = '"';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case '/':
                    c = '/';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                default:
                    return 0;
                }
                if (n < l) {
                    buf[n] = c;
                }
                n++;
            }
            break;
        default:
            if (n < l) {
                buf[n] = *p;
            }
            n++;
            break;
        }
        p++;
    }
    if (len) {
        *len = n;
    }
    p++;
    return p - s;
}

static size_t object_find(struct dotj_object *object, const char *key)
{
    struct dotj_object_entry *entry;
    size_t mask;
    unsigned int h;
    size_t i;

    h = hash(key);
    mask = object->cap - 1;
    for (i = 0; i < object->cap; i++) {
        entry = object->entries[(h + i) & mask];
        if (!entry) {
            break;
        }
        if (entry->keyhash == h) {
            if (strcmp(entry->key, key) == 0) {
                return (h + i) & mask;
            }
        }
    }
    return object->cap;
}

static size_t object_slot(struct dotj_object *object, const char *key)
{
    unsigned int h;
    size_t mask;
    size_t i;

    h = hash(key);
    mask = object->cap - 1;
    for (i = 0; i < object->cap; i++) {
        if (!object->entries[(h + i) & mask]) {
            return  (h + i) & mask;
        }
    }
    return object->cap;
}

static struct dotj_object *object_grow(struct dotj_object *object)
{
    size_t size;
    size_t cap;
    size_t i;
    struct dotj_object *to;

    if (object) {
        cap = min_bucket(object->len + 1);
        if (object->cap >= cap) {
            return object;
        }
    } else {
        cap = 1;
    }
    size = sizeof(struct dotj_object) + (cap - 1) * sizeof(struct dotj_object_entry);
    to = malloc(size);
    memset(to, 0, size);
    to->n.t = DOTJ_OBJECT;
    to->cap = cap;
    if (object) {
        for (i = 0; i < object->len; i++) {
            if (!object->entries[i]) {
                continue;
            }
            to->entries[object_slot(to, object->entries[i]->key)] = object->entries[i];
            to->len++;
        }
        free(object);
    }
    return to;
}

static struct dotj_array *array_grow(struct dotj_array *array) {
    struct dotj_array *ta;
    size_t cap;

    if (array) {
        if (array->len < array->cap) {
            return array;
        }
        if (array->cap >= 256) {
            cap = array->cap + 256;
        } else {
            cap = array->cap << 1;
        }
    } else {
        cap = 1;
    }
    ta = malloc(sizeof(struct dotj_array) + (cap - 1) * sizeof(struct dotj_node *));
    ta->n.t = DOTJ_ARRAY;
    ta->cap = cap;
    if (array) {
        ta->len = array->len;
        memcpy(ta->items, array->items, sizeof(struct dotj_node *) * array->len);
    } else {
        ta->len = 0;
    }
    return ta;
}

static size_t decode_string(const char *s, struct dotj_node **pnode)
{
    size_t len;
    struct dotj_string *str;
    const char *p = s;

    IGNORE_CHARS(p);
    if (!unescape_str(p, NULL, &len)) {
        return 0;
    }
    str = malloc(sizeof(struct dotj_string) + sizeof(char) * len - 1);
    str->n.t = DOTJ_STRING;
    str->len = len;
    *pnode = (struct dotj_node *)str;
    p += unescape_str(p, str->s, &str->len);
    str->s[str->len] = '\0';
    return p - s;
}

static size_t decode_number(const char *s, struct dotj_node **pnode)
{
    struct dotj_number *num;
    const char *b;
    const char *p = s;
    int state = 0;

    IGNORE_CHARS(p);
    b = p;
num_exp:
    if (*p == '+' || *p == '-') {
        p++;
    }
num_frac:
    while (*p >= '0' && *p <= '9') {
        p++;
        if (state == 0) {
            state = 1;
        } else if (state == 2) {
            state = 3;
        } else if (state == 4) {
            state = 5;
        }
    }
    if (state == 1 && *p == '.') {
        state = 2;
        p++;
        goto num_frac;
    }
    if ((state == 1 || state == 3) && (*p == 'E' || *p == 'e')) {
        state = 4;
        p++;
        goto num_exp;
    }
    if (!(state & 1)) {
        return 0;
    }
    num = malloc(sizeof(struct dotj_number) + p - b);
    num->n.t = DOTJ_NUMBER;
    memcpy(num->num.raw, b, p - b);
    num->num.raw[p - b] = '\0';
    *pnode = (struct dotj_node *)num;
    return p - s;
}

static size_t decode_array(const char *s, struct dotj_node **pnode)
{
    size_t n;
    struct dotj_node *node = NULL;
    struct dotj_array *array = NULL;
    const char *p = s;
    int state = 0;

    while (*p && state != 3) {
        IGNORE_CHARS(p);
        if (state == 0 && *p == '[') {
            state = 1;
        } else if (state == 1) {
            if (*p == ']') {
                if (!array) {
                    array = array_grow(NULL);
                    state = 3;
                } else {
                    break;
                }
            } else {
                n = decode_node(p, &node);
                if (n == 0) {
                    break;
                }
                p += n - 1;
                state = 2;
            }
        } else if (state == 2 && (*p == ',' || *p == ']')) {
            if (!array || array->len == array->cap) {
                array = array_grow(array);
            }
            array->items[array->len++] = node;
            node = NULL;
            if (*p == ',') {
                state = 1;
            } else {
                state = 3;
            }
        } else {
            break;
        }
        p++;
    }
    if (state != 3) {
        if (node) {
            dotjson_destroy(node);
        }
        if (array) {
            dotjson_destroy((struct dotj_node *)array);
        }
        return 0;
    }
    *pnode = (struct dotj_node *)array;
    return p - s;
}

static size_t decode_object(const char *s, struct dotj_node **pnode)
{
    size_t len;
    struct dotj_object_entry *entry = NULL;
    struct dotj_object *object = NULL;
    const char *p = s;
    int state = 0;

    while (*p && state != 5) {
        IGNORE_CHARS(p);
        if (state == 0 && *p == '{') {
            state = 1;
        } else if (state == 1 && *p == '"') {
            if (unescape_str(p, NULL, &len) == 0) {
                break;
            }
            entry = malloc(sizeof(struct dotj_object_entry) + len - 1);
            entry->keylen = len;
            p += unescape_str(p, entry->key, &entry->keylen) - 1;
            entry->key[len] = '\0';
            entry->keyhash = hash(entry->key);
            entry->val = NULL;
            state = 2;
        } else if (state == 2 && *p == ':') {
            state = 3;
        } else if (state == 3) {
            len = decode_node(p, &entry->val);
            if (len == 0) {
                break;
            }
            p += len - 1;
            state = 4;
        } else if (state == 4 && (*p == ',' || *p == '}')) {
            if (!object || object->len == object->cap) {
                object = object_grow(object);
            }
            object->entries[object_slot(object, entry->key)] = entry;
            object->len++;
            entry = NULL;
            if (*p == ',') {
                state = 1;
            } else {
                state = 5;
            }
        } else if (state == 1 && *p == '}') {
            if (!object) {
                object = object_grow(NULL);
                state = 5;
            } else {
                break;
            }
        } else {
            break;
        }
        p++;
    }
    if (state != 5) {
        if (entry) {
            dotjson_destroy(entry->val);
            free(entry);
        }
        if (object) {
            dotjson_destroy((struct dotj_node *)object);
        }
        return 0;
    }
    *pnode = (struct dotj_node *)object;
    return p - s;
}

static size_t decode_node(const char *s, struct dotj_node **pnode)
{
    const char *p = s;

    if (!pnode) {
        return 0;
    }
    IGNORE_CHARS(p);
    switch (*p) {
    case '{':
        p += decode_object(p, pnode);
        break;
    case '[':
        p += decode_array(p, pnode);
        break;
    case '"':
        p += decode_string(p, pnode);
        break;
    case 'n':
        if (p[1] == 'u' && p[2] == 'l' && p[3] == 'l') {
            *pnode = malloc(sizeof(struct dotj_node));
            (*pnode)->t = DOTJ_NULL;
            p += 4;
        }
        break;
    case 'f':
        if (p[1] == 'a' && p[2] == 'l' && p[3] == 's' && p[4] == 'e') {
            *pnode = malloc(sizeof(struct dotj_node));
            (*pnode)->t = DOTJ_FALSE;
            p += 5;
        }
        break;
    case 't':
        if (p[1] == 'r' && p[2] == 'u' && p[3] == 'e') {
            *pnode = malloc(sizeof(struct dotj_node));
            (*pnode)->t = DOTJ_TRUE;
            p += 4;
        }
        break;
    default:
        if ((*p >= '0' && *p <= '9') || *p == '+' || *p == '-') {
            p += decode_number(p, pnode);
        }
        break;
    }
    return p - s;
}

static size_t encode_string(struct dotj_string *str, char *buf, size_t len)
{
    char c;
    int ec;
    const char *p = str->s;
    size_t r = 0;

    if (len > r) {
        buf[r] = '"';
    }
    r++;
    ec = 0;
    while (*p) {
        switch (*p) {
        case '"':
            c = '"';
            ec = 1;
            break;
        case '\\':
            c = '\\';
            ec = 1;
            break;
        case '/':
            c = '/';
            ec = 1;
            break;
        case '\b':
            c = 'b';
            ec = 1;
            break;
        case '\f':
            c = 'f';
            ec = 1;
            break;
        case '\n':
            c = 'n';
            ec = 1;
            break;
        case '\r':
            c = 'r';
            ec = 1;
            break;
        case '\t':
            c = 't';
            ec = 1;
            break;
        default:
            c = *p;
            break;
        }
        if (ec) {
            if (len > r) {
                buf[r] = '\\';
            }
            r++;
            ec = 0;
        }
        if (len > r) {
            buf[r] = c;
        }
        r++;
        p++;
    }
    if (len > r) {
        buf[r] = '"';
    }
    r++;
    return r;
}

static size_t encode_object(struct dotj_object *object, char *buf, size_t len)
{
    size_t i;
    struct dotj_object_entry *entry;
    int empty = 1;
    size_t r = 0;

    if (len >= 1) {
        buf[0] = '{';
    }
    r++;
    for (i = 0; i < object->cap; i++) {
        entry = object->entries[i];
        if (!entry) {
            continue;
        }
        if (!empty) {
            if (len > r) {
                buf[r] = ',';
            }
            r++;
        } else {
            empty = 0;
        }
        if (len >= r + 2 + entry->keylen) {
            buf[r] = '"';
            memcpy(buf + r + 1, entry->key, entry->keylen);
            buf[r + 1 + entry->keylen] = '"';
        }
        r += 2 + entry->keylen;
        if (len > r) {
            buf[r] = ':';
        }
        r++;
        r += encode_node(entry->val, buf + r, len > r ? len - r : 0);
    }
    if (len > r) {
        buf[r] = '}';
    }
    r++;
    return r;
}

static size_t encode_node(struct dotj_node *node, char *buf, size_t len)
{
    size_t i;
    size_t r = 0;

    switch (node->t) {
    case DOTJ_NULL:
        if (len >= 4) {
            buf[0] = 'n';
            buf[1] = 'u';
            buf[2] = 'l';
            buf[3] = 'l';
        }
        r = 4;
        break;
    case DOTJ_FALSE:
        if (len >= 5) {
            buf[0] = 'f';
            buf[1] = 'a';
            buf[2] = 'l';
            buf[3] = 's';
            buf[4] = 'e';
        }
        r = 5;
        break;
    case DOTJ_TRUE:
        if (len >= 4) {
            buf[0] = 't';
            buf[1] = 'r';
            buf[2] = 'u';
            buf[3] = 'e';
        }
        r = 4;
        break;
    case DOTJ_NUMBER:
        r = strlen(((struct dotj_number *)node)->num.raw);
        if (len >= r) {
            memcpy(buf, ((struct dotj_number *)node)->num.raw, r);
        }
        break;
    case DOTJ_STRING:
        r = encode_string((struct dotj_string *)node, buf, len);
        break;
    case DOTJ_OBJECT:
        r = encode_object((struct dotj_object *)node, buf, len);
        break;
    case DOTJ_ARRAY:
        if (len > 1) {
            buf[r] = '[';
        }
        r = 1;
        for (i = 0; i < ((struct dotj_array *)node)->len; i++) {
            if (i > 0) {
                if (len > r) {
                    buf[r] = ',';
                }
                r++;
            }
            r += encode_node(((struct dotj_array *)node)->items[i], buf + r, len > r ? len - r : 0);
        }
        if (len > r) {
            buf[r] = ']';
        }
        r++;
        break;
    default:
        break;
    }
    return r;
}

size_t dotjson_decode(const char *json, struct dotj_node **pnode)
{
    struct dotj_node *node;
    size_t n;

    if (!json || !pnode) {
        return 0;
    }
    n = decode_object(json, &node);
    if (n != 0) {
        if (node->t == DOTJ_OBJECT || node->t == DOTJ_ARRAY) {
            *pnode = node;
            return n;
        } else {
            dotjson_destroy(node);
        }
    }
    return 0;
}

size_t dotjson_encode(struct dotj_node *root, char *buf, size_t len)
{
    size_t n;

    if (!root) {
        return 0;
    }
    n = encode_node(root, buf, buf && len > 0 ? len - 1 : 0);
    if (n != 0 && buf && len > n) {
        buf[n] = '\0';
    }
    return n;
}

void dotjson_destroy(struct dotj_node *root)
{
    size_t i;

    if (!root) {
        return;
    }
    switch (root->t) {
    case DOTJ_OBJECT:
        for (i = 0; i < ((struct dotj_object *)root)->len; i++) {
            if (!((struct dotj_object *)root)->entries[i]) {
                continue;
            }
            dotjson_destroy(((struct dotj_object *)root)->entries[i]->val);
            free(((struct dotj_object *)root)->entries[i]);
        }
        break;
    case DOTJ_ARRAY:
        for (i = 0; i < ((struct dotj_array *)root)->len; i++) {
            dotjson_destroy(((struct dotj_array *)root)->items[i]);
        }
        break;
    default:
        break;
    }
    free(root);
}

struct dotj_node *dotjson_object_get(struct dotj_object *object, const char *key)
{
    size_t i;

    i = object_find(object, key);
    if (i < object->cap) {
        return object->entries[i]->val;
    }
    return NULL;
}
