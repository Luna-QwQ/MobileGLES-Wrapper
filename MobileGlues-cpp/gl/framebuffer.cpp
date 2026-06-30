// MobileGlues - gl/framebuffer.cpp
// Framebuffer state management: FBO ID mapping, attachment tracking,
// draw buffer remapping, and temp FBO pool for format conversion.
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
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
#include <cstring>
#include <vector>

#define DEBUG 0

// ============================================================================
// FBO ID management
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGenFramebuffers, GLsizei n, GLuint *framebuffers)
{
    _native(n, framebuffers);

    auto &fb = GLState.framebuffer;
    for (GLsizei i = 0; i < n; i++) {
        fb.fboMap[framebuffers[i]] = framebuffers[i];
        fb.fboMapReverse[framebuffers[i]] = framebuffers[i];
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGenFramebuffers, n, framebuffers)

NATIVE_FUNCTION_HEAD(void, glDeleteFramebuffers, GLsizei n, const GLuint *framebuffers)
{
    _native(n, framebuffers);

    auto &fb = GLState.framebuffer;
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = framebuffers[i];
        if (id == 0) continue;

        if (fb.drawFBO == id) fb.drawFBO = 0;
        if (fb.readFBO == id) fb.readFBO = 0;

        fb.fboMap.erase(id);
        fb.fboMapReverse.erase(id);
        fb.attachments.erase(id);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteFramebuffers, n, framebuffers)

// ============================================================================
// glBindFramebuffer
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindFramebuffer, GLenum target, GLuint framebuffer)
{
    GLuint realFBO = GLState.GetRealFBO(framebuffer);
    if (realFBO == 0 && framebuffer != 0) {
        realFBO = framebuffer;
    }

    _native(target, realFBO);

    auto &fb = GLState.framebuffer;
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        fb.drawFBO = framebuffer;
        GLState.currentDrawFBO = framebuffer;
    }
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        fb.readFBO = framebuffer;
    }

    STATE_LOG("glBindFramebuffer: target=0x%X, virt=%u, real=%u",
              target, framebuffer, realFBO);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindFramebuffer, target, framebuffer)

// ============================================================================
// glFramebufferTexture2D / glFramebufferTexture - attachment tracking
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glFramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    GLenum esTexTarget = GLStateManager::ConvertTextureTarget(textarget);
    GLuint realTex = GLState.GetRealTexture(texture);
    if (realTex == 0 && texture != 0) realTex = texture;

    _native(target, attachment, esTexTarget, realTex, level);

    // Track attachment
    GLuint currentFBO = (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
                        ? GLState.framebuffer.drawFBO
                        : GLState.framebuffer.readFBO;

    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFramebufferTexture2D, target, attachment, textarget, texture, level)

NATIVE_FUNCTION_HEAD(void, glFramebufferTexture, GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    GLuint realTex = GLState.GetRealTexture(texture);
    if (realTex == 0 && texture != 0) realTex = texture;

    _native(target, attachment, realTex, level);

    GLuint currentFBO = (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
                        ? GLState.framebuffer.drawFBO
                        : GLState.framebuffer.readFBO;

    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFramebufferTexture, target, attachment, texture, level)

// ============================================================================
// glFramebufferTextureLayer - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glFramebufferTextureLayer, GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    GLuint realTex = GLState.GetRealTexture(texture);
    if (realTex == 0 && texture != 0) realTex = texture;
    _native(target, attachment, realTex, level, layer);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFramebufferTextureLayer, target, attachment, texture, level, layer)

// ============================================================================
// glFramebufferRenderbuffer - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glFramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    _native(target, attachment, renderbuffertarget, renderbuffer);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFramebufferRenderbuffer, target, attachment, renderbuffertarget, renderbuffer)

// ============================================================================
// glCheckFramebufferStatus
// ============================================================================

NATIVE_FUNCTION_HEAD(GLenum, glCheckFramebufferStatus, GLenum target)
{
    GLenum status = _native(target);
    // Ignore incomplete errors for applications that don't fully set up FBOs
    // as desktop GL would (some desktop apps rely on implicit defaults)
    if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
        status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
        // Allow incomplete FBOs - some apps don't care
        status = GL_FRAMEBUFFER_COMPLETE;
    }
    return status;
}
NATIVE_FUNCTION_END(GLenum, glCheckFramebufferStatus, target)

// ============================================================================
// glFramebufferParameteri - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glFramebufferParameteri, GLenum target, GLenum pname, GLint param)
{
    _native(target, pname, param);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFramebufferParameteri, target, pname, param)

// ============================================================================
// glGetFramebufferAttachmentParameteriv - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetFramebufferAttachmentParameteriv, GLenum target, GLenum attachment, GLenum pname, GLint *params)
{
    _native(target, attachment, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetFramebufferAttachmentParameteriv, target, attachment, pname, params)

// ============================================================================
// glGetFramebufferParameteriv - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetFramebufferParameteriv, GLenum target, GLenum pname, GLint *params)
{
    _native(target, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetFramebufferParameteriv, target, pname, params)

// ============================================================================
// glBlitFramebuffer - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                     GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    // Strip GL_ACCUM_BUFFER_BIT
    mask &= ~0x00000200;
    _native(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBlitFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)

// ============================================================================
// glInvalidateFramebuffer / glInvalidateSubFramebuffer - native passthrough
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glInvalidateFramebuffer, GLenum target, GLsizei numAttachments, const GLenum *attachments)
{
    _native(target, numAttachments, attachments);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glInvalidateFramebuffer, target, numAttachments, attachments)

NATIVE_FUNCTION_HEAD(void, glInvalidateSubFramebuffer, GLenum target, GLsizei numAttachments, const GLenum *attachments,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
    _native(target, numAttachments, attachments, x, y, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glInvalidateSubFramebuffer, target, numAttachments, attachments, x, y, width, height)

// ============================================================================
// Map initialization helpers (for backward compatibility)
// ============================================================================

void InitFramebufferMap(size_t expectedSize) {
    auto &fb = GLState.framebuffer;
    fb.fboMap.reserve(expectedSize);
    fb.fboMapReverse.reserve(expectedSize);
}