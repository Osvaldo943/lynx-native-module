# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


class Result:
    def __init__(self):
        pass

    def is_ok(self):
        assert False, "Result has not implemented this method"

    def is_err(self):
        return not self.is_ok()

    def get_value(self):
        assert False, "Result has not implemented this method"

    def get_code(self):
        assert False, "Result has not implemented this method"

    def get_msg(self):
        assert False, "Result has not implemented this method"

    def to_string(self):
        pass

    def __str__(self):
        return self.to_string()


class Err(Result):
    def __init__(self, code, msg):
        super().__init__()
        self.code = code
        self.msg = msg

    def is_ok(self):
        return False

    def get_value(self):
        assert False, "The Err object does not contain a value"

    def get_code(self):
        return self.code

    def get_msg(self):
        return self.msg

    def to_string(self):
        return f"Err: {self.msg}. code: {self.code.name}:{self.code.value}"


class Ok(Result):
    def __init__(self, value=None):
        super().__init__()
        self.value = value

    def is_ok(self):
        return True

    def get_value(self):
        return self.value

    def get_code(self):
        assert False, "The Ok object does not contain a code"

    def get_msg(self):
        assert False, "The Ok object does not contain a msg"

    def to_string(self):
        return "Ok"
