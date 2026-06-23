// MobileGlues - gl/gl.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

// ============================================================================
// Core OpenGL Wrapper - State Tracking Layer
//
// This file implements the core OpenGL ES 3.2 wrapper functions that require
// additional state tracking, format conversion, or custom emulation on top of
// the native GLES 3.2 calls. Functions that are pure native pass-throughs
// (no state tracking needed) are handled by the NATIVE_FUNCTION macros in
// gles/loader.h.
// ============================================================================

#include "../includes.h"
#include <GL/gl.h>
#include "glcorearb.h"
#include "log.h"
#include "../gles/loader.h"
#include "../config/settings.h"
#include "mg.h"
#include "framebuffer.h"
#include "buffer.h"
#include "texture.h"
#include "getter.h"
#include "random_string_gen.h"
#include "../version.h"
#include "../config/config.h"
#include "FSR1/FSR1.h"

#include <string>
#include <format>
#include <vector>
#include <random>
#include <cstring>
#include <cstdlib>

#define DEBUG 0

// ============================================================================
// External Declarations
// ============================================================================

extern GLuint current_draw_fbo;
extern std::vector<framebuffer_t> framebuffers;

// ============================================================================
// Local State
// ============================================================================

static GLclampd currentDepthValue;

// ============================================================================
// Section: Depth Clear Workaround (ANGLE depth-clear bug fix)
// ============================================================================

static GLuint g_depthClearProgram = 0;
static GLuint g_depthClearVAO = 0;
static GLuint g_depthClearVBO = 0;

static const GLfloat kFullScreenTri[3][2] = {{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}};

static const char* kDepthClearVS = R"glsl(
    #version 300 es
    layout(location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0, 1.0);
    }
)glsl";

static const char* kDepthClearFS = R"glsl(
    #version 300 es
    precision mediump float;
    out vec4 fragColor;
    void main() {
        fragColor = vec4(0.0);
    }
)glsl";

static void InitDepthClearCoreProfile() {
    if (g_depthClearProgram) return;

    auto compile = [&](GLenum type, const char* src) {
        GLuint s = GLES.glCreateShader(type);
        GLES.glShaderSource(s, 1, &src, nullptr);
        GLES.glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, kDepthClearVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kDepthClearFS);

    g_depthClearProgram = GLES.glCreateProgram();
    GLES.glAttachShader(g_depthClearProgram, vs);
    GLES.glAttachShader(g_depthClearProgram, fs);
    GLES.glLinkProgram(g_depthClearProgram);
    GLES.glDeleteShader(vs);
    GLES.glDeleteShader(fs);

    GLES.glGenVertexArrays(1, &g_depthClearVAO);
    GLES.glGenBuffers(1, &g_depthClearVBO);

    GLES.glBindVertexArray(g_depthClearVAO);
    GLES.glBindBuffer(GL_ARRAY_BUFFER, g_depthClearVBO);
    GLES.glBufferData(GL_ARRAY_BUFFER, sizeof(kFullScreenTri), kFullScreenTri, GL_STATIC_DRAW);

    GLES.glEnableVertexAttribArray(0);
    GLES.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    GLES.glBindBuffer(GL_ARRAY_BUFFER, 0);
    GLES.glBindVertexArray(0);
}

static void DrawDepthClearTri() {
    InitDepthClearCoreProfile();

    GLboolean prevColorMask[4];
    GLES.glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);
    GLboolean prevDepthMask;
    GLES.glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    GLint prevDepthFunc;
    GLES.glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);

    GLES.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    GLES.glDepthMask(GL_TRUE);
    GLES.glDepthFunc(GL_ALWAYS);

    GLES.glUseProgram(g_depthClearProgram);
    GLES.glBindVertexArray(g_depthClearVAO);
    GLES.glDrawArrays(GL_TRIANGLES, 0, 3);
    GLES.glBindVertexArray(0);
    GLES.glUseProgram(0);

    GLES.glDepthFunc(prevDepthFunc);
    GLES.glDepthMask(prevDepthMask);
    GLES.glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
}

// ============================================================================
// Section: Depth Clear Helper
// ============================================================================

void glClearDepth(GLclampd depth) {
    LOG()
    currentDepthValue = depth;
    GLES.glClearDepthf((float)depth);
    CHECK_GL_ERROR
}

// ============================================================================
// Section: Texture Unit Management
// ============================================================================

void glActiveTexture(GLenum texture) {
    LOG()
    LOG_D("glActiveTexture, texture = %s", glEnumToString(texture))

    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture enum: %s", glEnumToString(texture))
        return;
    }

    set_gl_state_current_tex_unit(texture - GL_TEXTURE0);
    GLES.glActiveTexture(texture);
    ActivateTextureUnit(texture - GL_TEXTURE0);
    CHECK_GL_ERROR
}

// ============================================================================
// Section: Program Management
// ============================================================================

void glUseProgram(GLuint program) {
    LOG()
    set_gl_state_current_program(program);
    GLES.glUseProgram(program);
}

// ============================================================================
// Section: Buffer Object Management
// ============================================================================

void glGenBuffers(GLsizei n, GLuint* buffers) {
    LOG()
    LOG_D("glGenBuffers(%i, %p)", n, buffers)
    for (int i = 0; i < n; ++i) {
        buffers[i] = gen_buffer();
    }
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    LOG()
    LOG_D("glDeleteBuffers(%i, %p)", n, buffers)
    for (int i = 0; i < n; ++i) {
        if (find_real_buffer(buffers[i])) {
            GLuint real_buff = find_real_buffer(buffers[i]);
            GLES.glDeleteBuffers(1, &real_buff);
            CHECK_GL_ERROR
        }
        remove_buffer(buffers[i]);
    }
}

GLboolean glIsBuffer(GLuint buffer) {
    LOG()
    LOG_D("glIsBuffer, buffer = %d", buffer)
    return has_buffer(buffer);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    LOG()
    LOG_D("glBindBuffer, target = %s, buffer = %d", glEnumToString(target), buffer)

    set_bound_buffer_by_target(target, buffer);

    // Save IBO binding to VAO
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        update_vao_ibo_binding(find_bound_array(), buffer);
    }

    if (!has_buffer(buffer) || buffer == 0) {
        GLES.glBindBuffer(target, buffer);
        CHECK_GL_ERROR
        return;
    }

    GLuint real_buffer = find_real_buffer(buffer);
    if (!real_buffer) {
        GLES.glGenBuffers(1, &real_buffer);
        modify_buffer(buffer, real_buffer);
        CHECK_GL_ERROR
    }

    LOG_D("glBindBuffer: %d -> %d", buffer, real_buffer)
    GLES.glBindBuffer(target, real_buffer);
    CHECK_GL_ERROR
}

// ============================================================================
// Section: Texture Object Management
// ============================================================================

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    LOG()
    INIT_CHECK_GL_ERROR
    GLES.glDeleteTextures(n, textures);
    CHECK_GL_ERROR_NO_INIT

    for (GLsizei i = 0; i < n; ++i) {
        MarkTextureObjectForDeletion(textures[i]);
    }
}

// ============================================================================
// Section: Framebuffer Management
// ============================================================================

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    ensure_max_attachments();
    framebuffer_t& fbo = get_framebuffer(framebuffer);

    if (framebuffer == 0 && target != GL_READ_FRAMEBUFFER) {
        framebuffer = FSR1_Context::g_renderFBO;
        FSR1_Context::g_dirty = true;
    }

    if (target != GL_READ_FRAMEBUFFER) {
        set_gl_state_current_draw_fbo(framebuffer);
    }

    if (framebuffer != 0) {
        init_framebuffer(fbo);
    }

    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER) {
        current_draw_fbo = framebuffer;
    }

    GLES.glBindFramebuffer(target, framebuffer);
}

// ============================================================================
// Section: Texture Image Operations
// ============================================================================

void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    GLenum transfer_format = format;

    LOG_D("mg_glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat),
          glEnumToString(internalFormat), width, height, border,
          glEnumToString(format), glEnumToString(type), pixels)

    internal_convert(reinterpret_cast<GLenum*>(&internalFormat), &type, &format);

    LOG_D("GLES.glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat),
          glEnumToString(internalFormat), width, height, border,
          glEnumToString(format), glEnumToString(type), pixels)

    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_2D) {
        int max1 = 4096;
        GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > max1) ? 0 : height);
        set_gl_state_proxy_intformat(internalFormat);
        return;
    }

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    // BGRA swizzle workaround for small textures (e.g. Xaero's minimap tiles)
    if (transfer_format == GL_BGRA && tex->format != transfer_format &&
        internalFormat == GL_RGBA8 && width <= 128 && height <= 128) {
        LOG_D("Detected GL_BGRA format @ tex = %d, do swizzle", tex->texture)

        if (tex->swizzle_param[0] == 0) {
            tex->swizzle_param[0] = GL_RED;
            tex->swizzle_param[1] = GL_GREEN;
            tex->swizzle_param[2] = GL_BLUE;
            tex->swizzle_param[3] = GL_ALPHA;
        }

        GLint r = tex->swizzle_param[0];
        GLint g = tex->swizzle_param[1];
        GLint b = tex->swizzle_param[2];
        GLint a = tex->swizzle_param[3];
        tex->swizzle_param[0] = g;
        tex->swizzle_param[1] = b;
        tex->swizzle_param[2] = a;
        tex->swizzle_param[3] = r;
        tex->format = transfer_format;

        GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, tex->swizzle_param[0]);
        GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, tex->swizzle_param[1]);
        GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, tex->swizzle_param[2]);
        GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, tex->swizzle_param[3]);
        CHECK_GL_ERROR
    }

    tex->format = format;
    GLES.glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
    CHECK_GL_ERROR
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type,
                     const void* pixels) {
    LOG()
    LOG_D("glTexSubImage2D, target = %s, level = %d, xoffset = %d, yoffset = %d, "
          "width = %d, height = %d, format = %s, type = %s, pixels = 0x%x",
          glEnumToString(target), level, xoffset, yoffset, width, height,
          glEnumToString(format), glEnumToString(type), pixels)

    // BGRA format conversion
    if (format == GL_BGRA && (type == GL_UNSIGNED_INT_8_8_8_8 || type == GL_UNSIGNED_INT_8_8_8_8_REV)) {
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
    }

    GLES.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    CHECK_GL_ERROR
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    LOG()
    INIT_CHECK_GL_ERROR

    GLint realInternalFormat;
    GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &realInternalFormat);
    internalFormat = (GLenum)realInternalFormat;

    LOG_D("glCopyTexImage2D, target: %d, level: %d, internalFormat: %d, x: %d, "
          "y: %d, width: %d, height: %d, border: %d",
          target, level, internalFormat, x, y, width, height, border)

    if (is_depth_format(internalFormat)) {
        // Depth textures cannot be copied directly; use blit workaround
        GLenum format = GL_DEPTH_COMPONENT;
        GLenum type = GL_UNSIGNED_INT;
        internal_convert(&internalFormat, &type, &format);
        GLES.glTexImage2D(target, level, (GLint)internalFormat, width, height, border, format, type, nullptr);
        CHECK_GL_ERROR_NO_INIT

        GLint prevDrawFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
        CHECK_GL_ERROR_NO_INIT

        GLuint tempDrawFBO;
        glGenFramebuffers(1, &tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT

        GLint currentTex;
        glGetIntegerv(get_binding_for_target(target), &currentTex);
        CHECK_GL_ERROR_NO_INIT
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);
        CHECK_GL_ERROR_NO_INIT

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            CHECK_GL_ERROR_NO_INIT
            glDeleteFramebuffers(1, &tempDrawFBO);
            CHECK_GL_ERROR_NO_INIT
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            CHECK_GL_ERROR_NO_INIT
            return;
        }
        CHECK_GL_ERROR_NO_INIT

        GLES.glBlitFramebuffer(x, y, x + width, y + height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        CHECK_GL_ERROR_NO_INIT

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        glDeleteFramebuffers(1, &tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
    } else {
        GLES.glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
        CHECK_GL_ERROR_NO_INIT
    }

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR_NO_INIT
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height) {
    LOG()
    GLint internalFormat;
    GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

    LOG_D("glCopyTexSubImage2D, target: %s, level: %d, xoffset: %d, yoffset: %d, "
          "x: %d, y: %d, width: %d, height: %d",
          glEnumToString(target), level, xoffset, yoffset, x, y, width, height)

    if (is_depth_format((GLenum)internalFormat)) {
        // Depth textures cannot be copied directly; use blit workaround
        GLint prevReadFBO, prevDrawFBO;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);

        GLuint tempDrawFBO;
        glGenFramebuffers(1, &tempDrawFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);

        GLint currentTex;
        glGetIntegerv(get_binding_for_target(target), &currentTex);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glDeleteFramebuffers(1, &tempDrawFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            return;
        }

        GLES.glBlitFramebuffer(x, y, x + width, y + height,
                               xoffset, yoffset, xoffset + width, yoffset + height,
                               GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempDrawFBO);
    } else {
        GLES.glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    }

    CHECK_GL_ERROR
}

// ============================================================================
// Section: Renderbuffer Operations
// ============================================================================

void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG()
    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorage, target: %s, internalFormat: %s, width: %d, height: %d",
          glEnumToString(target), glEnumToString(internalFormat), width, height)

    GLES.glRenderbufferStorage(target, internalFormat, width, height);
    CHECK_GL_ERROR_NO_INIT
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                      GLsizei width, GLsizei height) {
    LOG()
    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorageMultisample, target: %d, samples: %d, "
          "internalFormat: %d, width: %d, height: %d",
          target, samples, internalFormat, width, height)

    GLES.glRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
    CHECK_GL_ERROR_NO_INIT
}

// ============================================================================
// Section: Pixel Operations
// ============================================================================

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, void* pixels) {
    LOG()
    LOG_D("glReadPixels, x=%d, y=%d, width=%d, height=%d, format=0x%x, type=0x%x, pixels=0x%x",
          x, y, width, height, format, type, pixels)

    static int count = 0;
    GLenum prevFormat = format;

    // BGRA format conversion: GLES does not support BGRA readback natively
    if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8) {
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
    }

    LOG_D("glReadPixels converted, x=%d, y=%d, width=%d, height=%d, format=0x%x, type=0x%x, pixels=0x%x",
          x, y, width, height, format, type, pixels)

    GLES.glReadPixels(x, y, width, height, format, type, pixels);

#if GLOBAL_DEBUG || DEBUG
    if (prevFormat == GL_BGRA && type == GL_UNSIGNED_BYTE) {
        std::vector<uint8_t> px(width * height * sizeof(uint8_t) * 4, 0);
        GLES.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        GLES.glReadPixels(x, y, width, height, format, type, px.data());

        std::fstream fs(std::string(concatenate(mg_directory_path, "/readpixels/")) +
                        std::to_string(count++) + ".bin",
                        std::ios::out | std::ios::binary | std::ios::trunc);
        fs.write((const char*)px.data(), px.size());
        fs.close();
    }
#endif

    CHECK_GL_ERROR
}

void glPixelStorei(GLenum pname, GLint param) {
    LOG_D("glPixelStorei, pname = %s, param = %d", glEnumToString(pname), param)
    GLES.glPixelStorei(pname, param);
    CHECK_GL_ERROR
}

// ============================================================================
// Section: State Query
// ============================================================================

void glGetIntegerv(GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetIntegerv, pname: %s", glEnumToString(pname))

    switch (pname) {
    case GL_NUM_EXTENSIONS + GL_BACKEND_GETTER_MG:
        GLES.glGetIntegerv(pname - GL_BACKEND_GETTER_MG, params);
        return;

    case GL_CONTEXT_PROFILE_MASK:
        (*params) = GL_CONTEXT_CORE_PROFILE_BIT;
        break;

    case GL_NUM_EXTENSIONS: {
        static GLint num_extensions = -1;
        if (num_extensions == -1) {
            const GLubyte* ext_str = glGetString(GL_EXTENSIONS);
            if (ext_str) {
                std::string copy_str((const char*)ext_str);
                std::string token;
                size_t pos = 0;
                num_extensions = 0;
                while ((pos = copy_str.find(' ')) != std::string::npos) {
                    num_extensions++;
                    copy_str.erase(0, pos + 1);
                }
                if (!copy_str.empty()) num_extensions++;
            } else {
                num_extensions = 0;
            }
        }
        (*params) = num_extensions;
        break;
    }

    case GL_MAJOR_VERSION:
        (*params) = GLVersion.Major;
        break;

    case GL_MINOR_VERSION:
        (*params) = GLVersion.Minor;
        break;

    case GL_MAX_TEXTURE_IMAGE_UNITS: {
        int es_params = 16;
        GLES.glGetIntegerv(pname, &es_params);
        CHECK_GL_ERROR
        (*params) = es_params * 2;
        break;
    }

    case GL_CONTEXT_FLAGS:
        (*params) = GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT |
                    GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT |
                    GL_CONTEXT_FLAG_NO_ERROR_BIT;
        break;

    case GL_ARRAY_BUFFER_BINDING:
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
    case GL_COPY_READ_BUFFER_BINDING:
    case GL_COPY_WRITE_BUFFER_BINDING:
    case GL_DRAW_INDIRECT_BUFFER_BINDING:
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_PIXEL_PACK_BUFFER_BINDING:
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
    case GL_SHADER_STORAGE_BUFFER_BINDING:
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
    case GL_UNIFORM_BUFFER_BINDING:
        (*params) = (int)find_bound_buffer(pname);
        LOG_D("  -> %d", *params)
        break;

    case GL_VERTEX_ARRAY_BINDING:
        (*params) = (int)find_bound_array();
        break;

    default:
        GLES.glGetIntegerv(pname, params);
        LOG_D("  -> %d", *params)
        CHECK_GL_ERROR
    }
}

GLenum glGetError() {
    LOG()
    GLenum err = GLES.glGetError();
    if (err != GL_NO_ERROR) {
        LOG_W("glGetError\n -> %d", err)
        LOG_W("Now try to cheat.")
    }
    return GL_NO_ERROR;
}

const GLubyte* glGetString(GLenum name) {
    LOG()
    LOG_D("glGetString, %s", glEnumToString(name))

    switch (name) {
    case GL_VENDOR: {
        static std::string vendorString;
        if (vendorString.empty()) {
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                vendorString = "Swung0x48, BZLZHH, Tungsten, EternityQwQ";
            } else {
                const char choices[] = "AINM";
                vendorString = choices[rand() % 4];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = false;
                randStrOpts.minLength = 3;
                randStrOpts.maxLength = 8;
                randStrOpts.includeLowercase = false;
                randStrOpts.includeUppercase = false;
                randStrOpts.customChars = "IMenaNtMseAVlD";
                vendorString += GenerateRandomString(randStrOpts);
            }
        }
        return (const GLubyte*)vendorString.c_str();
    }

    case GL_VERSION: {
        static std::string versionString;
        if (versionString.empty()) {
            versionString = GLVersion.toString();
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                if (GLVersion.toInt(2) == DEFAULT_GL_VERSION) {
                    versionString += " MobileGLESWrapper ";
                } else {
                    Version defaultVersion = Version(DEFAULT_GL_VERSION);
                    versionString += " §4§l(" + defaultVersion.toString() + ") MobileGLESWrapper§r ";
                }
                versionString += std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + std::to_string(REVISION);
#if PATCH != 0
                versionString += "." + std::to_string(PATCH);
#endif
#if defined(VERSION_TYPE)
#if VERSION_TYPE == VERSION_ALPHA
                versionString += "·Alpha";
#elif VERSION_TYPE == VERSION_BETA
                versionString += "·Beta";
#elif VERSION_TYPE == VERSION_DEVELOPMENT
                versionString += "·Dev";
#elif VERSION_TYPE == VERSION_RC
                versionString += "·RC" + std::to_string(VERSION_RC_NUMBER);
#endif
#endif
                versionString += VERSION_SUFFIX;
            } else {
                const char choices[] = "AIN";
                versionString += " ";
                versionString += choices[rand() % 3];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = false;
                randStrOpts.customChars = " ";
                versionString += GenerateRandomString(randStrOpts);

                RandomStringOptions randStrOpts2;
                randStrOpts2.includeDigits = false;
                randStrOpts2.includeUppercase = false;
                randStrOpts2.minLength = 1;
                randStrOpts2.maxLength = 4;

                versionString += std::to_string(MAJOR) + GenerateRandomString(randStrOpts2) +
                                 std::to_string(MINOR) + GenerateRandomString(randStrOpts2) +
                                 std::to_string(REVISION) + GenerateRandomString(randStrOpts2) +
                                 std::to_string(PATCH) + GenerateRandomString(randStrOpts2);
            }
        }
        return (const GLubyte*)versionString.c_str();
    }

    case GL_RENDERER: {
        static std::string rendererString;
        if (rendererString.empty()) {
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                std::string gpuName = getGpuName();
                std::string glesName = getGLESName();
                rendererString = gpuName + " | " + glesName;
            } else {
                const char choices[] = "AINM";
                rendererString = choices[rand() % 4];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = true;
                randStrOpts.minLength = 6;
                randStrOpts.maxLength = 12;
                randStrOpts.includeLowercase = false;
                randStrOpts.includeUppercase = false;
                randStrOpts.customChars = "IRMenaNtfsoerAceVlDG";
                rendererString += GenerateRandomString(randStrOpts);

                int junkInfoTime = rand() % 3 + 1;
                for (int i = 0; i < junkInfoTime; ++i) {
                    rendererString += " ";
                    RandomStringOptions randStrOpts2;
                    randStrOpts2.minLength = 3;
                    randStrOpts2.maxLength = 6;
                    randStrOpts2.includeLowercase = false;
                    randStrOpts2.includeUppercase = false;
                    randStrOpts2.customChars = "IRenaNtfsoerAcieVDcsG";
                    rendererString += GenerateRandomString(randStrOpts2);
                }
            }
        }
        return (const GLubyte*)rendererString.c_str();
    }

    case GL_SHADING_LANGUAGE_VERSION: {
        static std::string shadingLangString;
        if (shadingLangString.empty()) {
            std::string baseVer = "4.60";
            if (global_settings.hide_mg_env_level >= HideMGEnvLevel::Level1) {
                shadingLangString = baseVer;
                int junkCount = rand() % 2 + 1;
                for (int i = 0; i < junkCount; ++i) {
                    shadingLangString += " ";
                    RandomStringOptions junkOpts;
                    junkOpts.minLength = 2;
                    junkOpts.maxLength = 5;
                    junkOpts.includeLowercase = false;
                    junkOpts.includeUppercase = false;
                    junkOpts.customChars = "IAneNDtVsaMIl";
                    shadingLangString += GenerateRandomString(junkOpts);
                }
            } else {
                shadingLangString = baseVer + " MobileGLESWrapper with glslang and SPIRV-Cross";
            }
        }
        return reinterpret_cast<const GLubyte*>(shadingLangString.c_str());
    }

    case GL_EXTENSIONS: {
        static std::string cached;
        cached = GetExtensionsList();
        return (const GLubyte*)cached.c_str();
    }

    case GL_SETTINGS_MG: {
        if (global_settings.hide_mg_env_level >= HideMGEnvLevel::Level1)
            return GLES.glGetString(name);

        static char* settings_string = nullptr;
        std::string tmp = dump_settings_string("  ");
        settings_string = strdup(tmp.c_str());
        return reinterpret_cast<const GLubyte*>(settings_string);
    }

    case GL_VERSION + GL_BACKEND_GETTER_MG:
    case GL_VENDOR + GL_BACKEND_GETTER_MG:
    case GL_RENDERER + GL_BACKEND_GETTER_MG:
    case GL_EXTENSIONS + GL_BACKEND_GETTER_MG:
    case GL_SHADING_LANGUAGE_VERSION + GL_BACKEND_GETTER_MG:
        if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled)
            return GLES.glGetString(name - GL_BACKEND_GETTER_MG);
        else
            return GLES.glGetString(name);

    default:
        return GLES.glGetString(name);
    }
}

const GLubyte* glGetStringi(GLenum name, GLuint index) {
    LOG()

    if (name == GL_EXTENSIONS + GL_BACKEND_GETTER_MG && global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
        return GLES.glGetStringi(name - GL_BACKEND_GETTER_MG, index);
    }

    typedef struct {
        GLenum name;
        const char** parts;
        GLuint count;
    } StringCache;

    static StringCache caches[] = {
        {GL_EXTENSIONS, nullptr, 0},
        {GL_VENDOR, nullptr, 0},
        {GL_VERSION, nullptr, 0},
        {GL_SHADING_LANGUAGE_VERSION, nullptr, 0}
    };

    static int initialized = 0;
    if (!initialized) {
        for (auto& cache : caches) {
            GLenum target = cache.name;
            const GLubyte* str = nullptr;
            const char* delimiter = " ";

            switch (target) {
            case GL_VENDOR:
                str = glGetString(GL_VENDOR);
                delimiter = ", ";
                break;
            case GL_VERSION:
                str = glGetString(GL_VERSION);
                delimiter = " .";
                break;
            case GL_SHADING_LANGUAGE_VERSION:
                str = glGetString(GL_SHADING_LANGUAGE_VERSION);
                break;
            case GL_EXTENSIONS:
                str = glGetString(GL_EXTENSIONS);
                break;
            default:
                return GLES.glGetStringi(name, index);
            }

            if (!str) continue;

            std::string copy_str((const char*)str);
            std::string token_str;
            size_t start = 0;
            size_t end = copy_str.find_first_of(delimiter);

            while (end != std::string::npos) {
                token_str = copy_str.substr(start, end - start);
                cache.parts = (const char**)realloc(cache.parts, (cache.count + 1) * sizeof(char*));
                cache.parts[cache.count++] = strdup(token_str.c_str());
                start = end + 1;
                end = copy_str.find_first_of(delimiter, start);
            }
            token_str = copy_str.substr(start);
            cache.parts = (const char**)realloc(cache.parts, (cache.count + 1) * sizeof(char*));
            cache.parts[cache.count++] = strdup(token_str.c_str());
        }
        initialized = 1;
    }

    for (auto& cache : caches) {
        if (cache.name == name) {
            if (index >= cache.count) {
                return nullptr;
            }
            return (const GLubyte*)cache.parts[index];
        }
    }

    return nullptr;
}

// ============================================================================
// Section: Miscellaneous
// ============================================================================

void glClear(GLbitfield mask) {
    LOG();
    LOG_D("glClear, mask = 0x%x", mask);

    INIT_CHECK_GL_ERROR
    CHECK_GL_ERROR_NO_INIT

    if (global_settings.angle == AngleMode::Enabled && mask == GL_DEPTH_BUFFER_BIT &&
        fabs(currentDepthValue - 1.0f) <= 0.001f && framebuffers[current_draw_fbo].color_attachments_all_none) {
        LOG_D("doing depth workaround")
        if (global_settings.angle_depth_clear_fix_mode == AngleDepthClearFixMode::Mode1)
            DrawDepthClearTri();
        else if (global_settings.angle_depth_clear_fix_mode == AngleDepthClearFixMode::Mode2) {
            const GLfloat clear_depth_value = 1.0f;
            GLES.glClearBufferfv(GL_DEPTH, 0, &clear_depth_value);
        }
        GLES.glClear(mask);
    } else {
        GLES.glClear(mask);
    }

    CHECK_GL_ERROR_NO_INIT;
}

void glHint(GLenum target, GLenum mode) {
    LOG()
    LOG_D("glHint, target = %s, mode = %s", glEnumToString(target), glEnumToString(mode))
    GLES.glHint(target, mode);
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    LOG()
    LOG_D("glViewport: x=%d, y=%d, w=%d, h=%d", x, y, w, h);

    if (w > FSR1_Context::g_pendingWidth || h > FSR1_Context::g_pendingHeight) {
        FSR1_Context::g_pendingWidth = w;
        FSR1_Context::g_pendingHeight = h;
        FSR1_Context::g_resolutionChanged = true;
    }

    GLES.glViewport(x, y, w, h);
}