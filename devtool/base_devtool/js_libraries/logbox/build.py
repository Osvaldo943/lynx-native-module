#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import subprocess
import sys

# Get the current directory of the script
current_dir = os.path.dirname(os.path.realpath(__file__))

# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..', '..'))

sys.path.append(root_path)
from tools.js_tools.pnpm_helper import run_pnpm_command

# Define the distribution path
dist_path = os.path.join(current_dir, 'dist')

# Define the Android target path
android_target_path = os.path.join(root_path, 'devtool', 'base_devtool', 'android', 'base_devtool', 'src', 'main', 'assets', 'logbox')

# Define the iOS target path
ios_target_path = os.path.join(root_path, 'devtool', 'base_devtool', 'darwin', 'ios', 'assets', 'logbox')

def build():
    # Change to the root directory
    os.chdir(root_path)

    # Create the distribution directory if it doesn't exist
    os.makedirs(dist_path, exist_ok=True)

    # Run the pnpm build command
    run_pnpm_command(['pnpm', '--filter', '@lynx-dev/logbox', 'build'],
                     root_path)

    # Remove the existing Android and iOS target directories
    if os.path.exists(android_target_path):
        shutil.rmtree(android_target_path)
    if os.path.exists(ios_target_path):
        shutil.rmtree(ios_target_path)

    # Create the Android and iOS target directories
    os.makedirs(android_target_path, exist_ok=True)
    os.makedirs(ios_target_path, exist_ok=True)

    # Copy the contents of the distribution directory to the Android and iOS target directories
    for item in os.listdir(dist_path):
        s = os.path.join(dist_path, item)
        d_android = os.path.join(android_target_path, item)
        d_ios = os.path.join(ios_target_path, item)
        if os.path.isdir(s):
            shutil.copytree(s, d_android)
            shutil.copytree(s, d_ios)
        else:
            shutil.copy2(s, d_android)
            shutil.copy2(s, d_ios)


if __name__ == "__main__":
    build()
