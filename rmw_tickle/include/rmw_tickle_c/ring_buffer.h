#pragma once

#include "rcutils/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ring_buffer {
    void* data;
    uint32_t  elem_size;
    uint32_t  capacity;
    uint32_t  head;
    uint32_t  tail;
};

int ring_buffer_create(struct ring_buffer* buffer, rcutils_allocator_t allocator, uint32_t elem_size, uint32_t capacity);
void ring_buffer_destroy(struct ring_buffer* buffer, rcutils_allocator_t allocator);
int ring_buffer_push(struct ring_buffer* buffer, void* element);
void* ring_buffer_pop(struct ring_buffer* buffer);
uint32_t ring_buffer_size(struct ring_buffer* buffer);

#ifdef __cplusplus
}
#endif
