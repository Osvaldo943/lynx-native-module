#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess
import sys
import argparse
import shutil

from pnpm_helper import get_pnpm_env, run_pnpm_command

# Set the root path and output path
rootPath = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '../..'))
outputPath = os.path.join(rootPath, 'js_libraries', 'lynx-core', 'output')


def usage():
    """
    Print the usage information.
    """
    print(
        "Usage: python build.py --platform [android, ios] --release_output [path] --dev_output [path] --version [version]"
    )


def build(platform, releaseOutput, devOutput, version):
    """
    Build the project.
    """
    try:
        # Change to the root directory
        os.chdir(rootPath)

        # Run pnpm build for @lynx-js/runtime-shared
        run_pnpm_command(
            ['pnpm', '--filter', '@lynx-js/runtime-shared', 'build'],
            os.getcwd())

        # Bundle lynx_core.js and lynx_core_dev.js
        env = get_pnpm_env()
        env['NODE_OPTIONS'] = '--max-old-space-size=8192'
        if version:
            env['version'] = version
        run_pnpm_command(
            ['pnpm', '--filter', f'@lynx-js/lynx-core', f'build:{platform}'],
            os.getcwd(), env)

        # Copy lynx_core.js if releaseOutput is provided
        if releaseOutput:
            releaseDir = os.path.dirname(releaseOutput)
            os.makedirs(releaseDir, exist_ok=True)
            shutil.copy(os.path.join(outputPath, 'lynx_core.js'),
                        releaseOutput)

        # Copy lynx_core_dev.js if devOutput is provided
        if devOutput:
            devDir = os.path.dirname(devOutput)
            os.makedirs(devDir, exist_ok=True)
            shutil.copy(os.path.join(outputPath, 'lynx_core_dev.js'),
                        devOutput)

    except subprocess.CalledProcessError as e:
        print(f"Error occurred during build: {e}", file=sys.stderr)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)


class VersionAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        if values is None:
            version = ""
        else:
            version = values
        setattr(namespace, self.dest, version)


def main():
    # Create an argument parser
    parser = argparse.ArgumentParser(
        description='Build the project with specified parameters.')

    # Add arguments
    parser.add_argument('--platform',
                        required=True,
                        choices=['android', 'ios'],
                        help='The platform to build [android, ios]')
    parser.add_argument('--release_output', help='The output of lynx_core.js')
    parser.add_argument('--dev_output', help='The output of lynx_core_dev.js')
    parser.add_argument('--version',
                        nargs='?',
                        action=VersionAction,
                        help='The version of lynx_core.js')

    # Parse the arguments
    args = parser.parse_args()

    # Call the build function with parsed arguments
    build(args.platform, args.release_output, args.dev_output, args.version)


if __name__ == "__main__":
    main()
