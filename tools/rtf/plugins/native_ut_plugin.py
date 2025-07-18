# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import copy
from datetime import datetime
import os.path

from core.container.native_ut_container import NativeUTContainer
from core.env.context import RTFContext
from core.env.env import RTFEnv
from core.env.trait_template import TraitTemplate
from core.options.options import Options
from core.utils.log import Log
from core.base.result import Ok
from core.base.summary import Summary, SummaryConsumer
from plugins.plugin import Plugin
import json


class NativeUTListener(Options.OptionsListener):
    def __init__(self):
        super().__init__()

    def did_include(self, options, builders, targets, coverage):
        builder_map = {}
        label = self.generate_label(options.workspace)
        tmp_builder = copy.deepcopy(builders)
        for builder_name in tmp_builder.keys():
            builder_new_name = f"{builder_name}_{label}"
            builder_map[builder_name] = builder_new_name
            builders[builder_new_name] = builders[builder_name]
            del builders[builder_name]

        for builder in builders.values():
            builder["output"] = os.path.join(options.workspace, builder["output"])

        for target in targets.values():
            target["cwd"] = os.path.join(options.workspace, target["cwd"])
            if "builder" in target:
                target["builder"] = builder_map[target["builder"]]
            else:
                target["builder"] = builder_map["default"]

        coverage["ignores"] = [
            os.path.join(options.workspace, ignore) for ignore in coverage["ignores"]
        ]


class TraceEventSummary(SummaryConsumer):
    def __init__(self):
        super().__init__()

    def should_skip(self, values):
        if len(values) == 0:
            return True
        if len(values) == 1 and "name" in values:
            return True
        return False

    def accept_aux(self, traces, summary: Summary):
        if not self.should_skip(summary.values):
            t = {
                "name": "native_ut_monitor",
                "ph": "i",
                "ts": f"{int(datetime.timestamp(datetime.now())*1000)}",
                "args": summary.values,
            }
            traces.append(t)
        for sub_summary in summary.childs:
            self.accept_aux(traces, sub_summary)

    def accept(self, summary: Summary):
        traces = {"traceEvents": []}
        self.accept_aux(traces["traceEvents"], summary)
        Log.info(traces)
        trace_path = os.path.join(
            RTFEnv.get_project_root_path(), "native-ut-trace-events.json"
        )
        with open(trace_path, "w") as f:
            f.write(json.dumps(traces))


class NativeUTPlugin(Plugin):
    def __init__(self):
        super().__init__("native-ut")

    def accept(self, args):
        template_names = args.names
        for template_name in template_names:
            template = os.path.join(
                RTFEnv.get_env("rtf_root_path"), f"native-ut-{template_name}.template"
            )
            with open(template, "r") as template_file:
                context = RTFContext(RTFEnv.get_project_root_path())
                context.options = Options(context, args.args)
                context.options.register_listener(NativeUTListener())
                exec(template_file.read(), context.export())
                template = TraitTemplate.trait_from_context(context)
            if args.command == "run":
                return self.__handle_run_command(template, args)
            elif args.command == "list":
                return self.__handle_list_command(template_name, template)

    def __handle_list_command(self, template_name, template):
        Log.info(f"Targets list for {template_name}")
        index = 1
        targets = template["targets"]
        for _, target in enumerate(targets.keys(), start=1):
            enable = targets[target]["enable"] if "enable" in targets[target] else True
            if enable:
                owners = ",".join(targets[target]["owners"])
                Log.info(f"[{index}]: {target}  owners:({owners})")
                index += 1
        return Ok()

    def __handle_run_command(self, template, args):
        container = NativeUTContainer(
            template["builder"], template["coverage"], "native-ut"
        )
        result = container.run(template["targets"], args.target)
        TraceEventSummary().accept(container.get_summary())
        return result

    def help(self):
        return "run targets of native-ut"

    def build_command_args(self, subparser):
        subparser_subparsers = subparser.add_subparsers(
            title="command",
            description="run|list",
            dest="command",
            required=True,
        )

        subparser_subparser = subparser_subparsers.add_parser("run", help="run targets")

        subparser_subparser.add_argument(
            "--names",
            dest="names",
            nargs="*",
            required=True,
            help="Specify the template name",
        )

        subparser_subparser.add_argument(
            "--target",
            dest="target",
            default="all",
            help="Specify the target name",
        )

        subparser_subparser.add_argument(
            "--args",
            dest="args",
            default="",
            help='User custom params, eg: --args="args_1=true, args_2=1"',
        )

        subparser_subparser = subparser_subparsers.add_parser(
            "list", help="list targets"
        )

        subparser_subparser.add_argument(
            "--names",
            dest="names",
            nargs="*",
            required=True,
            help="Specify the template name",
        )

        subparser_subparser.add_argument(
            "--args",
            dest="args",
            default="",
            help='User custom params, eg: --args="args_1=true, args_2=1"',
        )
