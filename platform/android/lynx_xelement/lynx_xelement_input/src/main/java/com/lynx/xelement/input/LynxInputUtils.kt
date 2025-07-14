// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.input

import android.os.Build
import android.text.Layout
import android.text.StaticLayout
import android.text.TextDirectionHeuristic
import android.text.TextDirectionHeuristics
import android.text.TextUtils
import android.view.Gravity
import android.view.View
import android.widget.EditText
import com.lynx.tasm.behavior.shadow.text.StaticLayoutCompat
import com.lynx.tasm.behavior.shadow.text.TextAttributes

class LynxInputUtils() {
    fun getLayoutInEditText(charSequence: CharSequence,
                            editText: EditText,
                            measuredWidth: Int,
                            measuredHeight: Int): Layout {
        val textAlign = when (editText.gravity) {
            Gravity.LEFT -> Layout.Alignment.ALIGN_NORMAL
            Gravity.CENTER -> Layout.Alignment.ALIGN_CENTER
            Gravity.RIGHT -> Layout.Alignment.ALIGN_OPPOSITE
            else -> Layout.Alignment.ALIGN_NORMAL
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val textDirectionHeuristics = when (editText.textDirection) {
                View.TEXT_DIRECTION_LTR -> TextDirectionHeuristics.LTR
                View.TEXT_DIRECTION_RTL -> TextDirectionHeuristics.RTL
                View.TEXT_DIRECTION_LOCALE -> TextDirectionHeuristics.LOCALE
                else -> TextDirectionHeuristics.LTR
            }
            val builder = StaticLayout.Builder
                .obtain(
                    charSequence,
                    0,
                    charSequence.length,
                    editText.paint,
                    measuredWidth
                )
                .setAlignment(textAlign)
                .setTextDirection(textDirectionHeuristics)
                .setLineSpacing(editText.lineSpacingExtra, editText.lineSpacingMultiplier)
                .setIncludePad(editText.includeFontPadding)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                builder.setUseLineSpacingFromFallbacks(true)
            }
            return builder.build()
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            val textDirectionHeuristics: TextDirectionHeuristic = when (editText.textDirection) {
                View.TEXT_DIRECTION_LTR -> TextDirectionHeuristics.LTR
                View.TEXT_DIRECTION_RTL -> TextDirectionHeuristics.RTL
                View.TEXT_DIRECTION_LOCALE -> TextDirectionHeuristics.LOCALE
                else -> TextDirectionHeuristics.LTR
            }
            return StaticLayoutCompat.get(
                charSequence,
                0,
                charSequence.length,
                editText.paint,
                measuredWidth,
                textAlign,
                editText.lineSpacingMultiplier,
                editText.lineSpacingExtra,
                editText.includeFontPadding,
                TextUtils.TruncateAt.END,
                TextAttributes.NOT_SET,
                textDirectionHeuristics
            )
        } else {
            return StaticLayout(
                charSequence,
                0,
                charSequence.length,
                editText.paint,
                measuredWidth,
                textAlign,
                editText.lineSpacingMultiplier,
                editText.lineSpacingExtra,
                editText.includeFontPadding,
                TextUtils.TruncateAt.END,
                measuredWidth
            )
        }
    }
}