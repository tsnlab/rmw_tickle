#pragma once

#include <stdint.h>

#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#else
#error "OS not supported"
#endif /* __linux__ */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rmw_tickle_thread {
    pthread_t thread;
} rmw_tickle_thread_t;

typedef struct rmw_tickle_mutex {
    pthread_mutex_t lock;
} rmw_tickle_mutex_t;

int32_t thread_create(rmw_tickle_thread_t* thread, void* (*routine)(void*), void* arg);
int32_t thread_join(rmw_tickle_thread_t* thread);
int32_t thread_sleep(uint32_t nanoseconds);

int32_t mutex_init(rmw_tickle_mutex_t* lock);
int32_t mutex_term(rmw_tickle_mutex_t* lock);
int32_t mutex_lock(rmw_tickle_mutex_t* lock);
int32_t mutex_unlock(rmw_tickle_mutex_t* lock);

#ifdef __cplusplus
}
#endif
