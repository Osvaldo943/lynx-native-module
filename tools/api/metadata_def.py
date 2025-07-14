# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from dataclasses import dataclass, field


@dataclass
class BaseParam:
    name: str
    type: str
    brief_desc: str


@dataclass
class BaseMemberType:
    MethodType = "method"
    PropertyType = "property"
    VariableType = "variable"
    TypedefType = "typedef"
    EnumType = "enum"
    DefineType = "define"
    UnknownType = "unknown"


@dataclass
class BaseMember:
    name: str
    type: BaseMemberType
    brief_desc: str
    detailed_desc: str
    definition: str
    prototype: str
    has_apidoc: bool
    params: list[BaseParam]
    returns: BaseParam = None
    note: list[str] = field(default_factory=list)
    info: list[str] = field(default_factory=list)
    caution: list[str] = field(default_factory=list)
    warning: list[str] = field(default_factory=list)
    since: str = ""

    def get_member_dump_str(self) -> str:
        spaces = f'{"  " if self.type != BaseMemberType.EnumType else ""}'
        return f"{spaces}{self.definition}"


@dataclass
class BaseObjectType:
    ClassType = "class"
    InterfaceType = "interface"
    EnumType = "enum"
    FileType = "file"
    StructType = "struct"
    CategoryType = "category"
    ProtocolType = "protocol"
    UnknownType = "unknown"


@dataclass
class BaseObject:
    name: str
    type: BaseObjectType
    brief_desc: str
    detailed_desc: str
    definition: str
    has_apidoc: bool
    language: str
    since: str
    children: list[BaseMember]

    def get_api_str(self) -> str:
        api_str: str = f"{self.definition} {{\n" if self.definition else ""
        for member in self.children:
            member_definition = member.get_member_dump_str()
            if member_definition:
                api_str += f"{member_definition}\n"
        if self.definition:
            api_str += "}\n\n"
        elif api_str:
            api_str += "\n"
        return api_str


@dataclass
class API:
    name: str
    parent_name: str
    kebab_name: str
    brief_desc: str
    android_member: BaseMember
    ios_member: BaseMember
    harmony_member: BaseMember
