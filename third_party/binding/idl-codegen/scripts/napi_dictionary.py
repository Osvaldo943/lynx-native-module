# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Generate template contexts of dictionaries for both v8 bindings and
implementation classes that are used by blink's core/modules.
"""

from blinkbuild.name_style_converter import NameStyleConverter
from idl_types import IdlType, STRING_TYPES
from utilities import to_snake_case
from napi_globals import includes
from napi_utilities import has_extended_attribute_value
import operator
import napi_types
import napi_utilities

DICTIONARY_H_INCLUDES = frozenset([
    'third_party/binding/napi/native_value_traits.h',
    # 'base/containers/span.h',
    # 'bindings/core/v8/native_value_traits.h',
    # 'bindings/core/v8/to_v8_for_core.h',
    # 'bindings/core/v8/v8_binding_for_core.h',
    # 'platform/heap/handle.h',
])

DICTIONARY_CPP_INCLUDES = frozenset([
    # 'base/stl_util.h',
    # 'platform/bindings/exception_state.h',
])


def getter_name_for_dictionary_member(member):
    name = napi_utilities.cpp_name(member)
    return NameStyleConverter(name).to_lower_camel_case()


def non_null_getter_name_for_dictionary_member(member):
    name = napi_utilities.cpp_name(member)
    return NameStyleConverter('{}_non_null'.format(name)).to_lower_camel_case()


def setter_name_for_dictionary_member(member):
    name = 'set_{}'.format(napi_utilities.cpp_name(member))
    return NameStyleConverter(name).to_lower_camel_case()


def null_setter_name_for_dictionary_member(member):
    if member.idl_type.is_nullable:
        name = 'set_{}_to_null'.format(napi_utilities.cpp_name(member))
        return NameStyleConverter(name).to_lower_camel_case()
    return None


def has_method_name_for_dictionary_member(member):
    name = NameStyleConverter('has_' + napi_utilities.cpp_name(member))
    return name.to_lower_camel_case()


def non_null_has_method_name_for_dictionary_member(member, for_non_null=False):
    name = 'has_{}_non_null'.format(napi_utilities.cpp_name(member))
    return NameStyleConverter(name).to_lower_camel_case()


def unwrap_nullable_if_needed(idl_type):
    if idl_type.is_nullable:
        return idl_type.inner_type
    return idl_type


# Context for V8 bindings


def dictionary_context(dictionary, interfaces_info, component_info):
    includes.clear()
    includes.update(DICTIONARY_CPP_INCLUDES)

    if 'RuntimeEnabled' in dictionary.extended_attributes:
        raise Exception(
            'Dictionary cannot be RuntimeEnabled: %s' % dictionary.name)

    members = [
        member_context(dictionary, member, component_info)
        for member in sorted(
            dictionary.members, key=operator.attrgetter('name'))
    ]

    for member in members:
        if member['runtime_enabled_feature_name']:
            includes.add('platform/runtime_enabled_features.h')
            break

    has_origin_trial_members = False
    for member in members:
        if member['origin_trial_feature_name']:
            has_origin_trial_members = True
            includes.add('core/origin_trials/origin_trials.h')
            includes.add('core/execution_context/execution_context.h')
            break

    parent_cpp_class = None
    header_includes = set(DICTIONARY_H_INCLUDES)
    if dictionary.parent:
        parent_dictionary = dictionary.parent
        IdlType(parent_dictionary).add_includes_for_type()
        parent_cpp_class = napi_utilities.cpp_name_from_interfaces_info(
            parent_dictionary, interfaces_info)
        header_includes.update(napi_types.includes_for_interface(parent_dictionary))

    for member in sorted(dictionary.members, key=operator.attrgetter('name')):
        if member.idl_type.is_union_type:
            for union_type in member.idl_type.flattened_member_types:
                header_includes.update(napi_types.includes_for_interface(union_type.name))
        elif member.idl_type.is_dictionary:
            header_includes.update(napi_types.includes_for_interface(member.idl_type.name))
        elif member.idl_type.is_sequence_type:
            header_includes.update(napi_types.includes_for_interface(member.idl_type.native_array_element_type.base_type))

    cpp_class = napi_utilities.cpp_name(dictionary)
    context = {
        'cpp_class':
        cpp_class,
        'has_origin_trial_members':
        has_origin_trial_members,
        'header_includes':
        header_includes,
        'members':
        members,
        'required_member_names':
        sorted([
            member.name for member in dictionary.members if member.is_required
        ]),
        'use_permissive_dictionary_conversion':
        'PermissiveDictionaryConversion' in dictionary.extended_attributes,
        'napi_class':
        napi_types.v8_type(cpp_class),
        'needs_native_setter':
        'NeedsNativeSetter' in dictionary.extended_attributes,
    }
    if dictionary.parent:
        IdlType(dictionary.parent).add_includes_for_type()
        parent_cpp_class = napi_utilities.cpp_name_from_interfaces_info(
            dictionary.parent, interfaces_info)
        header_includes.update(napi_types.includes_for_interface(dictionary.parent))
        context.update({
            'parent_cpp_class': parent_cpp_class,
            'parent_v8_class': napi_types.v8_type(parent_cpp_class),
            'header_includes': header_includes,
        })
    return context


def member_context(_, member, component_info):
    extended_attributes = member.extended_attributes
    idl_type = member.idl_type
    idl_type.add_includes_for_type(extended_attributes)
    unwrapped_idl_type = unwrap_nullable_if_needed(idl_type)
    cpp_type = unwrapped_idl_type.cpp_type

    sequence_element_type = None
    is_sequence_element_dictionary_type = False
    is_sequence_element_interface_type = False
    if idl_type.is_sequence_type:
         if idl_type.native_array_element_type.is_dictionary:
             is_sequence_element_dictionary_type = True
             sequence_element_type = idl_type.native_array_element_type.base_type
             sequence_element_cpp_type = napi_types.cpp_template_type('std::unique_ptr', sequence_element_type)
             cpp_type = napi_types.cpp_template_type('std::vector', sequence_element_cpp_type)
         elif idl_type.native_array_element_type.cpp_type_args() in STRING_TYPES:
             sequence_element_cpp_type = 'std::string'
             cpp_type = napi_types.cpp_template_type('std::vector', sequence_element_cpp_type)
         elif idl_type.native_array_element_type.is_interface_type:
             is_sequence_element_interface_type = True
             sequence_element_type = idl_type.native_array_element_type.base_type
             sequence_element_cpp_type = idl_type.native_array_element_type.base_type + '*'
             cpp_type = napi_types.cpp_template_type('std::vector', sequence_element_cpp_type)
    elif idl_type.is_union_type:
        cpp_type += "*"
    elif idl_type.is_string_type or idl_type.enum_type or cpp_type in STRING_TYPES:
        cpp_type = 'std::string'

    if member.is_required and member.default_value:
        raise Exception(
            'Required member %s must not have a default value.' % member.name)

    if idl_type.is_nullable and idl_type.inner_type.is_dictionary:
        raise Exception(
            'The inner type of nullable member %s must not be a dictionary.' %
            member.name)

    # In most cases, we don't have to distinguish `null` and `not present`,
    # and use null-states (e.g. nullptr, foo.IsUndefinedOrNull()) to show such
    # states for some types for memory usage and performance.
    # For types whose |has_explicit_presence| is True, we provide explicit
    # states of presence.
    has_explicit_presence = (idl_type.is_nullable
                             and idl_type.inner_type.is_interface_type)

    def default_values():
        if not member.default_value:
            return None, None
        if member.default_value.is_null:
            return None, 'v8::Null(isolate)'

        cpp_default_value = unwrapped_idl_type.literal_cpp_value(
            member.default_value)
        if idl_type.is_sequence_type:
            cpp_default_value = None

        v8_default_value = unwrapped_idl_type.cpp_value_to_v8_value(
            cpp_value=cpp_default_value,
            isolate='isolate',
            creation_context='creationContext')
        return cpp_default_value, v8_default_value

    cpp_default_value, v8_default_value = default_values()
    snake_case_name = to_snake_case(member.name)
    cpp_value = snake_case_name + "_cpp_value"
    v8_value = snake_case_name + "_value"
    has_value_or_default = snake_case_name + "_has_value_or_default"
    getter_name = getter_name_for_dictionary_member(member)
    runtime_features = component_info['runtime_enabled_features']

    sequence_element_idl_type = None
    if idl_type.is_sequence_type:
        if idl_type.native_array_element_type.is_numeric_type:
            sequence_element_idl_type = "Number"
        elif idl_type.native_array_element_type.is_string_type:
            sequence_element_idl_type = "String"
        elif idl_type.native_array_element_type.is_wrapper_type:
            sequence_element_idl_type = "Wrapper"
        elif idl_type.native_array_element_type.base_type == 'object' :
            sequence_element_idl_type = "Object"
        elif idl_type.native_array_element_type.base_type == 'ArrayBuffer':
            sequence_element_idl_type = "ArrayBuffer"
    idl_type_name, native_value_traits_support, raises_exception = napi_types.native_value_traits_idl_type(idl_type, sequence_element_idl_type)

    from functools import cmp_to_key
    def compare_dicts(d1, d2):
        if len(d1) != len(d2):
            return -1 if len(d1) < len(d2) else 1
        for key in sorted(d1):
            if key not in d2:
                return -1
            if d1[key] != d2[key]:
                return -1 if d1[key] < d2[key] else 1
        return 0

    union_types = []
    if idl_type.is_union_type:
        union_types = sorted([
            napi_types.process_union_type(union_type)
            for union_type in idl_type.flattened_member_types
        ], key=cmp_to_key(compare_dicts))

    sequence_element_nullable = False
    if idl_type.native_array_element_type:
        sequence_element_nullable = idl_type.native_array_element_type.is_nullable

    return {
        'cpp_default_value':
        cpp_default_value,
        'default_value':
        member.default_value,
        'cpp_type':
        cpp_type,
        'cpp_value':
        cpp_value,
        'cpp_value_to_v8_value':
        unwrapped_idl_type.cpp_value_to_v8_value(
            cpp_value='impl->%s()' % getter_name,
            isolate='isolate',
            creation_context='creationContext',
            extended_attributes=extended_attributes),
        'deprecate_as':
        napi_utilities.deprecate_as(member),
        'sequence_element_type':
        sequence_element_type,
        'sequence_element_nullable':
        sequence_element_nullable,
        'enum_type':
        idl_type.enum_type,
        'enum_values':
        idl_type.enum_values,
        'getter_name':
        getter_name,
        'has_explicit_presence':
        has_explicit_presence,
        'has_method_name':
        has_method_name_for_dictionary_member(member),
        'idl_type':
        idl_type.base_type,
        'is_callback_function_type':
        idl_type.is_callback_function,
        'is_interface_type':
        idl_type.is_interface_type,
        'is_nullable':
        idl_type.is_nullable,
        'is_boolean_type':
        member.idl_type.base_type == 'boolean',
        'is_numeric_type':
        member.idl_type.is_numeric_type,
        'is_string_type':
        member.idl_type.is_string_type,
        'is_wrapper_type':
        member.idl_type.is_wrapper_type,
        'is_sequence_type':
        member.idl_type.is_sequence_type,
        'is_dictionary':
        member.idl_type.is_dictionary,
        'is_sequence_element_interface_type':
        is_sequence_element_interface_type,
        'is_sequence_element_dictionary_type':
        is_sequence_element_dictionary_type,
        'is_object':
        unwrapped_idl_type.name == 'Object',
        'is_string_type':
        idl_type.preprocessed_type.is_string_type,
        'is_required':
        member.is_required,
        'name':
        member.name,
        # [RuntimeEnabled] for origin trial
        'origin_trial_feature_name':
        napi_utilities.origin_trial_feature_name(member, runtime_features),
        # [RuntimeEnabled] if not in origin trial
        'runtime_enabled_feature_name':
        napi_utilities.runtime_enabled_feature_name(member, runtime_features),
        'sequence_element_type':
        sequence_element_type,
        'setter_name':
        setter_name_for_dictionary_member(member),
        'has_value_or_default':
        has_value_or_default,
        'null_setter_name':
        null_setter_name_for_dictionary_member(member),
        'v8_default_value':
        v8_default_value,
        'v8_value':
        v8_value,
        'v8_value_to_local_cpp_value':
        idl_type.v8_value_to_local_cpp_value(
            extended_attributes,
            v8_value,
            cpp_value,
            isolate='isolate',
            use_exception_state=True),
        'impl_namespace':
        extended_attributes.get('ImplNamespace'),
        'idl_type_name':
        idl_type_name,
        'union_types':
        union_types,
    }


# Context for implementation classes


def dictionary_impl_context(dictionary, interfaces_info):
    def remove_duplicate_members(members):
        # When [ImplementedAs] is used, cpp_name can conflict. For example,
        # dictionary D { long foo; [ImplementedAs=foo, DeprecateAs=Foo] long oldFoo; };
        # This function removes such duplications, checking they have the same type.
        members_dict = {}
        for member in members:
            cpp_name = member['cpp_name']
            duplicated_member = members_dict.get(cpp_name)
            if duplicated_member and duplicated_member != member:
                raise Exception('Member name conflict: %s' % cpp_name)
            members_dict[cpp_name] = member
        return sorted(
            members_dict.values(), key=lambda member: member['cpp_name'])

    includes.clear()
    header_forward_decls = set()
    header_includes = set(['platform/heap/handle.h'])
    members = [
        member_impl_context(member, interfaces_info, header_includes,
                            header_forward_decls)
        for member in dictionary.members
    ]
    members = remove_duplicate_members(members)
    context = {
        'header_forward_decls': header_forward_decls,
        'header_includes': header_includes,
        'cpp_class': v8_utilities.cpp_name(dictionary),
        'members': members,
    }
    if dictionary.parent:
        context['parent_cpp_class'] = \
            v8_utilities.cpp_name_from_interfaces_info(dictionary.parent,
                                                       interfaces_info)
        parent_interface_info = interfaces_info.get(dictionary.parent)
        if parent_interface_info:
            context['header_includes'].add(
                parent_interface_info['include_path'])
    else:
        context['parent_cpp_class'] = 'IDLDictionaryBase'
        context['header_includes'].add(
            'bindings/core/v8/idl_dictionary_base.h')
    return context


def member_impl_context(member, interfaces_info, header_includes,
                        header_forward_decls):
    idl_type = unwrap_nullable_if_needed(member.idl_type)
    cpp_name = to_snake_case(v8_utilities.cpp_name(member))

    # In most cases, we don't have to distinguish `null` and `not present`,
    # and use null-states (e.g. nullptr, foo.IsUndefinedOrNull()) to show such
    # states for some types for memory usage and performance.
    # For types whose |has_explicit_presence| is True, we provide explicit
    # states of presence.
    has_explicit_presence = (member.idl_type.is_nullable
                             and member.idl_type.inner_type.is_interface_type)

    nullable_indicator_name = None
    if not idl_type.cpp_type_has_null_value or has_explicit_presence:
        nullable_indicator_name = 'has_' + cpp_name + '_'

    def has_method_expression():
        if nullable_indicator_name:
            return nullable_indicator_name
        if idl_type.is_union_type or idl_type.is_enum or idl_type.is_string_type:
            return '!%s_.IsNull()' % cpp_name
        if idl_type.name == 'Any':
            return '!({0}_.IsEmpty() || {0}_.IsUndefined())'.format(cpp_name)
        if idl_type.name == 'Object':
            return '!({0}_.IsEmpty() || {0}_.IsNull() || {0}_.IsUndefined())'.format(
                cpp_name)
        return '!!%s_' % cpp_name

    cpp_default_value = None
    if member.default_value:
        if not member.default_value.is_null or has_explicit_presence:
            cpp_default_value = idl_type.literal_cpp_value(
                member.default_value)

    forward_decl_name = idl_type.impl_forward_declaration_name
    if forward_decl_name:
        includes.update(idl_type.impl_includes_for_type(interfaces_info))
        header_forward_decls.add(forward_decl_name)
    else:
        header_includes.update(
            idl_type.impl_includes_for_type(interfaces_info))

    setter_value = 'value'
    non_null_type = idl_type.inner_type if idl_type.is_nullable else idl_type
    setter_inline = 'inline ' if (non_null_type.is_basic_type
                                  or non_null_type.is_enum
                                  or non_null_type.is_wrapper_type) else ''

    extended_attributes = member.extended_attributes

    return {
        'cpp_default_value':
        cpp_default_value,
        'cpp_name':
        cpp_name,
        'has_explicit_presence':
        has_explicit_presence,
        'getter_expression':
        cpp_name + '_',
        'getter_name':
        getter_name_for_dictionary_member(member),
        'has_method_expression':
        has_method_expression(),
        'has_method_name':
        has_method_name_for_dictionary_member(member),
        'is_nullable':
        member.idl_type.is_nullable,
        'is_traceable':
        idl_type.is_traceable,
        'member_cpp_type':
        idl_type.cpp_type_args(used_in_cpp_sequence=True,
                               extended_attributes=extended_attributes),
        'non_null_getter_name':
        non_null_getter_name_for_dictionary_member(member),
        'non_null_has_method_name':
        non_null_has_method_name_for_dictionary_member(member),
        'null_setter_name':
        null_setter_name_for_dictionary_member(member),
        'nullable_indicator_name':
        nullable_indicator_name,
        'rvalue_cpp_type':
        idl_type.cpp_type_args(used_as_rvalue_type=True,
                               extended_attributes=extended_attributes),
        'setter_inline':
        setter_inline,
        'setter_name':
        setter_name_for_dictionary_member(member),
        'setter_value':
        setter_value,
    }
