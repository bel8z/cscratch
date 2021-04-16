#include <stdio.h>
#include <stdlib.h>

#include "foundation/util.h"

void
print_array(i32 const *array, usize count)
{
    printf("{");
    for (usize i = 0; i < count; ++i)
    {
        printf("%d,", array[i]);
    }
    printf("}\n");
}

void
swap(i32 *a, i32 *b)
{
    i32 temp = *a;
    *a = *b;
    *b = temp;
}

void
reverse(i32 *array, usize size)
{
    for (usize i = 0; i < size / 2; ++i)
    {
        swap(array + i, array + size - i - 1);
    }
}

// void
// rotate_left(i32 *array, usize size, usize pos)
// {
//     if (pos == 0) return;

//     usize write = 0;
//     usize read = pos;
//     usize next_read = 0; // read position for when "read" hits "last"

//     while (write != read && read != size)
//     {
//         while (read != size)
//         {
//             if (write == next_read) next_read = read; // track where "first" went
//             swap(array + write, array + read);
//             write++;
//             read++;
//         }

//         read = next_read;
//         // rotate the remaining sequence into place
//     }
// }

void
rotate_right(i32 *array, usize size, usize pos)
{
    reverse(array, size);
    reverse(array, pos);
    reverse(array + pos, size - pos);
}

void
rotate_left(i32 *array, usize size, usize pos)
{

    reverse(array, size);
    reverse(array, size - pos);
    reverse(array + size - pos, pos);
}

int
main(void)
{
    for (usize pos = 0; pos < 6; ++pos)
    {
        i32 temp[] = {0, 1, 2, 3, 4, 5};
        rotate_left(temp, 6, pos);
        print_array(temp, 6);
    }

    for (usize pos = 0; pos < 6; ++pos)
    {
        i32 temp[] = {0, 1, 2, 3, 4, 5};
        rotate_right(temp, 6, pos);
        print_array(temp, 6);
    }

    return 0;
}
