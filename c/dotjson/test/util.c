#include "util.h"
#include <stdio.h>

void print_node(struct dotj_node *node)
{
    size_t i;

    switch (node->t) {
    case DOTJ_NULL:
        printf("<null>\n");
        break;
    case DOTJ_TRUE:
        printf("<true>\n");
        break;
    case DOTJ_FALSE:
        printf("<false>\n");
        break;
    case DOTJ_NUMBER:
        printf("<number: (%s)>\n", ((struct dotj_number *)node)->num.raw);
        break;
    case DOTJ_STRING:
        printf("<string: \"%s\">\n", ((struct dotj_string *)node)->s);
        break;
    case DOTJ_ARRAY:
        printf(">array<%lu, %lu>: [\n", ((struct dotj_array *)node)->len, ((struct dotj_array *)node)->cap);
        for (i = 0; i < ((struct dotj_array *)node)->len; i++) {
            print_node(((struct dotj_array *)node)->items[i]);
        }
        printf("]\n");
        break;
    case DOTJ_OBJECT:
        printf(">object<%lu, %lu>: {\n", ((struct dotj_object *)node)->len, ((struct dotj_object *)node)->cap);
        for (i = 0; i < ((struct dotj_object *)node)->cap; i++) {
            if (((struct dotj_object *)node)->entries[i]) {
                printf("\nkey: %s\nvalue: ", ((struct dotj_object *)node)->entries[i]->key);
                print_node(((struct dotj_object *)node)->entries[i]->val);
            }
        }
        printf("}\n");
        break;
    }
}

