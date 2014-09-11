// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_CHROMEOS_TOUCHSCREEN_DELEGATE_IMPL_H_
#define UI_DISPLAY_CHROMEOS_TOUCHSCREEN_DELEGATE_IMPL_H_

#include "base/memory/scoped_ptr.h"
#include "ui/display/chromeos/display_configurator.h"
#include "ui/display/display_export.h"

namespace ui {

class DISPLAY_EXPORT TouchscreenDelegateImpl
    : public DisplayConfigurator::TouchscreenDelegate {
 public:
  TouchscreenDelegateImpl();
  virtual ~TouchscreenDelegateImpl();

  // DisplayConfigurator::TouchscreenDelegate overrides:
  virtual void AssociateTouchscreens(
      std::vector<DisplayConfigurator::DisplayState>* displays) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchscreenDelegateImpl);
};

}  // namespace ui

#endif  // UI_DISPLAY_CHROMEOS_TOUCHSCREEN_DELEGATE_IMPL_H_
