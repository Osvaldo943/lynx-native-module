#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys
import shutil
import os
import argparse

def main():
  parser = argparse.ArgumentParser(description='Copy files or dirs.')
  parser.add_argument('--sources', nargs='+', help='source files or dirs.')
  parser.add_argument('--destinations', nargs='+', help='destination files or dirs.')
  
  args = parser.parse_args()

  sources = args.sources
  destinations = args.destinations
  
  if len(sources) != len(destinations):
    print("Error: The number of sources and destinations must be equal.")
    return -1

  for src, dst in zip(sources, destinations):
    if not os.path.exists(src):
      print(f"Error: Source '{src}' does not exist.")
      return -1

    if os.path.isdir(src):
      shutil.copytree(src, dst, dirs_exist_ok=True)
      print(f"Copied directory: {src} -> {dst}")
    else:
      if not os.path.exists(os.path.dirname(dst)):
        os.makedirs(os.path.dirname(dst))
      shutil.copy2(src, dst)
      print(f"Copied file: {src} -> {dst}")
  
  return 0

if __name__ == '__main__':
  sys.exit(main())
