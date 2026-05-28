#include <rmw_tickle_c/thread.h>

int32_t thread_create(rmw_tickle_thread_t* thread, void* (*routine)(void*), void* arg) {
    return pthread_create(&thread->thread, NULL, routine, arg);
}

int32_t thread_join(rmw_tickle_thread_t* thread) {
    return pthread_join(thread->thread, NULL);
}

int32_t thread_sleep(uint32_t nanoseconds) {
    return usleep(nanoseconds / 1000);
}

int32_t mutex_init(rmw_tickle_mutex_t* lock) {
    (void)lock;
    return 0;
}

int32_t mutex_term(rmw_tickle_mutex_t* lock) {
    (void)lock;
    return 0;
}

int32_t mutex_lock(rmw_tickle_mutex_t* lock) {
    (void)lock;
    return 0;
}

int32_t mutex_unlock(rmw_tickle_mutex_t* lock) {
    (void)lock;
    return 0;
}
