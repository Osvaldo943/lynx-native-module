# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.utils.log import Log
from plugins.android_ut_plugin import AndroidUTPlugin
from plugins.coverage_check_plugin import CoverageCheckerPlugin
from plugins.fuzzer_test_plugin import FuzzerTestPlugin
from plugins.native_ut_plugin import NativeUTPlugin
from plugins.project_init_plugin import InitPlugin
from core.base.result import Ok, Err
from core.base.constants import Constants


def PluginFactory(plugin_name):
    plugin = None
    if plugin_name == "NativeUT":
        plugin = NativeUTPlugin()
    elif plugin_name == "Init":
        plugin = InitPlugin()
    elif plugin_name == "CoverageChecker":
        plugin = CoverageCheckerPlugin()
    elif plugin_name == "AndroidUT":
        plugin = AndroidUTPlugin()
    elif plugin_name == "FuzzerTest":
        plugin = FuzzerTestPlugin()

    if plugin is None:
        return Err(Constants.PLUGIN_BUILD_ERR, f"Unsupported plugin type {plugin_name}")

    return Ok(plugin)
