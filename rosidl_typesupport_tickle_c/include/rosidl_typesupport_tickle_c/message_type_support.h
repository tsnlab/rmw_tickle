// Copyright 2014-2015 Open Source Robotics Foundation, Inc.
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

#ifndef ROSIDL_TYPESUPPORT_TICKLE_C__MESSAGE_TYPE_SUPPORT_H_
#define ROSIDL_TYPESUPPORT_TICKLE_C__MESSAGE_TYPE_SUPPORT_H_

#include "rosidl_runtime_c/message_type_support_struct.h"

typedef struct message_type_support_callbacks_t
{
  const char* message_namespace;
  const char* message_name;

  // size of msg struct without padding
  uint32_t data_size;

  // size of serialized msg
  int32_t (*data_encode_size)(void* data);

  // serialize
  int32_t (*data_encode)(void* data, uint8_t* payload, const int32_t len);

  // deserialize
  int32_t (*data_decode)(void* data, uint8_t* payload, const int32_t len, bool is_native_endian);

  // allocate if necessary
  void* (*data_alloc)(void);

  // deallocate if any
  void (*data_free)(void* data);

  // convert ROS 2 data to TickLE
  int32_t (*convert_data_to_tickle)(void*, void*);

  // convert TickLE data to ROS 2
  int32_t (*convert_data_from_tickle)(void*, void*);

} message_type_support_callbacks_t;

#endif  // ROSIDL_TYPESUPPORT_TICKLE_C__MESSAGE_TYPE_SUPPORT_H_
