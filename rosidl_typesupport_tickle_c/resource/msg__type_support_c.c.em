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
    'stdio.h',
    'stdint.h',
    'stdbool.h',
    'string.h',
    'rosidl_typesupport_tickle_c/identifier.h',
    'rosidl_typesupport_tickle_c/message_type_support.h',
    package_name + '/msg/rosidl_typesupport_tickle_c__visibility_control.h',
    include_base + '__struct.h',
    include_base + '__functions.h',
]

"""
encoder
decoder

basic type
nested type
string

sequence
array
"""

unique_message_identifier = '__'.join(message.structure.namespaced_type.namespaced_name())
message_name = message.structure.namespaced_type.name
message_name_prefix = '__'.join([package_name] + list(interface_path.parents[0].parts))
# tickle_type = tickle__ + unique_message_identifier
tickle_type = f"struct {unique_message_identifier}"

class UnsupportedError(Exception):
        pass
  
def get_type_size(member: Member) -> str:
    from rosidl_parser.definition import AbstractNestedType
    from rosidl_parser.definition import AbstractGenericString
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
    name_ = member.name
    type_str = ""
    type_str_prefix = ""
    nested_type = 0
    size = 0
    if isinstance(type_, AbstractNestedType):
        if isinstance(type_, Array):
            nested_type = 1
            size = type_.size
            nested_type = 1
            type_str_prefix += f"{size} * "
        elif isinstance(type_, BoundedSequence):
# TODO: compare capacity(type_.maximum_size) and size
            size = f"data_ptr->{name_}.size"
            nested_type = 2
        elif isinstance(type_, UnboundedSequence):
            size = f"data_ptr->{name_}.size"
            nested_type = 2
            #raise UnsupportedError(f"{type(type_)} not supported")
        else:
            raise UnsupportedError(f"{type(type_)} not supported")
        type_ = type_.value_type

    if isinstance(type_, AbstractGenericString):
# TODO: compare capacity(type_.maximum_size) and size
        if nested_type == 0:
            str_size = f"data_ptr->{name_}.size"
        elif nested_type == 1:
            str_size = f"data_ptr->{name_}[0].size"
        else:
            str_size = f"data_ptr->{name_}.data[i_].size"
        if isinstance(type_, AbstractString):
            type_str = f"{str_size} * sizeof(char)"
        else:
            type_str = f"{str_size} * sizeof(uint16_t)"

    elif isinstance(type_, NamedType) or isinstance(type_, NamespacedType):
        named_type_prefix = ""
        if nested_type == 0:
            name_ = f"&data_ptr->{name_}"
        elif nested_type == 1:
            name_ = f"&data_ptr->{name_}[0]"
        else:
            name_ = f"&data_ptr->{name_}.data[i_]"
        if isinstance(type_, NamespacedType):
            named_type_prefix = f"{'__'.join(type_.namespaced_name())}"
        else:
            named_type_prefix = f"{message_name_prefix}{type_.name}"
        type_str += f"{named_type_prefix}__callbacks.data_encode_size((struct tt_Data*){name_})"
    elif isinstance(type_, BasicType):
        type_str += f"sizeof({BASIC_IDL_TYPES_TO_C[type_.typename]})"
    else:
        raise UnsupportedError(f"{type(type_)} not supported")

    if nested_type == 2:
        loop = f"""        ({{
        int32_t size_sum = 0;
        for (uint32_t i_ = 0; i_ < {size}; ++i_) {{
            size_sum += {type_str};
        }}
        size_sum;
    }})"""
        return loop
    else:
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

@# TODO: separate generating encoder and decoder.
@[def generate_encoder(encode_or_decode: str, member: Member, get_type_size)]@
@{  
from rosidl_parser.definition import AbstractGenericString
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
from rosidl_generator_c import BASIC_IDL_TYPES_TO_C

class UnsupportedError(Exception):
    pass

size = 0
nested_type = 0
type_ = member.type
name_ = member.name
ref_str = ""
if isinstance(type_, AbstractNestedType):
    if isinstance(type_, Array):
        ref_str = f"data_ptr->{name_}"
        size = type_.size
        nested_type = 1
    elif isinstance(type_, BoundedSequence):
        ref_str = f"data_ptr->{name_}.data"
# TODO: compare capacity(type_.maximum_size) and size
        size = f"data_ptr->{name_}.size"
        nested_type = 2
    elif isinstance(type_, UnboundedSequence):
        ref_str = f"data_ptr->{name_}.data"
        size = f"data_ptr->{name_}.size"
        nested_type = 2
    else:
        raise UnsupportedError(f"{type(type_)} not supported")
    type_ = type_.value_type
elif isinstance(type_, AbstractNestableType):
    ref_str = f"data_ptr->{name_}"
    pass
else:
    #TODO: implement array of string
    #Below exception is commented out to avoid build failure
    #raise UnsupportedError(f"{type(type_)} not supported")
    pass
}@
@[if isinstance(type_, NamedType) or isinstance(type_, NamespacedType)]@
@{
type_name = ""
if isinstance(type_, NamedType):
    type_name = message_name_prefix + type_.name
else:
    type_name = '__'.join(type_.namespaced_name())
}@
@[    if encode_or_decode == "encode"]@
@[        if size != 0]@
    if (@(size) > 0) {
        size = @(type_name)__callbacks.data_encode_size((struct tt_Data*)&@(ref_str)[0]);
        for (int32_t i = 0; i < @(size); ++i) {
            ret = @(type_name)__callbacks.data_encode((struct tt_Data*)&@(ref_str)[i], payload, size);
            encoded += ret;
            payload += ret;
        }
    }
@[        else]@
    size = @(type_name)__callbacks.data_encode_size((struct tt_Data*)&@(ref_str));
    ret = @(type_name)__callbacks.data_encode((struct tt_Data*)&@(ref_str), payload, size);
    encoded += ret;
    payload += ret;
@[        end if]@
@[    else]@
@[        if size != 0]@
@# TODO: verify named type
    memcpy(&size, payload, sizeof(uint16_t));
    decoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    size = @(size);
    if (size > 0) {
        for (int32_t i = 0; i < size; ++i) {
            ret = @(type_name)__callbacks.data_decode((struct tt_Data*)&@(ref_str)[i], payload, len, is_native_endian);
            decoded += ret;
            payload += ret;
        }
    }
@[        else]@
    ret = @(type_name)__callbacks.data_decode((struct tt_Data*)&@(ref_str), payload, len, is_native_endian);
    decoded += ret;
    payload += ret;
@[        end if]@
@[    end if]@
@[elif isinstance(type_, AbstractGenericString)]@
@{
string_type = ""
char_type = ""
if isinstance(type_, AbstractString):
    string_type = "String"
    char_type = "char"
elif isinstance(type_, AbstractWString):
    string_type = "U16String"
    char_type = "uint16_t"
}@
@[    if nested_type > 0]@
@[        if encode_or_decode == "encode"]@
@# TODO: do not include size for static size array (nested_type == 1)
    size = @(size);
    memcpy(payload, &size, sizeof(uint16_t));
    encoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    for (size_t i = 0; i < @(size); ++i) {
        size = @(ref_str)[i].size;
        memcpy(payload, &size, sizeof(uint16_t));
        encoded += sizeof(uint16_t);
        payload += sizeof(uint16_t);
        size *= sizeof(@(char_type));
        memcpy(payload, @(ref_str)[i].data, size);
        encoded += size;
        payload += size;
    }
@[        else]@
    memcpy(&size, payload, sizeof(uint16_t));
    size = @(size);
    decoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    for (size_t i = 0; i < @(size); ++i) {
        memcpy(&size, payload, sizeof(uint16_t));
        decoded += sizeof(uint16_t);
        payload += sizeof(uint16_t);
        if (@(ref_str)[i].data == NULL) {
            if (rosidl_runtime_c__@(string_type)__init(&@(ref_str)[i]) == false) {
                fprintf(stderr, "%s:%d: failed to initialize string\n", __func__, __LINE__);
            }
        }
        if (rosidl_runtime_c__@(string_type)__assignn(&@(ref_str)[i], payload, size) == false) {
            fprintf(stderr, "%s:%d: failed to allocate string\n", __func__, __LINE__);
        }
        decoded += size;
        payload += size;
    }
@[        end if]@
@[    else]@
@[        if encode_or_decode == "encode"]@
    size = @(ref_str).size;
    memcpy(payload, &size, sizeof(uint16_t));
    encoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    size *= sizeof(@(char_type));
    memcpy(payload, @(ref_str).data, size);
    encoded += size;
    payload += size;
@[        else]@
@# TODO: check endian
    memcpy(&size, payload, sizeof(uint16_t));
    decoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    if (data_ptr->@(name_).data == NULL) {
        if (rosidl_runtime_c__@(string_type)__init(&@(ref_str)) == false) {
            fprintf(stderr, "%s:%d: failed to allocate string\n", __func__, __LINE__);
        }
    }
    if (rosidl_runtime_c__@(string_type)__assignn(&@(ref_str), payload, size) == false) {
        fprintf(stderr, "%s:%d: failed to allocate string\n", __func__, __LINE__);
    }
    decoded += size;
    payload += size;
@[        end if]@
@[    end if]@
@[else]@
@[    if nested_type == 2]@
@[        if encode_or_decode == "encode"]@
@# TODO: include tickle header and use _tt_memcpy
    if (@(size) > 0) {
        size = @(size);
        memcpy(payload, &size, sizeof(uint16_t));
        encoded += sizeof(uint16_t);
        payload += sizeof(uint16_t);
        size *= sizeof(@(BASIC_IDL_TYPES_TO_C[type_.typename]));
        memcpy(payload, data_ptr->@(name_).data, size);
        encoded += size;
        payload += size;
    }
@[        else]@
    memcpy(&size, payload, sizeof(uint16_t));
    decoded += sizeof(uint16_t);
    payload += sizeof(uint16_t);
    if (data_ptr->@(name_).data == NULL) {
@{typename_ = type_.typename.replace(' ', '_')}@
        if (rosidl_runtime_c__@(typename_)__Sequence__init(&data_ptr->@(name_), size) == false) {
            fprintf(stderr, "%s:%d: failed to allocate %s sequence\n", __func__, __LINE__, "@(typename_)");
        }
    }
    size *= sizeof(@(BASIC_IDL_TYPES_TO_C[type_.typename]));
    memcpy(data_ptr->@(name_).data, payload, size);
    decoded += size;
    payload += size;
@[        end if]@
@[    else]@
    size = sizeof(@(BASIC_IDL_TYPES_TO_C[type_.typename]));
@[        if encode_or_decode == "encode"]@
@# TODO: include tickle header and use _tt_memcpy
    memcpy(payload, &@(ref_str), size);
    encoded += size;
    payload += size;
@[        else]@
    memcpy(&@(ref_str), payload, size);
    decoded += size;
    payload += size;
@[        end if]@
@[    end if]@
@[end if]@
@[end def]@

int32_t encode_size_@(unique_message_identifier)(void* raw)
{
    @(tickle_type)* data_ptr = raw;

    (void)data_ptr;
    return
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
    @(tickle_type)* data_ptr = raw;
    int32_t   encoded = 0;
    int32_t   ret;
    uint16_t  size;

    (void)ret;
    if (@(unique_message_identifier)__callbacks.data_encode_size((struct tt_Data*)data_ptr) > len) {
        return -1;
    }

@[for member in message.structure.members]@
@# *(@(member.type)*)payload = data_ptr->@(member.name);
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
    @(tickle_type)* data_ptr = raw;
    int32_t   decoded = 0;
    int32_t   ret;
    uint16_t  size;

    (void)ret;

@# TODO: check encoded size
@# TODO: check is_native_endian and use _tt_bswap_16
@[for member in message.structure.members]@
@(generate_encoder("decode", member, get_type_size))
@[end for]@
    return decoded;
}

void* alloc_@(unique_message_identifier)(void)
{
    return NULL;
}

void free_@(unique_message_identifier)(void* raw)
{
    @(tickle_type)* data_ptr = raw;
    (void)data_ptr;
}

int32_t convert_to_tickle_from_@(unique_message_identifier)(void* to, void* from)
{
    @(tickle_type)* dst = to;
    struct @(unique_message_identifier)* src = from;

    (void)dst;
    (void)src;
    return 0;
}

int32_t convert_from_tickle_to_@(unique_message_identifier)(void* to, void* from)
{
    struct @(unique_message_identifier)* dst = to;
    @(tickle_type)* src = from;

    (void)dst;
    (void)src;
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
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_tickle_c, @(', '.join([package_name] + list(interface_path.parents[0].parts) + [message.structure.namespaced_type.name])))(void) {
  return &_@(message_name)__type_support;
}

#if defined(__cplusplus)
}
#endif
