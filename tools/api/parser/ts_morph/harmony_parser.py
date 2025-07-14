# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import sys
import os
import re
from metadata_def import BaseObject, BaseMember, BaseParam
from collections import deque
from env_setup import (
    NODE_PATH,
    API_CONFIG,
    LYNX_ROOT_PATH,
    API_DOC_ANNOTATION,
    HARMONY_API_PATH,
)
import subprocess


class Parser:
    def __init__(self):
        pass


class HarmonyParser(Parser):
    def __init__(self):
        super().__init__()
        if API_CONFIG.get("harmony") is None:
            self._need_to_parse = False
            return

        self.arkts_parser_path: str = os.path.join(os.path.dirname(__file__), "cli.ts")

        self._need_to_parse = True
        self.command: list[str] = ["npx", "ts-node", self.arkts_parser_path, "--file"]
        self._lynx_so_index_file = os.path.join(
            LYNX_ROOT_PATH, API_CONFIG["harmony"]["lynx_so_index_file"]
        )
        self.export_object_dict: dict = {}
        self.metadata_path: str = os.path.join(
            LYNX_ROOT_PATH, API_CONFIG["harmony"]["metadata_path"]
        )
        self.api_file: str = os.path.join(HARMONY_API_PATH, "lynx_harmony.api")

    def _ensure_parser(self) -> bool:
        esbuild_path = os.path.join(LYNX_ROOT_PATH, API_CONFIG["path"]["esbuild"])
        try:
            subprocess.run(
                [
                    esbuild_path,
                    "cli.ts",
                    "--bundle",
                    "--platform=node",
                    "--minify",
                    "--outfile=dist/arkts-parser.js",
                ],
                capture_output=True,
                text=True,
                check=True,
                cwd=os.path.dirname(__file__),
            )
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            print(f"install arkts parser failed: {e.stderr}", file=sys.stderr)
            return False
        return True

    def parse(self) -> list[BaseObject]:
        self.export_object_dict = self._parse_ets_exports_recursive()
        export_object_list = [
            {"path": path, "exports": exports}
            for path, exports in self.export_object_dict.items()
        ]
        object_in_json = json.dumps(export_object_list, indent=2)

        object_list: list[BaseObject] = []

        self.command.append(object_in_json)
        try:
            result = subprocess.run(
                self.command,
                capture_output=True,
                text=True,
                check=True,
                cwd=os.path.dirname(__file__),
            )
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            print(f"generate api metadata failed: {e.stderr}", file=sys.stderr)
            return object_list

        data_list = json.loads(result.stdout)

        object_list = [BaseObject(**data) for data in data_list]
        for object in object_list:
            object.children = [BaseMember(**member) for member in object.children]
            for member in object.children:
                member.params = (
                    [BaseParam(**param) for param in member.params]
                    if member.params
                    else []
                )
                member.returns = BaseParam(**member.returns) if member.returns else None

        return object_list

    def dump(self) -> bool:
        if not self._need_to_parse:
            return True

        if os.path.exists(self.api_file):
            os.remove(self.api_file)

        object_list: list[BaseObject] = self.parse()
        with open(self.api_file, "at") as f:
            f.write(API_DOC_ANNOTATION)
            f.writelines([object.get_api_str() for object in object_list])

        return True

    def _clean_exported_item(self, item_str: str) -> str:
        return item_str.split(" as ")[0].strip()

    def _resolve_absolute_path(
        self, current_file_abs_dir: str, relative_import_path: str
    ) -> str:
        path_to_resolve = relative_import_path
        if "liblynx.so" in path_to_resolve:
            return self._lynx_so_index_file

        if (
            not path_to_resolve.endswith(".so")
            and not os.path.splitext(path_to_resolve)[1]
        ):
            path_to_resolve += ".ets"

        return os.path.normpath(os.path.join(current_file_abs_dir, path_to_resolve))

    def _parse_single_file_content_exports(
        self,
        file_content: str,
        current_file_abs_path: str,
        target_exports_map: dict,
        files_to_process_queue: deque,
        processed_abs_paths_set: set,
    ):
        export_pattern = re.compile(
            r"export\s+(\*|\{[\s\S]*?\})\s+from\s+['\"]([^'\"]+)['\"];?"
        )

        current_file_directory = os.path.dirname(current_file_abs_path)

        for match in export_pattern.finditer(file_content):
            exported_specifier = match.group(1).strip()
            path_in_from_clause = match.group(2).strip()
            abs_path_to_potentially_queue = self._resolve_absolute_path(
                current_file_directory, path_in_from_clause
            )

            if exported_specifier == "*":
                if abs_path_to_potentially_queue.endswith(".ets"):
                    if os.path.isfile(abs_path_to_potentially_queue):
                        if (
                            abs_path_to_potentially_queue not in files_to_process_queue
                            and abs_path_to_potentially_queue
                            not in processed_abs_paths_set
                        ):
                            files_to_process_queue.append(abs_path_to_potentially_queue)
            else:
                items_str_inside_braces = exported_specifier[1:-1].strip()

                cleaned_exported_objects = []
                if items_str_inside_braces:
                    raw_object_strs = [
                        item.strip()
                        for item in items_str_inside_braces.split(",")
                        if item.strip()
                    ]
                    cleaned_exported_objects = [
                        self._clean_exported_item(obj_str)
                        for obj_str in raw_object_strs
                    ]

                if abs_path_to_potentially_queue not in target_exports_map:
                    target_exports_map[abs_path_to_potentially_queue] = []

                for obj_name in cleaned_exported_objects:
                    if (
                        obj_name
                        not in target_exports_map[abs_path_to_potentially_queue]
                    ):
                        target_exports_map[abs_path_to_potentially_queue].append(
                            obj_name
                        )

    def _parse_ets_exports_recursive(self) -> dict:
        files_to_process_queue = deque()
        if not os.path.isfile(self.metadata_path):
            print(f"'{self.metadata_path}' not found.")
            return {}

        files_to_process_queue.append(self.metadata_path)

        processed_abs_paths = set()
        final_exports_map = {}

        while files_to_process_queue:
            current_abs_path_to_process = files_to_process_queue.popleft()

            if current_abs_path_to_process in processed_abs_paths:
                continue

            processed_abs_paths.add(current_abs_path_to_process)

            try:
                with open(current_abs_path_to_process, "r", encoding="utf-8") as f:
                    file_content = f.read()
                self._parse_single_file_content_exports(
                    file_content,
                    current_abs_path_to_process,
                    final_exports_map,
                    files_to_process_queue,
                    processed_abs_paths,
                )
            except FileNotFoundError:
                print(f"'{current_abs_path_to_process}' not found. Skipping...")
            except Exception as e:
                print(f"'{current_abs_path_to_process}' processing error: {e}")

        return final_exports_map


if __name__ == "__main__":
    parser = HarmonyParser()
    parser.dump()
