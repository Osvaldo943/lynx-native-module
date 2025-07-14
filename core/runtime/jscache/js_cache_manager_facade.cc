// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager_facade.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jscache/js_cache_manager.h"
#include "core/runtime/jscache/quickjs/quickjs_cache_generator.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace piper {
namespace cache {
void JsCacheManagerFacade::PostCacheGenerationTask(
    const tasm::LynxTemplateBundle& bundle, const std::string& template_url,
    JSRuntimeType engine_type,
    std::unique_ptr<BytecodeGenerateCallback> callback) {
  std::unordered_map<std::string, JsContent> sources =
      bundle.GetJsBundle().GetAllJsFiles();
  bool is_card = bundle.IsCard();
  for (auto it = sources.begin(); it != sources.end();) {
    if (it->second.IsSourceCode()) {
      if (!is_card) {
        auto source_url = it->first;
        auto node = sources.extract(it++);
        node.key() = piper::App::GenerateDynamicComponentSourceUrl(template_url,
                                                                   source_url);
        sources.insert(std::move(node));
      } else {
        it++;
      }
    } else {
      it = sources.erase(it);
    }
  }
  if (sources.size() > 0) {
    PostCacheGenerationTask(template_url, std::move(sources), engine_type,
                            std::move(callback));
  }
}

void JsCacheManagerFacade::PostCacheGenerationTask(
    const std::string& template_url,
    std::unordered_map<std::string, JsContent> js_contents,
    JSRuntimeType engine_type,
    std::unique_ptr<BytecodeGenerateCallback> callback) {
  LOGI("JsCacheManagerFacade::PostCacheGenerationTask template_url: "
       << template_url << " engine_type: " << static_cast<int>(engine_type));
  switch (engine_type) {
    case JSRuntimeType::v8:
      LOGI("PostCacheGenerationTask for V8 is not supported");
      return;
    case JSRuntimeType::jsc:
      LOGI("PostCacheGenerationTask for JSC is not supported");
      return;
    case JSRuntimeType::quickjs: {
      PostCacheGenerationTaskQuickJs(template_url, std::move(js_contents),
                                     std::move(callback));
      return;
    }
    case JSRuntimeType::jsvm:
      LOGI("PostCacheGenerationTask for JSVM is not supported");
      return;
  }
}

void JsCacheManagerFacade::ClearBytecode(const std::string& template_url,
                                         JSRuntimeType engine_type) {
  if (engine_type == JSRuntimeType::quickjs) {
    JsCacheManager::GetQuickjsInstance().ClearCache(template_url);
  } else if (engine_type == JSRuntimeType::v8) {
    JsCacheManager::GetV8Instance().ClearCache(template_url);
  }
}

inline void JsCacheManagerFacade::PostCacheGenerationTaskQuickJs(
    const std::string& template_url,
    std::unordered_map<std::string, JsContent> js_contents,
    std::unique_ptr<BytecodeGenerateCallback> callback) {
#ifdef QUICKJS_CACHE_UNITTEST
  post_cache_generation_task_quickjs_for_testing(template_url, js_contents);
#else
  std::vector<std::unique_ptr<cache::CacheGenerator> > generators;
  for (const auto& iter : js_contents) {
    generators.push_back(std::make_unique<cache::QuickjsCacheGenerator>(
        std::move(iter.first), std::move(iter.second.GetBuffer())));
  }
  JsCacheManager::GetQuickjsInstance().RequestCacheGeneration(
      template_url, std::move(generators), false, std::move(callback));
#endif
}
}  // namespace cache
}  // namespace piper
}  // namespace lynx
