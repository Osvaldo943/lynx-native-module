# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
from jinja2 import Environment, FileSystemLoader, Template
from api_utils import camel_to_kebab_regex, remove_and_create_dir, api_format
from metadata_def import BaseObject, API
from parser.doxygen.doxygen_parser import DoxygenParser
from parser.ts_morph.harmony_parser import HarmonyParser

DOCS_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "docs",
)

API_GEN_PATH = os.path.join(
    DOCS_PATH,
    "gen",
)

NATIVE_API_EN_DOCS_PATH = os.path.join(
    DOCS_PATH,
    "gen",
    "en",
    "lynx-native-api",
)

TEMPLATES_PATH = os.path.join(
    DOCS_PATH,
    "templates",
)

_native_api_default_template = None


def store_object_in_api_desc_dict(
    platform: str, object: BaseObject, api_desc_dict: dict
):
    for member in object.children:
        if not member.has_apidoc:
            continue
        key = f"{object.brief_desc}-{member.brief_desc}"
        api = api_desc_dict.get(key, None)
        if api is None:
            api = API(
                name=member.name,
                parent_name=camel_to_kebab_regex(object.name),
                kebab_name=camel_to_kebab_regex(member.name),
                brief_desc=member.brief_desc,
                android_member=None,
                ios_member=None,
                harmony_member=None,
            )
            api_desc_dict[key] = api

        if platform == "android":
            api.android_member = member
        elif platform == "ios":
            api.ios_member = member
        elif platform == "harmony":
            api.harmony_member = member


def get_api_data_store_by_desc(platform_object_dict: dict) -> dict:
    api_desc_dict = {}
    for platform, object_list in platform_object_dict.items():
        for object in object_list:
            if not object.has_apidoc:
                continue
            store_object_in_api_desc_dict(platform, object, api_desc_dict)

    return api_desc_dict


def generate_object_list(platform: str) -> list[BaseObject]:
    parser = None
    if platform == "ios" or platform == "android":
        parser = DoxygenParser(platform)
    elif platform == "harmony":
        parser = HarmonyParser()
    if parser is None:
        print(f"generate object list failed: unknown {platform}")
        return []
    return parser.parse()


def _init_native_api_deault_template() -> Template:
    native_api_template_path = os.path.join(
        TEMPLATES_PATH,
        "lynx-native-api-template.mdx",
    )
    env = Environment(
        loader=FileSystemLoader(os.path.dirname(native_api_template_path)),
        trim_blocks=True,
        lstrip_blocks=True,
    )
    _native_api_default_template = env.get_template(
        os.path.basename(native_api_template_path)
    )
    return _native_api_default_template


def generate_docs_from_api_dict(api_desc_dict: dict) -> bool:
    global _native_api_default_template
    if _native_api_default_template is None:
        _native_api_default_template = _init_native_api_deault_template()

    remove_and_create_dir(API_GEN_PATH)
    for api in api_desc_dict.values():
        api_format(api)

        template = _native_api_default_template
        # Check if there is a custom template for the API.
        api_custom_template_path = os.path.join(
            TEMPLATES_PATH,
            f"{api.kebab_name}.mdx",
        )
        if os.path.exists(api_custom_template_path):
            with open(api_custom_template_path, "r") as f:
                template = Template(f.read(), trim_blocks=True, lstrip_blocks=True)
        api_file_path = os.path.join(
            NATIVE_API_EN_DOCS_PATH,
            api.parent_name,
            f"{api.kebab_name}.mdx",
        )
        if not os.path.isdir(os.path.dirname(api_file_path)):
            os.makedirs(os.path.dirname(api_file_path))
        with open(api_file_path, "w") as f:
            f.write(template.render(api=api))


def generate_website_api_doc(platform_list: list[str]) -> bool:
    platform_object_dict = {}
    for platform in platform_list:
        object_list = generate_object_list(platform)
        if object_list is None:
            print(f"generate object list failed for {platform} platform")
            return False
        platform_object_dict[platform] = object_list

    api_desc_dict = get_api_data_store_by_desc(platform_object_dict)
    return generate_docs_from_api_dict(api_desc_dict)


if __name__ == "__main__":
    result = generate_website_api_doc(["android", "ios", "harmony"])
