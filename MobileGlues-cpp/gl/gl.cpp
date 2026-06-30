// MobileGlues - gl/gl.cpp
// Core GL state wrappers: glClear, glClearDepth, glHint, glViewport, etc.
//
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "../includes.h"
#include <GL/gl.h>
#include "glcorearb.h"
#include "log.h"
#include "../gles/loader.h"
#include "mg.h"
#include <GLES3/gl32.h>

#define DEBUG 0

// ============================================================================
// glClear - handles legacy clear mask conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glClear, GLbitfield mask)
{
    // GL_ACCUM_BUFFER_BIT is not supported in ES, strip it
    mask &= ~0x00000200; // GL_ACCUM_BUFFER_BIT
    _native(mask);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glClear, mask)

// ============================================================================
// glClearDepth - desktop uses double, ES uses float
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glClearDepth, GLclampd depth)
{
    _native((GLfloat)depth);
    GLState.legacy.clearDepth = (GLfloat)depth;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glClearDepth, depth)

// ============================================================================
// glDepthRange - desktop uses double, ES uses float
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glDepthRange, GLclampd near_val, GLclampd far_val)
{
    static auto glDepthRangef = (decltype(&glDepthRangef))g_loader_handle("glDepthRangef");
    glDepthRangef((GLfloat)near_val, (GLfloat)far_val);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDepthRange, near_val, far_val)

// ============================================================================
// glViewport - track viewport state
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glViewport, GLint x, GLint y, GLsizei width, GLsizei height)
{
    _native(x, y, width, height);
    GLState.legacy.viewport[0] = x;
    GLState.legacy.viewport[1] = y;
    GLState.legacy.viewport[2] = width;
    GLState.legacy.viewport[3] = height;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glViewport, x, y, width, height)

// ============================================================================
// glHint - pass through (ES supports basic hints)
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glHint, GLenum target, GLenum mode)
{
    _native(target, mode);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glHint, target, mode)

// ============================================================================
// glScissor - track scissor state
// ============================================================================

// Already native in gl_native.cpp, but we need to track state
// glScissor in gl_native.cpp is NATIVE_FUNCTION_HEAD, so we add tracking here
// Note: glScissor is already defined in gl_native.cpp, so we skip redefinition
// and just use the native passthrough. State tracking for scissor is done in
// getter.cpp when glGetIntegerv(GL_SCISSOR_BOX) is called.

// ============================================================================
// glPolygonMode - not supported in ES core, track CPU-side
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glPolygonMode, GLenum face, GLenum mode)
{
    // ES 3.2 doesn't support glPolygonMode natively
    // Track it CPU-side for queries
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
        GLState.legacy.polygonMode[0] = mode;
    }
    if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
        GLState.legacy.polygonMode[1] = mode;
    }
    // No actual GL call - ES doesn't support polygon mode
}
NATIVE_FUNCTION_END_NO_RETURN(void, glPolygonMode, face, mode)

// ============================================================================
// glDrawBuffer - maps to glDrawBuffers in ES
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glDrawBuffer, GLenum mode)
{
    if (mode == GL_NONE) {
        GLenum none = GL_NONE;
        static auto glDrawBuffers = (decltype(&glDrawBuffers))g_loader_handle("glDrawBuffers");
        glDrawBuffers(1, &none);
        GLState.framebuffer.drawBuffers[0] = GL_NONE;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + MAX_DRAW_BUFFERS) {
        static auto glDrawBuffers = (decltype(&glDrawBuffers))g_loader_handle("glDrawBuffers");
        glDrawBuffers(1, &mode);
        GLState.framebuffer.drawBuffers[0] = mode;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode == GL_BACK || mode == GL_FRONT) {
        GLenum back = GL_BACK;
        static auto glDrawBuffers = (decltype(&glDrawBuffers))g_loader_handle("glDrawBuffers");
        glDrawBuffers(1, &back);
        GLState.framebuffer.drawBuffers[0] = GL_BACK;
        GLState.framebuffer.drawBufferCount = 1;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawBuffer, mode)

// ============================================================================
// glReadBuffer - track read buffer state
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glReadBuffer, GLenum mode)
{
    _native(mode);
    GLState.framebuffer.readBuffer = mode;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glReadBuffer, mode)

// ============================================================================
// glColorMask - track color write mask
// ============================================================================

// Already native in gl_native.cpp, state tracking done in getter.cpp

// ============================================================================
// glDepthMask - track depth write mask
// ============================================================================

// Already native in gl_native.cpp, state tracking done in getter.cpp

// ============================================================================
// glLineWidth - track line width
// ============================================================================

// Already native in gl_native.cpp, state tracking done in getter.cpp

// ============================================================================
// glPointSize - not in ES 3.2 core, track CPU-side
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glPointSize, GLfloat size)
{
    // ES 3.2 doesn't support glPointSize; use gl_PointSize in shader instead
    // Track CPU-side for queries
    GLState.legacy.lineWidth = size; // reuse lineWidth as point size placeholder
}
NATIVE_FUNCTION_END_NO_RETURN(void, glPointSize, size)