# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from core.base.result import Ok


class Container:
    def __init__(self):
        pass

    def run(self, targets, filter: str):
        re = self.before_test(targets, filter)
        if re.is_err():
            return re
        re = self.test()
        if re.is_err():
            return re
        re = self.after_test()
        if re.is_err():
            return re
        return Ok()

    def before_test(self, targets, filter: str):
        pass

    def after_test(self):
        pass

    def test(self):
        pass

    def get_summary(self):
        pass
