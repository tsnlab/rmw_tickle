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

    tickle_wait_set->context = context;
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

rmw_ret_t rmw_wait(rmw_subscriptions_t* subscriptions, rmw_guard_conditions_t* guard_conditions,
                   rmw_services_t* services, rmw_clients_t* clients, rmw_events_t* events, rmw_wait_set_t* wait_set,
                   const rmw_time_t* wait_timeout) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(wait_set->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_wait_set_t* tickle_wait_set = (rmw_tickle_wait_set_t*)wait_set->data;
    uint64_t timeout_ns;
    bool data_available = false;

    if (tickle_wait_set == NULL) {
        RMW_SET_ERROR_MSG("Wait set data is NULL");
        return RMW_RET_ERROR;
    } 
    if (wait_timeout == NULL) {
        timeout_ns = 0;
    } else {
        timeout_ns = wait_timeout->sec * tt_SECOND + wait_timeout->nsec;
    }

    if (subscriptions != NULL) {
        for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
            rmw_tickle_subscriber_t* sub = subscriptions->subscribers[i];

            if (tt_Subscriber_get_takable_count(&sub->tickle_subscriber) > 0) {
                data_available = true;
            }
        }
    }

    // call tt_Node_poll on every node
    if (data_available == false) {
        rmw_tickle_node_t* node_ptr = ((rmw_tickle_context_impl_t*)tickle_wait_set->context->impl)->node_list;
        while (node_ptr != NULL) {
            // NOTE: actual timeout = number of nodes * timeout_ns
            if (tt_Node_poll(&node_ptr->tickle_node) < 0) {
                return RMW_RET_ERROR;
            }
            node_ptr = node_ptr->next;
        }
    }

    if (subscriptions != NULL) {
        for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
            rmw_tickle_subscriber_t* sub = subscriptions->subscribers[i];

            if (tt_Subscriber_get_takable_count(&sub->tickle_subscriber) > 0) {
                data_available = true;
            } else {
                subscriptions->subscribers[i] = NULL;
            }
        }
    }
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

    return data_available ? RMW_RET_OK : RMW_RET_TIMEOUT;
}
