// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/style_object_encoder/style_object_parser.h"

#include <utility>

#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx::tasm {

bool StyleObjectParser::Parse(const rapidjson::Value &value) {
  if (!value.IsArray()) {
    return false;
  }
  for (const auto &styleObj : value.GetArray()) {
    if (!styleObj.IsObject()) {
      return false;
    }
    StyleMap style;
    auto configs = tasm::CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
        compile_options_);
    for (auto itr = styleObj.MemberBegin(); itr != styleObj.MemberEnd();
         ++itr) {
      const auto &name = itr->name.GetString();
      CSSPropertyID id = CSSProperty::GetPropertyID(name);
      if (!CSSProperty::IsPropertyValid(id)) {
        return false;
      }
      lepus::Value css_value = lepus::jsonValueTolepusValue(itr->value);
      UnitHandler::Process(id, css_value, style, configs);
    }
    style_objects_.emplace_back(std::move(style));
  }
  return true;
}

}  // namespace lynx::tasm
