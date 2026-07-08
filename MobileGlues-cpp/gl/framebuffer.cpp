// MobileGlues - gl/framebuffer.cpp
// Framebuffer state management: FBO ID mapping, attachment tracking,
// draw buffer remapping, and temp FBO pool for format conversion.
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// Architecture pattern (inspired by MobileGL-DirectGLES):
//   Public function = thin wrapper: Lock → _State → _Backend → Unlock
//   _State function = validation, enum conversion, GLState update, error recording
//   _Backend function = actual GLES call through the gles loader
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
// Forward declarations of internal helper functions
// ============================================================================

// Temporary FBO pool for hot-path texture operations
// (acquireTempFBO / releaseTempFBO — used by texture.cpp)
static std::vector<GLuint> g_tempFboPool;
static size_t g_tempFboPoolIndex = 0;

// ============================================================================
// Helper: validate framebuffer binding target
// ============================================================================

static bool IsValidFramebufferTarget(GLenum target) {
    return (target == GL_FRAMEBUFFER ||
            target == GL_DRAW_FRAMEBUFFER ||
            target == GL_READ_FRAMEBUFFER);
}

// ============================================================================
// Helper: get current FBO for a given target
// ============================================================================

static GLuint GetCurrentFBOForTarget(GLenum target) {
    if (target == GL_READ_FRAMEBUFFER) {
        return GLState.framebuffer.readFBO;
    }
    // GL_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER → drawFBO
    return GLState.framebuffer.drawFBO;
}

// ============================================================================
// Helper: temporarily bind a framebuffer (for DSA functions)
// ============================================================================

static void temporarilyBindFramebuffer(GLuint framebufferID, GLenum target = GL_DRAW_FRAMEBUFFER) {
    GLuint prev = (target == GL_READ_FRAMEBUFFER) ? GLState.framebuffer.readFBO : GLState.framebuffer.drawFBO;
    if (prev == framebufferID) return;
    GLuint realID = GLState.GetRealFBO(framebufferID);
    GLES.glBindFramebuffer(target, realID ? realID : framebufferID);
}

static void restoreTemporaryFramebufferBinding(GLenum target = GL_DRAW_FRAMEBUFFER) {
    GLuint prev = (target == GL_READ_FRAMEBUFFER) ? GLState.framebuffer.readFBO : GLState.framebuffer.drawFBO;
    GLuint realID = GLState.GetRealFBO(prev);
    GLES.glBindFramebuffer(target, realID ? realID : prev);
}

// ============================================================================
// Helper: temporarily bind a renderbuffer (for DSA functions)
// ============================================================================

static void temporarilyBindRenderbuffer(GLuint renderbufferID) {
    GLuint realID = GLState.renderbuffer.rbMap.count(renderbufferID) ?
                    GLState.renderbuffer.rbMap[renderbufferID] : renderbufferID;
    GLES.glBindRenderbuffer(GL_RENDERBUFFER, realID);
}

// ============================================================================
// ============================================================================
// FBO ID MANAGEMENT
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glGenFramebuffers
// ----------------------------------------------------------------------------

void GenFramebuffers_Backend(GLsizei n, GLuint *framebuffers) {
    GLES.glGenFramebuffers(n, framebuffers);
}

void GenFramebuffers_State(GLsizei n, GLuint *framebuffers) {
    if (n < 0 || !framebuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenFramebuffers: n < 0 or framebuffers is null"));
        return;
    }

    GenFramebuffers_Backend(n, framebuffers);

    auto &fb = GLState.framebuffer;
    for (GLsizei i = 0; i < n; i++) {
        fb.fboMap[framebuffers[i]] = framebuffers[i];
        fb.fboMapReverse[framebuffers[i]] = framebuffers[i];
    }
}

extern "C" GLAPI GLAPIENTRY void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    GLState.Lock();
    GenFramebuffers_State(n, framebuffers);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glDeleteFramebuffers
// ----------------------------------------------------------------------------

void DeleteFramebuffers_Backend(GLsizei n, const GLuint *framebuffers) {
    GLES.glDeleteFramebuffers(n, framebuffers);
}

void DeleteFramebuffers_State(GLsizei n, const GLuint *framebuffers) {
    if (n < 0 || !framebuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteFramebuffers: n < 0 or framebuffers is null"));
        return;
    }

    auto &fb = GLState.framebuffer;
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = framebuffers[i];
        if (id == 0) continue;

        if (fb.drawFBO == id) fb.drawFBO = 0;
        if (fb.readFBO == id) fb.readFBO = 0;
        if (GLState.currentDrawFBO == id) GLState.currentDrawFBO = 0;

        fb.fboMap.erase(id);
        fb.fboMapReverse.erase(id);
        fb.attachments.erase(id);
    }

    DeleteFramebuffers_Backend(n, framebuffers);
}

extern "C" GLAPI GLAPIENTRY void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    GLState.Lock();
    DeleteFramebuffers_State(n, framebuffers);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glIsFramebuffer
// ----------------------------------------------------------------------------

GLboolean IsFramebuffer_Backend(GLuint framebuffer) {
    return GLES.glIsFramebuffer(framebuffer);
}

GLboolean IsFramebuffer_State(GLuint framebuffer) {
    if (framebuffer == 0) return GL_FALSE;
    GLuint realID = GLState.GetRealFBO(framebuffer);
    if (realID == 0) return GL_FALSE;
    return IsFramebuffer_Backend(realID);
}

extern "C" GLAPI GLAPIENTRY GLboolean glIsFramebuffer(GLuint framebuffer) {
    GLState.Lock();
    GLboolean result = IsFramebuffer_State(framebuffer);
    GLState.Unlock();
    return result;
}

// ============================================================================
// ============================================================================
// FBO BINDING
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glBindFramebuffer
// ----------------------------------------------------------------------------

void BindFramebuffer_Backend(GLenum target, GLuint realFBO) {
    GLES.glBindFramebuffer(target, realFBO);
}

void BindFramebuffer_State(GLenum target, GLuint framebuffer) {
    // Validate target
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBindFramebuffer: invalid target"));
        return;
    }

    // Map virtual ID to real ID
    GLuint realFBO = (framebuffer != 0) ? GLState.GetRealFBO(framebuffer) : 0;
    if (framebuffer != 0 && realFBO == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindFramebuffer: framebuffer is not a valid FBO"));
        return;
    }

    // Update state bindings
    auto &fb = GLState.framebuffer;
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        fb.drawFBO = framebuffer;
        GLState.currentDrawFBO = framebuffer;
    }
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        fb.readFBO = framebuffer;
    }

    // Perform the actual GLES bind
    if (target == GL_FRAMEBUFFER) {
        BindFramebuffer_Backend(GL_DRAW_FRAMEBUFFER, realFBO);
        BindFramebuffer_Backend(GL_READ_FRAMEBUFFER, realFBO);
    } else {
        BindFramebuffer_Backend(target, realFBO);
    }

    STATE_LOG("glBindFramebuffer: target=0x%X, fbo=%u", target, framebuffer);
}

extern "C" GLAPI GLAPIENTRY void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    GLState.Lock();
    BindFramebuffer_State(target, framebuffer);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// RENDERBUFFER ID MANAGEMENT
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glGenRenderbuffers
// ----------------------------------------------------------------------------

void GenRenderbuffers_Backend(GLsizei n, GLuint *renderbuffers) {
    GLES.glGenRenderbuffers(n, renderbuffers);
}

void GenRenderbuffers_State(GLsizei n, GLuint *renderbuffers) {
    if (n < 0 || !renderbuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenRenderbuffers: n < 0 or renderbuffers is null"));
        return;
    }

    GenRenderbuffers_Backend(n, renderbuffers);

    auto &rb = GLState.renderbuffer;
    for (GLsizei i = 0; i < n; i++) {
        rb.rbMap[renderbuffers[i]] = renderbuffers[i];
        rb.rbMapReverse[renderbuffers[i]] = renderbuffers[i];
    }
}

extern "C" GLAPI GLAPIENTRY void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GLState.Lock();
    GenRenderbuffers_State(n, renderbuffers);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glDeleteRenderbuffers
// ----------------------------------------------------------------------------

void DeleteRenderbuffers_Backend(GLsizei n, const GLuint *renderbuffers) {
    GLES.glDeleteRenderbuffers(n, renderbuffers);
}

void DeleteRenderbuffers_State(GLsizei n, const GLuint *renderbuffers) {
    if (n < 0 || !renderbuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteRenderbuffers: n < 0 or renderbuffers is null"));
        return;
    }

    auto &rb = GLState.renderbuffer;
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = renderbuffers[i];
        if (id == 0) continue;
        rb.rbMap.erase(id);
        rb.rbMapReverse.erase(id);
    }

    DeleteRenderbuffers_Backend(n, renderbuffers);
}

extern "C" GLAPI GLAPIENTRY void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    GLState.Lock();
    DeleteRenderbuffers_State(n, renderbuffers);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glBindRenderbuffer
// ----------------------------------------------------------------------------

void BindRenderbuffer_Backend(GLenum target, GLuint realRBO) {
    GLES.glBindRenderbuffer(target, realRBO);
}

void BindRenderbuffer_State(GLenum target, GLuint renderbuffer) {
    if (target != GL_RENDERBUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBindRenderbuffer: target must be GL_RENDERBUFFER"));
        return;
    }

    GLuint realRBO = (renderbuffer != 0) ? GLState.renderbuffer.rbMap[renderbuffer] : 0;
    if (renderbuffer != 0 && realRBO == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindRenderbuffer: invalid renderbuffer ID"));
        return;
    }

    BindRenderbuffer_Backend(target, realRBO);
}

extern "C" GLAPI GLAPIENTRY void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    GLState.Lock();
    BindRenderbuffer_State(target, renderbuffer);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glIsRenderbuffer
// ----------------------------------------------------------------------------

GLboolean IsRenderbuffer_Backend(GLuint renderbuffer) {
    return GLES.glIsRenderbuffer(renderbuffer);
}

GLboolean IsRenderbuffer_State(GLuint renderbuffer) {
    if (renderbuffer == 0) return GL_FALSE;
    auto it = GLState.renderbuffer.rbMap.find(renderbuffer);
    if (it == GLState.renderbuffer.rbMap.end()) return GL_FALSE;
    return IsRenderbuffer_Backend(it->second);
}

extern "C" GLAPI GLAPIENTRY GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    GLState.Lock();
    GLboolean result = IsRenderbuffer_State(renderbuffer);
    GLState.Unlock();
    return result;
}

// ----------------------------------------------------------------------------
// glGetRenderbufferParameteriv
// ----------------------------------------------------------------------------

void GetRenderbufferParameteriv_Backend(GLenum target, GLenum pname, GLint *params) {
    GLES.glGetRenderbufferParameteriv(target, pname, params);
}

void GetRenderbufferParameteriv_State(GLenum target, GLenum pname, GLint *params) {
    if (target != GL_RENDERBUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetRenderbufferParameteriv: target must be GL_RENDERBUFFER"));
        return;
    }
    GetRenderbufferParameteriv_Backend(target, pname, params);
}

extern "C" GLAPI GLAPIENTRY void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GLState.Lock();
    GetRenderbufferParameteriv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// RENDERBUFFER STORAGE
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glRenderbufferStorage
// ----------------------------------------------------------------------------

void RenderbufferStorage_Backend(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLES.glRenderbufferStorage(target, internalFormat, width, height);
}

void RenderbufferStorage_State(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    if (target != GL_RENDERBUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glRenderbufferStorage: target must be GL_RENDERBUFFER"));
        return;
    }
    if (width <= 0 || height <= 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glRenderbufferStorage: width or height <= 0"));
        return;
    }

    // Convert desktop GL internal format to GLES if needed
    GLenum convertedFormat = GLStateManager::ConvertInternalFormat(internalFormat);

    LOG_D("glRenderbufferStorage, target: %d, internalFormat: 0x%X (converted: 0x%X), width: %d, height: %d",
          target, internalFormat, convertedFormat, width, height);

    RenderbufferStorage_Backend(target, convertedFormat, width, height);
}

extern "C" GLAPI GLAPIENTRY void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLState.Lock();
    RenderbufferStorage_State(target, internalFormat, width, height);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glRenderbufferStorageMultisample
// ----------------------------------------------------------------------------

void RenderbufferStorageMultisample_Backend(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLES.glRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
}

void RenderbufferStorageMultisample_State(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) {
    if (target != GL_RENDERBUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glRenderbufferStorageMultisample: target must be GL_RENDERBUFFER"));
        return;
    }
    if (samples < 0 || width <= 0 || height <= 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glRenderbufferStorageMultisample: invalid parameter"));
        return;
    }

    GLenum convertedFormat = GLStateManager::ConvertInternalFormat(internalFormat);

    LOG_D("glRenderbufferStorageMultisample, target: %d, samples: %d, internalFormat: 0x%X (converted: 0x%X), width: %d, height: %d",
          target, samples, internalFormat, convertedFormat, width, height);

    RenderbufferStorageMultisample_Backend(target, samples, convertedFormat, width, height);
}

extern "C" GLAPI GLAPIENTRY void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                                                   GLsizei width, GLsizei height) {
    GLState.Lock();
    RenderbufferStorageMultisample_State(target, samples, internalFormat, width, height);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// FRAMEBUFFER ATTACHMENT FUNCTIONS
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glFramebufferTexture2D
// ----------------------------------------------------------------------------

void FramebufferTexture2D_Backend(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GLES.glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void FramebufferTexture2D_State(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferTexture2D: invalid target"));
        return;
    }
    if (level < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glFramebufferTexture2D: level < 0"));
        return;
    }

    GLenum esTexTarget = GLStateManager::ConvertTextureTarget(textarget);

    FramebufferTexture2D_Backend(target, attachment, esTexTarget, texture, level);

    // Track attachment
    GLuint currentFBO = GetCurrentFBOForTarget(target);
    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}

extern "C" GLAPI GLAPIENTRY void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                                                         GLuint texture, GLint level) {
    GLState.Lock();
    FramebufferTexture2D_State(target, attachment, textarget, texture, level);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glFramebufferTexture3D
// ----------------------------------------------------------------------------

void FramebufferTexture3D_Backend(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    GLES.glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}

void FramebufferTexture3D_State(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferTexture3D: invalid target"));
        return;
    }

    GLenum esTexTarget = GLStateManager::ConvertTextureTarget(textarget);

    FramebufferTexture3D_Backend(target, attachment, esTexTarget, texture, level, zoffset);

    // Track attachment
    GLuint currentFBO = GetCurrentFBOForTarget(target);
    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}

extern "C" GLAPI GLAPIENTRY void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget,
                                                         GLuint texture, GLint level, GLint zoffset) {
    GLState.Lock();
    FramebufferTexture3D_State(target, attachment, textarget, texture, level, zoffset);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glFramebufferTexture
// ----------------------------------------------------------------------------

void FramebufferTexture_Backend(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    GLES.glFramebufferTexture(target, attachment, texture, level);
}

void FramebufferTexture_State(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferTexture: invalid target"));
        return;
    }
    if (level < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glFramebufferTexture: level < 0"));
        return;
    }

    FramebufferTexture_Backend(target, attachment, texture, level);

    GLuint currentFBO = GetCurrentFBOForTarget(target);
    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}

extern "C" GLAPI GLAPIENTRY void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    GLState.Lock();
    FramebufferTexture_State(target, attachment, texture, level);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glFramebufferTextureLayer
// ----------------------------------------------------------------------------

void FramebufferTextureLayer_Backend(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GLES.glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void FramebufferTextureLayer_State(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferTextureLayer: invalid target"));
        return;
    }

    FramebufferTextureLayer_Backend(target, attachment, texture, level, layer);

    GLuint currentFBO = GetCurrentFBOForTarget(target);
    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}

extern "C" GLAPI GLAPIENTRY void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture,
                                                            GLint level, GLint layer) {
    GLState.Lock();
    FramebufferTextureLayer_State(target, attachment, texture, level, layer);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glFramebufferRenderbuffer
// ----------------------------------------------------------------------------

void FramebufferRenderbuffer_Backend(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GLES.glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void FramebufferRenderbuffer_State(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferRenderbuffer: invalid target"));
        return;
    }

    FramebufferRenderbuffer_Backend(target, attachment, renderbuffertarget, renderbuffer);

    GLuint currentFBO = GetCurrentFBOForTarget(target);
    if (currentFBO) {
        auto &att = GLState.framebuffer.attachments[currentFBO][attachment];
        att.fbo = currentFBO;
        att.attachment = attachment;
    }
}

extern "C" GLAPI GLAPIENTRY void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget,
                                                            GLuint renderbuffer) {
    GLState.Lock();
    FramebufferRenderbuffer_State(target, attachment, renderbuffertarget, renderbuffer);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// FRAMEBUFFER STATUS / QUERY
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glCheckFramebufferStatus
// ----------------------------------------------------------------------------

GLenum CheckFramebufferStatus_Backend(GLenum target) {
    return GLES.glCheckFramebufferStatus(target);
}

GLenum CheckFramebufferStatus_State(GLenum target) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glCheckFramebufferStatus: invalid target"));
        return 0;
    }

    GLenum status = CheckFramebufferStatus_Backend(target);
    // Ignore incomplete errors for applications that don't fully set up FBOs
    // or use incompatible configurations (e.g. MRT with unsupported formats).
    // Desktop GL shaders (like ComplementaryUnbound) may use FBO configurations
    // that are not directly supported by GLES drivers.
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            // Treat all non-complete statuses as complete to prevent crashes
            LOG_D("glCheckFramebufferStatus: ignoring status 0x%X, returning COMPLETE", status);
            status = GL_FRAMEBUFFER_COMPLETE;
            break;
    }
    return status;
}

extern "C" GLAPI GLAPIENTRY GLenum glCheckFramebufferStatus(GLenum target) {
    GLState.Lock();
    GLenum result = CheckFramebufferStatus_State(target);
    GLState.Unlock();
    return result;
}

// ----------------------------------------------------------------------------
// glGetFramebufferAttachmentParameteriv
// ----------------------------------------------------------------------------

void GetFramebufferAttachmentParameteriv_Backend(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    GLES.glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

void GetFramebufferAttachmentParameteriv_State(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetFramebufferAttachmentParameteriv: invalid target"));
        return;
    }
    GetFramebufferAttachmentParameteriv_Backend(target, attachment, pname, params);
}

extern "C" GLAPI GLAPIENTRY void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname,
                                                                        GLint *params) {
    GLState.Lock();
    GetFramebufferAttachmentParameteriv_State(target, attachment, pname, params);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glGetFramebufferParameteriv
// ----------------------------------------------------------------------------

void GetFramebufferParameteriv_Backend(GLenum target, GLenum pname, GLint *params) {
    GLES.glGetFramebufferParameteriv(target, pname, params);
}

void GetFramebufferParameteriv_State(GLenum target, GLenum pname, GLint *params) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetFramebufferParameteriv: invalid target"));
        return;
    }
    GetFramebufferParameteriv_Backend(target, pname, params);
}

extern "C" GLAPI GLAPIENTRY void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GLState.Lock();
    GetFramebufferParameteriv_State(target, pname, params);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glFramebufferParameteri
// ----------------------------------------------------------------------------

void FramebufferParameteri_Backend(GLenum target, GLenum pname, GLint param) {
    GLES.glFramebufferParameteri(target, pname, param);
}

void FramebufferParameteri_State(GLenum target, GLenum pname, GLint param) {
    if (!IsValidFramebufferTarget(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFramebufferParameteri: invalid target"));
        return;
    }
    FramebufferParameteri_Backend(target, pname, param);
}

extern "C" GLAPI GLAPIENTRY void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
    GLState.Lock();
    FramebufferParameteri_State(target, pname, param);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DRAW / READ BUFFER MANAGEMENT
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glDrawBuffer - maps to glDrawBuffers in ES
// ----------------------------------------------------------------------------

void DrawBuffer_Backend(GLenum mode) {
    GLES.glDrawBuffers(1, &mode);
}

void DrawBuffer_State(GLenum mode) {
    // Validate mode
    if (mode != GL_NONE && mode != GL_BACK && mode != GL_FRONT &&
        !(mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawBuffer: invalid mode"));
        return;
    }

    GLenum esMode = mode;
    if (mode == GL_NONE) {
        GLState.framebuffer.drawBuffers[0] = GL_NONE;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31) {
        GLState.framebuffer.drawBuffers[0] = mode;
        GLState.framebuffer.drawBufferCount = 1;
    } else if (mode == GL_BACK || mode == GL_FRONT) {
        esMode = GL_BACK;
        GLState.framebuffer.drawBuffers[0] = GL_BACK;
        GLState.framebuffer.drawBufferCount = 1;
    }

    DrawBuffer_Backend(esMode);
}

extern "C" GLAPI GLAPIENTRY void glDrawBuffer(GLenum mode) {
    GLState.Lock();
    DrawBuffer_State(mode);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glDrawBuffers
// ----------------------------------------------------------------------------

void DrawBuffers_Backend(GLsizei n, const GLenum *bufs) {
    GLES.glDrawBuffers(n, bufs);
}

void DrawBuffers_State(GLsizei n, const GLenum *bufs) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawBuffers: n < 0"));
        return;
    }
    if (n > MAX_DRAW_BUFFERS) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawBuffers: n exceeds MAX_DRAW_BUFFERS"));
        return;
    }

    if (n > 0 && bufs) {
        GLState.framebuffer.drawBufferCount = n;
        for (GLsizei i = 0; i < n && i < MAX_DRAW_BUFFERS; i++) {
            GLState.framebuffer.drawBuffers[i] = bufs[i];
        }
    } else if (n == 0) {
        GLState.framebuffer.drawBufferCount = 0;
    }

    DrawBuffers_Backend(n, bufs);
}

extern "C" GLAPI GLAPIENTRY void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    GLState.Lock();
    DrawBuffers_State(n, bufs);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glReadBuffer
// ----------------------------------------------------------------------------

void ReadBuffer_Backend(GLenum mode) {
    GLES.glReadBuffer(mode);
}

void ReadBuffer_State(GLenum mode) {
    // Validate mode (basic: must be NONE, BACK, FRONT, or COLOR_ATTACHMENTi)
    if (mode != GL_NONE && mode != GL_BACK && mode != GL_FRONT &&
        !(mode >= GL_COLOR_ATTACHMENT0 && mode <= GL_COLOR_ATTACHMENT0 + 31)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glReadBuffer: invalid mode"));
        return;
    }

    GLState.framebuffer.readBuffer = mode;
    ReadBuffer_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glReadBuffer(GLenum mode) {
    GLState.Lock();
    ReadBuffer_State(mode);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// CLEAR BUFFER FUNCTIONS
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glClearBufferfv
// ----------------------------------------------------------------------------

void ClearBufferfv_Backend(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GLES.glClearBufferfv(buffer, drawbuffer, value);
}

void ClearBufferfv_State(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    if (!value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearBufferfv: value is null"));
        return;
    }
    ClearBufferfv_Backend(buffer, drawbuffer, value);
}

extern "C" GLAPI GLAPIENTRY void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GLState.Lock();
    ClearBufferfv_State(buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearBufferiv
// ----------------------------------------------------------------------------

void ClearBufferiv_Backend(GLenum buffer, GLint drawbuffer, const GLint *value) {
    GLES.glClearBufferiv(buffer, drawbuffer, value);
}

void ClearBufferiv_State(GLenum buffer, GLint drawbuffer, const GLint *value) {
    if (!value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearBufferiv: value is null"));
        return;
    }
    ClearBufferiv_Backend(buffer, drawbuffer, value);
}

extern "C" GLAPI GLAPIENTRY void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) {
    GLState.Lock();
    ClearBufferiv_State(buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearBufferuiv
// ----------------------------------------------------------------------------

void ClearBufferuiv_Backend(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GLES.glClearBufferuiv(buffer, drawbuffer, value);
}

void ClearBufferuiv_State(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    if (!value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearBufferuiv: value is null"));
        return;
    }
    ClearBufferuiv_Backend(buffer, drawbuffer, value);
}

extern "C" GLAPI GLAPIENTRY void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GLState.Lock();
    ClearBufferuiv_State(buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearBufferfi
// ----------------------------------------------------------------------------

void ClearBufferfi_Backend(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GLES.glClearBufferfi(buffer, drawbuffer, depth, stencil);
}

void ClearBufferfi_State(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    ClearBufferfi_Backend(buffer, drawbuffer, depth, stencil);
}

extern "C" GLAPI GLAPIENTRY void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GLState.Lock();
    ClearBufferfi_State(buffer, drawbuffer, depth, stencil);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// BLIT FRAMEBUFFER
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glBlitFramebuffer
// ----------------------------------------------------------------------------

void BlitFramebuffer_Backend(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                              GLbitfield mask, GLenum filter) {
    GLES.glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void BlitFramebuffer_State(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter) {
    // Strip GL_ACCUM_BUFFER_BIT (not supported in ES)
    mask &= ~0x00000200;

    // Validate filter if mask includes color/depth/stencil buffers
    if (mask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        if (filter != GL_NEAREST && filter != GL_LINEAR) {
            GLState.errorState.RecordError(ErrorCode::InvalidEnum,
                std::make_unique<GenericErrorInfo>("glBlitFramebuffer: invalid filter"));
            return;
        }
    }

    BlitFramebuffer_Backend(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

extern "C" GLAPI GLAPIENTRY void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                                    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                                    GLbitfield mask, GLenum filter) {
    GLState.Lock();
    BlitFramebuffer_State(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// READ PIXELS
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glReadPixels
// ----------------------------------------------------------------------------

void ReadPixels_Backend(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    GLES.glReadPixels(x, y, width, height, format, type, pixels);
}

void ReadPixels_State(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    if (width < 0 || height < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glReadPixels: width or height < 0"));
        return;
    }

    ReadPixels_Backend(x, y, width, height, format, type, pixels);
}

extern "C" GLAPI GLAPIENTRY void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                               GLenum format, GLenum type, void *pixels) {
    GLState.Lock();
    ReadPixels_State(x, y, width, height, format, type, pixels);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// INVALIDATE FRAMEBUFFER
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glInvalidateFramebuffer
// ----------------------------------------------------------------------------

void InvalidateFramebuffer_Backend(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    GLES.glInvalidateFramebuffer(target, numAttachments, attachments);
}

void InvalidateFramebuffer_State(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    if (!IsValidFramebufferTarget(target) && target != GL_FRAMEBUFFER) {
        // Re-check: GL_FRAMEBUFFER is valid for invalidate
        if (target != GL_FRAMEBUFFER) {
            GLState.errorState.RecordError(ErrorCode::InvalidEnum,
                std::make_unique<GenericErrorInfo>("glInvalidateFramebuffer: invalid target"));
            return;
        }
    }
    InvalidateFramebuffer_Backend(target, numAttachments, attachments);
}

extern "C" GLAPI GLAPIENTRY void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    GLState.Lock();
    InvalidateFramebuffer_State(target, numAttachments, attachments);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glInvalidateSubFramebuffer
// ----------------------------------------------------------------------------

void InvalidateSubFramebuffer_Backend(GLenum target, GLsizei numAttachments, const GLenum *attachments,
                                       GLint x, GLint y, GLsizei width, GLsizei height) {
    GLES.glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}

void InvalidateSubFramebuffer_State(GLenum target, GLsizei numAttachments, const GLenum *attachments,
                                     GLint x, GLint y, GLsizei width, GLsizei height) {
    if (!IsValidFramebufferTarget(target) && target != GL_FRAMEBUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glInvalidateSubFramebuffer: invalid target"));
        return;
    }
    InvalidateSubFramebuffer_Backend(target, numAttachments, attachments, x, y, width, height);
}

extern "C" GLAPI GLAPIENTRY void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments,
                                                             GLint x, GLint y, GLsizei width, GLsizei height) {
    GLState.Lock();
    InvalidateSubFramebuffer_State(target, numAttachments, attachments, x, y, width, height);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: FRAMEBUFFER CREATION
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glCreateFramebuffers
// ----------------------------------------------------------------------------

void CreateFramebuffers_Backend(GLsizei n, GLuint *framebuffers) {
    // glGenFramebuffers does the backend work
    GLES.glGenFramebuffers(n, framebuffers);
}

void CreateFramebuffers_State(GLsizei n, GLuint *framebuffers) {
    if (n <= 0 || !framebuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCreateFramebuffers: n <= 0 or framebuffers is null"));
        return;
    }

    for (GLsizei i = 0; i < n; ++i) {
        GLuint fboID = 0;
        GenFramebuffers_State(1, &fboID);
        if (fboID == 0) continue;

        // Temporarily bind to create the FBO object
        temporarilyBindFramebuffer(fboID);
        restoreTemporaryFramebufferBinding();
        framebuffers[i] = fboID;
    }

    LOG_D("[DSA] Created %d framebuffers successfully", n);
}

extern "C" GLAPI GLAPIENTRY void glCreateFramebuffers(GLsizei n, GLuint *framebuffers) {
    GLState.Lock();
    CreateFramebuffers_State(n, framebuffers);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glCreateRenderbuffers
// ----------------------------------------------------------------------------

void CreateRenderbuffers_Backend(GLsizei n, GLuint *renderbuffers) {
    GLES.glGenRenderbuffers(n, renderbuffers);
}

void CreateRenderbuffers_State(GLsizei n, GLuint *renderbuffers) {
    if (n <= 0 || !renderbuffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCreateRenderbuffers: n <= 0 or renderbuffers is null"));
        return;
    }

    for (GLsizei i = 0; i < n; ++i) {
        GLuint rboID = 0;
        GenRenderbuffers_State(1, &rboID);
        if (rboID == 0) continue;

        temporarilyBindRenderbuffer(rboID);
        renderbuffers[i] = rboID;
    }

    LOG_D("[DSA] Created %d renderbuffers successfully", n);
}

extern "C" GLAPI GLAPIENTRY void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GLState.Lock();
    CreateRenderbuffers_State(n, renderbuffers);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: NAMED FRAMEBUFFER TEXTURE ATTACHMENT
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glNamedFramebufferTexture
// ----------------------------------------------------------------------------

void NamedFramebufferTexture_State(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferTexture: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    FramebufferTexture_State(GL_DRAW_FRAMEBUFFER, attachment, texture, level);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X at level %d",
          texture, framebuffer, attachment, level);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    GLState.Lock();
    NamedFramebufferTexture_State(framebuffer, attachment, texture, level);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferTexture2D
// ----------------------------------------------------------------------------

void NamedFramebufferTexture2D_State(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferTexture2D: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    FramebufferTexture2D_State(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X (2D), level %d",
          texture, framebuffer, attachment, level);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferTexture2D(GLuint framebuffer, GLenum attachment, GLenum textarget,
                                                              GLuint texture, GLint level) {
    GLState.Lock();
    NamedFramebufferTexture2D_State(framebuffer, attachment, textarget, texture, level);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferTexture3D
// ----------------------------------------------------------------------------

void NamedFramebufferTexture3D_State(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture,
                                      GLint level, GLint zoffset) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferTexture3D: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    FramebufferTexture3D_State(GL_DRAW_FRAMEBUFFER, attachment, textarget, texture, level, zoffset);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X (3D), level %d, zoffset %d",
          texture, framebuffer, attachment, level, zoffset);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferTexture3D(GLuint framebuffer, GLenum attachment, GLenum textarget,
                                                              GLuint texture, GLint level, GLint zoffset) {
    GLState.Lock();
    NamedFramebufferTexture3D_State(framebuffer, attachment, textarget, texture, level, zoffset);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferTextureLayer
// ----------------------------------------------------------------------------

void NamedFramebufferTextureLayer_State(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferTextureLayer: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    FramebufferTextureLayer_State(GL_DRAW_FRAMEBUFFER, attachment, texture, level, layer);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X at level %d, layer %d",
          texture, framebuffer, attachment, level, layer);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture,
                                                                 GLint level, GLint layer) {
    GLState.Lock();
    NamedFramebufferTextureLayer_State(framebuffer, attachment, texture, level, layer);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferRenderbuffer
// ----------------------------------------------------------------------------

void NamedFramebufferRenderbuffer_State(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferRenderbuffer: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    FramebufferRenderbuffer_State(GL_DRAW_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Attached renderbuffer %u to framebuffer %u with attachment 0x%X",
          renderbuffer, framebuffer, attachment);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment,
                                                                 GLenum renderbuffertarget, GLuint renderbuffer) {
    GLState.Lock();
    NamedFramebufferRenderbuffer_State(framebuffer, attachment, renderbuffertarget, renderbuffer);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: NAMED FRAMEBUFFER DRAW/READ BUFFER
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glNamedFramebufferDrawBuffer
// ----------------------------------------------------------------------------

void NamedFramebufferDrawBuffer_State(GLuint framebuffer, GLenum mode) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferDrawBuffer: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    DrawBuffer_State(mode);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Set draw buffer mode 0x%X for framebuffer %u", mode, framebuffer);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum mode) {
    GLState.Lock();
    NamedFramebufferDrawBuffer_State(framebuffer, mode);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferDrawBuffers
// ----------------------------------------------------------------------------

void NamedFramebufferDrawBuffers_State(GLuint framebuffer, GLsizei n, const GLenum *bufs) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferDrawBuffers: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    DrawBuffers_State(n, bufs);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Set %d draw buffers for framebuffer %u", n, framebuffer);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs) {
    GLState.Lock();
    NamedFramebufferDrawBuffers_State(framebuffer, n, bufs);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedFramebufferReadBuffer
// ----------------------------------------------------------------------------

void NamedFramebufferReadBuffer_State(GLuint framebuffer, GLenum mode) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedFramebufferReadBuffer: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer, GL_READ_FRAMEBUFFER);
    ReadBuffer_State(mode);
    restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);

    LOG_D("[DSA] Set read buffer mode 0x%X for framebuffer %u", mode, framebuffer);
}

extern "C" GLAPI GLAPIENTRY void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode) {
    GLState.Lock();
    NamedFramebufferReadBuffer_State(framebuffer, mode);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: NAMED FRAMEBUFFER CLEAR
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glClearNamedFramebufferfv
// ----------------------------------------------------------------------------

void ClearNamedFramebufferfv_State(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    if (framebuffer == 0 || !value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearNamedFramebufferfv: invalid parameter"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    ClearBufferfv_State(buffer, drawbuffer, value);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Cleared framebuffer %u with float buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

extern "C" GLAPI GLAPIENTRY void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                                                            const GLfloat *value) {
    GLState.Lock();
    ClearNamedFramebufferfv_State(framebuffer, buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearNamedFramebufferiv
// ----------------------------------------------------------------------------

void ClearNamedFramebufferiv_State(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value) {
    if (framebuffer == 0 || !value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearNamedFramebufferiv: invalid parameter"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    ClearBufferiv_State(buffer, drawbuffer, value);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Cleared framebuffer %u with int buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

extern "C" GLAPI GLAPIENTRY void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                                                            const GLint *value) {
    GLState.Lock();
    ClearNamedFramebufferiv_State(framebuffer, buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearNamedFramebufferuiv
// ----------------------------------------------------------------------------

void ClearNamedFramebufferuiv_State(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value) {
    if (framebuffer == 0 || !value) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearNamedFramebufferuiv: invalid parameter"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    ClearBufferuiv_State(buffer, drawbuffer, value);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Cleared framebuffer %u with uint buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

extern "C" GLAPI GLAPIENTRY void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                                                             const GLuint *value) {
    GLState.Lock();
    ClearNamedFramebufferuiv_State(framebuffer, buffer, drawbuffer, value);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glClearNamedFramebufferfi
// ----------------------------------------------------------------------------

void ClearNamedFramebufferfi_State(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glClearNamedFramebufferfi: framebuffer is 0"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    ClearBufferfi_State(buffer, drawbuffer, depth, stencil);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Cleared framebuffer %u with depth+stencil buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

extern "C" GLAPI GLAPIENTRY void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                                                            GLfloat depth, GLint stencil) {
    GLState.Lock();
    ClearNamedFramebufferfi_State(framebuffer, buffer, drawbuffer, depth, stencil);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: NAMED FRAMEBUFFER STATUS / QUERY
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glCheckNamedFramebufferStatus
// ----------------------------------------------------------------------------

GLenum CheckNamedFramebufferStatus_State(GLuint framebuffer, GLenum target) {
    if (framebuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCheckNamedFramebufferStatus: framebuffer is 0"));
        return 0;
    }

    temporarilyBindFramebuffer(framebuffer, target);
    GLenum status = CheckFramebufferStatus_State(target);
    restoreTemporaryFramebufferBinding(target);

    LOG_D("[DSA] Checked framebuffer %u status: 0x%X", framebuffer, status);
    return status;
}

extern "C" GLAPI GLAPIENTRY GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    GLState.Lock();
    GLenum result = CheckNamedFramebufferStatus_State(framebuffer, target);
    GLState.Unlock();
    return result;
}

// ----------------------------------------------------------------------------
// glGetNamedFramebufferAttachmentParameteriv
// ----------------------------------------------------------------------------

void GetNamedFramebufferAttachmentParameteriv_State(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) {
    if (framebuffer == 0 || !params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetNamedFramebufferAttachmentParameteriv: invalid parameter"));
        return;
    }

    temporarilyBindFramebuffer(framebuffer);
    GetFramebufferAttachmentParameteriv_State(GL_DRAW_FRAMEBUFFER, attachment, pname, params);
    restoreTemporaryFramebufferBinding();

    LOG_D("[DSA] Retrieved framebuffer attachment parameter 0x%X for framebuffer %u, attachment 0x%X",
          pname, framebuffer, attachment);
}

extern "C" GLAPI GLAPIENTRY void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment,
                                                                             GLenum pname, GLint *params) {
    GLState.Lock();
    GetNamedFramebufferAttachmentParameteriv_State(framebuffer, attachment, pname, params);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glBlitNamedFramebuffer
// ----------------------------------------------------------------------------

void BlitNamedFramebuffer_State(GLuint readFramebuffer, GLuint drawFramebuffer,
                                 GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                 GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                 GLbitfield mask, GLenum filter) {
    temporarilyBindFramebuffer(readFramebuffer, GL_READ_FRAMEBUFFER);
    temporarilyBindFramebuffer(drawFramebuffer, GL_DRAW_FRAMEBUFFER);
    BlitFramebuffer_State(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);
    restoreTemporaryFramebufferBinding(GL_DRAW_FRAMEBUFFER);

    LOG_D("[DSA] Blitted from framebuffer %u to framebuffer %u", readFramebuffer, drawFramebuffer);
}

extern "C" GLAPI GLAPIENTRY void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer,
                                                         GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                                         GLbitfield mask, GLenum filter) {
    GLState.Lock();
    BlitNamedFramebuffer_State(readFramebuffer, drawFramebuffer,
                                srcX0, srcY0, srcX1, srcY1,
                                dstX0, dstY0, dstX1, dstY1, mask, filter);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// DSA: NAMED RENDERBUFFER
// ============================================================================
// ============================================================================

// ----------------------------------------------------------------------------
// glNamedRenderbufferStorage
// ----------------------------------------------------------------------------

void NamedRenderbufferStorage_State(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {
    if (renderbuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedRenderbufferStorage: renderbuffer is 0"));
        return;
    }

    temporarilyBindRenderbuffer(renderbuffer);
    RenderbufferStorage_State(GL_RENDERBUFFER, internalformat, width, height);

    LOG_D("[DSA] Set storage for renderbuffer %u with internal format 0x%X and size (%d, %d)",
          renderbuffer, internalformat, width, height);
}

extern "C" GLAPI GLAPIENTRY void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat,
                                                             GLsizei width, GLsizei height) {
    GLState.Lock();
    NamedRenderbufferStorage_State(renderbuffer, internalformat, width, height);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glNamedRenderbufferStorageMultisample
// ----------------------------------------------------------------------------

void NamedRenderbufferStorageMultisample_State(GLuint renderbuffer, GLsizei samples, GLenum internalformat,
                                                GLsizei width, GLsizei height) {
    if (renderbuffer == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glNamedRenderbufferStorageMultisample: renderbuffer is 0"));
        return;
    }

    temporarilyBindRenderbuffer(renderbuffer);
    RenderbufferStorageMultisample_State(GL_RENDERBUFFER, samples, internalformat, width, height);

    LOG_D("[DSA] Set multisample storage for renderbuffer %u with internal format 0x%X and size (%d, %d)",
          renderbuffer, internalformat, width, height);
}

extern "C" GLAPI GLAPIENTRY void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples,
                                                                        GLenum internalformat, GLsizei width, GLsizei height) {
    GLState.Lock();
    NamedRenderbufferStorageMultisample_State(renderbuffer, samples, internalformat, width, height);
    GLState.Unlock();
}

// ----------------------------------------------------------------------------
// glGetNamedRenderbufferParameteriv
// ----------------------------------------------------------------------------

void GetNamedRenderbufferParameteriv_State(GLuint renderbuffer, GLenum pname, GLint *params) {
    if (renderbuffer == 0 || !params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetNamedRenderbufferParameteriv: invalid parameter"));
        return;
    }

    temporarilyBindRenderbuffer(renderbuffer);
    GetRenderbufferParameteriv_State(GL_RENDERBUFFER, pname, params);

    LOG_D("[DSA] Retrieved renderbuffer parameter 0x%X for renderbuffer %u", pname, renderbuffer);
}

extern "C" GLAPI GLAPIENTRY void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params) {
    GLState.Lock();
    GetNamedRenderbufferParameteriv_State(renderbuffer, pname, params);
    GLState.Unlock();
}

// ============================================================================
// ============================================================================
// MAP INITIALIZATION HELPERS
// ============================================================================
// ============================================================================

void InitFramebufferMap(size_t expectedSize) {
    auto &fb = GLState.framebuffer;
    fb.fboMap.reserve(expectedSize);
    fb.fboMapReverse.reserve(expectedSize);
}

// ============================================================================
// ============================================================================
// TEMP FBO POOL (for hot-path texture operations)
// ============================================================================
// ============================================================================

GLuint acquireTempFBO() {
    if (g_tempFboPoolIndex < g_tempFboPool.size()) {
        return g_tempFboPool[g_tempFboPoolIndex++];
    }
    GLuint fbo;
    GLES.glGenFramebuffers(1, &fbo);
    g_tempFboPool.push_back(fbo);
    g_tempFboPoolIndex++;
    return fbo;
}

void releaseTempFBO(GLuint fbo) {
    // Simple: just decrement index. FBO is not deleted, just returned to pool.
    if (g_tempFboPoolIndex > 0) g_tempFboPoolIndex--;
}

void releaseAllTempFBOs() {
    g_tempFboPoolIndex = 0;
}

void cleanupTempFBOs() {
    for (GLuint fbo : g_tempFboPool) {
        GLES.glDeleteFramebuffers(1, &fbo);
    }
    g_tempFboPool.clear();
    g_tempFboPoolIndex = 0;
}