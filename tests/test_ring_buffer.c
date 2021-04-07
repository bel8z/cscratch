#include "ring_buffer/ring_buffer.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void print(RingIndex *ring, size_t *buff)
{

    printf("cap: %zu\n", ring->cap);
    printf("len: %zu\n", ring->len);
    printf("beg: %zu\n", ring->beg);
    printf("buf: ");

    if (ring->len > 0)
    {
        size_t end = (ring->beg + ring->len) % ring->cap;

        if (end > ring->beg)
        {
            for (size_t i = ring->beg; i < end; ++i)
            {
                printf("%zu,", buff[i]);
            }
        }
        else
        {
            for (size_t i = ring->beg; i < ring->cap; ++i)
            {
                printf("%zu,", buff[i]);
            }
            for (size_t i = 0; i < end; ++i) { printf("%zu,", buff[i]); }
        }
    }

    printf("\n");
}

int main()
{
    RingIndex ring = ring_init(5);
    size_t buffer[5];

    print(&ring, buffer);

    for (size_t i = 0; i < 7; ++i)
    {
        if (!rdeque_push_back(&ring, buffer, i))
        {
            printf(">>> Cannot push %zu\n", i);
        }
        print(&ring, buffer);
        printf("\n");
    }

    ring_clear(&ring);
    print(&ring, buffer);

    for (size_t i = 0; i < 7; ++i)
    {

        if (!rdeque_push_front(&ring, buffer, i))
        {
            printf(">>> Cannot push %zu\n", i);
        }
        print(&ring, buffer);
        printf("\n");
    }
    print(&ring, buffer);

    return 0;
}