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

#include <rcutils/logging_macros.h>
#include <rcutils/strdup.h>
#include <rmw/error_handling.h>
#include <rosidl_runtime_c/message_type_support_struct.h>
#include <rosidl_typesupport_tickle_c/message_type_support.h>
#include <rosidl_typesupport_tickle_c/identifier.h>

#include <rmw_tickle_c/rmw_tickle.h>

#ifdef MEASURE_LATENCY
#include <tracetools/tracetools.h>
#endif

rmw_ret_t rmw_init_subscription_allocation(const rosidl_message_type_support_t* type_support,
                                           const rosidl_runtime_c__Sequence__bound* message_bounds,
                                           rmw_subscription_allocation_t* allocation) {
    (void)type_support;
    (void)message_bounds;
    (void)allocation;
    RCUTILS_LOG_DEBUG("function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_fini_subscription_allocation(rmw_subscription_allocation_t* allocation) {
    (void)allocation;
    RCUTILS_LOG_DEBUG("function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t* rmw_create_subscription(const rmw_node_t* node, const rosidl_message_type_support_t* type_support,
                                            const char* topic_name, const rmw_qos_profile_t* qos_policies,
                                            const rmw_subscription_options_t* subscription_options) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(type_support, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_name, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(qos_policies, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscription_options, NULL);

    struct tt_Topic* topic = NULL;
    rmw_tickle_subscriber_t* rmw_tickle_subscriber = NULL;
    rmw_tickle_node_t* rmw_tickle_node = (rmw_tickle_node_t*)node->data;

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

    // Allocate memory for the subscription
    rmw_tickle_subscriber = rmw_tickle_node->allocator.zero_allocate(1, sizeof(rmw_tickle_subscriber_t), rmw_tickle_node->allocator.state);
    if (rmw_tickle_subscriber == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for subscription");
        goto fail;
    }
    // Store node and type support references
    rmw_tickle_subscriber->node = rmw_tickle_node;
    rmw_tickle_subscriber->type_support = type_support_handle;

    char* allocated_topic_name = rcutils_strdup(topic_name, rmw_tickle_node->allocator);
    if (allocated_topic_name == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE RMW topic name");
        goto fail;
    }

    // Set up the RMW subscription structure (embedded in tickle_subscriber)
    rmw_subscription_t* rmw_subscription = &rmw_tickle_subscriber->rmw_subscription;
    rmw_subscription->implementation_identifier = RMW_TICKLE_IDENTIFIER;
    rmw_subscription->data = rmw_tickle_subscriber;
    rmw_subscription->topic_name = allocated_topic_name;
    rmw_subscription->options = *subscription_options;
    rmw_subscription->can_loan_messages = false;

    // Initialize TickLE subscriber
    topic = rmw_tickle_node->allocator.zero_allocate(1, sizeof(struct tt_Topic), rmw_tickle_node->allocator.state);
    if (topic == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE topic");
        goto fail;
    }

    // Initialize topic with basic information and type support callbacks
    const message_type_support_callbacks_t* type_support_callbacks = type_support_handle->data;
    topic->data_size = type_support_callbacks->data_size;
    topic->data_encode_size = (tt_DATA_ENCODE_SIZE)type_support_callbacks->data_encode_size;
    topic->data_encode = (tt_DATA_ENCODE)type_support_callbacks->data_encode;
    topic->data_decode = (tt_DATA_DECODE)type_support_callbacks->data_decode;
    topic->data_free = (tt_DATA_FREE)type_support_callbacks->data_free;

    topic->name = allocated_topic_name;
    topic->history_depth = 10; // Default QoS
    topic->deadline_duration = 0;
    topic->lifespan_duration = 0;

    tt_SUBSCRIBER_CALLBACK callback = NULL; // We'll handle messages in rmw_take instead

    RCUTILS_LOG_DEBUG("%s :topic_name=%s", __func__, topic_name);
    int32_t result = tt_Node_create_subscriber(&rmw_tickle_subscriber->node->tickle_node,
                                               &rmw_tickle_subscriber->tickle_subscriber, topic, topic_name, callback);
    if (result != 0) {
        RMW_SET_ERROR_MSG("Failed to create TickLE subscriber");
        goto fail;
    }

    // Store topic reference for later use
    rmw_tickle_subscriber->tickle_subscriber.topic = topic;
    RCUTILS_LOG_DEBUG("Created TickLE subscription for topic: %s", topic_name);
    return rmw_subscription;
fail:
    if (topic != NULL) {
        if (topic->name != NULL) {
            rmw_tickle_node->allocator.deallocate((void*)topic->name, rmw_tickle_node->allocator.state);
        }
        rmw_tickle_node->allocator.deallocate(topic, rmw_tickle_node->allocator.state);
    }
    if (rmw_tickle_subscriber != NULL) {
        rmw_tickle_node->allocator.deallocate(rmw_tickle_subscriber, rmw_tickle_node->allocator.state);
    }
    return NULL;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t* node, rmw_subscription_t* subscription) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(subscription->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }
    RCUTILS_LOG_DEBUG("Destroyed TickLE subscription for topic: %s", subscription->topic_name);

    rmw_tickle_node_t* rmw_tickle_node = (rmw_tickle_node_t*)node->data;
    rmw_tickle_subscriber_t* rmw_tickle_subscriber = (rmw_tickle_subscriber_t*)subscription->data;
    if (rmw_tickle_subscriber != NULL) {
        // Destroy TickLE subscriber
        int32_t result = tt_Subscriber_destroy(&rmw_tickle_subscriber->tickle_subscriber);
        if (result != 0) {
            RCUTILS_LOG_WARN("Failed to destroy TickLE subscriber, error code: %d", result);
        }

        // Free the topic if it was allocated
        if (rmw_tickle_subscriber->tickle_subscriber.topic != NULL) {
            rmw_tickle_node->allocator.deallocate((void*)rmw_tickle_subscriber->tickle_subscriber.topic->name, rmw_tickle_node->allocator.state);
            rmw_tickle_node->allocator.deallocate(rmw_tickle_subscriber->tickle_subscriber.topic, rmw_tickle_node->allocator.state);
        }
        rmw_tickle_node->allocator.deallocate(rmw_tickle_subscriber, rmw_tickle_node->allocator.state);
    }

    return RMW_RET_OK;
}

rmw_ret_t rmw_take_internal(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                             rmw_message_info_t* message_info, rmw_subscription_allocation_t* allocation) {
    (void)allocation; // Not used in this implementation
    uint32_t ip = 0;
    uint16_t port = 0;
    uint64_t source_timestamp = 0;

    if (strcmp(subscription->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_subscriber_t* rmw_tickle_subscriber = (rmw_tickle_subscriber_t*)subscription->data;
    if (rmw_tickle_subscriber == NULL) {
        RMW_SET_ERROR_MSG("Subscription data is NULL");
        return RMW_RET_ERROR;
    }

    // Poll the TickLE node for incoming messages
    // In a real implementation, this would check for new messages from the network
    *taken = false;
    if (tt_Subscriber_take(&rmw_tickle_subscriber->tickle_subscriber, ros_message, &source_timestamp) == false) {
        return RMW_RET_OK; // Timeout
    }
    *taken = true;

    // TODO: message ordering and QoS
    if (message_info) {
        message_info->source_timestamp = source_timestamp;
    }
#ifdef MEASURE_LATENCY
    TRACETOOLS_TRACEPOINT(
        rmw_take,
        (void *)subscription,
        (void *)ros_message,
        source_timestamp,
        *taken);
#endif
    return RMW_RET_OK;
}

rmw_ret_t rmw_take_with_info(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                             rmw_message_info_t* message_info, rmw_subscription_allocation_t* allocation) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(message_info, RMW_RET_INVALID_ARGUMENT);
    return rmw_take_internal(subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t rmw_take(const rmw_subscription_t* subscription, void* ros_message, bool* taken,
                   rmw_subscription_allocation_t* allocation) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);
    return rmw_take_internal(subscription, ros_message, taken, NULL, allocation);
}

rmw_ret_t rmw_take_serialized_message(const rmw_subscription_t* subscription,
                                      rmw_serialized_message_t* serialized_message, bool* taken,
                                      rmw_subscription_allocation_t* allocation) {
    (void)subscription;
    (void)serialized_message;
    (void)allocation;

    if (taken != NULL) {
        *taken = false;
    }

    RCUTILS_LOG_DEBUG("rmw_take_serialized_message: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_serialized_message_with_info(const rmw_subscription_t* subscription,
                                                rmw_serialized_message_t* serialized_message, bool* taken,
                                                rmw_message_info_t* message_info,
                                                rmw_subscription_allocation_t* allocation) {
    (void)subscription;
    (void)serialized_message;
    (void)taken;
    (void)message_info;
    (void)allocation;

    RCUTILS_LOG_DEBUG("rmw_take_serialized_message_with_info: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message(const rmw_subscription_t* subscription, void** ros_message, bool* taken,
                                  rmw_subscription_allocation_t* allocation) {
    (void)subscription;
    (void)ros_message;
    (void)taken;
    (void)allocation;

    RCUTILS_LOG_DEBUG("rmw_take_loaned_message: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message_with_info(const rmw_subscription_t* subscription, void** ros_message, bool* taken,
                                            rmw_message_info_t* message_info,
                                            rmw_subscription_allocation_t* allocation) {
    (void)subscription;
    (void)ros_message;
    (void)taken;
    (void)message_info;
    (void)allocation;

    RCUTILS_LOG_DEBUG("rmw_take_loaned_message_with_info: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_subscription(const rmw_subscription_t* subscription, void* loaned_message) {
    (void)subscription;
    (void)loaned_message;

    RCUTILS_LOG_DEBUG("rmw_return_loaned_message_from_subscription: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_count_matched_publishers(const rmw_subscription_t* subscription, size_t* publisher_count) {
    (void)subscription;
    (void)publisher_count;

    RCUTILS_LOG_DEBUG("rmw_subscription_count_matched_publishers: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_event_init(rmw_event_t* event, const rmw_subscription_t* subscription,
                                      rmw_event_type_t event_type) {
    (void)event;
    (void)subscription;
    (void)event_type;

    RCUTILS_LOG_DEBUG("rmw_subscription_event_init: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_set_content_filter(rmw_subscription_t* subscription,
                                              const rmw_subscription_content_filter_options_t* options) {
    (void)subscription;
    (void)options;

    RCUTILS_LOG_DEBUG("rmw_subscription_set_content_filter: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_get_content_filter(const rmw_subscription_t* subscription, rcutils_allocator_t* allocator,
                                              rmw_subscription_content_filter_options_t* options) {
    (void)subscription;
    (void)allocator;
    (void)options;

    RCUTILS_LOG_DEBUG("rmw_subscription_get_content_filter: function not implemented for TickLE");
    return RMW_RET_UNSUPPORTED;
}
