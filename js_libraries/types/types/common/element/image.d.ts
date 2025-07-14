// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, ImageErrorEvent, ImageLoadEvent } from '../events';
import { StandardProps } from '../props';

/**
 * Used to display images
 */
export interface ImageProps extends StandardProps {
  /**
   * Supports http/https/base64
   * @defaultValue ""
   * @since 0.2
   */
  src?: string;

  /**
   * Specifies image cropping/scaling mode
   * scaleToFill: Scales the image without preserving the aspect ratio, stretching the image to fill the element
   * aspectFit: Scales the image while preserving aspect ratio so that the long side is fully visible
   * aspectFill: Scales the image while preserving aspect ratio, ensuring the short side fills the element
   * center: Does not scale the image; image is centered
   * @defaultValue "scaleToFill"
   * @since 0.2
   */
  mode?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * ARGB_8888: 32-bit memory per pixel, supports semi-transparent images
   * RGB_565: 16-bit memory per pixel, reduces memory usage but loses transparency
   * @defaultValue "ARGB_8888"
   * @since 1.4
   */
  'image-config'?: 'ARGB_8888' | 'RGB_565';

  /**
   * Placeholder image, used same as src
   * @defaultValue ""
   * @since 1.4
   */
  placeholder?: string;

  /**
   * Image blur radius
   * @defaultValue "3px"
   * @since 0.2
   */
  'blur-radius'?: string;

  /**
   * Stretchable area for 9patch images, in percentage or decimal, four values for top, right, bottom, left
   * @defaultValue "0.2 10% 0.3 20%"
   * @since 1.4
   */
  'cap-insets'?: string;

  /**
   * Number of times an animated image plays
   * @defaultValue 1
   * @since 1.4
   */
  'loop-count'?: number;

  /**
   * Image won't load if its size is 0, but will load if prefetch-width is set
   * @defaultValue "100px"
   * @since 1.4
   */
  'prefetch-width'?: string;

  /**
   * Image won't load if its size is 0, but will load if prefetch-height is set
   * @defaultValue "100px"
   * @since 1.4
   */
  'prefetch-height'?: string;

  /**
   * If true, URL mapping is skipped. LynxView's custom ImageInterceptor won't work
   * @defaultValue false
   * @since 1.5
   */
  'skip-redirection'?: boolean;

  /**
   * Reduces the chance of OOM by downsampling large images, requires container support
   * @defaultValue false
   * @since iOS 2.0
   */
  downsampling?: boolean;

  /**
   * When set to true and the <image> element has no width or height,
   * the size of the <image> will be automatically adjusted
   * to match the image's original dimensions after the image is successfully loaded,
   * ensuring that the aspect ratio is maintained.
   * @defaultValue false
   * @since 2.6
   */
  'auto-size'?: boolean;

  /**
   * When set to true, the <image> will only clear the previously displayed image resource after a new image has successfully loaded.
   * The default behavior is to clear the image resource before starting a new load.
   * This can resolve flickering issues when the image src is switched and reloaded. It is not recommended to enable this in scenarios where there is node reuse in views like lists.
   * @defaultValue false
   * @since 2.7
   */
  'defer-src-invalidation'?: boolean;

  /**
   * Specifies whether the animated image should start playing automatically once it is loaded.
   * @defaultValue true
   * @since 2.11
   */
  'autoplay'?: boolean;

  /**
   * Changes the color of all non-transparent pixels to the tint-color specified. The value is a <color>.
   * @defaultValue ""
   * @since 2.12
   */
  'tint-color'?: string;

  /**
   * Support outputting image monitoring information in bindload
   * @defaultValue false
   * @since 2.12
   */
  'extra-load-info'?: boolean;

  /**
   * Disables unexpected iOS built-in animations
   * @defaultValue true
   * @since iOS 2.0
   */
  'implicit-animation'?: boolean;

  /**
   * Add custom parameters to image
   * @since 2.17
   */
  'additional-custom-info'?: { [key: string]: string };

  /**
   * Image load success event
   * @since 0.2
   */
  bindload?: (e: LoadEvent) => void;

  /**
   * Image load error event
   * @since 0.2
   */
  binderror?: (e: ErrorEvent) => void;

  /**
   * The animation will call back when it starts playing.
   * @since 2.11
   */
  bindstartplay?: (e: BaseEvent) => void;

  /**
   * Call back after one loop time of the animation is played.
   * @since 2.11
   */
  bindcurrentloopcomplete?: (e: BaseEvent) => void;

  /**
   * It will be called after the animation has been played for all loop-count times. If the loop-count is not set, it will not be called back.
   * @since 2.11
   */
  bindfinalloopcomplete?: (e: BaseEvent) => void;
}

export type LoadEvent = BaseEvent<'load', ImageLoadEvent>;
export type ErrorEvent = BaseEvent<'error', ImageErrorEvent>;

/**
 * Restart the animation playback method controlled by the front end, and the animation playback progress and loop count will be reset.
 * @Android
 * @iOS
 * @deprecated Deprecated. Some scenarios may not call back the call result. It is recommended to use resumeAnimation instead.
 */
export interface ImageStartAnimMethod extends BaseMethod {
  method: 'startAnimate';
}

/**
 * Resumes the animation, without resetting the loop-count.
 * @Android
 * @iOS
 * @since 2.11
 */
export interface ImageResumeAnimMethod extends BaseMethod {
  method: 'resumeAnimation';
}

/**
 * Pauses the animation, without resetting the loop-count.
 * @Android
 * @iOS
 * @since 2.11
 */
export interface ImagePauseAnimMethod extends BaseMethod {
  method: 'pauseAnimation';
}

/**
 * Stops the animation, and it will reset the loop-count.
 * @Android
 * @iOS
 * @since 2.11
 */
export interface ImageStopAnimMethod extends BaseMethod {
  method: 'stopAnimation';
}

export type ImageUIMethods = ImageStartAnimMethod | ImageResumeAnimMethod | ImagePauseAnimMethod | ImageStopAnimMethod;
