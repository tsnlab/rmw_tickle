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
    'string.h',
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
tickle_type = f"struct {unique_message_identifier}"

class UnsupportedError(Exception):
        pass
  
def get_type_size(member: Member) -> str:
    from rosidl_parser.definition import AbstractNestedType
    from rosidl_parser.definition import AbstractString
    from rosidl_parser.definition import AbstractGenericString
    from rosidl_parser.definition import AbstractWString
    from rosidl_parser.definition import Array
    from rosidl_parser.definition import BasicType
    from rosidl_parser.definition import BoundedSequence
    from rosidl_parser.definition import BoundedString
    from rosidl_parser.definition import BoundedWString
    from rosidl_parser.definition import NamedType
    from rosidl_parser.definition import NamespacedType
    from rosidl_parser.definition import UnboundedSequence
    from rosidl_parser.definition import UnboundedString
    from rosidl_parser.definition import UnboundedWString
    from rosidl_generator_c import BASIC_IDL_TYPES_TO_C

    type_ = member.type
    name = member.name
    type_str_prefix = ""
    type_str = ""
    size = 0
    nested_type = 0
    if isinstance(type_, AbstractNestedType):
        if isinstance(type_, Array):
            nested_type = 1
            size = type_.size
        elif isinstance(type_, BoundedSequence):
            nested_type = 2
            size = type_.maximum_size
        elif isinstance(type_, UnboundedSequence):
            nested_type = 2
            size = f"data->{name}.size"
            #raise UnsupportedError(f"{type(type_)} not supported")
        else:
            raise UnsupportedError(f"{type(type_)} not supported")
        type_ = type_.value_type
        type_str_prefix += f"{size} * "

    if isinstance(type_, AbstractGenericString):
        # TODO: compare capacity and size
        str_size = ""
        if nested_type == 1:
            type_str_prefix += f"data->{name}[0].size"
        elif nested_type == 2:
            type_str_prefix += f"data->{name}.data[0].size"
        else:
            type_str_prefix += f"data->{name}.size"

        if isinstance(type_, AbstractString):
            type_str_prefix += " * sizeof(char)"
        elif isinstance(type_, AbstractWString):
            type_str_prefix += " * sizeof(uint16_t)"

    elif isinstance(type_, NamedType):
        type_str += f"{message_name_prefix}{type_.name}__callbacks.data_encode_size((struct tt_Data*)&data->@({name}))"
    elif isinstance(type_, NamespacedType):
        type_str += f"{'__'.join(type_.namespaced_name())}__callbacks.data_encode_size((struct tt_Data*)&data->{name})"
    elif isinstance(type_, BasicType):
        type_str += f"sizeof({BASIC_IDL_TYPES_TO_C[type_.typename]})"
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    return type_str_prefix + type_str
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
        keys.add('/'.join(type_.namespaces + ['detail', convert_camel_case_to_lower_case_underscore(typename)]) + '__rosidl_typesupport_tickle_c.h')
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

@[def generate_encoder(encode_or_decode: str, member: Member, get_type_size)]@
@{  
from rosidl_parser.definition import AbstractNestedType
from rosidl_parser.definition import AbstractNestableType
from rosidl_parser.definition import AbstractString
from rosidl_parser.definition import AbstractWString
from rosidl_parser.definition import Array
from rosidl_parser.definition import BoundedSequence
from rosidl_parser.definition import BoundedString
from rosidl_parser.definition import BoundedWString
from rosidl_parser.definition import NamedType
from rosidl_parser.definition import NamespacedType
from rosidl_parser.definition import UnboundedSequence
from rosidl_parser.definition import UnboundedString
from rosidl_parser.definition import UnboundedWString

class UnsupportedError(Exception):
    pass

sequence_len = 0
type_ = member.type
ref_str = ""
arr_ref_str = ""
if isinstance(type_, AbstractNestedType):
    if isinstance(type_, Array):
        ref_str = f"&data->{member.name}[0]"
        arr_ref_str = f"&data->{member.name}[i]"
        sequence_len = type_.size
    # NOTE: BoundedSequence and UnboundedSequence should be grouped together
    elif isinstance(type_, BoundedSequence):
        ref_str = f"&data->{member.name}.data[0]"
        arr_ref_str = f"&data->{member.name}.data[i]"
        sequence_len = type_.maximum_size
    elif isinstance(type_, UnboundedSequence):
        ref_str = f"&data->{member.name}.data[0]"
        arr_ref_str = f"&data->{member.name}.data[i]"
        sequence_len = -1
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    type_ = type_.value_type
elif isinstance(type_, AbstractNestableType):
    ref_str = f"&data->{member.name}"
    pass
else:
    #TODO: implement array of string
    #Below exception is commented out to avoid build failure
    #raise UnsupportedError(f"{type(type_)} not supported")
    pass
}@
@[if isinstance(type_, NamedType) or isinstance(type_, NamespacedType)]@
@{
if isinstance(type_, NamedType):
    type_name = message_name_prefix + type_.name
else:
    type_name = '__'.join(type_.namespaced_name())
}@
@[    if sequence_len > 0]@
    size = @(type_name)__callbacks.data_encode_size((struct tt_Data*)@(ref_str));
    for (int32_t i = 0; i < @(sequence_len); ++i) {
        ret = @(type_name)__callbacks.data_@(encode_or_decode)((struct tt_Data*)@(arr_ref_str), payload, size@(encode_or_decode == "encode" ? "" ! ", is_native_endian" ));
        @(encode_or_decode)d += ret;
        payload += ret;
    }
@[    elif sequence_len == -1]@
    size = @(type_name)__callbacks.data_encode_size((struct tt_Data*)@(ref_str));
    for (int32_t i = 0; i < data->@(member.name).size; ++i) {
        ret = @(type_name)__callbacks.data_@(encode_or_decode)((struct tt_Data*)@(arr_ref_str), payload, size@(encode_or_decode == "encode" ? "" ! ", is_native_endian" ));
        @(encode_or_decode)d += ret;
        payload += ret;
    }
@[    else]@
    size = @(type_name)__callbacks.data_encode_size((struct tt_Data*)@(ref_str));
    ret = @(type_name)__callbacks.data_@(encode_or_decode)((struct tt_Data*)&data->@(member.name), payload, size@(encode_or_decode == "encode" ? "" ! ", is_native_endian" ));
    @(encode_or_decode)d += ret;
    payload += ret;
@[    end if]@
@[else]@
    size = @(get_type_size(member));
@[    if encode_or_decode == "encode"]@
@# TODO: include tickle header and use _tt_memcpy
    memcpy(payload, &data->@(member.name), size);
@[    else]@
    memcpy(&data->@(member.name), payload, size);
@[    end if]@
    @(encode_or_decode)d += size;
    payload += size;
@[end if]@
@[end def]@

int32_t encode_size_@(unique_message_identifier)(void* raw)
{
    @(tickle_type)* data = raw;

    (void)data;
    return@
@[for member in message.structure.members]@
@[    if member == message.structure.members[0]]@
@(' ')@
@[    else]@
@('           ')@
@[    end if]@
@(get_type_size(member))@(member == message.structure.members[-1] ? ';' ! ' +')
@[end for]@
}

int32_t encode_@(unique_message_identifier)(
    void* raw,
    uint8_t* payload,
    const int32_t len)
{
    @(tickle_type)* data = raw;
    int32_t encoded = 0;
    int32_t ret;
    int32_t size;

    if (@(unique_message_identifier)__callbacks.data_encode_size((struct tt_Data*)data) > len) {
        return -1;
    }

@[for member in message.structure.members]@
@# *(@(member.type)*)payload = data->@(member.name);
@(generate_encoder("encode", member, get_type_size))
@[end for]@
    return encoded;
}

int32_t decode_@(unique_message_identifier)(
    void* raw,
    uint8_t* payload,
    const int32_t len,
    bool is_native_endian)
{
    @(tickle_type)* data = raw;
    int32_t decoded = 0;
    int32_t ret;
    int32_t size;

    if (@(unique_message_identifier)__callbacks.data_encode_size((struct tt_Data*)data) > len) {
        return -1;
    }

@[for member in message.structure.members]@
@(generate_encoder("decode", member, get_type_size))
@[end for]@
    return decoded;
}

void* alloc_@(unique_message_identifier)(void)
{
//  uint32_t  size = sizeof(@(tickle_type));
//  return malloc(size);
    return NULL;
}

void free_@(unique_message_identifier)(void* raw)
{
    @(tickle_type)* data = raw;
    (void)data;
//  free(data);
}

int32_t convert_to_tickle_from_@(unique_message_identifier)(void* to, void* from)
{
    @(tickle_type)* dst = to;
    struct @(unique_message_identifier)* src = from;

    return 0;
}

int32_t convert_from_tickle_to_@(unique_message_identifier)(void* to, void* from)
{
    struct @(unique_message_identifier)* dst = to;
    @(tickle_type)* src = from;

    return 0;
}




@# // Collect the callback functions and provide a function to get the type support struct.

message_type_support_callbacks_t @(unique_message_identifier)__callbacks = {
  "@(message_name_prefix)",
  "@(message_name)",
  sizeof(@(tickle_type)),
  encode_size_@(unique_message_identifier),
  encode_@(unique_message_identifier),
  decode_@(unique_message_identifier),
  alloc_@(unique_message_identifier),
  free_@(unique_message_identifier),
  convert_to_tickle_from_@(unique_message_identifier),
  convert_from_tickle_to_@(unique_message_identifier),
};

static rosidl_message_type_support_t _@(message_name)__type_support = {
  ROSIDL_TYPESUPPORT_TICKLE_C__IDENTIFIER,
  &@(unique_message_identifier)__callbacks,
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
