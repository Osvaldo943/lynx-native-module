# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


class Summary:
    def __init__(self):
        self.childs = []
        self.values = {}

    def insert(self, key, value):
        self.values[key] = value

    def append(self, summary):
        self.childs.append(summary)


class SummaryConsumer:
    def __init__(self):
        pass

    def accept(self, summary: Summary):
        pass
