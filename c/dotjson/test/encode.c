#include "../dotjson.c"
#include "util.h"
#include <stdio.h>

void test_encode_string()
{
    struct dotj_string *str;
    char *buf;
    size_t len;

    printf("len %lu\n", decode_string("\"\\nabc\\u4F60\\u597D\\u4E16\\u754Cefd\"", (struct dotj_node **)&str));
    print_node((struct dotj_node *)str);
    puts("===test_encode_string===");
    len = dotjson_encode((struct dotj_node *)str, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode((struct dotj_node *)str, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy((struct dotj_node *)str);
    puts("==========end==========");
}

void test_encode_number()
{
    struct dotj_number *num;
    char *buf;
    size_t len;

    printf("len %lu\n", decode_number(" 123459.123e10", (struct dotj_node **)&num));
    print_node((struct dotj_node *)num);
    puts("===test_encode_number===");
    len = dotjson_encode((struct dotj_node *)num, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode((struct dotj_node *)num, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy((struct dotj_node *)num);
    puts("==========end==========");
}

void test_encode_object()
{
    struct dotj_node *node;
    char *buf;
    size_t len;
    const char *json1 = "{}";
    const char *json2 = "{\"true\":true,\"false\":false,\"null\":null,\"string\":\"string\",\"number\":123}";
    const char *json3 = "{\"a\":[0]}";

    puts("===test_encode_object===");
    puts("=========case 0=========");
    printf("len %lu\n", decode_object(json1, &node));
    print_node(node);
    puts("=========output==========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("=========case 1=========");
    printf("len %lu\n", decode_object(json2, &node));
    print_node(node);
    puts("=========output=========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("=========case 2=========");
    printf("len %lu\n", decode_object(json3, &node));
    print_node(node);
    puts("=========output=========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("==========end==========");
}

void test_encode_array()
{
    struct dotj_node *node;
    char *buf;
    size_t len;
    const char *json1 = "[]";
    const char *json2 = "[true,false,null,\"string\",123]";
    const char *json3 = "[{\"a\":0}]";

    puts("===test_encode_array====");
    puts("=========case 0=========");
    printf("len %lu\n", decode_array(json1, &node));
    print_node(node);
    puts("=========output=========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("=========case 1=========");
    printf("len %lu\n", decode_array(json2, &node));
    print_node(node);
    puts("=========output=========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("=========case 2=========");
    printf("len %lu\n", decode_array(json3, &node));
    print_node(node);
    puts("=========output=========");
    len = dotjson_encode(node, NULL, 0);
    buf = malloc(len + 1);
    dotjson_encode(node, buf, len + 1);
    puts(buf);
    free(buf);
    dotjson_destroy(node);
    puts("==========end==========");
}

int main()
{
    test_encode_string();
    test_encode_number();
    test_encode_object();
    test_encode_array();
    return 0;
}
