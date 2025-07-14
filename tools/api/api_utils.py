# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import subprocess
import sys
import shutil
from env_setup import DOXYGEN_PATH, HANDLE_FAILED_INSTRUCTION, CLANG_FORMAT_PATH
from metadata_def import API


def remove_dirs(dir_path):
    """Remove all directories under the specified path
    Args:
        dir_path (str): Path to the directory
    """
    if os.path.exists(dir_path) and os.path.isdir(dir_path):
        try:
            shutil.rmtree(dir_path)
        except Exception as e:
            print(f"Failed to remove directory {dir_path}: {e}", file=sys.stderr)


def remove_and_create_dir(dir_path):
    remove_dirs(dir_path)
    os.makedirs(dir_path, exist_ok=True)


def is_doxygen_installed():
    """Check if doxygen is installed in the system

    Returns:
        bool: True if installed, False otherwise
    """
    try:
        subprocess.check_output([DOXYGEN_PATH, "--version"])
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(
            f"Doxygen not found, please check {HANDLE_FAILED_INSTRUCTION} for more infomation",
            file=sys.stderr,
        )
        return False


def camel_to_kebab_regex(s):
    if not s:
        return s
    words = re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?=[A-Z][a-z]|\b)", s)
    return "-".join(word.lower() for word in words)


def api_format(api: API):
    if api.ios_member:
        api_prototype = clang_format(api.ios_member.prototype, ".m")
        if api_prototype:
            api.ios_member.prototype = api_prototype
    if api.android_member:
        api_prototype = clang_format(api.android_member.prototype, ".java")
        if api_prototype:
            api.android_member.prototype = api_prototype


def clang_format(code: str, file_extension: str, style="Google") -> str:
    try:
        process = subprocess.run(
            [
                CLANG_FORMAT_PATH,
                f"--style={style}",
                f"--assume-filename={file_extension}",
            ],
            input=code,
            text=True,
            capture_output=True,
            check=True,
        )
        return process.stdout
    except subprocess.CalledProcessError as e:
        print(f"clang format failed: {e.stderr}")
        return ""
    except FileNotFoundError:
        print(f"{CLANG_FORMAT_PATH} not found")
        return ""
