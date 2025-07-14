// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEVTOOL_PLATFORM_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_DEVTOOL_PLATFORM_EMBEDDER_H_

#include <memory>
#include <string>
#include <vector>

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {

namespace tasm {
class TemplateData;
}  // namespace tasm

namespace devtool {

class LynxDevToolProxy;
class InspectorOwnerEmbedder;
class ScreenCastHelperEmbedder;
class PageReloadHelperEmbedder;

class DevtoolPlatformEmbedder
    : public std::enable_shared_from_this<DevtoolPlatformEmbedder> {
 public:
  DevtoolPlatformEmbedder();
  ~DevtoolPlatformEmbedder();

  void Init(devtool::LynxDevToolProxy* proxy,
            const std::shared_ptr<InspectorOwnerEmbedder>& owner);

  int FindNodeIdForLocation(float x, float y);

  void StartCasting(int32_t quality, int32_t max_width, int32_t max_height);

  void StopCasting();

  void ContinueCasting();

  void PauseCasting();

  void OnLoadTemplate(const std::string& url, const std::vector<uint8_t>& tem,
                      const std::shared_ptr<tasm::TemplateData>& init_data);

  void Reload(bool ignore_cache);

  void Navigate(const std::string& url);

  std::string GetTemplateUrl();

  std::shared_ptr<tasm::TemplateData> getTemplateDate();

  std::shared_ptr<devtool::DevToolPlatformFacade> GetDevtoolPlatformFacade() {
    return devtool_platform_facade_;
  }

  void GetLynxScreenShot();

 public:
  void SendScreenCast(const std::string& data,
                      const lynx::devtool::ScreenMetadata& metadata);
  void SendScreenCapture(const std::string& data);

  void SendConsoleEvent(const std::string& message, int32_t level,
                        int64_t time_stamp);

  void DispatchScreencastVisibilityChanged(bool status);

  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout);

 private:
  std::shared_ptr<lynx::devtool::DevToolPlatformFacade>
      devtool_platform_facade_;
  std::unique_ptr<ScreenCastHelperEmbedder> cast_helper_;
  std::unique_ptr<PageReloadHelperEmbedder> reload_helper_;
  std::weak_ptr<InspectorOwnerEmbedder> weak_owner_;
  devtool::LynxDevToolProxy* proxy_ = nullptr;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEVTOOL_PLATFORM_EMBEDDER_H_
