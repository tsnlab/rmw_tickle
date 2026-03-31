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

#include <stdint.h>
#include <stdlib.h>

#include <rcutils/allocator.h>
#include <rcutils/logging_macros.h>
#include <rcutils/strdup.h>
#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/rmw.h>
#include <rosidl_runtime_c/message_type_support_struct.h>
#include <rosidl_typesupport_tickle_c/message_type_support.h>
#include <rosidl_typesupport_tickle_c/identifier.h>
#include <tickle/tickle.h>

#include "rmw_tickle_c/rmw_tickle.h"

rmw_ret_t rmw_init_publisher_allocation(const rosidl_message_type_support_t* type_support,
                                        const rosidl_runtime_c__Sequence__bound* message_bounds,
                                        rmw_publisher_allocation_t* allocation) {
    (void)type_support;
    (void)message_bounds;
    (void)allocation;
    RCUTILS_LOG_DEBUG("function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_fini_publisher_allocation(rmw_publisher_allocation_t* allocation) {
    (void)allocation;
    RCUTILS_LOG_DEBUG("function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_publisher_t* rmw_create_publisher(const rmw_node_t* node, const rosidl_message_type_support_t* type_support,
                                      const char* topic_name, const rmw_qos_profile_t* qos_policies,
                                      const rmw_publisher_options_t* publisher_options) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(type_support, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_name, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(qos_policies, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher_options, NULL);

    if (strcmp(node->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return NULL;
    }

    // Get type support handler from type support library
    const rosidl_message_type_support_t* type_support_handle = get_message_typesupport_handle(
        type_support, ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER); 
    if (type_support_handle == NULL) {
        RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to get type support handler for topic \"%s\"", topic_name);
        return NULL;
    }

    rmw_tickle_node_t* tickle_node = (rmw_tickle_node_t*)node->data;

    // Allocate memory for the publisher
    rmw_tickle_publisher_t* tickle_publisher = malloc(sizeof(rmw_tickle_publisher_t));
    if (tickle_publisher == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for publisher");
        goto fail_pub_create;
    }

    // Initialize the publisher structure
    memset(tickle_publisher, 0, sizeof(rmw_tickle_publisher_t));

    // Set up the RMW publisher structure (embedded in tickle_publisher)
    rmw_publisher_t* rmw_publisher = &tickle_publisher->rmw_publisher;
    char* allocated_topic_name;

    allocated_topic_name = rcutils_strdup(topic_name, tickle_node->allocator);
    if (allocated_topic_name == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE RMW topic name");
        goto fail_topic_name_alloc;
    }
    rmw_publisher->implementation_identifier = RMW_TICKLE_IDENTIFIER;
    rmw_publisher->data = tickle_publisher;
    rmw_publisher->topic_name = allocated_topic_name;
    rmw_publisher->options = *publisher_options;
    rmw_publisher->can_loan_messages = false;

    // Store node and type support references
    tickle_publisher->node = (rmw_tickle_node_t*)node->data;
    tickle_publisher->type_support = type_support_handle;

    // Initialize TickLE publisher
    // TODO: Create a dummy topic for now - in a real implementation, this would be created based on type_support
    struct tt_Topic* topic = malloc(sizeof(struct tt_Topic));
    if (topic == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE topic");
        goto fail_topic_alloc;
    }

    // Initialize topic with basic information and type support callbacks
    const message_type_support_callbacks_t* type_support_callbacks = type_support_handle->data;
    topic->data_size = type_support_callbacks->data_size;
    topic->data_encode_size = (tt_DATA_ENCODE_SIZE)type_support_callbacks->data_encode_size;
    topic->data_encode = (tt_DATA_ENCODE)type_support_callbacks->data_encode;
    topic->data_decode = (tt_DATA_DECODE)type_support_callbacks->data_decode;
//  topic->data_alloc = type_support_callbacks->alloc;
    topic->data_free = (tt_DATA_FREE)type_support_callbacks->data_free;

    topic->name = allocated_topic_name;
    topic->history_depth = 10; // Default QoS
    topic->deadline_duration = 0;
    topic->lifespan_duration = 0;

    int32_t result = tt_Node_create_publisher(&tickle_publisher->node->tickle_node, &tickle_publisher->tickle_publisher,
                                              topic, allocated_topic_name);
    if (result != 0) {
        RMW_SET_ERROR_MSG("Failed to create TickLE publisher");
        goto fail_tickle_pub_create;
    }

    // Store topic reference for later use
    tickle_publisher->tickle_publisher.topic = topic;

    RCUTILS_LOG_DEBUG("Created TickLE publisher for topic: %s", allocated_topic_name);

    return rmw_publisher;
fail_tickle_pub_create:
    free(topic);
fail_topic_alloc:
    free(allocated_topic_name);
fail_topic_name_alloc:
    free(tickle_publisher);
fail_pub_create:
    return NULL;
}

rmw_ret_t rmw_destroy_publisher(rmw_node_t* node, rmw_publisher_t* publisher) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(publisher->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }
    RCUTILS_LOG_DEBUG("Destroying TickLE publisher for topic: %s", publisher->topic_name);

    rmw_tickle_node_t* tickle_node = (rmw_tickle_node_t*)node->data;
    rmw_tickle_publisher_t* tickle_publisher = (rmw_tickle_publisher_t*)publisher->data;
    if (tickle_publisher != NULL) {
        // Destroy TickLE publisher
        int32_t result = tt_Publisher_destroy(&tickle_publisher->tickle_publisher);
        if (result != 0) {
            RCUTILS_LOG_WARN("Failed to destroy TickLE publisher, error code: %d", result);
        }

        // Free the topic if it was allocated
        if (tickle_publisher->tickle_publisher.topic != NULL) {
            tickle_node->allocator.deallocate(tickle_publisher->tickle_publisher.topic->name, tickle_node->allocator.state);
            free(tickle_publisher->tickle_publisher.topic);
        }

        free(tickle_publisher);
    }

    return RMW_RET_OK;
}

rmw_ret_t rmw_publish(const rmw_publisher_t* publisher, const void* ros_message,
                      rmw_publisher_allocation_t* allocation) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
    (void)allocation; // Not used in this implementation

    if (strcmp(publisher->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_publisher_t* tickle_publisher = (rmw_tickle_publisher_t*)publisher->data;
    if (tickle_publisher == NULL) {
        RMW_SET_ERROR_MSG("Publisher data is NULL");
        return RMW_RET_ERROR;
    }

    const message_type_support_callbacks_t* type_support_callbacks = tickle_publisher->type_support->data;

    // Create a data structure for TickLE
    /*
    // NOTE: currently using malloc instead of RMW allocator 
    tickle_data = type_support_callbacks->data_alloc();
    if (tickle_data == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE data");
        return RMW_RET_ERROR;
    }
    type_support_callbacks->convert_data_to_tickle(tickle_data, ros_message);
    int32_t result = tt_Publisher_publish(&tickle_publisher->tickle_publisher, tickle_data);
    // Note: We don't free data here since it's pointing to the tickle_data
    */

    // Publish data through TickLE.
    // 
    int32_t result = tt_Publisher_publish(&tickle_publisher->tickle_publisher, (struct tt_Data*)ros_message);

    if (result != 0) {
        RMW_SET_ERROR_MSG("Failed to publish message via TickLE");
        return RMW_RET_ERROR;
    }

    tt_Node_flush(&tickle_publisher->node->tickle_node);

    RCUTILS_LOG_DEBUG("Successfully published message via TickLE");
    return RMW_RET_OK;
}

rmw_ret_t rmw_publish_loaned_message(const rmw_publisher_t* publisher, void* ros_message,
                                     rmw_publisher_allocation_t* allocation) {
    (void)publisher;
    (void)ros_message;
    (void)allocation;

    RCUTILS_LOG_DEBUG("rmw_publish_loaned_message: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publish_serialized_message(const rmw_publisher_t* publisher,
                                         const rmw_serialized_message_t* serialized_message,
                                         rmw_publisher_allocation_t* allocation) {
    (void)publisher;
    (void)serialized_message;
    (void)allocation;

    RCUTILS_LOG_DEBUG("rmw_publish_serialized_message: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_borrow_loaned_message(const rmw_publisher_t* publisher, const rosidl_message_type_support_t* type_support,
                                    void** ros_message) {
    (void)publisher;
    (void)type_support;
    (void)ros_message;

    RCUTILS_LOG_DEBUG("rmw_borrow_loaned_message: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_publisher(const rmw_publisher_t* publisher, void* loaned_message) {
    (void)publisher;
    (void)loaned_message;

    RCUTILS_LOG_DEBUG("rmw_return_loaned_message_from_publisher: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_count_matched_subscriptions(const rmw_publisher_t* publisher, size_t* subscription_count) {
    (void)publisher;
    (void)subscription_count;

    RCUTILS_LOG_DEBUG("rmw_publisher_count_matched_subscriptions: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_event_init(rmw_event_t* event, const rmw_publisher_t* publisher, rmw_event_type_t event_type) {
    (void)event;
    (void)publisher;
    (void)event_type;

    RCUTILS_LOG_DEBUG("rmw_publisher_event_init: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_assert_liveliness(const rmw_publisher_t* publisher) {
    (void)publisher;

    RCUTILS_LOG_DEBUG("rmw_publisher_assert_liveliness: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_publisher_wait_for_all_acked(const rmw_publisher_t* publisher, rmw_time_t wait_timeout) {
    (void)publisher;
    (void)wait_timeout;

    RCUTILS_LOG_DEBUG("rmw_publisher_wait_for_all_acked: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}
