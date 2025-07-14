// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.media;

import android.graphics.drawable.Drawable;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import java.io.Closeable;

/**
 * @apidoc
 * @brief `LynxMediaResourceFetcher` is defined inside LynxEngine and
 * injected from outside to implement the path redirection capability
 * of `Image` and other third-party resources.
 */
public abstract class LynxMediaResourceFetcher {
  /**
   * @apidoc
   * @brief `LynxEngine` redirects the image path by calling this method internally,
   * and the return result is required to be of `String` type
   * @param request Request for the resource.
   * @param callback The target url.
   * @note This method must be implemented.
   */
  public abstract String shouldRedirectUrl(LynxResourceRequest request);

  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to determine whether the
   * resource path exists on disk.
   *
   * @param url Input path.
   * @return TRUE if is a local path, FALSE if not a local path and UNDEFINED if not sure.
   * @note This method is optional to be implemented.
   */
  public OptionalBool isLocalResource(String url) {
    return OptionalBool.UNDEFINED;
  }

  /**
   * @apidoc
   * @brief `LynxEngine` will use this method to obtain the bitmap information of the image,
   * and the return content must be of `Closeable` type.
   *
   * @param request Request for the resource.
   * @param callback Response with the needed drawable.
   * @note This method is optional to be implemented.
   */
  public void fetchImage(LynxResourceRequest request, LynxResourceCallback<Closeable> callback) {}
}
