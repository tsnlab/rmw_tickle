#include <rmw_tickle_c/ring_buffer.h>

struct ring_buffer ring_buffer_create(rcutils_allocator_t allocator, uint32_t elem_size, uint32_t capacity) {
    struct ring_buffer buffer = {0,};
    ring_buffer_init(&buffer, allocator, elem_size, capacity);
    return buffer;
}

int32_t ring_buffer_init(struct ring_buffer* buffer, rcutils_allocator_t allocator, uint32_t elem_size, uint32_t capacity) {
    if (buffer == NULL || elem_size == 0 || capacity == 0) {
        return -1;
    }
    buffer->elem_size = 8 * (1 + (elem_size - 1) / 8); // 8 byte alignment
    buffer->capacity = capacity;
    buffer->read_end = 0;
    buffer->write_end = 0;
    buffer->data = allocator.zero_allocate(capacity + 1, elem_size, allocator.state);
    if (buffer->data == NULL) {
        return -1;
    }
    return 0;
}

void ring_buffer_destroy(struct ring_buffer* buffer, rcutils_allocator_t allocator) {
    allocator.deallocate(buffer->data, allocator.state);
    *buffer = (struct ring_buffer){0, };
}

// read_end is inclusive, write_end is exclusive index
int32_t ring_buffer_push(struct ring_buffer* buffer, void* push_from) {
    uint32_t write_end = buffer->write_end;
    uint32_t new_write_end = (write_end + 1) % (buffer->capacity + 1);

    // buffer is full
    if (new_write_end == buffer->read_end) {
        return -1;
    }
    memcpy(&buffer->data[write_end * elem_size], element, buffer->elem_size);
    buffer->write_end = new_write_end;
    return 0;
}

int32_t ring_buffer_pop(struct ring_buffer* buffer, void* pop_to) {
    uint32_t read_end = buffer->read_end;

    // nothing to read
    if (read_end == buffer->write_end) {
        return -1;
    }
    memcpy(element, &buffer->data[read_end * elem_size], buffer->elem_size);
    buffer->read_end = (read_end + 1) % (buffer->capacity + 1);
    return 0;
}

uint32_t ring_buffer_size(struct ring_buffer* buffer) {
    uint32_t read_end = buffer->read_end;
    uint32_t write_end = buffer->write_end;

    if (read_end < write_end) {
        return write_end - read_end;
    } else {
        return write_end + buffer->capacity + 1 - read_end;
    }
}
