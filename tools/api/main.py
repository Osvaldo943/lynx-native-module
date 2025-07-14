# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import sys
from api_dump import (
    update_api_metadata,
    generate_api_doc,
)
from env_setup import guarantee_generated_files
from api_doc import generate_website_api_doc


def main():
    """API Metadata Management CLI

    Handles command-line interface for:
    - Updating reference API metadata files
    - Comparing generated metadata with references

    Command Line Arguments:
        --update (-u): Update mode - overwrites reference files
        --platform (-p): Target platform(s) [both|ios|android]
        --doc: Generate API documentation

    Usage Examples:
        python main.py -u -p both    # Update both platforms
        python main.py -d -p ios     # Show iOS differences
        python main.py -u -p android # Update Android only
    """
    # Initialize argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-u", "--update", action="store_true", default=False, help="Update API metadata"
    )
    parser.add_argument(
        "-p",
        "--platform",
        choices=["both", "ios", "android"],
        default="both",
        help="Specify the platform",
    )
    parser.add_argument(
        "--doc", action="store_true", default=False, help="Generate API doc"
    )

    # Process command line arguments
    args = parser.parse_args()
    ios_result = True
    android_result = True

    # Handle update operation
    if args.update:
        # Ensure generated files are up-to-date
        guarantee_generated_files()
        # iOS platform update
        if args.platform == "both" or args.platform == "ios":
            ios_result = update_api_metadata(
                os.path.dirname(os.path.abspath(__file__)), "ios"
            )
        # Android platform update
        if args.platform == "both" or args.platform == "android":
            android_result = update_api_metadata(
                os.path.dirname(os.path.abspath(__file__)), "android"
            )
        sys.exit(0 if (ios_result and android_result) else 1)

    # Handle doc operation
    if args.doc:
        # Ensure generated files are up-to-date
        guarantee_generated_files()
        platform_list = []
        if args.platform == "both" or args.platform == "android":
            platform_list.append("android")
        if args.platform == "both" or args.platform == "ios":
            platform_list.append("ios")
        result = generate_website_api_doc(platform_list)
        sys.exit(0 if result else 1)


if __name__ == "__main__":
    main()
