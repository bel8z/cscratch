#include "list.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Data
{
    List data_list;

    uint32_t id;
} Data;

int
main(void)
{
    List list = list_create(&list);

    Data data[5] = {0};
    size_t const num_data = sizeof(data) / sizeof(data[0]);

    for (size_t i = 0; i < num_data; ++i)
    {
        data[i].id = i;
        list_push_tail(&list, &data[i].data_list);
    }

    List const *node = list_head(&list);

    while (node != list_end(&list))
    {
        Data const *item = list_entry(node, Data, data_list);
        fprintf(stderr, "%u, ", item->id);
        node = node->next;
    }
}
