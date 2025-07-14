# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from plugins.plugin import Plugin
from plugins.plugin_factory import PluginFactory
from core.base.result import Ok


class PluginManager:
    def __init__(self):
        self.plugins = {}

    def init_plugin(self, plugins):
        for plugin_name in plugins:
            plugin = PluginFactory(plugin_name)
            if plugin.is_err():
                return plugin
            self.register_plugin(plugin.get_value())
        return Ok()

    def register_plugin(self, plugin: Plugin):
        self.plugins[plugin.name] = plugin

    def dispatch_args(self, args):
        plugin = self.plugins[args.plugin]
        return plugin.accept(args)
