// Copyright 2024 TickLE Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stddef.h>
#include <unistd.h>

#include <rcutils/logging_macros.h>
#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/rmw.h>

#include "rmw_tickle_c/rmw_tickle.h"

rmw_wait_set_t* rmw_create_wait_set(rmw_context_t* context, size_t max_conditions) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);

    if (strcmp(context->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return NULL;
    }

    // Allocate memory for the wait set
    rmw_tickle_wait_set_t* tickle_wait_set = rmw_allocate(sizeof(rmw_tickle_wait_set_t));
    if (tickle_wait_set == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for wait set");
        return NULL;
    }

    // Initialize the wait set structure
    memset(tickle_wait_set, 0, sizeof(rmw_tickle_wait_set_t));

    tickle_wait_set->node_list_head = ((rmw_tickle_context_impl_t*)context->impl)->node_list_head;

    // Set up the RMW wait set structure (embedded in tickle_wait_set)
    rmw_wait_set_t* rmw_wait_set = &tickle_wait_set->rmw_wait_set;

    rmw_wait_set->implementation_identifier = RMW_TICKLE_IDENTIFIER;
    rmw_wait_set->data = tickle_wait_set;

    // Initialize guard conditions array
    if (max_conditions > 0) {
        tickle_wait_set->guard_conditions = rmw_allocate(sizeof(rmw_tickle_guard_condition_t*) * max_conditions);
        if (tickle_wait_set->guard_conditions == NULL) {
            rmw_free(tickle_wait_set);
            RMW_SET_ERROR_MSG("Failed to allocate memory for guard conditions array");
            return NULL;
        }
        memset(tickle_wait_set->guard_conditions, 0, sizeof(rmw_tickle_guard_condition_t*) * max_conditions);
    }

    tickle_wait_set->guard_condition_count = max_conditions;

    RCUTILS_LOG_DEBUG("Created TickLE wait set with %zu max conditions", max_conditions);
    return rmw_wait_set;
}

rmw_ret_t rmw_destroy_wait_set(rmw_wait_set_t* wait_set) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(wait_set->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_wait_set_t* tickle_wait_set = (rmw_tickle_wait_set_t*)wait_set->data;
    if (tickle_wait_set != NULL) {
        // Free guard conditions array
        if (tickle_wait_set->guard_conditions != NULL) {
            rmw_free(tickle_wait_set->guard_conditions);
        }

        rmw_free(tickle_wait_set);
    }

    RCUTILS_LOG_DEBUG("Destroyed TickLE wait set");
    return RMW_RET_OK;
}

static rmw_tickle_node_t* get_next_node(rmw_tickle_node_t* node) {
    if (node->list_node.next == NULL) {
        return NULL;
    }
    return (rmw_tickle_node_t*)((char*)node->list_node.next - offsetof(rmw_tickle_node_t, list_node));
}

rmw_ret_t rmw_wait(rmw_subscriptions_t* subscriptions, rmw_guard_conditions_t* guard_conditions,
                   rmw_services_t* services, rmw_clients_t* clients, rmw_events_t* events, rmw_wait_set_t* wait_set,
                   const rmw_time_t* wait_timeout) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(wait_set->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_wait_set_t* tickle_wait_set = (rmw_tickle_wait_set_t*)wait_set->data;
    if (tickle_wait_set == NULL) {
        RMW_SET_ERROR_MSG("Wait set data is NULL");
        return RMW_RET_ERROR;
    }

    // Implement a basic wait mechanism using TickLE node polling
    // In a real implementation, this would:
    // 1. Poll all subscriptions for new messages
    // 2. Check guard conditions for triggers
    // 3. Poll services for new requests
    // 4. Poll clients for new responses
    // 5. Handle events
    // 6. Wait for the specified timeout or until something is ready

    static uint64_t count = 0;
    static uint64_t second = 0;
    struct timespec ts;

    ++count;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (second != ts.tv_sec) {
        printf("rmw_wait second=%lu, count=%lu\n", second, count);
        second = ts.tv_sec;
        count = 0;
    }

    struct timespec ts1;
    struct timespec ts2;
    bool timeout = true;
    uint8_t buffer[tt_MAX_BUFFER_LENGTH] = {0, };
    struct rmw_tickle_node_t* node = tickle_wait_set->node_list_head;

    // NOTE: if any packet including node update arrives, timeout is false.
    clock_gettime(CLOCK_REALTIME, &ts1);
    while (node != NULL) {
        if (tt_Node_receive_packet(&node->tickle_node, buffer, tt_MAX_BUFFER_LENGTH) >= 0) {
            timeout = false;
        }
        node = get_next_node(node);
    }
    clock_gettime(CLOCK_REALTIME, &ts2);
    if (subscriptions != NULL) {
        for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
            rmw_tickle_subscriber_t* sub = subscriptions->subscribers[i];
            
            if (ring_buffer_size(&sub->rx_queue) == 0) {
                subscriptions->subscribers[i] = NULL;
            }
        }
    }
    uint64_t elapsed_time = (ts2.tv_sec - ts1.tv_sec) * 1000000000UL + (ts2.tv_nsec - ts1.tv_nsec);
    printf("elapsed_time=%lu us\n", elapsed_time / 1000);
    if (guard_conditions != NULL) {
        guard_conditions->guard_condition_count = 0;
    }
    if (services != NULL) {
        services->service_count = 0;
    }
    if (clients != NULL) {
        clients->client_count = 0;
    }
    if (events != NULL) {
        events->event_count = 0;
    }

    return timeout ? RMW_RET_TIMEOUT : RMW_RET_OK;
}
