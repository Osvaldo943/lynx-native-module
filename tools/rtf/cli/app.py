# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from plugin_manager.plugin_manager import PluginManager
from args_parser.args_parser import ArgsParser
from core.env.env import RTFEnv
from core.utils.log import Log


def main():
    RTFEnv.init_project_env()
    plugin_manager = PluginManager()
    result = plugin_manager.init_plugin(RTFEnv.config.plugins)
    if result.is_err():
        Log.error(f"Plugin init error : {result}")
        return result.get_code().value
    args_parser = ArgsParser()
    args_parser.init_subparsers(plugin_manager.plugins)
    args = args_parser.parse_args()

    if args.plugin is not None:
        result = plugin_manager.dispatch_args(args)
        if result.is_err():
            Log.error(f"Plugin({args.plugin}) error : {result}")
            return result.get_code().value
        else:
            Log.success(f"Plugin({args.plugin}) success!")
    else:
        args_parser.print_help()

    return 0


if __name__ == "__main__":
    exit(main())
