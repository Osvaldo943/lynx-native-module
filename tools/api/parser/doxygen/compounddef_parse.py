# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from metadata_def import BaseObject, BaseObjectType
from doxmlparser import compound as doxcompound
from parser.doxygen import memberdef_parse
from parser.doxygen.common_parse import briefdescription_parse_in_online, is_apidoc


def class_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.ClassType
    abstract_str = "abstract " if compounddef.get_abstract() is not None else ""
    basecompoundref_str = (
        ", ".join(
            [
                basecompoundref.valueOf_
                for basecompoundref in compounddef.get_basecompoundref()
            ]
        )
        if compounddef.get_basecompoundref() is not None
        else ""
    )
    object.definition = f"public class {abstract_str}{compounddef.get_compoundname()} : {basecompoundref_str}"
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def enum_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.EnumType
    object.definition = f"public enum {compounddef.get_compoundname()}"
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def interface_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.InterfaceType
    object.definition = f"public interface {compounddef.get_compoundname()}"
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def file_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.FileType
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def struct_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.StructType
    object.definition = f"public struct {compounddef.get_compoundname()}"
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def category_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.CategoryType
    object.definition = f"public interface {compounddef.get_compoundname()}"
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def protocol_parse(object: BaseObject, compounddef) -> bool:
    object.type = BaseObjectType.ProtocolType
    basecompoundref_str = (
        ", ".join(
            [
                basecompoundref.valueOf_
                for basecompoundref in compounddef.get_basecompoundref()
            ]
        )
        if compounddef.get_basecompoundref() is not None
        else ""
    )
    object.definition = (
        f"public protocol {compounddef.get_compoundname()} : {basecompoundref_str}"
    )
    object.children = memberdef_parse.parse(object, compounddef)
    return True


def parse(compounddef) -> BaseObject:
    compounddef_parse_func = COMPOUNDDEF_PARSE_DICT.get(compounddef.get_kind())
    if compounddef_parse_func is not None:
        object = BaseObject(
            name=compounddef.get_compoundname().split("::")[-1],
            type=BaseObjectType.UnknownType,
            brief_desc=briefdescription_parse_in_online(
                compounddef.get_briefdescription()
            ),
            detailed_desc=compounddef.get_detaileddescription(),
            definition="",
            has_apidoc=False,
            language=compounddef.get_language(),
            since="",
            children=[],
        )
        if compounddef_parse_func(object, compounddef):
            object.has_apidoc = is_apidoc(compounddef)
            return object
    elif compounddef.get_kind() not in IGNORE_COMPOUNDDEF_LIST:
        print(f"unknown compounddef kind: {compounddef.get_kind()}")
    return None


COMPOUNDDEF_PARSE_DICT = {
    doxcompound.DoxCompoundKind.CLASS: class_parse,
    "enum": enum_parse,
    doxcompound.DoxCompoundKind.INTERFACE: interface_parse,
    doxcompound.DoxCompoundKind.FILE: file_parse,
    doxcompound.DoxCompoundKind.STRUCT: struct_parse,
    doxcompound.DoxCompoundKind.CATEGORY: category_parse,
    doxcompound.DoxCompoundKind.PROTOCOL: protocol_parse,
}

IGNORE_COMPOUNDDEF_LIST = [
    doxcompound.DoxCompoundKind.DIR,
    doxcompound.DoxCompoundKind.NAMESPACE,
    doxcompound.DoxCompoundKind.PAGE,
    doxcompound.DoxCompoundKind.EXAMPLE,
]
