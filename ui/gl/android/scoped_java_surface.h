// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_ANDROID_SCOPED_JAVA_SURFACE_H_
#define UI_GL_ANDROID_SCOPED_JAVA_SURFACE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/move.h"
#include "ui/gl/gl_export.h"

namespace gfx {

class SurfaceTextureBridge;

// A helper class for holding a scoped reference to a Java Surface instance.
// When going out of scope, Surface.release() is called on the Java object to
// make sure server-side references (esp. wrt graphics memory) are released.
class GL_EXPORT ScopedJavaSurface {
  MOVE_ONLY_TYPE_FOR_CPP_03(ScopedJavaSurface, RValue);

 public:
  ScopedJavaSurface();

  // Wraps an existing Java Surface object in a ScopedJavaSurface.
  explicit ScopedJavaSurface(const base::android::JavaRef<jobject>& surface);

  // Creates a Java Surface from a SurfaceTexture and wraps it in a
  // ScopedJavaSurface.
  explicit ScopedJavaSurface(const SurfaceTextureBridge* surface_texture);

  // Move constructor. Take the surface from another ScopedJavaSurface object,
  // the latter no longer owns the surface afterwards.
  ScopedJavaSurface(RValue rvalue);
  ScopedJavaSurface& operator=(RValue rhs);

  // Creates a ScopedJavaSurface that is owned externally, i.e.,
  // someone else is responsible to call Surface.release().
  static ScopedJavaSurface AcquireExternalSurface(jobject surface);

  ~ScopedJavaSurface();

  // Check whether the surface is an empty one.
  bool IsSurfaceEmpty();

  const base::android::JavaRef<jobject>& j_surface() const {
    return j_surface_;
  }

 private:
  // Performs destructive move from |other| to this.
  void MoveFrom(ScopedJavaSurface& other);

  // Sets whether the reference to the underlying resource (see
  // Surface.release()) will released during the destruction of this object.
  // |True| by default.
  void SetAutoRelease(bool auto_release);

  bool auto_release_;

  base::android::ScopedJavaGlobalRef<jobject> j_surface_;
};

}  // namespace gfx

#endif  // UI_GL_ANDROID_SCOPED_JAVA_SURFACE_H_
