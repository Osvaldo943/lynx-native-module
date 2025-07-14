// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/page_reload_helper_embedder.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/renderer/data/template_data.h"

namespace lynx {
namespace devtool {

PageReloadHelperEmbedder::PageReloadHelperEmbedder(
    devtool::LynxDevToolProxy* proxy)
    : proxy_(proxy), init_data_(nullptr) {}

void PageReloadHelperEmbedder::OnLoadTemplate(
    const std::string& url, const std::vector<uint8_t>& tem,
    const std::shared_ptr<tasm::TemplateData>& init_data) {
  binary_ = tem;
  url_ = url;

  init_data_ = init_data;
}

std::string PageReloadHelperEmbedder::GetURL() { return url_; }

void PageReloadHelperEmbedder::Reload(bool ignore_cache) {
  lynx::base::UIThread::GetRunner()->PostTask(
      [proxy = proxy_, url = url_, binary = binary_, init_data = init_data_] {
        if (proxy != nullptr) {
          proxy->ReloadTemplate(url, binary, init_data);
        }
      });
}

void PageReloadHelperEmbedder::Navigate(const std::string& url) {
  url_ = url;
  binary_.clear();

  if (proxy_ != nullptr) {
    proxy_->LoadTemplateFromURL(url);
  }
}

std::shared_ptr<tasm::TemplateData>
PageReloadHelperEmbedder::GetTemplateData() {
  return init_data_;
}

}  // namespace devtool
}  // namespace lynx
