# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.checker.android_coverage_checker import AndroidCoverageChecker
from core.checker.ios_coverage_checker import iOSCoverageChecker
from core.checker.native_coverage_checker import NativeCoverageChecker
from core.base.result import Err, Ok
from core.base.constants import Constants


def CheckerFactory(check_type: str):
    checker = None
    if check_type == "android":
        checker = AndroidCoverageChecker()
    elif check_type == "ios":
        checker = iOSCoverageChecker()
    elif check_type == "cpp":
        checker = NativeCoverageChecker()
    if checker is None:
        return Err(
            Constants.COVERAGE_CHECKER_BUILD_ERR, f"Unsupport check_type {check_type}"
        )
    return Ok(checker)
