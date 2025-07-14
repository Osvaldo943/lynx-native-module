#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys
import os
import argparse
from subprocess import Popen

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--sources', type=str, help='source files.')
  parser.add_argument('--script', type=str, help='action foreach script.')
  parser.add_argument('--arguments', type=str, help='arguments of action foreach script.')
  parser.add_argument('--target-dir', type=str, help='action target directory.')
  parser.add_argument('--gn-out-dir', type=str, help='gn out directory.')
  parser.add_argument('--root-dir', type=str, help='project root directory.')
  
  args = parser.parse_args()

  sources = args.sources.split(';')
  script = args.script
  arguments = args.arguments.split('@&')
  target_dir = args.target_dir
  gn_out_dir = args.gn_out_dir
  root_dir = args.root_dir

  if not os.path.exists(script):
    print(f"Error: Script '{script}' does not exist.")
    return -1
  if len(sources) <= 0:
    print(f"Error: sources is empty.")
    return -1

  for src in sources:
    if not os.path.exists(src):
      print(f"Error: Source '{src}' does not exist.")
      return -1

    args_for_src = list(arguments)
    for index, arg in enumerate(args_for_src):
      if '{{source}}' in arg:
        args_for_src[index] = arg.replace('{{source}}', src)
      if '{{source_file_part}}' in arg:
        file_name = src.split('/')[-1]
        args_for_src[index] = arg.replace('{{source_file_part}}', file_name)
      if '{{source_name_part}}' in arg:
        file_name = src.split('/')[-1]
        if '.' in file_name:
          file_name = file_name.split('.')[0]
        args_for_src[index] = arg.replace('{{source_name_part}}', file_name)
      if '{{source_dir}}' in arg:
        file_dir = os.path.dirname(src)
        args_for_src[index] = arg.replace('{{source_dir}}', file_dir)
      if '{{source_root_relative_dir}}' in arg:
        file_dir = os.path.dirname(src)
        file_rel_dir = os.path.relpath(src, root_dir)
        args_for_src[index] = arg.replace('{{source_root_relative_dir}}', file_rel_dir)
      if '{{source_gen_dir}}' in arg:
        file_dir = os.path.dirname(src)
        gen_dir = os.path.join(gn_out_dir, file_dir.replace(root_dir, 'gen'))
        args_for_src[index] = arg.replace('{{source_gen_dir}}', gen_dir)
      if '{{source_out_dir}}' in arg:
        file_dir = os.path.dirname(src)
        out_dir = os.path.join(gn_out_dir, file_dir.replace(root_dir, 'obj'))
        args_for_src[index] = arg.replace('{{source_out_dir}}', out_dir)
      if '{{source_target_relative}}' in arg:
        file_path = os.path.relpath(src, target_dir)
        args_for_src[index] = arg.replace('{{source_target_relative}}', file_path)

    arguments_str = ' '.join(args_for_src)
    cmd = f"python3 {script} {arguments_str}"
    # print(f"execute command: {cmd}")
    p = Popen(cmd, shell=True, universal_newlines=True, encoding="utf-8")
    p.communicate()
    if p.returncode!= 0:
      sys.exit(1)
  
  return 0

if __name__ == '__main__':
  sys.exit(main())
