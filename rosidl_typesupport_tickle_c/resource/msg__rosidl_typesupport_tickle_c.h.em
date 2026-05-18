@# Included from rosidl_typesupport_tickle_c/resource/idl__rosidl_typesupport_tickle_c.h.em
@{
header_files = [
    'stdbool.h',
    'stdint.h',
    'rosidl_runtime_c/message_type_support_struct.h',
    'rosidl_typesupport_interface/macros.h',
    'rosidl_typesupport_tickle_c/message_type_support.h',
    package_name + '/msg/rosidl_typesupport_tickle_c__visibility_control.h',
]
}@
@[for header_file in header_files]@
@[    if header_file in include_directives]@
// already included above
// @
@[    else]@
@{include_directives.add(header_file)}@
@[    end if]@
@[    if '/' not in header_file]@
#include <@(header_file)>
@[    else]@
#include "@(header_file)"
@[    end if]@
@[end for]@

@{
unique_message_identifier = '__'.join(message.structure.namespaced_type.namespaced_name())
}@

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_TICKLE_C_PUBLIC_@(package_name)
extern message_type_support_callbacks_t @(unique_message_identifier)__callbacks;

ROSIDL_TYPESUPPORT_TICKLE_C_PUBLIC_@(package_name)
const rosidl_message_type_support_t*
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_tickle_c, @(', '.join([package_name] + list(interface_path.parents[0].parts) + [message.structure.namespaced_type.name])))(void);

#ifdef __cplusplus
}
#endif
