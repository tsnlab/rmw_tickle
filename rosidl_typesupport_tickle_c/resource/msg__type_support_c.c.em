@# Included from rosidl_typesupport_tickle_c/resource/idl__type_support_c.c.em
@{
from rosidl_pycommon import convert_camel_case_to_lower_case_underscore
from rosidl_generator_c import idl_structure_type_to_c_typename
from rosidl_generator_type_description import GET_DESCRIPTION_FUNC
from rosidl_generator_type_description import GET_HASH_FUNC
from rosidl_generator_type_description import GET_SOURCES_FUNC
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractNestableType
from rosidl_parser.definition import AbstractGenericString
from rosidl_parser.definition import AbstractSequence
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import BoundedString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import SERVICE_REQUEST_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_RESPONSE_MESSAGE_SUFFIX
from rosidl_parser.definition import SERVICE_EVENT_MESSAGE_SUFFIX
from rosidl_parser.definition import ACTION_FEEDBACK_SUFFIX
from rosidl_parser.definition import ACTION_GOAL_SUFFIX
from rosidl_parser.definition import ACTION_RESULT_SUFFIX
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import UnboundedSequence
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import NamedType
from rosidl_parser.definition import Member

include_parts = [package_name] + list(interface_path.parents[0].parts) + [
    'detail', convert_camel_case_to_lower_case_underscore(interface_path.stem)]
include_base = '/'.join(include_parts)

header_files = [
    'stdint.h',
    'stdbool.h',
    'rosidl_typesupport_tickle_c/identifier.h',
    'rosidl_typesupport_tickle_c/message_type_support.h',
    package_name + '/msg/rosidl_typesupport_tickle_c__visibility_control.h',
    include_base + '__struct.h',
    include_base + '__functions.h',
]

unique_message_identifier = '__'.join(message.structure.namespaced_type.namespaced_name())
message_name = message.structure.namespaced_type.name
message_name_prefix = '__'.join([package_name] + list(interface_path.parents[0].parts))
# tickle_type = tickle__ + unique_message_identifier
tickle_type = unique_message_identifier
}@

@[for header_file in header_files]@
@[    if header_file in include_directives]@
// already included above
@[    else]@
@{include_directives.add(header_file)}@
@[    end if]@
@[    if '/' not in header_file]@
#include <@(header_file)>
@[    else]@
#include "@(header_file)"
@[    end if]@
@[end for]@

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

// includes and forward declarations of message dependencies and their conversion functions

@# // Include the message header for each non-primitive field.
#if defined(__cplusplus)
extern "C"
{
#endif

@{
includes = {}
for member in message.structure.members:
    keys = set([])
    if isinstance(member.type, AbstractSequence) and isinstance(member.type.value_type, BasicType):
        keys.add('rosidl_runtime_c/primitives_sequence.h')
        keys.add('rosidl_runtime_c/primitives_sequence_functions.h')
    type_ = member.type
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, AbstractString):
        keys.add('rosidl_runtime_c/string.h')
        keys.add('rosidl_runtime_c/string_functions.h')
    elif isinstance(type_, AbstractWString):
        keys.add('rosidl_runtime_c/u16string.h')
        keys.add('rosidl_runtime_c/u16string_functions.h')
    elif isinstance(type_, NamespacedType):
        import sys
        if (
            type_.name.endswith(SERVICE_REQUEST_MESSAGE_SUFFIX) or
            type_.name.endswith(SERVICE_RESPONSE_MESSAGE_SUFFIX)
        ):
            continue
        if (
            type_.name.endswith(ACTION_GOAL_SUFFIX) or
            type_.name.endswith(ACTION_RESULT_SUFFIX) or
            type_.name.endswith(ACTION_FEEDBACK_SUFFIX)
        ):
            typename = type_.name.rsplit('_', 1)[0]
        else:
            typename = type_.name
        keys.add('/'.join(type_.namespaces + ['detail', convert_camel_case_to_lower_case_underscore(typename)]) + '__functions.h')
    for key in keys:
        if key not in includes:
            includes[key] = set([])
        includes[key].add(member.name)
}@
@[for header_file in sorted(includes.keys())]@
@[    if header_file in include_directives]@
// already included above
@[    else]@
@{include_directives.add(header_file)}@
@[    end if]@
#include "@(header_file)"  // @(', '.join(sorted(includes[header_file])))
@[end for]@

// forward declare type support functions
@{
forward_declares = {}
for member in message.structure.members:
    type_ = member.type
    if isinstance(type_, AbstractNestedType):
        type_ = type_.value_type
    if isinstance(type_, NamespacedType):
        key = (*type_.namespaces, type_.name)
        if key not in includes:
            forward_declares[key] = set([])
        forward_declares[key].add(member.name)
}@
@[for key in sorted(forward_declares.keys())]@
@[  if key[0] != package_name]@
ROSIDL_TYPESUPPORT_TICKLE_C_IMPORT_@(package_name)
@[  end if]@
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_tickle_c, @(', '.join(key)))();
@[end for]@

@# // Make callback functions specific to this message type.

typedef @(unique_message_identifier) _@(message_name)_type;

@[def get_type_size(member: Member)]@
@{
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import Array
from rosidl_parser.definition import BasicType
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import BoundedString
from rosidl_parser.definition import BoundedWString
from rosidl_parser.definition import NamedType
from rosidl_parser.definition import NamespacedType
from rosidl_generator_c import BASIC_IDL_TYPES_TO_C

class UnsupportedError(Exception):
    pass

type_ = member.type
size = 0
string_type = 0
named_type = 0
if isinstance(type_, AbstractNestedType):
    if isinstance(type_, Array):
        size = type_.size
    elif isinstance(type_, BoundedSequence):
        size = type_.maximum_size
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    type_ = type_.value_type
    if isinstance(type_, AbstractString):
        raise UnsupportedError(f"{type(type_)} not supported")
if isinstance(type_, AbstractString):
    if isinstance(type_, BoundedString):
        string_type = 1
    elif isinstance(type_, BoundedWString):
        string_type = 2
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    size = type_.maximum_size
elif isinstance(type_, NamedType):
    named_type = 1
elif isinstance(type_, NamespacedType):
    named_type = 2
elif isinstance(type_, BasicType):
    pass
else:
    raise UnsupportedError(f"{type(type_)} not supported")
}@
@[    if size > 0]@
@(f"{size} * ")@
@[    end if]@
@[    if named_type == 1]@
@(message_name_prefix)@(type_.name)_callbacks.data_encode_size((struct tt_Data*)&data->@(member.name))@
@[    elif named_type == 2]@
@(type_.namespaced_name())_callbacks.data_encode_size((struct tt_Data*)&data->@(member.name))@
@[    elif string_type == 1]@
sizeof(char)@
@[    elif string_type == 2]@
sizeof(uint16_t)@
@[    else]@
sizeof(@(BASIC_IDL_TYPES_TO_C[type_.typename]))@
@[    end if]@
@[end def]@

@[def generate_encoder_impl(encode_or_decode: str, member: Member)]@
@{  
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractNestableType
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import Array
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import BoundedString
from rosidl_parser.definition import BoundedWString
from rosidl_parser.definition import NamedType
from rosidl_parser.definition import NamespacedType

class UnsupportedError(Exception):
    pass

size = 0
type_ = member.type
if isinstance(type_, AbstractNestedType):
    if isinstance(type_, Array):
        size = type_.size
    elif isinstance(type_, BoundedSequence):
        size = type_.maximum_size
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    type_ = type_.value_type
    if isinstance(type_, AbstractString):
        raise UnsupportedError(f"{type(type_)} not supported")
elif isinstance(type_, AbstractNestableType):
    pass
else:
    raise UnsupportedError(f"{type(type_)} not supported")
}@
@[if isinstance(type_, NamedType) or isinstance(type_, NamespacedType)]@
@[    if isinstance(type_, NamedType)]@
@{        type_name = message_name_prefix + type_.name}@
@[    else]@
@{        type_name = '__'.join(type_.namespaced_name())}@
@[    end if]@
@[    if size > 0]@
    size = @(type_name)_callbacks.data_encode_size((struct tt_Data*)&data->@(member.name)[0]);
    for (int32_t i = 0; i < @(size); ++i) {
        ret = @(type_name)_callbacks.data_@(encode_or_decode)((struct tt_Data*)&data->@(member.name)[i], payload, size@(encode_or_decode == "encode" ? "" ! ", is_native_endian" ));
        @(encode_or_decode)d += ret;
        payload += ret;
    }
@[    else]@
    size = @(type_name)_callbacks.data_encode_size((struct tt_Data*)&data->@(member.name));
    ret = @(type_name)_callbacks.data_@(encode_or_decode)((struct tt_Data*)&data->@(member.name), payload, size@(encode_or_decode == "encode" ? "" ! ", is_native_endian" ));
    @(encode_or_decode)d += ret;
    payload += ret;
@[    end if]@
@[else]@
@[    if encode_or_decode == "encode"]@
    _tt_memcpy(payload, &data->@(member.name), size);
@[    else]@
    _tt_memcpy(&data->@(member.name), payload, size);
@[    end if]@
    @(encode_or_decode)d += size;
    payload += size;
@[end if]@
@[end def]@

@#[def generate_encoder(member: Member)]@
@#(generate_encoder_impl("encode", member))
@#[end def]@

@#[def generate_decoder(member: Member)]@
@#(generate_encoder_impl("decode", member))
@#[end def]@



int32_t encode_size_@(unique_message_identifier)(
    @(tickle_type)* data)
{
    return@
@[for member in message.structure.members]@
@[    if member == message.structure.members[0]]@
@(' ')@
@[    else]@
@('           ')
@[    end if]@
@get_type_size(member)@(member == message.structure.members[-1] ? ';' ! ' +')
@[end for]@
}

int32_t encode_@(unique_message_identifier)(
    @(tickle_type)* data,
    uint8_t* payload,
    const int32_t len)
{
    int32_t encoded = 0;
    int32_t ret;
    int32_t size;

    if (_@(message_name)__encode_size(data) > len) {
        return -1;
    }

@[for member in message.structure.members]@
@# *(@(member.type)*)payload = data->@(member.name);
@[if not (isinstance(type_, NamedType) or isinstance(type_, NamespacedType))]@
    size = @(get_type_size(member));
@[end if]@
@(generate_encoder_impl("encode", member))
@[end for]@
    return encoded;
}

int32_t decode_@(unique_message_identifier)(
    @(tickle_type)* data,
    uint8_t* payload,
    const int32_t len,
    bool is_native_endian)
{
    int32_t decoded = 0;
    int32_t ret;
    int32_t size;

    if (_@(message_name)__encode_size(data) > len) {
        return -1;
    }

@[for member in message.structure.members]@
@[if not (isinstance(type_, NamedType) or isinstance(type_, NamespacedType))]@
    size = @(get_type_size(member));
@[end if]@
@(generate_encoder_impl("decode", member))
@[end for]@
    return decoded;
}

void free_@(unique_message_identifier)(
    @(tickle_type)* data)
{
    (void)data;
}

int32_t convert_to_tickle_from_@(unique_message_identifier)(
    @(tickle_type)* to,
    void* from)
{
    return 0;
}

int32_t convert_from_tickle_to_@(unique_message_identifier)(
    void* to,
    @(tickle_type)* from)
{
    return 0;
}




@# // Collect the callback functions and provide a function to get the type support struct.

message_type_support_callbacks_t @(unique_message_identifier)_callbacks = {
  "@(message_name_prefix)",
  "@(message_name)",
  sizeof(struct @(tickle_type)),
  encode_size_@(unique_message_identifier),
  encode_@(unique_message_identifier),
  decode_@(unique_message_identifier),
  free_@(unique_message_identifier),
  convert_to_tickle_from_@(unique_message_identifier),
  convert_from_tickle_to_@(unique_message_identifier),
};

static rosidl_message_type_support_t _@(message_name)__type_support = {
  ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER,
  &@(unique_message_identifier)_callbacks,
  get_message_typesupport_handle_function,

  &@(idl_structure_type_to_c_typename(message.structure.namespaced_type))__@(GET_HASH_FUNC),
  &@(idl_structure_type_to_c_typename(message.structure.namespaced_type))__@(GET_DESCRIPTION_FUNC),
  &@(idl_structure_type_to_c_typename(message.structure.namespaced_type))__@(GET_SOURCES_FUNC),
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_tickle_c, @(', '.join([package_name] + list(interface_path.parents[0].parts) + [message.structure.namespaced_type.name])))() {
  return &_@(message_name)__type_support;
}

#if defined(__cplusplus)
}
#endif
