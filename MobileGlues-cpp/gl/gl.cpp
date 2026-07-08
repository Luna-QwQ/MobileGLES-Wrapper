// MobileGlues - gl/gl.cpp
// Core GL state wrappers: glClearDepth, glClear, glHint, glDrawBuffer, glReadBuffer.
// NOTE: glDepthRange, glPolygonMode, glPointSize are in gl_stub.cpp
//       glViewport is in FSR1/FSR1.cpp
//       glGenTextures is in gl_native.cpp
//
// Architecture (DirectGLES pattern):
//   Public function = thin wrapper: Lock → _State → Unlock
//   _State function  = validation + state management + call _Backend
//   _Backend function = actual GLES call through the loader
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
#include "framebuffer.h"
#include "../gles/loader.h"
#include "mg.h"
#include <GLES3/gl32.h>

#define DEBUG 0

// ============================================================================
// glClearDepth - desktop uses double, ES uses float
// ============================================================================

static void glClearDepth_Backend(GLclampd depth) {
    GLES.glClearDepthf((float)depth);
}

static void glClearDepth_State(GLclampd depth) {
    // Validate: depth must be in [0.0, 1.0]
    if (depth < 0.0 || depth > 1.0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearDepth: depth out of range [0, 1]"));
        return;
    }
    // Update state
    GLState.renderState.SetClearDepth((GLfloat)depth);
    GLState.legacy.clearDepth = (GLfloat)depth;
    glClearDepth_Backend(depth);
}

extern "C" GLAPI GLAPIENTRY void glClearDepth(GLclampd depth) {
    GLState.Lock();
    glClearDepth_State(depth);
    GLState.Unlock();
}

// ============================================================================
// glClear - handles legacy clear mask conversion
// ============================================================================

static void glClear_Backend(GLbitfield mask) {
    GLES.glClear(mask);
}

static void glClear_State(GLbitfield mask) {
    // GL_ACCUM_BUFFER_BIT is not supported in ES, strip it
    mask &= ~0x00000200; // GL_ACCUM_BUFFER_BIT

    // Validate: mask must not contain unsupported bits after stripping
    constexpr GLbitfield validMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (mask & ~validMask) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClear: invalid mask bits"));
        return;
    }

    glClear_Backend(mask);
}

extern "C" GLAPI GLAPIENTRY void glClear(GLbitfield mask) {
    GLState.Lock();
    glClear_State(mask);
    GLState.Unlock();
}

// ============================================================================
// glHint - pass through (ES supports basic hints)
// ============================================================================

static void glHint_Backend(GLenum target, GLenum mode) {
    GLES.glHint(target, mode);
}

static bool IsValidHintTarget(GLenum target) {
    switch (target) {
        case GL_LINE_SMOOTH_HINT:
        case GL_POLYGON_SMOOTH_HINT:
        case GL_TEXTURE_COMPRESSION_HINT:
        case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
        case GL_GENERATE_MIPMAP_HINT:
        case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
            return true;
        default:
            return false;
    }
}

static bool IsValidHintMode(GLenum mode) {
    switch (mode) {
        case GL_FASTEST:
        case GL_NICEST:
        case GL_DONT_CARE:
            return true;
        default:
            return false;
    }
}

static void glHint_State(GLenum target, GLenum mode) {
    // Validate target
    if (!IsValidHintTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glHint: invalid target"));
        return;
    }
    // Validate mode
    if (!IsValidHintMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glHint: invalid mode"));
        return;
    }

    glHint_Backend(target, mode);
}

extern "C" GLAPI GLAPIENTRY void glHint(GLenum target, GLenum mode) {
    GLState.Lock();
    glHint_State(target, mode);
    GLState.Unlock();
}

// ============================================================================
// glDrawBuffer - maps to glDrawBuffers in ES
// ============================================================================

static void glDrawBuffer_Backend(GLenum mode) {
    if (mode == GL_NONE) {
        GLenum none = GL_NONE;
        GLES.glDrawBuffers(1, &none);
    } else if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31) {
        GLES.glDrawBuffers(1, &mode);
    } else if (mode == GL_BACK || mode == GL_FRONT) {
        GLenum back = GL_BACK;
        GLES.glDrawBuffers(1, &back);
    }
}

static bool IsValidDrawBufferMode(GLenum mode) {
    if (mode == GL_NONE) return true;
    if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31) return true;
    if (mode == GL_BACK || mode == GL_FRONT || mode == GL_FRONT_AND_BACK || mode == GL_FRONT_LEFT
        || mode == GL_FRONT_RIGHT || mode == GL_BACK_LEFT || mode == GL_BACK_RIGHT
        || mode == GL_LEFT || mode == GL_RIGHT) {
        return true;
    }
    return false;
}

static void glDrawBuffer_State(GLenum mode) {
    // Validate mode
    if (!IsValidDrawBufferMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawBuffer: invalid mode"));
        return;
    }

    // Update state
    if (mode == GL_NONE) {
        GLState.framebuffer.drawBuffers[0] = GL_NONE;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31) {
        GLState.framebuffer.drawBuffers[0] = mode;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode == GL_BACK || mode == GL_FRONT) {
        GLState.framebuffer.drawBuffers[0] = GL_BACK;
        GLState.framebuffer.drawBufferCount = 1;
    }

    glDrawBuffer_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glDrawBuffer(GLenum mode) {
    GLState.Lock();
    glDrawBuffer_State(mode);
    GLState.Unlock();
}

// ============================================================================
// glReadBuffer - track read buffer state
// ============================================================================

static void glReadBuffer_Backend(GLenum mode) {
    GLES.glReadBuffer(mode);
}

static bool IsValidReadBufferMode(GLenum mode) {
    if (mode == GL_NONE) return true;
    if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31) return true;
    if (mode == GL_BACK || mode == GL_FRONT || mode == GL_FRONT_AND_BACK
        || mode == GL_LEFT || mode == GL_RIGHT) {
        return true;
    }
    return false;
}

static void glReadBuffer_State(GLenum mode) {
    // Validate mode
    if (!IsValidReadBufferMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glReadBuffer: invalid mode"));
        return;
    }

    // Update state
    GLState.framebuffer.readBuffer = mode;

    glReadBuffer_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glReadBuffer(GLenum mode) {
    GLState.Lock();
    glReadBuffer_State(mode);
    GLState.Unlock();
}