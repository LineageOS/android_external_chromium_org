// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Includes the platform independent and platform dependent GL headers.
// Only include this in cc files. It pulls in system headers, including
// the X11 headers on linux, which define all kinds of macros that are
// liable to cause conflicts.

#ifndef APP_GFX_GL_GL_BINDINGS_H_
#define APP_GFX_GL_GL_BINDINGS_H_

#include <GL/gl.h>
#include <GL/glext.h>

#include "build/build_config.h"

// The standard OpenGL native extension headers are also included.
#if defined(OS_WIN)
#include <GL/wglext.h>
#elif defined(OS_LINUX)
#include <GL/glx.h>
#include <GL/glxext.h>

// Undefine some macros defined by X headers. This is why this file should only
// be included in .cc files.
#undef Bool
#undef None
#undef Status

#elif defined(OS_MACOSX)
#include <OpenGL/OpenGL.h>
#endif

#if defined(OS_WIN)
#define GL_BINDING_CALL WINAPI
#else
#define GL_BINDING_CALL
#endif

// Forward declare OSMesa types.
typedef struct osmesa_context *OSMesaContext;
typedef void (*OSMESAproc)();

#if defined(OS_WIN)

// Forward declare EGL types.
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef int EGLint;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);

#endif  // OS_WIN

#include "gl_bindings_autogen_gl.h"
#include "gl_bindings_autogen_osmesa.h"

#if defined(OS_WIN)
#include "gl_bindings_autogen_egl.h"
#include "gl_bindings_autogen_wgl.h"
#elif defined(OS_LINUX)
#include "gl_bindings_autogen_glx.h"
#endif

namespace gfx {

// Find an entry point to the mock GL implementation.
void* GL_BINDING_CALL GetMockGLProcAddress(const char* name);

}  // namespace gfx

#endif  // APP_GFX_GL_GL_BINDINGS_H_
