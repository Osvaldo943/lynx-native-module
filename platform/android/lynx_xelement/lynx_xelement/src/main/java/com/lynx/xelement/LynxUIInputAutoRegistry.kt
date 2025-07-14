// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement


import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.xelement.input.LynxUIInput

@LynxGeneratorName(packageName = "com.lynx.xelement")
@LynxBehavior(tagName = ["input"], isCreateAsync = false)
open class LynxUIInputAutoRegistry(context: LynxContext) : LynxUIInput(context) {
    
}
