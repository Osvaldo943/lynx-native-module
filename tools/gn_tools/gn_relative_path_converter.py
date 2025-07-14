#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import re
import argparse

def convert_absolute_to_relative(match, current_dir_parts, lynx_index):
    """Convert the matched absolute path to a relative path based on the current directory.
    
    Args:
        match: The regex match object containing the absolute path.
        current_dir_parts: List of directory components for the current file's directory.
        lynx_index: Index of the 'lynx' directory in current_dir_parts.
    
    Returns:
        str: The converted relative path.
    """
    full_match = match.group(0)
    path_part = match.group(1)  # Path part after '//lynx/', e.g., "xx/yy.gni"
    path_parts = path_part.split(os.sep)
    
    # Get the part of the current directory after the lynx directory
    current_sub_dir_parts = current_dir_parts[lynx_index + 1:]
    
    # Find the common prefix length
    common_prefix_length = 0
    for i in range(min(len(current_sub_dir_parts), len(path_parts))):
        if current_sub_dir_parts[i] == path_parts[i]:
            common_prefix_length += 1
        else:
            break
    
    # Calculate the number of '../' needed
    parent_levels = len(current_sub_dir_parts) - common_prefix_length
    relative_prefix = "../" * parent_levels if parent_levels > 0 else ""
    
    # Construct the relative path
    relative_path = f"{relative_prefix}{'/'.join(path_parts[common_prefix_length:])}"
    
    # Add the target (e.g., ":zz") if it exists in the original match
    if len(match.groups()) > 1 and match.group(2):
        relative_path += match.group(2)
    
    return relative_path

def process_file(file_path, lynx_dir):
    """Process a single GN/GNI file to convert absolute paths to relative paths.
    
    Args:
        file_path: Path to the file to process.
        lynx_dir: Name of the 'lynx' directory.
    
    Returns:
        bool: True if the file was modified, False otherwise.
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        # Try alternative encoding
        try:
            with open(file_path, 'r', encoding='latin-1') as f:
                content = f.read()
        except Exception as e:
            print(f"Failed to read file {file_path}: {e}")
            return False
    
    # Get directory components of the current file's path
    current_dir = os.path.dirname(file_path)
    current_dir_parts = current_dir.split(os.sep)
    
    # Find the position of the lynx directory in the path
    try:
        lynx_index = current_dir_parts.index(lynx_dir)
    except ValueError:
        # Skip if 'lynx' directory not found in the current path
        return False
    
    # Regular expression pattern to match //lynx/... paths, including optional targets (e.g., :zz)
    pattern = re.compile(r'//lynx/([^:\s]+)(:[^\s]+)?')
    
    # Replace all matched paths with relative paths
    new_content, count = pattern.subn(
        lambda match: convert_absolute_to_relative(match, current_dir_parts, lynx_index),
        content
    )
    
    if count > 0:
        # Write the modified content back to the file
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Modified {file_path}, replaced {count} paths")
            return True
        except Exception as e:
            print(f"Failed to write file {file_path}: {e}")
            return False
    
    return False

def main():
    '''
    Usage: python3 tools/gn_tools/gn_relative_path_converter.py . in lynx directory.
    '''
    """Main function to handle command line arguments and process directories."""
    parser = argparse.ArgumentParser(description='Convert //lynx/ paths in GN/GNI files to relative paths')
    parser.add_argument('directory', help='Directory to process')
    parser.add_argument('--lynx-dir', default='lynx', help='Name of the lynx directory (default: lynx)')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be modified without writing changes')
    args = parser.parse_args()

    # Convert the input directory to an absolute path
    args.directory = os.path.abspath(args.directory)
    
    # Check if the directory exists
    if not os.path.isdir(args.directory):
        print(f"Error: Directory '{args.directory}' does not exist")
        return
    
    # Recursively process the directory
    modified_count = 0
    total_count = 0
    
    for root, _, files in os.walk(args.directory):
        for file in files:
            if file.endswith(('.gni', '.gn')):
                file_path = os.path.join(root, file)
                total_count += 1
                
                if args.dry_run:
                    # Simulate processing without modifying files
                    try:
                        with open(file_path, 'r', encoding='utf-8') as f:
                            content = f.read()
                        pattern = re.compile(r'//lynx/([^:\s]+)(:[^\s]+)?')
                        matches = pattern.findall(content)
                        if matches:
                            print(f"File {file_path} contains {len(matches)} convertible paths")
                            modified_count += 1
                    except Exception:
                        continue
                else:
                    # Process the file
                    if process_file(file_path, args.lynx_dir):
                        modified_count += 1
    
    print(f"Processing complete: Scanned {total_count} files, modified {modified_count} files")

if __name__ == "__main__":
    main()