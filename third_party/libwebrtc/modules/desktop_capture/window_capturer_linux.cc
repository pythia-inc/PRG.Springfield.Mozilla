/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>

#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer.h"

#if defined(WEBRTC_USE_PIPEWIRE)
#if defined(WEBRTC_MOZILLA_BUILD)
#include "modules/desktop_capture/linux/wayland/moz_base_capturer_pipewire.h"
#else
#include "modules/desktop_capture/linux/wayland/base_capturer_pipewire.h"
#endif
#endif  // defined(WEBRTC_USE_PIPEWIRE)

#if defined(WEBRTC_USE_X11)
#include "modules/desktop_capture/linux/x11/window_capturer_x11.h"
#endif  // defined(WEBRTC_USE_X11)

namespace webrtc {

// static
std::unique_ptr<DesktopCapturer> DesktopCapturer::CreateRawWindowCapturer(
    const DesktopCaptureOptions& options) {
#if defined(WEBRTC_USE_PIPEWIRE)
  if (options.allow_pipewire() && DesktopCapturer::IsRunningUnderWayland()) {
#if defined(WEBRTC_MOZILLA_BUILD)
    return BaseCapturerPipeWire::CreateRawCapturer(options);
#else
    return std::make_unique<BaseCapturerPipeWire>(options);
#endif
  }
#endif  // defined(WEBRTC_USE_PIPEWIRE)

#if defined(WEBRTC_USE_X11)
  return WindowCapturerX11::CreateRawWindowCapturer(options);
#else
  return nullptr;
#endif  // defined(WEBRTC_USE_X11)
}

}  // namespace webrtc
