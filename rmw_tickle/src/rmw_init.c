#include <stdio.h>
#include <string.h>

#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw_tickle_c/rmw_tickle.h"

static void* polling_thread_routine(void* arg);

rmw_ret_t rmw_init_options_init(rmw_init_options_t* init_options, rcutils_allocator_t allocator) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(allocator.allocate, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(allocator.deallocate, RMW_RET_INVALID_ARGUMENT);

    init_options->instance_id = 0;
    init_options->domain_id = 0;
    // Initialize security options to zero
    memset(&init_options->security_options, 0, sizeof(rmw_security_options_t));
    init_options->enclave = NULL;
    init_options->allocator = allocator;
    init_options->impl = NULL;

    // Set the implementation identifier
    init_options->implementation_identifier = RMW_TICKLE_IDENTIFIER;

    return RMW_RET_OK;
}

rmw_ret_t rmw_init_options_fini(rmw_init_options_t* init_options) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
    rcutils_allocator_t* allocator = &init_options->allocator;
    RCUTILS_CHECK_ALLOCATOR(allocator, return RMW_RET_INVALID_ARGUMENT);
    if (strcmp(init_options->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }
    return RMW_RET_OK;
}

rmw_ret_t rmw_init_options_copy(const rmw_init_options_t* src, rmw_init_options_t* dst) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(src, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(dst, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(src->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    if (NULL != dst->implementation_identifier) {
        RMW_SET_ERROR_MSG("expected zero-initialized dst");
        return RMW_RET_INVALID_ARGUMENT;
    }

    // Copy the basic structure
    memcpy(dst, src, sizeof(rmw_init_options_t));

    // Copy the enclave string if it exists
    if (src->enclave != NULL) {
        dst->enclave = rcutils_strdup(src->enclave, src->allocator);
        if (NULL == dst->enclave) {
            return RMW_RET_BAD_ALLOC;
        }
    } else {
        dst->enclave = NULL;
    }

    return RMW_RET_OK;
}

rmw_ret_t rmw_init(const rmw_init_options_t* options, rmw_context_t* context) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(options->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Expected implementation identifier to be " RMW_TICKLE_IDENTIFIER);
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    context->instance_id = 0;
    context->implementation_identifier = RMW_TICKLE_IDENTIFIER;
    context->options = *options;

    // Allocate and initialize context implementation
    rmw_tickle_context_impl_t* impl = (rmw_tickle_context_impl_t*)options->allocator.allocate(
        sizeof(rmw_tickle_context_impl_t), options->allocator.state);
    if (impl == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate context implementation");
        return RMW_RET_BAD_ALLOC;
    }

    // Initialize the context implementation
    memset(impl, 0, sizeof(rmw_tickle_context_impl_t));

    // Initialize graph guard condition
    impl->graph_guard_condition.implementation_identifier = RMW_TICKLE_IDENTIFIER;
    impl->graph_guard_condition.data = NULL;
    impl->polling_flag = true;

    mutex_init(&impl->polling_lock);
    thread_create(&impl->polling_thread, polling_thread_routine, impl);

    context->impl = (rmw_context_impl_t*)impl;

    _tt_CONFIG.broadcast = "192.168.10.255";

    return RMW_RET_OK;
}

rmw_ret_t rmw_shutdown(rmw_context_t* context) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(context->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Expected implementation identifier to be " RMW_TICKLE_IDENTIFIER);
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }
    rmw_tickle_context_impl_t* impl = (rmw_tickle_context_impl_t*)context->impl;

    mutex_lock(&impl->polling_lock);
    impl->polling_flag = false;
    mutex_unlock(&impl->polling_lock);
    thread_join(&impl->polling_thread);
    if (mutex_term(&impl->polling_lock) != 0) {
        RCUTILS_LOG_ERROR("Failed to destroy mutex. make sure to unlock before destroy.");
    }
    return RMW_RET_OK;
}

rmw_ret_t rmw_context_fini(rmw_context_t* context) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(context->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Expected implementation identifier to be " RMW_TICKLE_IDENTIFIER);
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    // Free the context implementation
    if (context->impl != NULL) {
        context->options.allocator.deallocate(context->impl, context->options.allocator.state);
        context->impl = NULL;
    }

    // Finalize the init options
    rmw_ret_t ret = rmw_init_options_fini(&context->options);
    if (ret != RMW_RET_OK) {
        return ret;
    }

    return RMW_RET_OK;
}

static rmw_tickle_node_t* get_next_node(rmw_tickle_node_t* node) {
    if (node->list_node.next == NULL) {
        return NULL;
    }
    return (rmw_tickle_node_t*)((char*)node->list_node.next - offsetof(rmw_tickle_node_t, list_node));
}

void* polling_thread_routine(void* arg) {
    rmw_tickle_context_impl_t* impl = arg;
    rmw_tickle_node_t* node;
    uint8_t buffer[tt_MAX_BUFFER_LENGTH];

    while (1) {
        mutex_lock(&impl->polling_lock);
        if (impl->polling_flag == false) {
            mutex_unlock(&impl->polling_lock);
            return NULL;
        }
        node = impl->node_list_head;
        while (node != NULL) {
            mutex_lock(&node->tx_lock);
            tt_Node_peek_scheduler(&node->tickle_node); // Update TickLE Node, Flush Tx buffer periodically
            mutex_unlock(&node->tx_lock);
            tt_Node_receive_packet(&node->tickle_node, buffer, tt_MAX_BUFFER_LENGTH);
            node = get_next_node(node);
        }
        mutex_unlock(&impl->polling_lock);
    }
    return NULL;
}
