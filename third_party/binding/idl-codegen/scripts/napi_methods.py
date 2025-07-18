# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Generate template values for methods.

Extends IdlArgument with property |default_cpp_value|.
Extends IdlTypeBase and IdlUnionType with property |union_arguments|.

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""

import os
import sys

sys.path.append(
    os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'scripts'))
from blinkbuild.name_style_converter import NameStyleConverter
from idl_definitions import IdlArgument, IdlOperation
from idl_types import IdlTypeBase, IdlUnionType, inherits_interface
from napi_globals import includes
import napi_types
import napi_utilities

# TODO(crbug.com/1174969): Remove this once Python2 is obsoleted.
if sys.version_info.major != 2:
    basestring = str


def method_is_visible(method, interface_is_partial):
    if 'overloads' in method:
        return method['overloads']['visible'] and not (
            method['overloads']['has_partial_overloads']
            and interface_is_partial)
    return method['visible'] and 'overload_index' not in method


def is_origin_trial_enabled(method):
    return bool(method['origin_trial_feature_name'])


def is_secure_context(method):
    return bool(method['overloads']['secure_context_test_all'] if 'overloads'
                in method else method['secure_context_test'])


def is_conditionally_enabled(method):
    exposed = method['overloads']['exposed_test_all'] \
        if 'overloads' in method else method['exposed_test']
    return exposed or is_secure_context(method)


def filter_conditionally_enabled(methods, interface_is_partial):
    return [
        method for method in methods
        if (method_is_visible(method, interface_is_partial)
            and is_conditionally_enabled(method)
            and not is_origin_trial_enabled(method))
    ]


def custom_registration(method):
    # TODO(dcheng): Currently, bindings must create a function object for each
    # realm as a hack to support the incumbent realm. Remove the need for custom
    # registration when Blink properly supports the incumbent realm.
    if method['is_cross_origin']:
        return True
    if 'overloads' in method:
        return (method['overloads']['runtime_determined_lengths']
                or (method['overloads']['runtime_enabled_all']
                    and not is_conditionally_enabled(method)))
    return method[
        'runtime_enabled_feature_name'] and not is_conditionally_enabled(
            method)


def filter_custom_registration(methods, interface_is_partial):
    return [
        method for method in methods
        if (method_is_visible(method, interface_is_partial)
            and custom_registration(method))
    ]


def filter_method_configuration(methods, interface_is_partial):
    return [
        method for method in methods
        if method_is_visible(method, interface_is_partial)
        and not is_origin_trial_enabled(method)
        and not is_conditionally_enabled(method)
        and not custom_registration(method)
    ]


def method_filters():
    return {
        'custom_registration': filter_custom_registration,
        'has_method_configuration': filter_method_configuration,
        'argument_literal': filter_argument_literal,
        'js_argument_literal': filter_js_argument_literal,
        'command_argument_literal': filter_command_argument_literal
    }


def filter_argument_literal(argument):
    if argument['is_dictionary'] or argument['is_string_type'] or argument['is_sequence_type'] or argument['is_callback_function']:
        return 'std::move(arg' + str(argument['index']) + '_' + argument['name'] + ')'
    return 'arg' + str(argument['index']) + '_' + argument['name']


def filter_js_argument_literal(argument):
    return argument['name']

def filter_command_argument_literal(argument):
    if argument['is_numeric_type'] or argument['is_boolean_type']:
        if argument['idl_type'] == 'long long':
            return '(int64_t)(command.' + argument['name'] + ')'
        elif argument['idl_type'] == 'unsigned long long':
            return '(uint64_t)(command.' + argument['name'] + ')'
        else:
            return 'command.' + argument['name']
    elif argument['is_wrapper_type']:
        return argument['name']
    elif argument['is_typed_array']:
        return argument['name']
    elif argument['is_string_type'] or argument.get('is_sequence_type', False):
        return 'std::move(%s)' % argument['name']
    elif argument['idl_type'] == 'ArrayBuffer':
        if argument['is_nullable']:
            return 'command.is_null ? nullptr : (buffer + current), command.%sByteLengthGen' % argument['name']
        return 'buffer + current, command.%sByteLengthGen' % argument['name']
    elif argument['idl_type'] == 'ArrayBufferView':
        return argument['name']
    elif argument['enum_type']:
        return argument['enum_type'] + '[command.' + argument['name'] + ']'
    elif argument['union_types']:
        return argument['name']
    elif argument['is_dictionary']:
        return 'std::move({})'.format(argument['name'])
    elif argument['idl_type'] == 'object': # Generic objects are not supported in command buffer
        return ''

def use_local_result(method):
    extended_attributes = method.extended_attributes
    idl_type = method.idl_type
    return (has_extended_attribute_value(method, 'CallWith', 'ScriptState')
            or 'NewObject' in extended_attributes
            or 'RaisesException' in extended_attributes
            or idl_type.is_union_type or idl_type.is_dictionary
            or idl_type.is_explicit_nullable
            or napi_utilities.high_entropy(method) == 'Direct')


def runtime_call_stats_context(interface, method):
    includes.add('platform/bindings/runtime_call_stats.h')
    generic_counter_name = (
        'Blink_' + v8_utilities.cpp_name(interface) + '_' + method.name)
    (method_counter, extended_attribute_defined) = \
        v8_utilities.rcs_counter_name(method, generic_counter_name)
    trace_event_name = interface.name + '.' + method.name
    return {
        'extended_attribute_defined':
        extended_attribute_defined,
        'method_counter':
        method_counter,
        'origin_safe_method_getter_counter':
        generic_counter_name + '_OriginSafeMethodGetter',
        'trace_event_name':
        trace_event_name,
    }


def method_context(interface, method, component_info, interfaces_info, is_visible=True):
    arguments = method.arguments
    extended_attributes = method.extended_attributes
    idl_type = method.idl_type
    is_static = method.is_static
    name = method.name

    if is_visible:
        idl_type.add_includes_for_type(extended_attributes)

    this_cpp_value = cpp_value(interface, method, len(arguments))

    is_raises_exception = 'RaisesException' in extended_attributes

    if is_raises_exception :
        includes.add(
            'third_party/binding/napi/exception_state.h'
        )

    argument_contexts = [
        argument_context(
            interface, method, argument, index, interfaces_info, is_visible=is_visible)
        for index, argument in enumerate(arguments)
    ]

    # |return_async| is True only if the method's return type supports async.
    return_async = False
    if interfaces_info.get(idl_type.base_type, {}).get('async_created', False):
        return_async = True

    # Temporary: remove after all sync methods are migrated to command buffer.
    handles_async = return_async
    for argument in arguments:
        if interfaces_info.get(argument.idl_type.base_type, {}).get('async_created', False):
            handles_async = True
            break

    returns_data_container = False
    if interfaces_info.get(idl_type.base_type, {}).get('data_container', False):
        returns_data_container = True

    union_types = []
    if idl_type.is_union_type:
        union_types = sorted([
            napi_types.process_union_type(union_type)
            for union_type in idl_type.flattened_member_types
        ])

    runtime_features = component_info['runtime_enabled_features']

    sequence_element_type = None
    sequence_element_idl_type = None
    if idl_type.is_sequence_type:
        if idl_type.native_array_element_type.is_numeric_type:
            sequence_element_type = "Number"
        elif idl_type.native_array_element_type.is_string_type:
            sequence_element_type = "String"
        elif idl_type.native_array_element_type.is_wrapper_type:
            sequence_element_type = "Object"
        sequence_element_idl_type = idl_type.native_array_element_type.base_type

    cpp_type = (napi_types.cpp_template_type('base::Optional', idl_type.cpp_type)
                if idl_type.is_explicit_nullable else napi_types.cpp_type(
        idl_type, extended_attributes=extended_attributes))

    return {
        # 'activity_logging_world_list':
        # v8_utilities.activity_logging_world_list(method),  # [ActivityLogging]
        'arguments':
        argument_contexts,
        'camel_case_name':
        NameStyleConverter(name).to_upper_camel_case(),
        'cpp_type':
        cpp_type,
        'cpp_value':
        this_cpp_value,
        'cpp_type_initializer':
        idl_type.cpp_type_initializer,
        # 'high_entropy':
        # v8_utilities.high_entropy(method),  # [HighEntropy]
        # 'deprecate_as':
        # v8_utilities.deprecate_as(method),  # [DeprecateAs]
        # 'do_not_test_new_object':
        # 'DoNotTestNewObject' in extended_attributes,
        # 'exposed_test':
        # v8_utilities.exposed(method, interface),  # [Exposed]
        # 'has_exception_state':
        # is_raises_exception or is_check_security_for_receiver or any(
        #     argument for argument in arguments
        #     if argument_conversion_needs_exception_state(method, argument)),
        # 'has_optional_argument_without_default_value':
        # any(True for argument_context in argument_contexts
        #     if argument_context['is_optional_without_default_value']),
        'idl_type':
        idl_type.base_type,
        # 'is_call_with_execution_context':
        # has_extended_attribute_value(method, 'CallWith', 'ExecutionContext'),
        # 'is_call_with_script_state':
        # is_call_with_script_state,
        # 'is_call_with_this_value':
        # is_call_with_this_value,
        # 'is_ce_reactions':
        # is_ce_reactions,
        # 'is_check_security_for_receiver':
        # is_check_security_for_receiver,
        # 'is_check_security_for_return_value':
        # is_check_security_for_return_value,
        # 'is_cross_origin':
        # 'CrossOrigin' in extended_attributes,
        # 'is_custom':
        # 'Custom' in extended_attributes,
        # 'is_explicit_nullable':
        # idl_type.is_explicit_nullable,
        # 'is_new_object':
        # 'NewObject' in extended_attributes,
        # 'is_partial_interface_member':
        # 'PartialInterfaceImplementedAs' in extended_attributes,
        # 'is_per_world_bindings':
        # 'PerWorldBindings' in extended_attributes,
        'is_enum':
        idl_type.is_enum,
        'is_raises_exception':
        is_raises_exception,
        'is_nullable':
        idl_type.is_nullable,
        'is_static':
        is_static,
        'is_boolean_type':
        idl_type.base_type == 'boolean',
        'is_numeric_type':
        idl_type.is_numeric_type,
        'is_string_type':
        idl_type.is_string_type,
        'is_wrapper_type':
        idl_type.is_wrapper_type,
        'is_dictionary':
        idl_type.is_dictionary,
        'is_sequence_type':
        idl_type.is_sequence_type,
        'sequence_element_type':
        sequence_element_type,
        'sequence_element_idl_type':
        sequence_element_idl_type,
        'union_types':
        union_types,
        # 'is_unforgeable':
        # is_unforgeable(method),
        # 'is_variadic':
        # arguments and arguments[-1].is_variadic,
        # 'measure_as':
        # v8_utilities.measure_as(method, interface),  # [MeasureAs]
        'name':
        name,
        'number_of_arguments':
        len(arguments),
        'number_of_required_arguments':
        len([
            argument for argument in arguments
            if not (argument.is_optional or argument.is_variadic)
        ]),
        'number_of_required_or_variadic_arguments':
        len([argument for argument in arguments if not argument.is_optional]),
        'on_instance':
        napi_utilities.on_instance(interface, method),
        'on_interface':
        napi_utilities.on_interface(interface, method),
        # 'on_prototype':
        # v8_utilities.on_prototype(interface, method),
        # # [RuntimeEnabled] for origin trial
        # 'origin_trial_feature_name':
        # v8_utilities.origin_trial_feature_name(method, runtime_features),
        'property_attributes':
        property_attributes(interface, method),
        'returns_promise':
        method.returns_promise,
        # 'runtime_call_stats':
        # runtime_call_stats_context(interface, method),
        # # [RuntimeEnabled] if not in origin trial
        # 'runtime_enabled_feature_name':
        # v8_utilities.runtime_enabled_feature_name(method, runtime_features),
        # # [SecureContext]
        # 'secure_context_test':
        # v8_utilities.secure_context(method, interface),
        # # [Affects]
        # 'side_effect_type':
        # side_effect_type,
        'snake_case_name':
        NameStyleConverter(name).to_snake_case(),
        'use_output_parameter_for_result':
        idl_type.use_output_parameter_for_result,
        # 'use_local_result':
        # use_local_result(method),
        'v8_set_return_value':
        v8_set_return_value(interface.name, method, this_cpp_value),
        'v8_set_return_value_for_main_world':
        v8_set_return_value(
            interface.name, method, this_cpp_value, for_main_world=True),
        'visible':
        is_visible,
        # 'world_suffixes':
        # ['', 'ForMainWorld'] if 'PerWorldBindings' in extended_attributes else
        # [''],  # [PerWorldBindings],
        'forced_sync':
        'ForcedSync' in extended_attributes,
        'flushed_within_frame':
        'FlushedWithinFrame' in extended_attributes,
        'may_chain':
        'MayChain' in extended_attributes,
        'from_shared':
        interface.sharing_impl and method.from_shared,
        'return_async':
        return_async,
        'handles_async':
        handles_async,
        'interface_name':
        interface.name,
        'returns_data_container':
        returns_data_container,
        'has_inout_buffer':
        any(True for argument_context in argument_contexts
            if argument_context.get('is_inout_buffer', False)),
        'implicit_buffer_size':
        'ImplicitBufferSize' in extended_attributes,
    }


def argument_context(interface, method, argument, index, interfaces_info={}, is_visible=True):
    extended_attributes = argument.extended_attributes
    idl_type = argument.idl_type
    if idl_type.has_string_context:
        includes.add(
            'third_party/blink/renderer/bindings/core/v8/generated_code_helper.h'
        )
    # if is_visible:
    #     idl_type.add_includes_for_type(extended_attributes)
    this_cpp_value = cpp_value(interface, method, index)
    is_variadic_wrapper_type = argument.is_variadic and idl_type.is_wrapper_type

    has_type_checking_interface = idl_type.is_wrapper_type

    set_default_value = argument.set_default_value
    this_cpp_type = idl_type.cpp_type_args(
        extended_attributes=extended_attributes,
        raw_type=True,
        used_as_variadic_argument=argument.is_variadic)
    snake_case_name = NameStyleConverter(argument.name).to_snake_case()

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

    sequence_element_type = None
    if idl_type.is_sequence_type:
        if idl_type.native_array_element_type.is_numeric_type:
            sequence_element_type = "Number"
        elif idl_type.native_array_element_type.is_string_type:
            sequence_element_type = "String"
        elif idl_type.native_array_element_type.is_wrapper_type :
            sequence_element_type = "Wrapper"
        elif idl_type.native_array_element_type.base_type == 'object' :
            sequence_element_type = "Object"
        elif idl_type.native_array_element_type.base_type == 'ArrayBuffer':
            sequence_element_type = "ArrayBuffer"

    idl_type_name, native_value_traits_support, raises_exception = napi_types.native_value_traits_idl_type(idl_type, sequence_element_type)

    async_created = False
    if interfaces_info.get(argument.idl_type.base_type, {}).get('async_created', False):
        async_created = True

    context = {
        'cpp_type': (napi_types.cpp_template_type(
            'base::Optional', this_cpp_type) if idl_type.is_explicit_nullable
                     and not argument.is_variadic else this_cpp_type),
        'cpp_value':
        this_cpp_value,
        # FIXME: check that the default value's type is compatible with the argument's
        'enum_type':
        idl_type.enum_type,
        'enum_values':
        idl_type.enum_values,
        'handle':
        '%s_handle' % snake_case_name,
        # TODO(peria): remove once [DefaultValue] removed and just use
        # argument.default_value. https://crbug.com/924419
        'has_default':
        'DefaultValue' in extended_attributes or set_default_value,
        'has_type_checking_interface':
        has_type_checking_interface,
        # Dictionary is special-cased, but arrays and sequences shouldn't be
        'idl_type':
        idl_type.base_type,
        'idl_type_object':
        idl_type,
        'index':
        index,
        'is_callback_function':
        idl_type.is_callback_function,
        'is_callback_interface':
        idl_type.is_callback_interface,
        # FIXME: Remove generic 'Dictionary' special-casing
        'is_dictionary':
        idl_type.is_dictionary,
        'is_explicit_nullable':
        idl_type.is_explicit_nullable,
        'is_nullable':
        idl_type.is_nullable,
        'is_optional':
        argument.is_optional,
        'is_variadic':
        argument.is_variadic,
        'is_variadic_wrapper_type':
        is_variadic_wrapper_type,
        'is_boolean_type':
        idl_type.base_type == 'boolean',
        'is_numeric_type':
        idl_type.is_numeric_type,
        'is_string_type':
        idl_type.is_string_type,
        'is_wrapper_type':
        idl_type.is_wrapper_type,
        'is_sequence_type':
        idl_type.is_sequence_type,
        'sequence_element_type':
        sequence_element_type,
        'is_typed_array':
        idl_type.is_typed_array,
        'union_types':
        union_types,
        'local_cpp_variable':
        snake_case_name,
        'name':
        argument.name,
        'set_default_value':
        set_default_value,
        'idl_type_name':
        idl_type_name,
        'native_value_traits_support':
        native_value_traits_support,
        'raises_exception':
        raises_exception,
        'async_created':
        async_created,
        'is_inout_buffer':
        'InOutBuffer' in extended_attributes,
        # 'use_permissive_dictionary_conversion':
        # 'PermissiveDictionaryConversion' in extended_attributes,
        # 'v8_set_return_value':
        # v8_set_return_value(interface.name, method, this_cpp_value),
        # 'v8_set_return_value_for_main_world':
        # v8_set_return_value(
        #     interface.name, method, this_cpp_value, for_main_world=True),
        # 'v8_value_to_local_cpp_value':
        # v8_value_to_local_cpp_value(interface.name, method, argument, index),
    }
    context.update({
        'is_optional_without_default_value':
        context['is_optional'] and not context['has_default']
        and not context['is_dictionary']
        and not context['is_callback_interface'],
    })
    return context


################################################################################
# Value handling
################################################################################


def cpp_value(interface, method, number_of_arguments):
    # Truncate omitted optional arguments
    arguments = method.arguments[:number_of_arguments]
    cpp_arguments = []

    if method.is_constructor:
        call_with_values = interface.extended_attributes.get(
            'ConstructorCallWith')
    else:
        call_with_values = method.extended_attributes.get('CallWith')
    cpp_arguments.extend(napi_utilities.call_with_arguments(call_with_values))

    # Members of IDL partial interface definitions are implemented in C++ as
    # static member functions, which for instance members (non-static members)
    # take *impl as their first argument
    if ('PartialInterfaceImplementedAs' in method.extended_attributes
            and not method.is_static):
        cpp_arguments.append('*impl')
    for argument in arguments:
        variable_name = NameStyleConverter(argument.name).to_snake_case()
        cpp_arguments.append(variable_name)

    # If a method returns an IDL dictionary or union type, the return value is
    # passed as an argument to impl classes.
    idl_type = method.idl_type
    if idl_type and idl_type.use_output_parameter_for_result:
        cpp_arguments.append('result')

    # if ('RaisesException' in method.extended_attributes
    #         or (method.is_constructor and has_extended_attribute_value(
    #             interface, 'RaisesException', 'Constructor'))):
    #     cpp_arguments.append('exception_state')

    if method.name == 'Constructor':
        base_name = 'Create'
    elif method.name == 'NamedConstructor':
        base_name = 'CreateForJSConstructor'
    else:
        base_name = napi_utilities.cpp_name(method)

    cpp_method_name = napi_utilities.scoped_name(interface, method, base_name)
    return '%s(%s)' % (cpp_method_name, ', '.join(cpp_arguments))


def v8_set_return_value(interface_name,
                        method,
                        cpp_value,
                        for_main_world=False):
    idl_type = method.idl_type
    extended_attributes = method.extended_attributes
    if not idl_type or idl_type.name == 'void':
        # Constructors and void methods don't have a return type
        return None

    # [CallWith=ScriptState], [RaisesException]
    # if use_local_result(method):
    #     if idl_type.is_explicit_nullable:
    #         # result is of type base::Optional<T>
    #         cpp_value = 'result.value()'
    #     else:
    #         cpp_value = 'result'

    script_wrappable = 'impl' if inherits_interface(interface_name,
                                                    'Node') else ''
    return idl_type.v8_set_return_value(
        cpp_value,
        extended_attributes,
        script_wrappable=script_wrappable,
        for_main_world=for_main_world,
        is_static=method.is_static)


def v8_value_to_local_cpp_variadic_value(argument,
                                         index,
                                         for_constructor_callback=False):
    assert argument.is_variadic
    idl_type = v8_types.native_value_traits_type_name(
        argument.idl_type, argument.extended_attributes, True)
    execution_context_if_needed = ''
    if argument.idl_type.has_string_context:
        execution_context_if_needed = ', bindings::ExecutionContextFromV8Wrappable(impl)'
        if for_constructor_callback:
            execution_context_if_needed = ', CurrentExecutionContext(info.GetIsolate())'
    assign_expression = 'ToImplArguments<%s>(info, %s, exception_state%s)' % (
        idl_type, index, execution_context_if_needed)
    return {
        'assign_expression': assign_expression,
        'check_expression': 'exception_state.HadException()',
        'cpp_name': NameStyleConverter(argument.name).to_snake_case(),
        'declare_variable': False,
    }


def v8_value_to_local_cpp_value(interface_name, method, argument, index):
    extended_attributes = argument.extended_attributes
    idl_type = argument.idl_type
    name = NameStyleConverter(argument.name).to_snake_case()
    v8_value = 'info[{index}]'.format(index=index)
    for_constructor_callback = method.name == 'Constructor'

    if argument.is_variadic:
        return v8_value_to_local_cpp_variadic_value(
            argument, index, for_constructor_callback=for_constructor_callback)
    return idl_type.v8_value_to_local_cpp_value(
        extended_attributes,
        v8_value,
        name,
        declare_variable=False,
        use_exception_state=method.returns_promise,
        for_constructor_callback=for_constructor_callback)


################################################################################
# Auxiliary functions
################################################################################


# [NotEnumerable], [LegacyUnforgeable]
def property_attributes(interface, method):
    extended_attributes = method.extended_attributes
    property_attributes_list = []
    if 'NotEnumerable' in extended_attributes:
        property_attributes_list.append('v8::DontEnum')
    # if is_unforgeable(method):
    #     property_attributes_list.append('v8::ReadOnly')
    #     property_attributes_list.append('v8::DontDelete')
    return property_attributes_list


def argument_set_default_value(argument):
    idl_type = argument.idl_type
    default_value = argument.default_value
    variable_name = NameStyleConverter(argument.name).to_snake_case()
    if not default_value:
        return None
    if idl_type.is_dictionary:
        if argument.default_value.is_null:
            return None
        if argument.default_value.value == '{}':
            return None
        raise Exception('invalid default value for dictionary type')
    if idl_type.is_array_or_sequence_type:
        if default_value.value != '[]':
            raise Exception('invalid default value for sequence type: %s' %
                            default_value.value)
        # Nothing to do when we set an empty sequence as default value, but we
        # need to return non-empty value so that we don't generate method calls
        # without this argument.
        return '/* Nothing to do */'
    if idl_type.is_union_type:
        if argument.default_value.is_null:
            if not idl_type.includes_nullable_type:
                raise Exception(
                    'invalid default value for union type: null for %s' %
                    idl_type.name)
            # Union container objects are "null" initially.
            return '/* null default value */'
        if default_value.value == "{}":
            member_type = idl_type.dictionary_member_type
        elif isinstance(default_value.value, basestring):
            member_type = idl_type.string_member_type
        elif isinstance(default_value.value, (int, float)):
            member_type = idl_type.numeric_member_type
        elif isinstance(default_value.value, bool):
            member_type = idl_type.boolean_member_type
        else:
            member_type = None
        if member_type is None:
            raise Exception('invalid default value for union type: %r for %s' %
                            (default_value.value, idl_type.name))
        member_type_name = (member_type.inner_type.name
                            if member_type.is_nullable else member_type.name)
        return '%s.Set%s(%s)' % (variable_name, member_type_name,
                                 member_type.literal_cpp_value(default_value))
    return '%s = %s' % (variable_name,
                        idl_type.literal_cpp_value(default_value))


IdlArgument.set_default_value = property(argument_set_default_value)


def method_returns_promise(method):
    return method.idl_type and method.idl_type.name == 'Promise'


IdlOperation.returns_promise = property(method_returns_promise)


def argument_conversion_needs_exception_state(method, argument):
    idl_type = argument.idl_type
    return (idl_type.v8_conversion_needs_exception_state
            or argument.is_variadic
            or (method.returns_promise and idl_type.is_string_type))
