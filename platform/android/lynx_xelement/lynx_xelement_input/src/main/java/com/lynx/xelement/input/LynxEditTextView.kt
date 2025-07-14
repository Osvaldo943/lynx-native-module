// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input

import android.annotation.SuppressLint
import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import kotlin.math.abs

const val SCROLL_THRESHOLD = 10

class LynxEditTextView: LynxEditText {

    interface OnSelectionChangeListener {
        fun onSelectionChange(selStart: Int, selEnd: Int)
    }

    var onSelectionChangeListener:OnSelectionChangeListener? = null

    private var mAdjustInputMode = LYNX_EDIT_TEXT_LIGHT_INPUT_MODE_UNDEFINED
    private var mTouchStartX: Float = 0f
    private var mTouchStartY: Float = 0f
    private var mStartScrollY: Int = 0
    private var mIsScrolled: Boolean = false

    constructor(context: Context): super(context) {
    }

    constructor(context: Context, attrs: AttributeSet): super(context, attrs) {
    }

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int):
            super(context, attrs, defStyleAttr) {
    }

    override fun onSelectionChanged(selStart: Int, selEnd: Int) {
        super.onSelectionChanged(selStart, selEnd)
        onSelectionChangeListener?.onSelectionChange(selStart, selEnd)
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (!isFocusable) {
            return false
        }

        if (event == null) {
            return super.onTouchEvent(null)
        }

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                parent.requestDisallowInterceptTouchEvent(true)
                mTouchStartX = event.x
                mTouchStartY = event.y
                mStartScrollY = scrollY
            }
            MotionEvent.ACTION_MOVE -> {
                if ((!canScrollVertically(1) && event.y < mTouchStartY)
                    || (!canScrollVertically(-1) && event.y > mTouchStartY)) {
                    parent.requestDisallowInterceptTouchEvent(false)
                }
            }
            MotionEvent.ACTION_UP -> {
                parent.requestDisallowInterceptTouchEvent(false)
                mTouchStartX = 0f
                mTouchStartY = 0f
                mIsScrolled = Math.abs(scrollY - mStartScrollY) > SCROLL_THRESHOLD
            }
            MotionEvent.ACTION_CANCEL -> {
                parent.requestDisallowInterceptTouchEvent(false)
                mTouchStartX = 0f
                mTouchStartY = 0f
                mIsScrolled = abs(scrollY - mStartScrollY) > SCROLL_THRESHOLD
            }
        }
        return super.onTouchEvent(event)
    }
}
