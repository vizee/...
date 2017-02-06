#include "../dotjson.c"
#include "util.h"

int main()
{
    struct dotj_node *root;
    const char *json = "{/* block comment */\"foo\":1,// line comment\n\"bar\":2}";

    if (dotjson_decode(json, &root) == 0) {
        return 1;
    }
    print_node(root);
    dotjson_destroy(root);
    return 0;
}
