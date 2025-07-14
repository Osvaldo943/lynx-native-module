# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from metadata_def import BaseMember, BaseMemberType, BaseParam, BaseObject
from doxmlparser import compound as doxcompound
from parser.doxygen.common_parse import (
    briefdescription_parse_in_online,
    is_apidoc,
    detail_section_parse,
    func_prototype_parse,
)


def function_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.MethodType
    member_object.definition = (
        f"public {memberdef.get_definition()}{memberdef.get_argsstring()};"
    )
    member_object.prototype = func_prototype_parse(object, memberdef)
    member_object.returns = BaseParam(
        name="", type=memberdef.get_type().get_valueOf_(), brief_desc=""
    )
    detaileddescription_obj = memberdef.get_detaileddescription()
    detail_section_parse(detaileddescription_obj, member_object)

    for para_obj in detaileddescription_obj.get_para():
        for param_list_obj in para_obj.get_parameterlist():
            for param_item_obj in param_list_obj.get_parameteritem():
                param_name = ""
                for param_name_list_obj in param_item_obj.get_parameternamelist():
                    for param_name_obj in param_name_list_obj.get_parametername():
                        param_name = param_name_obj.get_valueOf_().strip()

                member_object.params.append(
                    BaseParam(
                        name=param_name,
                        type="",
                        brief_desc=briefdescription_parse_in_online(
                            param_item_obj.get_parameterdescription()
                        ),
                    )
                )
        for simplesect in para_obj.get_simplesect():
            if simplesect.get_kind() == "return":
                member_object.returns.brief_desc = briefdescription_parse_in_online(
                    simplesect
                )

    for param in memberdef.get_param():
        param_name = param.get_declname()
        param_type = param.get_type()
        for member_param in member_object.params:
            if member_param.name == param_name:
                member_param.type = param_type.get_valueOf_().strip()
                break
    return True


def variable_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.VariableType
    member_object.definition = (
        f"public {memberdef.get_definition()} {member_object.name};"
    )
    return True


def property_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.PropertyType
    member_object.definition = (
        f"public {memberdef.get_definition()} {member_object.name};"
    )
    return True


def typedef_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.TypedefType
    member_object.definition = memberdef.get_definition()
    return True


def enum_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.EnumType
    enumvalue_list = memberdef.get_enumvalue()
    member_object.definition = f"public enum {memberdef.get_name()} {{"
    for enumvalue in enumvalue_list:
        member_object.definition += f"\n  {enumvalue.get_name()}"
    member_object.definition += "\n}\n"
    return True


def define_parse(object: BaseObject, member_object: BaseMember, memberdef) -> bool:
    member_object.type = BaseMemberType.DefineType
    member_object.definition = f"define {memberdef.get_name()}"
    defname_list = memberdef.get_defname()
    member_object.definition += ",".join(defname_list) + "\n"
    initializer_element = memberdef.get_initializer()
    if initializer_element:
        member_object.definition += initializer_element.text + "\n"
    return True


def parse(object: BaseObject, compounddef, only_public=True) -> list[BaseMember]:
    member_list = []
    for sectiondef in compounddef.get_sectiondef():
        for memberdef in sectiondef.get_memberdef():
            if only_public and memberdef.get_prot() not in [
                doxcompound.DoxProtectionKind.PUBLIC
            ]:
                continue
            memberdef_kind = memberdef.get_kind()
            memberdef_parse_func = MEMBERDEF_PARSE_DICT.get(memberdef_kind)
            if memberdef_parse_func is not None:
                member_object = BaseMember(
                    name=memberdef.get_name(),
                    type=BaseMemberType.UnknownType,
                    brief_desc=briefdescription_parse_in_online(
                        memberdef.get_briefdescription()
                    ),
                    detailed_desc=memberdef.get_detaileddescription(),
                    definition="",
                    prototype="",
                    has_apidoc=False,
                    params=[],
                    returns=None,
                )
                if memberdef_parse_func(object, member_object, memberdef):
                    member_object.has_apidoc = is_apidoc(memberdef)
                    member_list.append(member_object)
            elif memberdef_kind not in MEMBERDEF_IGNORE_LIST:
                print(f"unknown memberdef: {memberdef_kind}")
    return member_list


MEMBERDEF_PARSE_DICT = {
    doxcompound.DoxMemberKind.FUNCTION: function_parse,
    doxcompound.DoxMemberKind.VARIABLE: variable_parse,
    doxcompound.DoxMemberKind.PROPERTY: property_parse,
    doxcompound.DoxMemberKind.TYPEDEF: typedef_parse,
    doxcompound.DoxMemberKind.ENUM: enum_parse,
}

MEMBERDEF_IGNORE_LIST = [doxcompound.DoxMemberKind.DEFINE]
