// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_RAW_TEXT_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_RAW_TEXT_ELEMENT_H_

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class RawTextElement : public FiberElement {
 public:
  RawTextElement(ElementManager* manager);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new RawTextElement(*this, clone_resolved_props));
  }

  bool is_raw_text() const override { return true; }
  void SetText(const lepus::Value& text);
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  ParallelFlushReturn PrepareForCreateOrUpdate() override;

  const base::String& content() const { return content_; }
  size_t content_utf16_length() const { return content_utf16_length_; }

  constexpr const static char kRawTextTag[] = "raw-text";
  constexpr const static char kTextAttr[] = "text";

 protected:
  RawTextElement(const RawTextElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

 private:
  base::String content_;
  // TODO(linxs): Use base::String.length_utf16() after its implementation has
  // been optimized
  size_t content_utf16_length_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_RAW_TEXT_ELEMENT_H_
