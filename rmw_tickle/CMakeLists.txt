cmake_minimum_required(VERSION 3.8)
project(rmw_tickle VERSION 0.0.1)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(rmw REQUIRED)
find_package(rcutils REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_tickle_c REQUIRED)
find_package(tracetools REQUIRED)
find_package(Threads REQUIRED)

if(MEASURE_LATENCY)
  add_compile_options(-DMEASURE_LATENCY)
endif()

# if(NOT DEFINED ENV{TICKLE_PATH} OR "$ENV{TICKLE_PATH}" STREQUAL "")
#   message(FATAL_ERROR "Environment variable TICKLE_PATH is not set or is empty.")
# endif()

# Include directories
include_directories(include)
include_directories(tickle/include)

# Create rmw_tickle_c library
add_library(${PROJECT_NAME} SHARED
  src/rmw_init.c
  src/rmw_get_implementation_identifier.c
  src/rmw_get_serialization_format.c
  src/identifiers.c
  src/rmw_features.c
  src/rmw_node.c
  src/rmw_event.c
  src/rmw_qos.c
  src/rmw_count.c
  src/rmw_gid.c
  src/rmw_service.c
  src/rmw_logging.c
  src/rmw_topic_info.c
  src/rmw_qos_check.c
  src/rmw_network_flow.c
  src/rmw_publisher.c
  src/rmw_subscription.c
  src/rmw_client.c
  src/rmw_serialize.c
  src/rmw_guard_condition.c
  src/rmw_wait_set.c
  src/rmw_event_handling.c
  src/rmw_node_names.c
  src/rmw_topic_names_and_types.c
  src/rmw_publisher_names_and_types.c
  src/rmw_service_names_and_types_by_node.c
  src/rmw_event.c
  src/rmw_dynamic_message.c
  src/rmw_serialization_support.c
  src/rmw_callback.c
  src/thread_pthread.c
  src/ring_buffer.c
  tickle/src/encoding.c
  tickle/src/hal_linux.c
  tickle/src/log.c
  tickle/src/tickle.c
)

# Link libraries
ament_target_dependencies(${PROJECT_NAME}
  rmw
  rcutils
  rosidl_runtime_c
  rosidl_typesupport_tickle_c
  tracetools
)

# Set include directories for the target
# target_include_directories(rmw_tickle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tickle/include)

# Link with pthread and tickle library
target_link_libraries(${PROJECT_NAME}
  Threads::Threads
  rosidl_typesupport_tickle_c::rosidl_typesupport_tickle_c
  tracetools::tracetools
  # $ENV{TICKLE_PATH}/libtickle.a
)

# Export
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(
  rmw
  rcutils
  rosidl_runtime_c
)

configure_rmw_library(${PROJECT_NAME})

# RMW CMake function that receives "<language>:<typesupport1>[:<typesupport2>]"
register_rmw_implementation("c:rosidl_typesupport_c:rosidl_typesupport_tickle_c")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Install
install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

ament_package()
