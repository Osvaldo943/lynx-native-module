# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import enum
import os.path
import subprocess

from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log
from core.base.result import Err, Ok
from core.base.constants import Constants


class AndroidTargetType(enum.Enum):
    APPLICATION = "application"
    LIBRARY = "library"


class AndroidUTTarget(Target):
    def __init__(self, params, name):
        super().__init__(params, name)

    def init_self_info(self):
        self.package = self.params["package"]
        self.symbol = self.params["symbol"] if "symbol" in self.params else None
        self.coverage_data_path = os.path.join(
            RTFEnv.get_project_root_path(), f"coverage_{self.name}.ec"
        )
        self.build_tasks.append(self.params["task"])

    def get_coverage_raw_data(self):
        return self.coverage_data_path

    def has_error(self):
        if super().has_error():
            return True
        # When the APK crashes or the test fails, the process exit code is 0, which is considered successful.
        # In this case, it is necessary to retrieve information from the logs.
        with open(self.log_file, "r") as log_file:
            content = log_file.read()
            if "FAILURES!!!" in content:
                return True
            if "Error in" in content:
                return True
            if "Process crashed" in content:
                return True
        return False

    def has_crash(self):
        if super().has_crash():
            return True
        with open(self.log_file, "r") as log_file:
            content = log_file.read()
            if "Process crashed" in content:
                return True
        return False

    def pull_xml_data(self):
        xml_pull_cmd = (
            f"adb -s {self.global_info['device_name']} pull "
            f"/sdcard/Android/data/{self.package}/files/{self.name}_output.xml "
            f"{RTFEnv.get_project_root_path()}"
        )
        try:
            subprocess.check_call(xml_pull_cmd, shell=True)
            self.insert_global_info(
                "test_xml",
                os.path.join(RTFEnv.get_project_root_path(), f"{self.name}_output.xml"),
            )
        except Exception as e:
            Log.warning(f"{self.name} pull xml data failed!")

    def uninstall_apk(self):
        uninstall_cmd = (
            f"adb -s {self.global_info['device_name']} uninstall {self.package}"
        )
        subprocess.check_call(uninstall_cmd, shell=True)

    def pull_resource(self):
        if not self.coverage:
            return
        self.pull_xml_data()
        if not self.has_error():
            pull_ec_file_cmd = f"adb -s {self.global_info['device_name']} pull /sdcard/coverage_{self.name}.ec {RTFEnv.get_project_root_path()}"
            subprocess.check_call(pull_ec_file_cmd, shell=True)

    def install_apk_aux(self):
        retry_times = 1
        retry = 0
        timeout = 120
        while True:
            Log.info(f"Installing apk for {self.name}")
            install_cmd = f"adb -s {self.global_info['device_name']} install -g {self.target_path}"
            try:
                subprocess.check_call(install_cmd, shell=True, timeout=timeout)
                return Ok()
            except subprocess.TimeoutExpired:
                Log.error(f"adb install timeout, retry ({retry}/{retry_times})")
                if retry < retry_times:
                    retry += 1
                    handler = self.global_info["restart_device_handler"]
                    if handler is not None:
                        handler()
                else:
                    break
            except Exception as e:
                return Err(
                    Constants.ANDROID_APK_INSTALL_ERR,
                    f"adb install failed for {self.name} \n: {e}",
                )
        return Err(
            Constants.ANDROID_APK_INSTALL_ERR,
            f"adb install {self.target_path} timeout: {timeout}s",
        )

    def install_apk(self):
        return self.install_apk_aux()

    def run(self):
        result = self.install_apk()
        if result.is_err():
            return result
        run_command = (
            f"adb -s {self.global_info['device_name']} shell am instrument -w -e package "
            f"com -e debug false -e coverage true -e coverageFile /sdcard/coverage_{self.name}.ec "
            f"-e module {self.name} {self.package}/androidx.test.runner.AndroidJUnitRunner"
        )
        log_file = open(self.log_file, "w")
        Log.info(f"Start run {self.name}")
        self.process = subprocess.Popen(
            run_command, shell=True, stdout=log_file, stderr=log_file
        )
        return Ok()

    def wait(self):
        if self.process:
            self.process.wait()
            self.pull_resource()
            self.uninstall_apk()


class AndroidUTApplicationTarget(AndroidUTTarget):
    def __init__(self, params, name):
        super().__init__(params, name)

    def init_self_info(self):
        super().init_self_info()
        assert (
            "application_task" in self.params
        ), "Error: Application target miss params `application_task`"
        assert (
            "application_apk" in self.params
        ), "Error: Application target miss params `application_task`"
        assert (
            "application_package" in self.params
        ), "Error: Application target miss params `application_package`"
        self.application_task = self.params["application_task"]
        self.application_apk = self.params["application_apk"]
        self.application_package = self.params["application_package"]
        self.build_tasks.append(self.application_task)

    def uninstall_apk(self):
        super().uninstall_apk()
        uninstall_cmd = f"adb -s {self.global_info['device_name']} uninstall {self.application_package}"
        subprocess.check_call(uninstall_cmd, shell=True)

    def install_apk(self):
        result = super().install_apk()
        if result.is_err():
            return result
        Log.info(f"Installing application apk for {self.name}")
        application_apk_path = os.path.join(
            RTFEnv.get_project_root_path(), self.application_apk
        )
        install_cmd = f"adb -s {self.global_info['device_name']} install -g {application_apk_path}"
        try:
            subprocess.check_call(install_cmd, shell=True)
            return Ok()
        except Exception as e:
            return Err(
                Constants.ANDROID_APK_INSTALL_ERR,
                f"adb install failed for {self.name} \n: {e}",
            )
