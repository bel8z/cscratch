#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Storage Storage;

struct Storage
{
    size_t size;
    void *data;
    bool (*resize)(Storage *s, size_t new_size);
    void (*free)(Storage *s);
};

static inline bool
no_resize(Storage *s, size_t new_size)
{
    (void)s;
    (void)new_size;
    return false;
};

static inline void
no_free(Storage *s)
{
    (void)s;
};

static inline Storage
storage_fixed_init(void *block, size_t size)
{
    return (Storage){
        .data = block, .size = size, .resize = no_resize, .free = no_free};
}
