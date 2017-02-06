#include "../dotjson.c"
#include "util.h"
#include <stdio.h>

void test_decode_string()
{
    struct dotj_string *str;

    puts("===test_decode_string===");
    printf("len %lu\n", decode_string("\"\\nabc\\u4F60\\u597D\\u4E16\\u754Cefd\"", (struct dotj_node **)&str));
    print_node((struct dotj_node *)str);
    dotjson_destroy((struct dotj_node *)str);
    puts("==========end==========");
}

void test_decode_number()
{
    struct dotj_number *num;

    puts("===test_decode_number===");
    printf("len %lu\n", decode_number(" 123459.123e10", (struct dotj_node **)&num));
    print_node((struct dotj_node *)num);
    dotjson_destroy((struct dotj_node *)num);
    puts("==========end==========");
}

void test_decode_object()
{
    struct dotj_node *node;
    const char *json1 = "{}";
    const char *json2 = "{\"true\":true,\"false\":false,\"null\":null,\"string\":\"string\",\"number\":123}";
    const char *json3 = "{\"a\":[0]}";

    puts("===test_decode_object===");
    printf("len %lu\n", decode_object(json1, &node));
    print_node(node);
    dotjson_destroy(node);
    printf("len %lu\n", decode_object(json2, &node));
    print_node(node);
    dotjson_destroy(node);
    printf("len %lu\n", decode_object(json3, &node));
    print_node(node);
    dotjson_destroy(node);
    puts("==========end==========");
}

void test_decode_array()
{
    struct dotj_node *node;
    const char *json1 = "[]";
    const char *json2 = "[true,false,null,\"string\",123]";
    const char *json3 = "[{\"a\":0}]";

    puts("===test_decode_array====");
    printf("len %lu\n", decode_array(json1, &node));
    print_node(node);
    dotjson_destroy(node);
    printf("len %lu\n", decode_array(json2, &node));
    print_node(node);
    dotjson_destroy(node);
    printf("len %lu\n", decode_array(json3, &node));
    print_node(node);
    dotjson_destroy(node);
    puts("==========end==========");
}

int main()
{
    test_decode_string();
    test_decode_number();
    test_decode_object();
    test_decode_array();
    return 0;
}
