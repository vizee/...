#include "../dotjson.c"
#include "util.h"

int main()
{
    struct dotj_node *root;
    const char *json = "{\"true\":true,\"false\":false,\"null\":null,\"string\":\"string\",\"number\":123}";

    if (dotjson_decode(json, &root) == 0) {
        return 1;
    }
    print_node(dotjson_object_get((struct dotj_object *)root, "number"));
    dotjson_destroy(root);
    return 0;
}
