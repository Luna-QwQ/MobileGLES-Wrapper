// MobileGlues - gl/texture.cpp
// Texture state management: ID mapping, unit-tracking, format conversion,
// 1D→2D texture remapping, pixel store state, and BGRA conversion.
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
// Texture ID management
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGenTextures, GLsizei n, GLuint *textures)
{
    // Already native in gl_native.cpp (no virtual→real mapping needed for textures)
    // But we track them for format conversion
    _native(n, textures);

    auto &ts = GLState.texture;
    for (GLsizei i = 0; i < n; i++) {
        TextureObject obj;
        obj.id = textures[i];
        ts.textureMap[textures[i]] = obj;
        ts.textureMapReverse[textures[i]] = textures[i];
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGenTextures, n, textures)

NATIVE_FUNCTION_HEAD(void, glDeleteTextures, GLsizei n, const GLuint *textures)
{
    _native(n, textures);

    auto &ts = GLState.texture;
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = textures[i];
        if (id == 0) continue;

        ts.textureMap.erase(id);
        ts.textureMapReverse.erase(id);

        // Remove from texture unit bindings
        for (int unit = 0; unit < MAX_TEXTURE_UNITS; unit++) {
            for (auto &pair : ts.texUnits[unit].binding) {
                if (pair.second == id) pair.second = 0;
            }
        }
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteTextures, n, textures)

// ============================================================================
// glActiveTexture - track active texture unit
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glActiveTexture, GLenum texture)
{
    _native(texture);
    GLState.texture.activeUnit = texture - GL_TEXTURE0;
    GLState.currentTexUnit = GLState.texture.activeUnit;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glActiveTexture, texture)

// ============================================================================
// glBindTexture - ID mapping and 1D→2D conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindTexture, GLenum target, GLuint texture)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);

    GLuint realTex = GLState.GetRealTexture(texture);
    if (realTex == 0 && texture != 0) {
        // Texture not in our map, use as-is (shouldn't happen normally)
        realTex = texture;
    }

    _native(esTarget, realTex);

    // Track binding per-unit
    int unit = GLState.texture.activeUnit;
    GLenum bindingTarget = GLStateManager::TargetToBindingTarget(target);
    GLState.texture.texUnits[unit].binding[bindingTarget] = texture;

    // Update texture info if we have it
    if (texture != 0) {
        auto *info = GLState.GetTextureInfo(texture);
        if (info) {
            info->target = esTarget;
        }
    }

    STATE_LOG("glBindTexture: target=0x%X→0x%X, unit=%d, virt=%u, real=%u",
              target, esTarget, unit, texture, realTex);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindTexture, target, texture)

// ============================================================================
// glTexImage2D - format conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glTexImage2D, GLenum target, GLint level, GLint internalformat,
                     GLsizei width, GLsizei height, GLint border,
                     GLenum format, GLenum type, const void *pixels)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);
    GLenum esFormat = GLStateManager::ConvertFormat(format);
    GLenum esType = GLStateManager::ConvertType(type);

    // 1D texture → 2D with height=1
    if (target == GL_TEXTURE_1D || target == GL_PROXY_TEXTURE_1D) {
        height = 1;
    }

    _native(esTarget, level, esInternalFormat, width, height, border, esFormat, esType, pixels);

    // Track texture info
    int unit = GLState.texture.activeUnit;
    GLenum bindingTarget = GLStateManager::TargetToBindingTarget(target);
    GLuint boundTex = GLState.texture.texUnits[unit].binding[bindingTarget];
    if (boundTex) {
        auto *info = GLState.GetTextureInfo(boundTex);
        if (info) {
            info->target = esTarget;
            info->internalFormat = esInternalFormat;
            info->width = width;
            info->height = (target == GL_TEXTURE_1D) ? 1 : height;
            info->isDepthStencil = GLStateManager::IsDepthStencilFormat(internalformat);
            info->isCompressed = false;
        }
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexImage2D, target, level, internalformat, width, height, border, format, type, pixels)

NATIVE_FUNCTION_HEAD(void, glTexImage3D, GLenum target, GLint level, GLint internalformat,
                     GLsizei width, GLsizei height, GLsizei depth, GLint border,
                     GLenum format, GLenum type, const void *pixels)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);
    GLenum esFormat = GLStateManager::ConvertFormat(format);
    GLenum esType = GLStateManager::ConvertType(type);

    _native(esTarget, level, esInternalFormat, width, height, depth, border, esFormat, esType, pixels);

    int unit = GLState.texture.activeUnit;
    GLenum bindingTarget = GLStateManager::TargetToBindingTarget(target);
    GLuint boundTex = GLState.texture.texUnits[unit].binding[bindingTarget];
    if (boundTex) {
        auto *info = GLState.GetTextureInfo(boundTex);
        if (info) {
            info->target = esTarget;
            info->internalFormat = esInternalFormat;
            info->width = width;
            info->height = height;
            info->depth = depth;
            info->isDepthStencil = GLStateManager::IsDepthStencilFormat(internalformat);
            info->isCompressed = false;
        }
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexImage3D, target, level, internalformat, width, height, depth, border, format, type, pixels)

// ============================================================================
// glTexSubImage2D / glTexSubImage3D
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esFormat = GLStateManager::ConvertFormat(format);
    GLenum esType = GLStateManager::ConvertType(type);

    if (target == GL_TEXTURE_1D) {
        height = 1;
        yoffset = 0;
    }

    _native(esTarget, level, xoffset, yoffset, width, height, esFormat, esType, pixels);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexSubImage2D, target, level, xoffset, yoffset, width, height, format, type, pixels)

NATIVE_FUNCTION_HEAD(void, glTexSubImage3D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esFormat = GLStateManager::ConvertFormat(format);
    GLenum esType = GLStateManager::ConvertType(type);

    _native(esTarget, level, xoffset, yoffset, zoffset, width, height, depth, esFormat, esType, pixels);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexSubImage3D, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)

// ============================================================================
// glCopyTexImage2D / glCopyTexSubImage2D
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glCopyTexImage2D, GLenum target, GLint level, GLenum internalformat,
                     GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);

    if (target == GL_TEXTURE_1D) {
        height = 1;
    }

    _native(esTarget, level, esInternalFormat, x, y, width, height, border);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCopyTexImage2D, target, level, internalformat, x, y, width, height, border)

NATIVE_FUNCTION_HEAD(void, glCopyTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);

    if (target == GL_TEXTURE_1D) {
        height = 1;
        yoffset = 0;
    }

    _native(esTarget, level, xoffset, yoffset, x, y, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCopyTexSubImage2D, target, level, xoffset, yoffset, x, y, width, height)

NATIVE_FUNCTION_HEAD(void, glCopyTexSubImage3D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, level, xoffset, yoffset, zoffset, x, y, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCopyTexSubImage3D, target, level, xoffset, yoffset, zoffset, x, y, width, height)

// ============================================================================
// glTexStorage2D / glTexStorage3D
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glTexStorage2D, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);

    if (target == GL_TEXTURE_1D) {
        height = 1;
    }

    _native(esTarget, levels, esInternalFormat, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexStorage2D, target, levels, internalformat, width, height)

NATIVE_FUNCTION_HEAD(void, glTexStorage3D, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);
    _native(esTarget, levels, esInternalFormat, width, height, depth);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexStorage3D, target, levels, internalformat, width, height, depth)

// ============================================================================
// Compressed texture functions
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glCompressedTexImage2D, GLenum target, GLint level, GLenum internalformat,
                     GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    if (target == GL_TEXTURE_1D) height = 1;
    _native(esTarget, level, internalformat, width, height, border, imageSize, data);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCompressedTexImage2D, target, level, internalformat, width, height, border, imageSize, data)

NATIVE_FUNCTION_HEAD(void, glCompressedTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    if (target == GL_TEXTURE_1D) { height = 1; yoffset = 0; }
    _native(esTarget, level, xoffset, yoffset, width, height, format, imageSize, data);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCompressedTexSubImage2D, target, level, xoffset, yoffset, width, height, format, imageSize, data)

NATIVE_FUNCTION_HEAD(void, glCompressedTexImage3D, GLenum target, GLint level, GLenum internalformat,
                     GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, level, internalformat, width, height, depth, border, imageSize, data);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCompressedTexImage3D, target, level, internalformat, width, height, depth, border, imageSize, data)

NATIVE_FUNCTION_HEAD(void, glCompressedTexSubImage3D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCompressedTexSubImage3D, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)

// ============================================================================
// glTexParameter - parameter conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glTexParameterf, GLenum target, GLenum pname, GLfloat param)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, pname, param);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexParameterf, target, pname, param)

NATIVE_FUNCTION_HEAD(void, glTexParameteri, GLenum target, GLenum pname, GLint param)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);

    // Convert legacy texture parameters
    GLint esParam = param;
    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER:
            // GL_LINEAR, GL_NEAREST etc. are the same between desktop and ES
            break;
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_TEXTURE_WRAP_R:
            if (param == GL_CLAMP) esParam = GL_CLAMP_TO_EDGE;
            break;
        default:
            break;
    }

    _native(esTarget, pname, esParam);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexParameteri, target, pname, param)

NATIVE_FUNCTION_HEAD(void, glTexParameterfv, GLenum target, GLenum pname, const GLfloat *params)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexParameterfv, target, pname, params)

NATIVE_FUNCTION_HEAD(void, glTexParameteriv, GLenum target, GLenum pname, const GLint *params)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);

    GLint esParam[4];
    memcpy(esParam, params, sizeof(GLint) * 4);
    switch (pname) {
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_TEXTURE_WRAP_R:
            if (esParam[0] == GL_CLAMP) esParam[0] = GL_CLAMP_TO_EDGE;
            break;
        default:
            break;
    }

    _native(esTarget, pname, esParam);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexParameteriv, target, pname, params)

// ============================================================================
// glGetTexParameter
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetTexParameterfv, GLenum target, GLenum pname, GLfloat *params)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetTexParameterfv, target, pname, params)

NATIVE_FUNCTION_HEAD(void, glGetTexParameteriv, GLenum target, GLenum pname, GLint *params)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetTexParameteriv, target, pname, params)

// ============================================================================
// glGetTexLevelParameter (proxy texture handling)
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetTexLevelParameteriv, GLenum target, GLint level, GLenum pname, GLint *params)
{
    // Handle proxy textures (desktop-specific)
    if (target == GL_PROXY_TEXTURE_1D || target == GL_PROXY_TEXTURE_2D ||
        target == GL_PROXY_TEXTURE_3D || target == GL_PROXY_TEXTURE_CUBE_MAP) {
        // Return dummy values for proxy textures
        switch (pname) {
            case GL_TEXTURE_WIDTH:      *params = 4096; break;
            case GL_TEXTURE_HEIGHT:     *params = 4096; break;
            case GL_TEXTURE_DEPTH:      *params = 256; break;
            case GL_TEXTURE_INTERNAL_FORMAT: *params = GL_RGBA8; break;
            case GL_TEXTURE_RED_SIZE:   *params = 8; break;
            case GL_TEXTURE_GREEN_SIZE: *params = 8; break;
            case GL_TEXTURE_BLUE_SIZE:  *params = 8; break;
            case GL_TEXTURE_ALPHA_SIZE: *params = 8; break;
            case GL_TEXTURE_COMPRESSED: *params = GL_FALSE; break;
            default:                    *params = 0; break;
        }
        return;
    }

    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    if (target == GL_TEXTURE_1D) {
        // For 1D textures, height is always 1
        if (pname == GL_TEXTURE_HEIGHT) {
            *params = 1;
            return;
        }
    }
    _native(esTarget, level, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetTexLevelParameteriv, target, level, pname, params)

NATIVE_FUNCTION_HEAD(void, glGetTexLevelParameterfv, GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    if (target == GL_PROXY_TEXTURE_1D || target == GL_PROXY_TEXTURE_2D ||
        target == GL_PROXY_TEXTURE_3D || target == GL_PROXY_TEXTURE_CUBE_MAP) {
        GLint intVal = 0;
        glGetTexLevelParameteriv(target, level, pname, &intVal);
        *params = (GLfloat)intVal;
        return;
    }
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget, level, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetTexLevelParameterfv, target, level, pname, params)

// ============================================================================
// glGenerateMipmap
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGenerateMipmap, GLenum target)
{
    GLenum esTarget = GLStateManager::ConvertTextureTarget(target);
    _native(esTarget);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGenerateMipmap, target)

// ============================================================================
// glPixelStorei - track pixel store state
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glPixelStorei, GLenum pname, GLint param)
{
    _native(pname, param);

    auto &ts = GLState.texture;
    switch (pname) {
        case GL_UNPACK_ALIGNMENT:  ts.unpackAlignment = param; break;
        case GL_UNPACK_ROW_LENGTH: ts.unpackRowLength = param; break;
        case GL_UNPACK_IMAGE_HEIGHT: ts.unpackImageHeight = param; break;
        case GL_UNPACK_SKIP_PIXELS: ts.unpackSkipPixels = param; break;
        case GL_UNPACK_SKIP_ROWS:   ts.unpackSkipRows = param; break;
        case GL_UNPACK_SKIP_IMAGES: ts.unpackSkipImages = param; break;
        case GL_PACK_ALIGNMENT:     ts.packAlignment = param; break;
        case GL_PACK_ROW_LENGTH:    ts.packRowLength = param; break;
        case GL_PACK_IMAGE_HEIGHT:  ts.packImageHeight = param; break;
        case GL_PACK_SKIP_PIXELS:   ts.packSkipPixels = param; break;
        case GL_PACK_SKIP_ROWS:     ts.packSkipRows = param; break;
        case GL_PACK_SKIP_IMAGES:   ts.packSkipImages = param; break;
        default: break;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glPixelStorei, pname, param)

// ============================================================================
// glReadPixels - BGRA format conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{
    GLenum esFormat = GLStateManager::ConvertFormat(format);
    GLenum esType = GLStateManager::ConvertType(type);

    if (format == GL_BGRA && type == GL_UNSIGNED_BYTE) {
        // Read as RGBA and swap R/B
        std::vector<GLubyte> rgba(width * height * 4);
        _native(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        GLubyte *dst = (GLubyte *)pixels;
        for (GLsizei i = 0; i < width * height; i++) {
            dst[i * 4 + 0] = rgba[i * 4 + 2]; // B
            dst[i * 4 + 1] = rgba[i * 4 + 1]; // G
            dst[i * 4 + 2] = rgba[i * 4 + 0]; // R
            dst[i * 4 + 3] = rgba[i * 4 + 3]; // A
        }
    } else {
        _native(x, y, width, height, esFormat, esType, pixels);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glReadPixels, x, y, width, height, format, type, pixels)

// ============================================================================
// glRenderbufferStorage - format conversion
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glRenderbufferStorage, GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);
    _native(target, esInternalFormat, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glRenderbufferStorage, target, internalformat, width, height)

NATIVE_FUNCTION_HEAD(void, glRenderbufferStorageMultisample, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    GLenum esInternalFormat = GLStateManager::ConvertInternalFormat(internalformat);
    _native(target, samples, esInternalFormat, width, height);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glRenderbufferStorageMultisample, target, samples, internalformat, width, height)

// ============================================================================
// Map initialization helpers (for backward compatibility)
// ============================================================================

void InitTextureMap(size_t expectedSize) {
    auto &ts = GLState.texture;
    ts.textureMap.reserve(expectedSize);
    ts.textureMapReverse.reserve(expectedSize);
}