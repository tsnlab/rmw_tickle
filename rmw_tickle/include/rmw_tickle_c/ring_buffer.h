#pragma once

#include "rcutils/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ring_buffer {
    uint8_t* data;
    uint32_t  elem_size;
    uint32_t  capacity;
    uint32_t  write_end;
    uint32_t  read_end;
};

struct ring_buffer ring_buffer_create(rcutils_allocator_t allocator, uint32_t elem_size, uint32_t capacity);
int32_t ring_buffer_init(struct ring_buffer* buffer, rcutils_allocator_t allocator, uint32_t elem_size, uint32_t capacity);
void ring_buffer_destroy(struct ring_buffer* buffer, rcutils_allocator_t allocator);
int32_t ring_buffer_push(struct ring_buffer* buffer, void* push_from);
int32_t ring_buffer_pop(struct ring_buffer* buffer, void* pop_to);
uint32_t ring_buffer_size(struct ring_buffer* buffer);

#ifdef __cplusplus
}
#endif
