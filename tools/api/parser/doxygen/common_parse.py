# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from doxmlparser import compound as doxcompound
from metadata_def import BaseMember, BaseObject


def briefdescription_parse(breifdescription) -> list[str]:
    result = []
    for para_obj in breifdescription.get_para():
        formatted_text = parse_mixed_content(para_obj.content_)
        final_string = " ".join(formatted_text.split())
        result.append(final_string)
    return result


def briefdescription_parse_in_online(breifdescription) -> str:
    return "\n".join(brief for brief in briefdescription_parse(breifdescription))


def parse_mixed_content(content_list):
    parts = []
    for item in content_list:
        if item.category == doxcompound.MixedContainer.CategoryText:
            parts.append(item.value)
        elif item.category == doxcompound.MixedContainer.CategoryComplex:
            tag_name = item.name
            tag_obj = item.value
            if isinstance(tag_obj, doxcompound.docListType):
                continue
            if isinstance(tag_obj, doxcompound.refTextType):
                parts.append(tag_obj.get_valueOf_())
                continue

            inner_text = parse_mixed_content(tag_obj.content_)

            if tag_name == "ulink":
                url = tag_obj.get_url()
                parts.append(f"[{inner_text}]({url})")
            elif tag_name == "computeroutput":
                parts.append(f"`{inner_text}`")
            elif tag_name == "emphasis":
                parts.append(f"*{inner_text}*")
            elif tag_name == "bold":
                parts.append(f"**{inner_text}**")
            else:
                parts.append(inner_text)

    return "".join(parts)


def is_apidoc(object) -> bool:
    detaileddescription_obj = object.get_detaileddescription()
    for para_obj in detaileddescription_obj.get_para():
        for xrefsect in para_obj.get_xrefsect():
            if "APIDoc" in xrefsect.get_xreftitle():
                return True
    return False


def detail_section_parse(detail, member: BaseMember) -> None:
    for para_obj in detail.get_para():
        for xrefsect in para_obj.get_xrefsect():
            if "Note" in xrefsect.get_xreftitle():
                member.note.extend(
                    briefdescription_parse(xrefsect.get_xrefdescription())
                )
            elif "Info" in xrefsect.get_xreftitle():
                member.info.extend(
                    briefdescription_parse(xrefsect.get_xrefdescription())
                )
            elif "Caution" in xrefsect.get_xreftitle():
                member.caution.extend(
                    briefdescription_parse(xrefsect.get_xrefdescription())
                )
            elif "Warning" in xrefsect.get_xreftitle():
                member.warning.extend(
                    briefdescription_parse(xrefsect.get_xrefdescription())
                )


def func_prototype_parse(object: BaseObject, memberdef) -> str:
    if object.language == "Java":
        definition = memberdef.get_definition()
        parts = definition.split()
        if parts:
            parts[-1] = parts[-1].split(".")[-1]
        definition_after_parse = " ".join(parts)
        return f"public {definition_after_parse}{memberdef.get_argsstring()};"
    elif object.language == "Objective-C":
        prefix = "+" if memberdef.get_static() == "yes" else "-"
        return_type_obj = memberdef.get_type()
        return_type = ""
        if return_type_obj and hasattr(return_type_obj, "content_"):
            return_type = parse_mixed_content(return_type_obj.content_)

        method_name_parts = memberdef.get_name().split(":")

        params = memberdef.get_param()
        param_strings = []
        for i, param in enumerate(params):
            param_type_obj = param.get_type()
            param_type = ""
            if param_type_obj and hasattr(param_type_obj, "content_"):
                param_type = parse_mixed_content(param_type_obj.content_)

            param_name = param.get_declname() if param.get_declname() else ""
            param_str = f"({param_type.strip()}){param_name}"
            param_strings.append(param_str)

        prototype = f"{prefix} ({return_type.strip()})"
        for i, part in enumerate(method_name_parts):
            if part:
                prototype += f"{part}"
                if i < len(param_strings):
                    prototype += f":{param_strings[i]} "

        prototype = f"{prototype.strip()};"
        return prototype
    else:
        return f"public {memberdef.get_definition()}{memberdef.get_argsstring()};"
