// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_ACTIONABLE_VIEW_H_
#define ASH_SYSTEM_TRAY_ACTIONABLE_VIEW_H_

#include "ash/ash_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/views/view.h"

namespace ash {

// A focusable view that performs an action when user clicks on it, or presses
// enter or space when focused. Note that the action is triggered on mouse-up,
// instead of on mouse-down. So if user presses the mouse on the view, then
// moves the mouse out of the view and then releases, then the action will not
// be performed.
// Exported for SystemTray.
class ASH_EXPORT ActionableView : public views::View {
 public:
  static const char kViewClassName[];

  ActionableView();

  ~ActionableView() override;

  void SetAccessibleName(const base::string16& name);
  const base::string16& accessible_name() const { return accessible_name_; }

 protected:
  void OnPaintFocus(gfx::Canvas* canvas);

  // Returns the bounds to paint the focus rectangle in.
  virtual gfx::Rect GetFocusBounds();

  // Performs an action when user clicks on the view (on mouse-press event), or
  // presses a key when this view is in focus. Returns true if the event has
  // been handled and an action was performed. Returns false otherwise.
  virtual bool PerformAction(const ui::Event& event) = 0;

  // Overridden from views::View.
  const char* GetClassName() const override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  void GetAccessibleState(ui::AXViewState* state) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;

  // Overridden from ui::EventHandler.
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  base::string16 accessible_name_;
  bool has_capture_;

  DISALLOW_COPY_AND_ASSIGN(ActionableView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_ACTIONABLE_VIEW_H_
