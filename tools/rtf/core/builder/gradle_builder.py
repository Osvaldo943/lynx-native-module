# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess
from typing import Callable

from core.builder.builder import Builder
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log
from core.base.result import Err, Ok
from core.base.constants import Constants


class GradleBuilder(Builder):
    def __init__(self, args: [str], workspace: str):
        super().__init__()
        self.args = args
        self.workspace = workspace

    def pre_action(self, skip: Callable[[], bool] = None):
        try:
            if skip is None or not skip():
                command = "./gradlew clean"
                subprocess.check_call(command, shell=True, cwd=self.workspace)
            return Ok()
        except Exception as e:
            return Err(Constants.CALL_COMMAND_ERR, f"run {command} failed : {e}")

    def build(self, target: Target):
        for task in target.build_tasks:
            build_cmd = f"./gradlew {task} {' '.join(self.args)}"
            try:
                Log.info(f"{task} start build!")
                subprocess.check_call(build_cmd, shell=True, cwd=self.workspace)
                target.target_path = os.path.join(
                    RTFEnv.get_project_root_path(), target.params["apk"]
                )
                Log.success(f"{task} build success!")
                return Ok()
            except Exception as e:
                return Err(Constants.CALL_COMMAND_ERR, f"{task} build failed!\n{e}")
