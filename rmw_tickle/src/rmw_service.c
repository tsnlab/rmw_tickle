// Copyright 2024 TickLE RMW Implementation
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
#include <rmw/error_handling.h>
#include <rosidl_runtime_c/service_type_support_struct.h>
#include <rosidl_typesupport_tickle_c/service_type_support.h>
#include <rosidl_typesupport_tickle_c/message_type_support.h>
#include <rosidl_typesupport_tickle_c/identifier.h>

#include <rmw_tickle_c/rmw_tickle.h>

rmw_service_t* rmw_create_service(const rmw_node_t* node, const rosidl_service_type_support_t* type_support,
                                  const char* service_name, const rmw_qos_profile_t* qos_policies) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(type_support, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(service_name, NULL);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(qos_policies, NULL);

    rmw_tickle_service_t* rmw_tickle_service = NULL;
    struct tt_Service* service = NULL;
    rmw_tickle_node_t* rmw_tickle_node = (rmw_tickle_node_t*)node->data;

    if (strcmp(node->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return NULL;
    }

    // Get type support handler from type support library
    const rosidl_service_type_support_t* type_support_handle = get_service_typesupport_handle(
        type_support, ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER); 
    if (type_support_handle == NULL) {
        RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to get type support handler for service \"%s\"", service_name);
        return NULL;
    }
    const service_type_support_callbacks_t* type_support_callbacks = type_support_handle->data;
    const rosidl_message_type_support_t* req_type_support_handle = get_message_typesupport_handle(
        type_support_callbacks->request_members(), ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER);
    if (req_type_support_handle == NULL) {
        RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to get type support handler for request of service \"%s\"", service_name);
        return NULL;
    }
    const rosidl_message_type_support_t* res_type_support_handle = get_message_typesupport_handle(
        type_support_callbacks->response_members(), ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER);
    if (res_type_support_handle == NULL) {
        RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to get type support handler for response of service \"%s\"", service_name);
        return NULL;
    }

    // Allocate memory for the service
    rmw_tickle_service = rmw_tickle_node->allocator.zero_allocate(1, sizeof(rmw_tickle_service_t), rmw_tickle_node->allocator.state);
    if (rmw_tickle_service == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for service");
        goto fail;
    }
    // Store node and type support references
    rmw_tickle_service->node = rmw_tickle_node;
    rmw_tickle_service->type_support = type_support;

    // Set up the RMW service structure (embedded in rmw_tickle_service)
    rmw_service_t* rmw_service = &rmw_tickle_service->rmw_service;
    rmw_service->implementation_identifier = RMW_TICKLE_IDENTIFIER;
    rmw_service->data = rmw_tickle_service;
    // NOTE: uses service name argument unlike rmw_publisher that uses strdup-ed topic name
    rmw_service->service_name = service_name;

    // Initialize TickLE server
    // Create a dummy service for now - in a real implementation, this would be created based on type_support
    service = rmw_tickle_node->allocator.zero_allocate(1, sizeof(struct tt_Service), rmw_tickle_node->allocator.state);
    if (service == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE service");
        goto fail;
    }

    // Initialize service with basic information
    const message_type_support_callbacks_t* request_callbacks = req_type_support_handle->data;
    const message_type_support_callbacks_t* response_callbacks = res_type_support_handle->data;
    service->name = service_name;
    service->request_size = request_callbacks->data_size;
    service->request_encode_size = (tt_REQUEST_ENCODE_SIZE)request_callbacks->data_encode_size;
    service->request_encode = (tt_REQUEST_ENCODE)request_callbacks->data_encode;
    service->request_decode = (tt_REQUEST_DECODE)request_callbacks->data_decode;
    service->request_free = (tt_REQUEST_FREE)request_callbacks->data_free;

    service->response_size = response_callbacks->data_size;
    service->response_encode_size = (tt_RESPONSE_ENCODE_SIZE)response_callbacks->data_encode_size;
    service->response_encode = (tt_RESPONSE_ENCODE)response_callbacks->data_encode;
    service->response_decode = (tt_RESPONSE_DECODE)response_callbacks->data_decode;
    service->response_free = (tt_RESPONSE_FREE)response_callbacks->data_free;

    service->call_retry_interval = 0;
    service->call_retry_count = 0;

    // Create a dummy callback for now
    // In a real implementation, this would handle incoming service requests
    tt_SERVER_CALLBACK callback = NULL; // We'll handle requests in rmw_take_request instead

    int32_t result = tt_Node_create_server(&rmw_tickle_service->node->tickle_node, &rmw_tickle_service->tickle_server, service,
                                           service_name, callback);
    if (result != 0) {
        RMW_SET_ERROR_MSG("Failed to create TickLE server");
        goto fail;
    }

    // Store service reference for later use
    rmw_tickle_service->tickle_server.service = service;

    RCUTILS_LOG_DEBUG("Created TickLE service: %s", service_name);
    return rmw_service;
fail:
    if (service != NULL) {
        rmw_tickle_node->allocator.deallocate(service, rmw_tickle_node->allocator.state);
    }
    if (rmw_tickle_service != NULL) {
        rmw_tickle_node->allocator.deallocate(rmw_tickle_service, rmw_tickle_node->allocator.state);
    }
    return NULL;
}

rmw_ret_t rmw_destroy_service(rmw_node_t* node, rmw_service_t* service) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(service->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_service_t* rmw_tickle_service = (rmw_tickle_service_t*)service->data;
    if (rmw_tickle_service != NULL) {
        // Destroy TickLE server
        int32_t result = tt_Server_destroy(&rmw_tickle_service->tickle_server);
        if (result != 0) {
            RCUTILS_LOG_WARN("Failed to destroy TickLE server, error code: %d", result);
        }

        // Free the service if it was allocated
        if (rmw_tickle_service->tickle_server.service != NULL) {
            free(rmw_tickle_service->tickle_server.service);
        }

        free(rmw_tickle_service);
    }

    RCUTILS_LOG_INFO("Destroyed TickLE service: %s", service->service_name);
    return RMW_RET_OK;
}

rmw_ret_t rmw_take_request(const rmw_service_t* service, rmw_service_info_t* request_header, void* ros_request,
                           bool* taken) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(service->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_service_t* rmw_tickle_service = (rmw_tickle_service_t*)service->data;
    if (rmw_tickle_service == NULL) {
        RMW_SET_ERROR_MSG("Service data is NULL");
        return RMW_RET_ERROR;
    }

    // For now, we'll simulate request reception
    // In a real implementation, this would poll the TickLE node for incoming service requests
    // and deserialize them into the ros_request buffer

    // Check if there are any requests available
    // This is a simplified implementation - in reality, we would need to:
    // 1. Poll the TickLE node for new service requests
    // 2. Check if any requests match this service
    // 3. Deserialize the request data into ros_request

    // For now, we'll just return no request available
    *taken = false;

    RCUTILS_LOG_DEBUG("rmw_take_request: No requests available (simplified implementation)");
    return RMW_RET_OK;
}

rmw_ret_t rmw_send_response(const rmw_service_t* service, rmw_request_id_t* request_id, void* ros_response) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(request_id, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(service->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    rmw_tickle_service_t* rmw_tickle_service = (rmw_tickle_service_t*)service->data;
    if (rmw_tickle_service == NULL) {
        RMW_SET_ERROR_MSG("Service data is NULL");
        return RMW_RET_ERROR;
    }

    // For now, we'll simulate response sending
    // In a real implementation, this would serialize the ros_response
    // and send it back to the client using the TickLE API

    // Create a dummy response structure for TickLE
    // In a real implementation, this would serialize the ROS response
    struct tt_Response* response = malloc(sizeof(struct tt_Response));
    if (response == NULL) {
        RMW_SET_ERROR_MSG("Failed to allocate memory for TickLE response");
        return RMW_RET_ERROR;
    }

    // For now, we'll just simulate sending the response
    // In a complete implementation, we would serialize the ros_response here
    // and use the TickLE API to send it back to the client

    // Free the response structure
    free(response);

    RCUTILS_LOG_DEBUG("Successfully sent service response via TickLE (simplified implementation)");
    return RMW_RET_OK;
}

rmw_ret_t rmw_service_server_is_available(const rmw_node_t* node, const rmw_client_t* client, bool* is_available) {
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
    RCUTILS_CHECK_ARGUMENT_FOR_NULL(is_available, RMW_RET_INVALID_ARGUMENT);

    if (strcmp(node->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match for node");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    if (strcmp(client->implementation_identifier, RMW_TICKLE_IDENTIFIER) != 0) {
        RMW_SET_ERROR_MSG("Implementation identifiers does not match for client");
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
    }

    // For now, we don't have service availability checking in TickLE
    *is_available = false;
    printf("rmw_service_server_is_available: returning false (not implemented for TickLE)\n");
    return RMW_RET_OK;
}
