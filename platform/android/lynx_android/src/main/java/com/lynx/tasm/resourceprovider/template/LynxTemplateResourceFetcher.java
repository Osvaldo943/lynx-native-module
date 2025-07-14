// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.template;

import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;

/**
 * @apidoc
 * @brief `LynxTemplateResourceFetcher` is defined inside LynxEngine and
 * injected from outside to implement the resource loading interface of
 * [Bundle](/guide/spec.html#lynx-bundle-or-bundle) and [Lazy Bundle](/guide/spec.html#lazy-bundle)
 * etc.
 */
public abstract class LynxTemplateResourceFetcher {
  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to obtain the contents of Bundle
   * and Lazy Bundle. The returned result type can contain `byte[]` or `TemplateBundle`.
   *
   * @param request Request for the requiring content file.
   * @param callback Response with the requiring content file: byteArray or TemplateBundle
   * @note This method must be implemented.
   */
  public abstract void fetchTemplate(
      LynxResourceRequest request, LynxResourceCallback<TemplateProviderResult> callback);

  /**
   * fetch SSRData of lynx.
   *
   * @param request
   * @param callback response with the requiring ssr data.
   */
  public abstract void fetchSSRData(
      LynxResourceRequest request, LynxResourceCallback<byte[]> callback);
}
