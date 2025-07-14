# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
from env_setup import *
from api_utils import is_doxygen_installed
from parser.doxygen.doxygen_parser import DoxygenParser
from parser.doxygen.doxygen_config import DoxygenConfig
from parser.ts_morph.harmony_parser import HarmonyParser


def update_api_metadata(api_path, platform):
    if platform in ["ios", "android"]:
        parser = DoxygenParser(platform)
    elif platform == "harmony":
        parser = HarmonyParser()
    else:
        print(f"unsupported platform: {platform}")
        return False
    if not parser.dump():
        print(f"update {platform} api metadata failed")
        return False
    print(f"update {platform} api metadata success")

    return True


def generate_api_doc(api_path, platform):
    """Generate API documentation using doxygen
    Args:
        api_path (str): Path to API documentation root
        platform (str): Target platform ('ios' or 'android')
    Returns:
        bool: True if generation succeeded, False otherwise
    """
    if not os.path.exists(api_path):
        print(f"{api_path} not found")
        return False
    if not is_doxygen_installed():
        return False

    doxygen_config = DoxygenConfig(
        platform,
        enable_generate_xml=False,
        enable_generate_html=True,
    )
    return doxygen_config.execute(api_path, platform)
