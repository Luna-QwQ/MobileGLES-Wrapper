// MobileGlues - gl/texture.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

// ============================================================================
// Texture Module Architecture (OpenGL ES 3.2)
//
// Rule: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// Architecture pattern (DirectGLES):
//   Public function = thin wrapper: Lock → _State → _Backend → Unlock
//   _State function: validates parameters, converts desktop GL enums to GLES,
//                    updates GLState, records errors
//   _Backend function: performs the actual GLES call through the gles loader
//
// Native (ES 3.2 directly supports):
//   - glTexImage2D, glTexImage3D, glTexSubImage2D, glTexSubImage3D
//   - glTexStorage2D, glTexStorage3D
//   - glCompressedTexImage2D, glCompressedTexImage3D
//   - glCompressedTexSubImage2D, glCompressedTexSubImage3D
//   - glCopyTexImage2D, glCopyTexSubImage2D, glCopyTexSubImage3D
//   - glGenerateMipmap
//   - glTexParameterf, glTexParameteri, glTexParameteriv, glTexParameterfv
//   - glGetTexParameteriv, glGetTexParameterfv
//   - glGetTexLevelParameteriv, glGetTexLevelParameterfv
//   - glTexBuffer, glTexBufferRange
//   - glBindTexture (with 1D→2D mapping for legacy targets)
// ============================================================================

#include "texture.h"
#include "GLES3/gl32.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_map>

#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <malloc.h>

#include "../gles/gles.h"
#include "../gles/loader.h"
#include "drawing.h"
#include "framebuffer.h"
#include "log.h"
#include "mg.h"
#include <GL/gl.h>

#define DEBUG 0

// ============================================================================
// Internal helpers: mip level size calculation
// ============================================================================

int nlevel(int size, int level) {
    if (size) {
        size >>= level;
        if (!size) size = 1;
    }
    return size;
}

// ============================================================================
// Texture target enum conversion
// ============================================================================

// Lookup table: TextureTarget → GLenum (dense enum 0..23)
static const GLenum kTextureTargetToGLEnum[] = {
    GL_TEXTURE_1D,                   // TEXTURE_1D = 0
    GL_PROXY_TEXTURE_1D,             // PROXY_TEXTURE_1D = 1
    GL_TEXTURE_1D_ARRAY,             // TEXTURE_1D_ARRAY = 2
    GL_PROXY_TEXTURE_1D_ARRAY,       // PROXY_TEXTURE_1D_ARRAY = 3
    GL_TEXTURE_2D,                   // TEXTURE_2D = 4
    GL_PROXY_TEXTURE_2D,             // PROXY_TEXTURE_2D = 5
    GL_TEXTURE_2D_ARRAY,             // TEXTURE_2D_ARRAY = 6
    GL_PROXY_TEXTURE_2D_ARRAY,       // PROXY_TEXTURE_2D_ARRAY = 7
    GL_TEXTURE_2D_MULTISAMPLE,       // TEXTURE_2D_MULTISAMPLE = 8
    GL_PROXY_TEXTURE_2D_MULTISAMPLE, // PROXY_TEXTURE_2D_MULTISAMPLE = 9
    GL_TEXTURE_2D_MULTISAMPLE_ARRAY,         // TEXTURE_2D_MULTISAMPLE_ARRAY = 10
    GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY,   // PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY = 11
    GL_TEXTURE_3D,                   // TEXTURE_3D = 12
    GL_PROXY_TEXTURE_3D,             // PROXY_TEXTURE_3D = 13
    GL_TEXTURE_RECTANGLE,            // TEXTURE_RECTANGLE = 14
    GL_PROXY_TEXTURE_RECTANGLE,      // PROXY_TEXTURE_RECTANGLE = 15
    GL_TEXTURE_CUBE_MAP,             // TEXTURE_CUBE_MAP = 16
    GL_PROXY_TEXTURE_CUBE_MAP,       // PROXY_TEXTURE_CUBE_MAP = 17
    GL_TEXTURE_CUBE_MAP_ARRAY,       // TEXTURE_CUBE_MAP_ARRAY = 18
    GL_PROXY_TEXTURE_CUBE_MAP_ARRAY, // PROXY_TEXTURE_CUBE_MAP_ARRAY = 19
    GL_TEXTURE_BUFFER,               // TEXTURE_BUFFER = 20
};

GLenum ConvertTextureTargetToGLEnum(TextureTarget target) {
    auto idx = static_cast<unsigned>(target);
    if (idx < sizeof(kTextureTargetToGLEnum) / sizeof(kTextureTargetToGLEnum[0])) [[likely]] {
        return kTextureTargetToGLEnum[idx];
    }
    return GL_TEXTURE_2D;
}

// Sorted key-value pair for binary search lookup (GLenum → TextureTarget)
struct TexTargetEntry {
    GLenum key;
    TextureTarget value;
};

static const TexTargetEntry kGLEnumToTextureTarget[] = {
    {GL_TEXTURE_1D, TextureTarget::TEXTURE_1D},
    {GL_TEXTURE_2D, TextureTarget::TEXTURE_2D},
    {GL_PROXY_TEXTURE_1D, TextureTarget::PROXY_TEXTURE_1D},
    {GL_PROXY_TEXTURE_2D, TextureTarget::PROXY_TEXTURE_2D},
    {GL_TEXTURE_3D, TextureTarget::TEXTURE_3D},
    {GL_PROXY_TEXTURE_3D, TextureTarget::PROXY_TEXTURE_3D},
    {GL_TEXTURE_RECTANGLE, TextureTarget::TEXTURE_RECTANGLE},
    {GL_PROXY_TEXTURE_RECTANGLE, TextureTarget::PROXY_TEXTURE_RECTANGLE},
    {GL_TEXTURE_CUBE_MAP, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_X, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Y, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, TextureTarget::TEXTURE_CUBE_MAP},
    {GL_PROXY_TEXTURE_CUBE_MAP, TextureTarget::PROXY_TEXTURE_CUBE_MAP},
    {GL_TEXTURE_1D_ARRAY, TextureTarget::TEXTURE_1D_ARRAY},
    {GL_PROXY_TEXTURE_1D_ARRAY, TextureTarget::PROXY_TEXTURE_1D_ARRAY},
    {GL_TEXTURE_2D_ARRAY, TextureTarget::TEXTURE_2D_ARRAY},
    {GL_PROXY_TEXTURE_2D_ARRAY, TextureTarget::PROXY_TEXTURE_2D_ARRAY},
    {GL_TEXTURE_BUFFER, TextureTarget::TEXTURE_BUFFER},
    {GL_TEXTURE_CUBE_MAP_ARRAY, TextureTarget::TEXTURE_CUBE_MAP_ARRAY},
    {GL_PROXY_TEXTURE_CUBE_MAP_ARRAY, TextureTarget::PROXY_TEXTURE_CUBE_MAP_ARRAY},
    {GL_TEXTURE_2D_MULTISAMPLE, TextureTarget::TEXTURE_2D_MULTISAMPLE},
    {GL_PROXY_TEXTURE_2D_MULTISAMPLE, TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE},
    {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY},
    {GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY, TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY},
};

static constexpr size_t kTexTargetEntryCount = sizeof(kGLEnumToTextureTarget) / sizeof(kGLEnumToTextureTarget[0]);

TextureTarget ConvertGLEnumToTextureTarget(GLenum target) {
    // Switch-based direct lookup — faster than binary search for this hot path
    switch (target) {
    case GL_TEXTURE_1D:                   return TextureTarget::TEXTURE_1D;
    case GL_PROXY_TEXTURE_1D:             return TextureTarget::PROXY_TEXTURE_1D;
    case GL_TEXTURE_2D:                   return TextureTarget::TEXTURE_2D;
    case GL_PROXY_TEXTURE_2D:             return TextureTarget::PROXY_TEXTURE_2D;
    case GL_TEXTURE_3D:                   return TextureTarget::TEXTURE_3D;
    case GL_PROXY_TEXTURE_3D:             return TextureTarget::PROXY_TEXTURE_3D;
    case GL_TEXTURE_RECTANGLE:            return TextureTarget::TEXTURE_RECTANGLE;
    case GL_PROXY_TEXTURE_RECTANGLE:      return TextureTarget::PROXY_TEXTURE_RECTANGLE;
    case GL_TEXTURE_CUBE_MAP:             return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:  return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_PROXY_TEXTURE_CUBE_MAP:       return TextureTarget::PROXY_TEXTURE_CUBE_MAP;
    case GL_TEXTURE_1D_ARRAY:             return TextureTarget::TEXTURE_1D_ARRAY;
    case GL_PROXY_TEXTURE_1D_ARRAY:       return TextureTarget::PROXY_TEXTURE_1D_ARRAY;
    case GL_TEXTURE_2D_ARRAY:             return TextureTarget::TEXTURE_2D_ARRAY;
    case GL_PROXY_TEXTURE_2D_ARRAY:       return TextureTarget::PROXY_TEXTURE_2D_ARRAY;
    case GL_TEXTURE_BUFFER:               return TextureTarget::TEXTURE_BUFFER;
    case GL_TEXTURE_CUBE_MAP_ARRAY:       return TextureTarget::TEXTURE_CUBE_MAP_ARRAY;
    case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY: return TextureTarget::PROXY_TEXTURE_CUBE_MAP_ARRAY;
    case GL_TEXTURE_2D_MULTISAMPLE:       return TextureTarget::TEXTURE_2D_MULTISAMPLE;
    case GL_PROXY_TEXTURE_2D_MULTISAMPLE: return TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:         return TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY;
    case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:   return TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY;
    default:                              return TextureTarget::UNKNWON;
    }
}

// ============================================================================
// Texture map management (tracking bound texture objects)
// ============================================================================

const int MAX_TEXTURE_IMAGE_UNITS = 32;

class TextureBindingSlot {
public:
    using TargetEnum = TextureTarget;

    TextureBindingSlot() : m_target((TargetEnum)0), m_boundObject(nullptr) {}

    explicit TextureBindingSlot(TargetEnum target) : m_target(target), m_boundObject(nullptr) {}

    void Bind(TextureObject* object) {
        // Track reverse mapping: remove old binding, add new binding
        if (m_boundObject && m_boundObject != object) {
            m_boundObject->binding_slots.erase(
                reinterpret_cast<uintptr_t>(this));
        }
        m_boundObject = object;
        if (object) {
            object->binding_slots.insert(reinterpret_cast<uintptr_t>(this));
        }
    }

    TextureObject* GetBoundObject() const { return m_boundObject; }

    TargetEnum GetTarget() const { return m_target; }

private:
    TargetEnum m_target;
    TextureObject* m_boundObject;
};

class TextureUnit {
public:
    TextureBindingSlot& GetBindingSlot(TextureBindingSlot::TargetEnum target) { return m_slots[(int)target]; }

private:
    std::array<TextureBindingSlot, (int)TextureTarget::TEXTURES_COUNT> m_slots;
};

static std::vector<TextureObject*> BufferObjectsVec;
static std::array<TextureUnit, MAX_TEXTURE_IMAGE_UNITS> TextureUnits;
static int CurrentTextureUnitIndex = 0;

void InitTextureMap(size_t expectedSize) {
    BufferObjectsVec.reserve(expectedSize);
}

TextureObject* GetOrCreateTextureObject(GLuint index) {
    if (index >= BufferObjectsVec.size()) {
        BufferObjectsVec.resize(index + 1, nullptr);
    }

    auto& obj = BufferObjectsVec[index];
    if (!obj) {
        obj = new TextureObject();
        obj->texture = index;
    }
    return obj;
}

void ActivateTextureUnit(int unit) {
    if (unit < 0 || unit >= MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture unit: %d", unit);
        return;
    }
    CurrentTextureUnitIndex = unit;
}

int GetCurrentTextureUnitIndex() {
    return CurrentTextureUnitIndex;
}

TextureUnit& GetTextureUnit(int unit) {
    if (unit < 0 || unit >= MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture unit: %d", unit);
        return TextureUnits[0];
    }
    return TextureUnits[unit];
}

void MarkTextureObjectForDeletion(unsigned texture) {
    if (texture >= BufferObjectsVec.size() || !BufferObjectsVec[texture]) {
        LOG_D("Texture %u not found in BufferObjectsVec!", texture);
        return;
    }

    auto textureObject = BufferObjectsVec[texture];

    // Use reverse mapping to find and clear only the slots that reference this object
    for (auto slotPtr : textureObject->binding_slots) {
        auto* slot = reinterpret_cast<TextureBindingSlot*>(slotPtr);
        slot->Bind(nullptr);
    }
    textureObject->binding_slots.clear();

    BufferObjectsVec[texture] = nullptr;
    delete textureObject;
}

TextureObject* mgGetTexObjectByTarget(GLenum target) {
    return GetTextureUnit(GetCurrentTextureUnitIndex())
        .GetBindingSlot(ConvertGLEnumToTextureTarget(target))
        .GetBoundObject();
}

TextureObject* mgGetTexObjectByID(unsigned texture) {
    if (texture >= BufferObjectsVec.size() || !BufferObjectsVec[texture]) {
        LOG_E("Texture %u not found in BufferObjectsVec!", texture);
        return nullptr;
    }
    return BufferObjectsVec[texture];
}

// ============================================================================
// Internal format conversion helper
// ============================================================================

// ============================================================================
// Lookup table for GL_RED type→internalformat mapping.
// Sorted by GLenum type for fast binary search.
// ============================================================================
struct RedTypeMapping {
    GLenum type;
    GLenum internalformat;
    GLenum format;
};
static const RedTypeMapping kRedTypeMappings[] = {
    {GL_UNSIGNED_BYTE, GL_R8,        GL_RED},
    {GL_BYTE,          GL_R8_SNORM,  GL_RED},
    {GL_HALF_FLOAT,    GL_R16F,      GL_RED},
    {GL_FLOAT,         GL_R32F,      GL_RED},
};
static constexpr size_t kRedTypeMappingCount = sizeof(kRedTypeMappings) / sizeof(kRedTypeMappings[0]);

// ============================================================================
// Inline mapping for various internal formats to format and type
// Most common formats (RGBA8, RGBA, RGBA16F, R8, RGBA32F) are handled first
// with early return for maximum performance on the hot path.
// ============================================================================
void internal_convert(GLenum* internal_format, GLenum* type, GLenum* format) {
    if (format && *format == GL_BGRA) *format = GL_RGBA;

    // Fast path: most common formats first
    switch (*internal_format) {
    // --- Most common formats (hot path) ---
    case GL_RGBA8:
    case GL_RGBA:
        if (type) *type = GL_UNSIGNED_BYTE;
        if (format) *format = GL_RGBA;
        return;
    case GL_RGBA16F:
        if (type) *type = GL_HALF_FLOAT;
        return;
    case GL_R8:
        if (format) *format = GL_RED;
        if (type) *type = GL_UNSIGNED_BYTE;
        return;
    case GL_RGBA32F:
    case GL_RGB32F:
        if (type) *type = GL_FLOAT;
        return;

    // --- GL_RED: lookup-table dispatch (replaces nested switch) ---
    case GL_RED:
        if (type) {
            GLenum t = *type;
            // Linear search is faster than binary search for 4 entries
            for (size_t i = 0; i < kRedTypeMappingCount; ++i) {
                if (kRedTypeMappings[i].type == t) {
                    *internal_format = kRedTypeMappings[i].internalformat;
                    if (format) *format = kRedTypeMappings[i].format;
                    return;
                }
            }
            LOG_E("Unsupported type for GL_RED: %s", glEnumToString(*type));
            if (type) *type = GL_UNSIGNED_BYTE;
            *internal_format = GL_R8;
            if (format) *format = GL_RED;
        }
        return;

    // --- Less common formats ---
    case GL_DEPTH_COMPONENT16:
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_DEPTH_COMPONENT24:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32:
        *internal_format = GL_DEPTH_COMPONENT;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_DEPTH_COMPONENT:
        LOG_D("Find GL_DEPTH_COMPONENT: internalFormat: %s, format: %s, type: %s", glEnumToString(*internal_format),
              glEnumToString(*format), glEnumToString(*type));
        if (type) {
            *internal_format = GL_DEPTH_COMPONENT;
            *type = GL_UNSIGNED_INT;
        }
        break;
    case GL_DEPTH_STENCIL:
        *internal_format = GL_DEPTH32F_STENCIL8;
        if (type) *type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        break;
    case GL_RGB10_A2:
        if (type) *type = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    case GL_RGB5_A1:
        if (type) *type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case GL_COMPRESSED_RED_RGTC1:
    case GL_COMPRESSED_RG_RGTC2:
        LOG_E("GL_COMPRESSED_RED_RGTC1 or GL_COMPRESSED_RG_RGTC2 is not supported!");
        break;
    case GL_SRGB8:
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RGB9_E5:
        if (type) *type = GL_UNSIGNED_INT_5_9_9_9_REV;
        break;
    case GL_R11F_G11F_B10F:
        if (type) *type = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (format) *format = GL_RGB;
        break;
    case GL_RGBA32UI:
    case GL_RGB32UI:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RGBA32I:
    case GL_RGB32I:
        if (type) *type = GL_INT;
        break;
    case GL_RGBA16: {
        if (g_gles_caps.GL_EXT_texture_norm16) {
            if (type) *type = GL_UNSIGNED_SHORT;
        } else {
            *internal_format = GL_RGBA16F;
            if (type) *type = GL_FLOAT;
        }
        break;
    }
    case GL_R16:
        *internal_format = GL_R16F;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB16:
        *internal_format = GL_RGB16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RGB16F:
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RG16:
        *internal_format = GL_RG16F;
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RG;
        break;
    case GL_R8_SNORM:
        if (format) *format = GL_RED;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16F:
        if (format) *format = GL_RED;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_R8UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_R16I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_R32UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_R32I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_RG8:
        if (format) *format = GL_RG;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8_SNORM:
        if (format) *format = GL_RG;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16F:
        if (format) *format = GL_RG;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RG32F:
        if (format) *format = GL_RG;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RG8UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_RG16I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_RG32UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RG32I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_R32F:
        if (format) *format = GL_RED;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGBA8_SNORM:
        if (format) *format = GL_RGBA;
        if (type) *type = GL_BYTE;
        break;
    default:
        // fallback handling for GL_RGB8, GL_RGBA16_SNORM etc.
        if (*internal_format == GL_RGB8) {
            if (type && *type != GL_UNSIGNED_BYTE) *type = GL_UNSIGNED_BYTE;
            if (format) *format = GL_RGB;
        } else if (*internal_format == GL_RGBA16_SNORM) {
            if (type && *type != GL_SHORT) *type = GL_SHORT;
        }
        break;
    }
}

// ============================================================================
// Internal helpers: depth format detection, binding query
// ============================================================================

static int is_depth_format(GLenum format) {
    switch (format) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
        return 1;
    default:
        return 0;
    }
}

static GLenum get_binding_for_target(GLenum target) {
    switch (target) {
    case GL_TEXTURE_2D:
        return GL_TEXTURE_BINDING_2D;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return GL_TEXTURE_BINDING_CUBE_MAP;
    default:
        return 0;
    }
}

// ============================================================================
// Convenience macro: get texture object from bound target
// ============================================================================

#define GET_TEXTURE_OBJECT(target)                                                                                     \
    unsigned __currentUnitIndex = GetCurrentTextureUnitIndex();                                                        \
    auto& __currentUnit = GetTextureUnit(__currentUnitIndex);                                                          \
    auto targetR = ConvertGLEnumToTextureTarget(target);                                                               \
    if (targetR == TextureTarget::UNKNWON) {                                                                           \
        LOG_E("%s: Unknown texture target: %s", __func__, glEnumToString(target))                                      \
        return;                                                                                                        \
    }                                                                                                                  \
    auto& __bindingSlot = __currentUnit.GetBindingSlot(targetR);                                                       \
    auto tex = __bindingSlot.GetBoundObject()

// ============================================================================
// glBindTexture: _Backend, _State, Public
// ============================================================================

void BindTexture_Backend(GLenum target, GLuint texture) {
    GLES.glBindTexture(target, texture);
}

void BindTexture_State(GLenum target, GLuint texture) {
    LOG()
    LOG_D("glBindTexture(%s, %d)", glEnumToString(target), texture)
    INIT_CHECK_GL_ERROR

    const int currentUnitIndex = GetCurrentTextureUnitIndex();

    BindTexture_Backend(target, texture);
    CHECK_GL_ERROR_NO_INIT

    // Track GL_TEXTURE_2D binding per-unit to avoid glGetIntegerv GPU queries
    if (target == GL_TEXTURE_2D) {
        g_tracked_tex2d_binding[currentUnitIndex] = texture;
    }

    // Update GLState.texture binding tracking
    if (currentUnitIndex < MAX_TEXTURE_UNITS) {
        GLState.texture.texUnits[currentUnitIndex].binding[target] = texture;
    }

    auto& currentUnit = GetTextureUnit(currentUnitIndex);
    auto targetR = ConvertGLEnumToTextureTarget(target);
    if (targetR == TextureTarget::UNKNWON) {
        LOG_E("glBindTexture: Unknown texture target: %s", glEnumToString(target));
        return;
    }
    auto& bindingSlot = currentUnit.GetBindingSlot(targetR);

    // Unbind: skip TextureObject creation for texture 0
    if (texture == 0) [[unlikely]] {
        bindingSlot.Bind(nullptr);
        return;
    }

    auto textureObject = GetOrCreateTextureObject(texture);
    if (!textureObject) {
        LOG_W("glBindTexture: Failed to get or create texture object for ID %d, it may be not tracked", texture);
        return;
    }
    bindingSlot.Bind(textureObject);
    textureObject->target = targetR;
}

GLAPI GLAPIENTRY void glBindTexture(GLenum target, GLuint texture) {
    GLState.Lock();
    BindTexture_State(target, texture);
    GLState.Unlock();
}

// ============================================================================
// glDeleteTextures: _Backend, _State, Public
// ============================================================================

void DeleteTextures_Backend(GLsizei n, const GLuint* textures) {
    GLES.glDeleteTextures(n, textures);
}

void DeleteTextures_State(GLsizei n, const GLuint* textures) {
    LOG()
    INIT_CHECK_GL_ERROR
    DeleteTextures_Backend(n, textures);
    CHECK_GL_ERROR_NO_INIT

    for (GLsizei i = 0; i < n; ++i) {
        // Update GLState.texture ID mapping
        GLState.texture.textureMap.erase(textures[i]);
        // Also remove from reverse map
        for (auto it = GLState.texture.textureMapReverse.begin(); it != GLState.texture.textureMapReverse.end(); ++it) {
            if (it->second == textures[i]) {
                GLState.texture.textureMapReverse.erase(it);
                break;
            }
        }
        // Clear per-unit bindings
        for (int unit = 0; unit < MAX_TEXTURE_UNITS; ++unit) {
            for (auto& kv : GLState.texture.texUnits[unit].binding) {
                if (kv.second == textures[i]) {
                    kv.second = 0;
                }
            }
        }

        MarkTextureObjectForDeletion(textures[i]);
        // Invalidate CPU-side texture binding tracking
        for (int unit = 0; unit < MAX_TEXTURE_IMAGE_UNITS; ++unit) {
            if (g_tracked_tex2d_binding[unit] == textures[i]) {
                g_tracked_tex2d_binding[unit] = 0;
            }
        }
    }
}

GLAPI GLAPIENTRY void glDeleteTextures(GLsizei n, const GLuint* textures) {
    GLState.Lock();
    DeleteTextures_State(n, textures);
    GLState.Unlock();
}

// ============================================================================
// glActiveTexture: _Backend, _State, Public
// ============================================================================

void ActiveTexture_Backend(GLenum texture) {
    GLES.glActiveTexture(texture);
}

void ActiveTexture_State(GLenum texture) {
    LOG()
    LOG_D("glActiveTexture, texture = %s", glEnumToString(texture))
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture enum: %s", glEnumToString(texture))
        return;
    }

    set_gl_state_current_tex_unit(texture - GL_TEXTURE0);
    GLState.texture.activeUnit = texture - GL_TEXTURE0;
    ActiveTexture_Backend(texture);
    ActivateTextureUnit(texture - GL_TEXTURE0);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glActiveTexture(GLenum texture) {
    GLState.Lock();
    ActiveTexture_State(texture);
    GLState.Unlock();
}

// ============================================================================
// Native texture image specification (ES 3.2)
// ============================================================================

// --- glTexImage2D: _Backend, _State, Public ---

void TexImage2D_Backend(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                        GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    GLES.glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

void TexImage2D_State(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                      GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    GLenum transfer_format = format;

    LOG_D("mg_glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat), glEnumToString(internalFormat), width, height,
          border, glEnumToString(format), glEnumToString(type), pixels)
    GLenum internalFormat_mut = (GLenum)internalFormat;
    internal_convert(&internalFormat_mut, &type, &format);
    internalFormat = (GLint)internalFormat_mut;

    LOG_D("GLES.glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat), glEnumToString(internalFormat), width, height,
          border, glEnumToString(format), glEnumToString(type), pixels)
    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_2D) {
        static int maxTexSize = -1;
        if (maxTexSize < 0) GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
        set_gl_state_proxy_width(((width << level) > maxTexSize) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > maxTexSize) ? 0 : height);
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

    if (transfer_format == GL_BGRA && tex->format != transfer_format && internalFormat == GL_RGBA8 && width <= 128 &&
        height <= 128) { // xaero has 64x64 tiles...hack here
        LOG_D("Detected GL_BGRA format @ tex = %d, do swizzle", tex->texture)
        if (tex->swizzle_param[0] == 0) { // assert this as never called glTexParameteri(...,
                                          // GL_TEXTURE_SWIZZLE_R, ...)
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

    TexImage2D_Backend(target, level, internalFormat, width, height, border, format, type, pixels);

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    GLState.Lock();
    TexImage2D_State(target, level, internalFormat, width, height, border, format, type, pixels);
    GLState.Unlock();
}

// --- glTexImage3D: _Backend, _State, Public ---

void TexImage3D_Backend(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                        GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    GLES.glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

void TexImage3D_State(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                      GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    LOG_D("glTexImage3D, target: 0x%x, level: %d, internalFormat: 0x%x, width: "
          "0x%x, height: %d, depth: %d, border: %d, format: 0x%x, type: %d",
          target, level, internalFormat, width, height, depth, border, format, type)

    GLenum internalFormat_mut = (GLenum)internalFormat;
    internal_convert(&internalFormat_mut, &type, &format);
    internalFormat = (GLint)internalFormat_mut;
    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_3D) {
        static int maxTexSize3D = -1;
        if (maxTexSize3D < 0) GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize3D);
        set_gl_state_proxy_width(((width << level) > maxTexSize3D) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > maxTexSize3D) ? 0 : height);
        set_gl_state_proxy_intformat(internalFormat);
        return;
    }

    TexImage3D_Backend(target, level, internalFormat, width, height, depth, border, format, type, pixels);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                   GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    GLState.Lock();
    TexImage3D_State(target, level, internalFormat, width, height, depth, border, format, type, pixels);
    GLState.Unlock();
}

// --- glTexSubImage2D: _Backend, _State, Public ---

void TexSubImage2D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                           GLenum format, GLenum type, const void* pixels) {
    GLES.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void TexSubImage2D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, const void* pixels) {
    LOG()

    LOG_D("glTexSubImage2D, target = %s, level = %d, xoffset = %d, yoffset = %d, "
          "width = %d, height = %d, format = %s, type = %s, pixels = 0x%x",
          glEnumToString(target), level, xoffset, yoffset, width, height, glEnumToString(format), glEnumToString(type),
          pixels)

    if (format == GL_BGRA && (type == GL_UNSIGNED_INT_8_8_8_8 || type == GL_UNSIGNED_INT_8_8_8_8_REV)) {
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_R,  GL_BLUE);
        glTexParameteri(target, GL_TEXTURE_SWIZZLE_B,  GL_RED);

        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
    }

    TexSubImage2D_Backend(target, level, xoffset, yoffset, width, height, format, type, pixels);

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                      GLsizei height, GLenum format, GLenum type, const void* pixels) {
    GLState.Lock();
    TexSubImage2D_State(target, level, xoffset, yoffset, width, height, format, type, pixels);
    GLState.Unlock();
}

// --- glTexSubImage3D: _Backend, _State, Public ---

void TexSubImage3D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
                           GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    GLES.glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void TexSubImage3D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
                         GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    LOG()
    LOG_D("glTexSubImage3D, target: %s, level: %d, xoffset: %d, yoffset: %d, zoffset: %d, "
          "width: %d, height: %d, depth: %d, format: %s, type: %s",
          glEnumToString(target), level, xoffset, yoffset, zoffset, width, height, depth, glEnumToString(format),
          glEnumToString(type))

    TexSubImage3D_Backend(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                      GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                      const void* pixels) {
    GLState.Lock();
    TexSubImage3D_State(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    GLState.Unlock();
}

// --- glTexStorage2D: _Backend, _State, Public ---

void TexStorage2D_Backend(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLES.glTexStorage2D(target, levels, internalFormat, width, height);
}

void TexStorage2D_State(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG()
    LOG_D("glTexStorage2D, target: %d, levels: %d, internalFormat: %d, width: "
          "%d, height: %d",
          target, levels, internalFormat, width, height)

    internal_convert(&internalFormat, nullptr, nullptr);
    TexStorage2D_Backend(target, levels, internalFormat, width, height);

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

    GLenum ERR = GLES.glGetError();
    if (ERR != GL_NO_ERROR) LOG_E("glTexStorage2D ERROR: %d", ERR)
}

GLAPI GLAPIENTRY void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width,
                                     GLsizei height) {
    GLState.Lock();
    TexStorage2D_State(target, levels, internalFormat, width, height);
    GLState.Unlock();
}

// --- glTexStorage3D: _Backend, _State, Public ---

void TexStorage3D_Backend(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height,
                          GLsizei depth) {
    GLES.glTexStorage3D(target, levels, internalFormat, width, height, depth);
}

void TexStorage3D_State(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height,
                        GLsizei depth) {
    LOG()
    LOG_D("glTexStorage3D, target: %d, levels: %d, internalFormat: %d, width: "
          "%d, height: %d, depth: %d",
          target, levels, internalFormat, width, height, depth)

    internal_convert(&internalFormat, nullptr, nullptr);

    TexStorage3D_Backend(target, levels, internalFormat, width, height, depth);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width,
                                     GLsizei height, GLsizei depth) {
    GLState.Lock();
    TexStorage3D_State(target, levels, internalFormat, width, height, depth);
    GLState.Unlock();
}

// ============================================================================
// Native compressed texture functions (ES 3.2)
// ============================================================================

// --- glCompressedTexImage2D: _Backend, _State, Public ---

void CompressedTexImage2D_Backend(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                  GLint border, GLsizei imageSize, const void* data) {
    GLES.glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

void CompressedTexImage2D_State(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                GLint border, GLsizei imageSize, const void* data) {
    LOG()
    LOG_D("glCompressedTexImage2D, target: %s, level: %d, internalformat: %s, width: %d, height: %d, "
          "border: %d, imageSize: %d",
          glEnumToString(target), level, glEnumToString(internalformat), width, height, border, imageSize)

    CompressedTexImage2D_Backend(target, level, internalformat, width, height, border, imageSize, data);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalformat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                             GLsizei height, GLint border, GLsizei imageSize, const void* data) {
    GLState.Lock();
    CompressedTexImage2D_State(target, level, internalformat, width, height, border, imageSize, data);
    GLState.Unlock();
}

// --- glCompressedTexImage3D: _Backend, _State, Public ---

void CompressedTexImage3D_Backend(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                  GLsizei depth, GLint border, GLsizei imageSize, const void* data) {
    GLES.glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}

void CompressedTexImage3D_State(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                GLsizei depth, GLint border, GLsizei imageSize, const void* data) {
    LOG()
    LOG_D("glCompressedTexImage3D, target: %s, level: %d, internalformat: %s, width: %d, height: %d, "
          "depth: %d, border: %d, imageSize: %d",
          glEnumToString(target), level, glEnumToString(internalformat), width, height, depth, border, imageSize)

    CompressedTexImage3D_Backend(target, level, internalformat, width, height, depth, border, imageSize, data);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalformat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                             GLsizei height, GLsizei depth, GLint border, GLsizei imageSize,
                                             const void* data) {
    GLState.Lock();
    CompressedTexImage3D_State(target, level, internalformat, width, height, depth, border, imageSize, data);
    GLState.Unlock();
}

// --- glCompressedTexSubImage2D: _Backend, _State, Public ---

void CompressedTexSubImage2D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                     GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
    GLES.glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void CompressedTexSubImage2D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                   GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
    LOG()
    LOG_D("glCompressedTexSubImage2D, target: %s, level: %d, xoffset: %d, yoffset: %d, "
          "width: %d, height: %d, format: %s, imageSize: %d",
          glEnumToString(target), level, xoffset, yoffset, width, height, glEnumToString(format), imageSize)

    CompressedTexSubImage2D_Backend(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                                GLsizei width, GLsizei height, GLenum format, GLsizei imageSize,
                                                const void* data) {
    GLState.Lock();
    CompressedTexSubImage2D_State(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    GLState.Unlock();
}

// --- glCompressedTexSubImage3D: _Backend, _State, Public ---

void CompressedTexSubImage3D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                     GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize,
                                     const void* data) {
    GLES.glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize,
                                   data);
}

void CompressedTexSubImage3D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize,
                                   const void* data) {
    LOG()
    LOG_D("glCompressedTexSubImage3D, target: %s, level: %d, xoffset: %d, yoffset: %d, zoffset: %d, "
          "width: %d, height: %d, depth: %d, format: %s, imageSize: %d",
          glEnumToString(target), level, xoffset, yoffset, zoffset, width, height, depth, glEnumToString(format),
          imageSize)

    CompressedTexSubImage3D_Backend(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize,
                                    data);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                                GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                                GLenum format, GLsizei imageSize, const void* data) {
    GLState.Lock();
    CompressedTexSubImage3D_State(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize,
                                  data);
    GLState.Unlock();
}

// ============================================================================
// Native copy texture functions (ES 3.2)
// ============================================================================

// --- glCopyTexImage2D: _Backend, _State, Public ---

void CopyTexImage2D_Backend(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width,
                            GLsizei height, GLint border) {
    GLES.glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}

void CopyTexImage2D_State(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width,
                          GLsizei height, GLint border) {
    LOG()

    INIT_CHECK_GL_ERROR

    GLint realInternalFormat;
    GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &realInternalFormat);
    internalFormat = (GLenum)realInternalFormat;

    LOG_D("glCopyTexImage2D, target: %d, level: %d, internalFormat: %d, x: %d, "
          "y: %d, width: %d, height: %d, border: %d",
          target, level, internalFormat, x, y, width, height, border)

    if (is_depth_format(internalFormat)) {
        GLenum format = GL_DEPTH_COMPONENT;
        GLenum type = GL_UNSIGNED_INT;
        internal_convert(&internalFormat, &type, &format);
        GLES.glTexImage2D(target, level, (GLint)internalFormat, width, height, border, format, type, nullptr);
        CHECK_GL_ERROR_NO_INIT
        // Use CPU-side cached FBO bindings instead of glGetIntegerv GPU round-trips
        GLint prevDrawFBO = GLState.framebuffer.drawFBO;
        CHECK_GL_ERROR_NO_INIT
        GLuint tempDrawFBO = acquireTempFBO();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        // Use CPU-side tracked texture binding instead of glGetIntegerv
        GLint currentTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
        CHECK_GL_ERROR_NO_INIT
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);
        CHECK_GL_ERROR_NO_INIT

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            CHECK_GL_ERROR_NO_INIT
            releaseTempFBO(tempDrawFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            CHECK_GL_ERROR_NO_INIT
            return;
        }
        CHECK_GL_ERROR_NO_INIT

        GLES.glBlitFramebuffer(x, y, x + width, y + height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        CHECK_GL_ERROR_NO_INIT

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
    } else {
        CopyTexImage2D_Backend(target, level, internalFormat, x, y, width, height, border);
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

GLAPI GLAPIENTRY void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y,
                                       GLsizei width, GLsizei height, GLint border) {
    GLState.Lock();
    CopyTexImage2D_State(target, level, internalFormat, x, y, width, height, border);
    GLState.Unlock();
}

// --- glCopyTexSubImage2D: _Backend, _State, Public ---

void CopyTexSubImage2D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                               GLsizei width, GLsizei height) {
    GLES.glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void CopyTexSubImage2D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                             GLsizei width, GLsizei height) {
    LOG()
    GLint internalFormat;
    GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

    LOG_D("glCopyTexSubImage2D, target: %s, level: %d, xoffset: %d, yoffset: %d, "
          "x: %d, y: %d, width: %d, height: %d",
          glEnumToString(target), level, xoffset, yoffset, x, y, width, height)

    if (is_depth_format((GLenum)internalFormat)) {
        // Use CPU-side cached FBO bindings instead of glGetIntegerv GPU round-trips
        GLint prevReadFBO = GLState.framebuffer.readFBO;
        GLint prevDrawFBO = GLState.framebuffer.drawFBO;

        GLuint tempDrawFBO = acquireTempFBO();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);

        GLint currentTex;
        // Use CPU-side tracked texture binding for 2D (common case),
        // fall back to glGetIntegerv for cubemap targets
        if (target == GL_TEXTURE_2D) {
            currentTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
        } else {
            glGetIntegerv(get_binding_for_target(target), &currentTex);
        }
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
            releaseTempFBO(tempDrawFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            return;
        }

        GLES.glBlitFramebuffer(x, y, x + width, y + height, xoffset, yoffset, xoffset + width, yoffset + height,
                               GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempDrawFBO);
    } else {
        CopyTexSubImage2D_Backend(target, level, xoffset, yoffset, x, y, width, height);
    }

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                                          GLsizei width, GLsizei height) {
    GLState.Lock();
    CopyTexSubImage2D_State(target, level, xoffset, yoffset, x, y, width, height);
    GLState.Unlock();
}

// --- glCopyTexSubImage3D: _Backend, _State, Public ---

void CopyTexSubImage3D_Backend(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x,
                               GLint y, GLsizei width, GLsizei height) {
    GLES.glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void CopyTexSubImage3D_State(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y,
                             GLsizei width, GLsizei height) {
    LOG()
    LOG_D("glCopyTexSubImage3D, target: %s, level: %d, xoffset: %d, yoffset: %d, zoffset: %d, "
          "x: %d, y: %d, width: %d, height: %d",
          glEnumToString(target), level, xoffset, yoffset, zoffset, x, y, width, height)

    CopyTexSubImage3D_Backend(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                          GLint x, GLint y, GLsizei width, GLsizei height) {
    GLState.Lock();
    CopyTexSubImage3D_State(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    GLState.Unlock();
}

// ============================================================================
// Native mipmap generation (ES 3.2)
// ============================================================================

// --- glGenerateMipmap: _Backend, _State, Public ---

void GenerateMipmap_Backend(GLenum target) {
    GLES.glGenerateMipmap(target);
}

void GenerateMipmap_State(GLenum target) {
    LOG()
    LOG_D("glGenerateMipmap, target: %s", glEnumToString(target))
    GenerateMipmap_Backend(target);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGenerateMipmap(GLenum target) {
    GLState.Lock();
    GenerateMipmap_State(target);
    GLState.Unlock();
}

// ============================================================================
// Native texture parameter functions (ES 3.2)
// ============================================================================

// --- glTexParameterf: _Backend, _State, Public ---

void TexParameterf_Backend(GLenum target, GLenum pname, GLfloat param) {
    GLES.glTexParameterf(target, pname, param);
}

void TexParameterf_State(GLenum target, GLenum pname, GLfloat param) {
    LOG()
    pname = pname_convert(pname);
    LOG_D("glTexParameterf, target: %d, pname: %d, param: %f", target, pname, param)

    if (pname == GL_TEXTURE_LOD_BIAS_QCOM && !g_gles_caps.GL_QCOM_texture_lod_bias) {
        LOG_D("Does not support GL_QCOM_texture_lod_bias, skipped!")
        return;
    }

    TexParameterf_Backend(target, pname, param);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    GLState.Lock();
    TexParameterf_State(target, pname, param);
    GLState.Unlock();
}

// --- glTexParameteri: _Backend, _State, Public ---

void TexParameteri_Backend(GLenum target, GLenum pname, GLint param) {
    GLES.glTexParameteri(target, pname, param);
}

void TexParameteri_State(GLenum target, GLenum pname, GLint param) {
    LOG()
    pname = pname_convert(pname);
    LOG_D("glTexParameteri, pname: 0x%x", pname)

    if (pname == GL_TEXTURE_LOD_BIAS_QCOM && !g_gles_caps.GL_QCOM_texture_lod_bias) {
        LOG_D("Does not support GL_QCOM_texture_lod_bias, skipped!")
        return;
    }

    TexParameteri_Backend(target, pname, param);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GLState.Lock();
    TexParameteri_State(target, pname, param);
    GLState.Unlock();
}

// --- glTexParameteriv: _Backend, _State, Public ---

void TexParameteriv_Backend(GLenum target, GLenum pname, const GLint* params) {
    GLES.glTexParameteriv(target, pname, params);
}

void TexParameteriv_State(GLenum target, GLenum pname, const GLint* params) {
    LOG()
    LOG_D("glTexParameteriv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))

    if (pname == GL_TEXTURE_SWIZZLE_RGBA) {
        LOG_D("find GL_TEXTURE_SWIZZLE_RGBA, now use glTexParameteri")
        if (params) {
            // deferred those call to draw call?
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, params[0]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, params[1]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, params[2]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, params[3]);

            // save states for now
            GET_TEXTURE_OBJECT(target);
            tex->swizzle_param[0] = params[0];
            tex->swizzle_param[1] = params[1];
            tex->swizzle_param[2] = params[2];
            tex->swizzle_param[3] = params[3];
        } else {
            LOG_E("glTexParameteriv: params is nullptr for GL_TEXTURE_SWIZZLE_RGBA")
        }
    } else {
        TexParameteriv_Backend(target, pname, params);
    }

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {
    GLState.Lock();
    TexParameteriv_State(target, pname, params);
    GLState.Unlock();
}

// --- glTexParameterfv: _Backend, _State, Public ---

void TexParameterfv_Backend(GLenum target, GLenum pname, const GLfloat* params) {
    GLES.glTexParameterfv(target, pname, params);
}

void TexParameterfv_State(GLenum target, GLenum pname, const GLfloat* params) {
    LOG()
    LOG_D("glTexParameterfv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    TexParameterfv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    GLState.Lock();
    TexParameterfv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// Native texture query functions (ES 3.2)
// ============================================================================

// --- glGetTexParameteriv: _Backend, _State, Public ---

void GetTexParameteriv_Backend(GLenum target, GLenum pname, GLint* params) {
    GLES.glGetTexParameteriv(target, pname, params);
}

void GetTexParameteriv_State(GLenum target, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTexParameteriv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    GetTexParameteriv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTexParameteriv_State(target, pname, params);
    GLState.Unlock();
}

// --- glGetTexParameterfv: _Backend, _State, Public ---

void GetTexParameterfv_Backend(GLenum target, GLenum pname, GLfloat* params) {
    GLES.glGetTexParameterfv(target, pname, params);
}

void GetTexParameterfv_State(GLenum target, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetTexParameterfv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    GetTexParameterfv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    GLState.Lock();
    GetTexParameterfv_State(target, pname, params);
    GLState.Unlock();
}

// --- glGetTexLevelParameterfv: _Backend, _State, Public ---

void GetTexLevelParameterfv_Backend(GLenum target, GLint level, GLenum pname, GLfloat* params) {
    GLES.glGetTexLevelParameterfv(target, level, pname, params);
}

void GetTexLevelParameterfv_State(GLenum target, GLint level, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetTexLevelParameterfv,target: %d, level: %d, pname: %d", target, level, pname)
    if (gl_state) {
        GLenum rtarget = map_tex_target(target);
        if (rtarget == GL_PROXY_TEXTURE_2D) {
            switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = (float)nlevel(gl_state->proxy_width, level);
                return;
            case GL_TEXTURE_HEIGHT:
                (*params) = (float)nlevel(gl_state->proxy_height, level);
                return;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = (float)gl_state->proxy_intformat;
                return;
            default:
                return;
            }
        }
    }
    GetTexLevelParameterfv_Backend(target, level, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params) {
    GLState.Lock();
    GetTexLevelParameterfv_State(target, level, pname, params);
    GLState.Unlock();
}

// --- glGetTexLevelParameteriv: _Backend, _State, Public ---

void GetTexLevelParameteriv_Backend(GLenum target, GLint level, GLenum pname, GLint* params) {
    GLES.glGetTexLevelParameteriv(target, level, pname, params);
}

void GetTexLevelParameteriv_State(GLenum target, GLint level, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTexLevelParameteriv,target: %s, level: %d, pname: %s", glEnumToString(target), level,
          glEnumToString(pname))
    if (gl_state) {
        GLenum rtarget = map_tex_target(target);
        if (rtarget == GL_PROXY_TEXTURE_2D) {
            switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = nlevel(gl_state->proxy_width, level);
                return;
            case GL_TEXTURE_HEIGHT:
                (*params) = nlevel(gl_state->proxy_height, level);
                return;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = (GLint)gl_state->proxy_intformat;
                return;
            default:
                return;
            }
        }
    }
    LOG_D("es.glGetTexLevelParameteriv,target: %s, level: %d, pname: %s", glEnumToString(target), level,
          glEnumToString(pname))
    GetTexLevelParameteriv_Backend(target, level, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTexLevelParameteriv_State(target, level, pname, params);
    GLState.Unlock();
}

// ============================================================================
// Renderbuffer functions (native)
// ============================================================================

// --- glRenderbufferStorage: _Backend, _State, Public ---

void RenderbufferStorage_Backend(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLES.glRenderbufferStorage(target, internalFormat, width, height);
}

void RenderbufferStorage_State(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG()

    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorage, target: %s, internalFormat: %s, width: %d, "
          "height: %d",
          glEnumToString(target), glEnumToString(internalFormat), width, height)

    RenderbufferStorage_Backend(target, internalFormat, width, height);

    CHECK_GL_ERROR_NO_INIT
}

GLAPI GLAPIENTRY void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    GLState.Lock();
    RenderbufferStorage_State(target, internalFormat, width, height);
    GLState.Unlock();
}

// --- glRenderbufferStorageMultisample: _Backend, _State, Public ---

void RenderbufferStorageMultisample_Backend(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width,
                                            GLsizei height) {
    GLES.glRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
}

void RenderbufferStorageMultisample_State(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width,
                                          GLsizei height) {
    LOG()

    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorageMultisample, target: %d, samples: %d, "
          "internalFormat: %d, width: %d, height: %d",
          target, samples, internalFormat, width, height)

    RenderbufferStorageMultisample_Backend(target, samples, internalFormat, width, height);

    CHECK_GL_ERROR_NO_INIT
}

GLAPI GLAPIENTRY void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                                       GLsizei width, GLsizei height) {
    GLState.Lock();
    RenderbufferStorageMultisample_State(target, samples, internalFormat, width, height);
    GLState.Unlock();
}

// ============================================================================
// Other texture functions (keep existing logic)
// ============================================================================

// --- glGetTexImage: _Backend, _State, Public ---

// GetTexImage has no direct _Backend since it's implemented via FBO+ReadPixels
void GetTexImage_State(GLenum target, GLint level, GLenum format, GLenum type, void* pixels) {
    LOG()
    LOG_D("glGetTexImage, target: 0x%x, level: %d, format: 0x%x, type: 0x%x, pixels: 0x%x", target, level, format, type,
          pixels)
    // Use CPU-side cached FBO bindings instead of glGetIntegerv GPU round-trips
    GLint prevDrawFBO = GLState.framebuffer.drawFBO;
    GLint prevReadFBO = GLState.framebuffer.readFBO;

    GLuint tempFBO = acquireTempFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);

    GLint textureId = 0;
    GLenum textureBindingTarget;
    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        textureBindingTarget = GL_TEXTURE_BINDING_CUBE_MAP;
    } else if (target == GL_TEXTURE_2D) {
        textureBindingTarget = GL_TEXTURE_BINDING_2D;
    } else {
        LOG_E("glGetTexImage: Unsupported or complex target: 0x%x", target)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempFBO);
        return;
    }
    // Use CPU-side tracked texture binding for 2D (common case),
    // fall back to glGetIntegerv for cubemap targets
    if (textureBindingTarget == GL_TEXTURE_BINDING_2D) {
        textureId = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    } else {
        glGetIntegerv(textureBindingTarget, &textureId);
    }

    if (textureId == 0) {
        LOG_E("glGetTexImage: No texture bound to the specified target.")
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempFBO);
        return;
    }

    GLint width = 0, height = 0;
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);

    if (width == 0 || height == 0) {
        LOG_E("glGetTexImage: Texture level %d has zero width or height.", level)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempFBO);
        return;
    }

    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, textureId, level);
    } else if (target == GL_TEXTURE_2D) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, level);
    }

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
        LOG_E("glGetTexImage: Failed to create complete framebuffer. Status: 0x%x", fboStatus)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(tempFBO);
        return;
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glReadPixels(0, 0, width, height, format, type, pixels);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    releaseTempFBO(tempFBO);
}

GLAPI GLAPIENTRY void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void* pixels) {
    GLState.Lock();
    GetTexImage_State(target, level, format, type, pixels);
    GLState.Unlock();
}

#if GLOBAL_DEBUG || DEBUG
#include "../config/config.h"
#include <fstream>
#endif

// --- glReadPixels: _Backend, _State, Public ---

void ReadPixels_Backend(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
    GLES.glReadPixels(x, y, width, height, format, type, pixels);
}

void ReadPixels_State(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
    LOG()
    LOG_D("glReadPixels, x=%d, y=%d, width=%d, height=%d, format=0x%x, "
          "type=0x%x, pixels=0x%x",
          x, y, width, height, format, type, pixels)

    static int count = 0;
    GLenum prevFormat = format;

    if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8) {
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
    }
    LOG_D("glReadPixels converted, x=%d, y=%d, width=%d, height=%d, format=0x%x, "
          "type=0x%x, pixels=0x%x",
          x, y, width, height, format, type, pixels)
    ReadPixels_Backend(x, y, width, height, format, type, pixels);

#if GLOBAL_DEBUG || DEBUG
    if (prevFormat == GL_BGRA && type == GL_UNSIGNED_BYTE) {
        std::vector<uint8_t> px(width * height * sizeof(uint8_t) * 4, 0);
        GLES.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        GLES.glReadPixels(x, y, width, height, format, type, px.data());

        std::fstream fs(std::string(concatenate(mg_directory_path, "/readpixels/")) + std::to_string(count++) + ".bin",
                        std::ios::out | std::ios::binary | std::ios::trunc);
        fs.write((const char*)px.data(), px.size());
        fs.close();
    }
#endif
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                   void* pixels) {
    GLState.Lock();
    ReadPixels_State(x, y, width, height, format, type, pixels);
    GLState.Unlock();
}

// --- glClearTexImage: _State, Public (no _Backend, uses FBO) ---

void ClearTexImage_State(GLuint texture, GLint level, GLenum format, GLenum type, const void* data) {
    LOG()
    LOG_D("glClearTexImage, texture: %d, level: %d, format: %d, type: %d", texture, level, format, type)
    INIT_CHECK_GL_ERROR_FORCE
    // Use CPU-side cached FBO bindings instead of glGetIntegerv GPU round-trips
    GLuint fbo;
    GLint prevDrawFBO = GLState.framebuffer.drawFBO;
    GLint prevReadFBO = GLState.framebuffer.readFBO;
    fbo = acquireTempFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    CHECK_GL_ERROR_NO_INIT

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, level);

    CHECK_GL_ERROR_NO_INIT
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) [[unlikely]] {
        LOG_D("  -> exit")
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        releaseTempFBO(fbo);
        CHECK_GL_ERROR_NO_INIT
        return;
    }

    GLES.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    CHECK_GL_ERROR_NO_INIT

    if (data != nullptr) {
        if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
            auto* byteData = static_cast<const GLubyte*>(data);
            GLES.glClearColor((float)byteData[0] / 255.0f, (float)byteData[1] / 255.0f, (float)byteData[2] / 255.0f,
                              (float)byteData[3] / 255.0f);
        } else if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
            auto* byteData = static_cast<const GLubyte*>(data);
            GLES.glClearColor((float)byteData[0] / 255.0f, (float)byteData[1] / 255.0f, (float)byteData[2] / 255.0f,
                              1.0f);
        } else if (format == GL_RGBA && type == GL_FLOAT) {
            auto* floatData = static_cast<const GLfloat*>(data);
            GLES.glClearColor(floatData[0], floatData[1], floatData[2], floatData[3]);
        } else if (format == GL_RGB && type == GL_FLOAT) {
            auto* floatData = static_cast<const GLfloat*>(data);
            GLES.glClearColor(floatData[0], floatData[1], floatData[2], 1.0f);
        } else if (format == GL_DEPTH_COMPONENT && type == GL_FLOAT) {
            auto* depthData = static_cast<const GLfloat*>(data);
            GLES.glClearDepthf(depthData[0]);
            GLES.glClear(GL_DEPTH_BUFFER_BIT);
        } else if (format == GL_STENCIL_INDEX && type == GL_UNSIGNED_BYTE) {
            auto* stencilData = static_cast<const GLubyte*>(data);
            GLES.glClearStencil(stencilData[0]);
            GLES.glClear(GL_STENCIL_BUFFER_BIT);
        }
    }
    CHECK_GL_ERROR_NO_INIT

    if (format == GL_DEPTH_COMPONENT || format == GL_STENCIL_INDEX) {
        GLES.glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        CHECK_GL_ERROR_NO_INIT
    } else {
        GLES.glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL_ERROR_NO_INIT
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    releaseTempFBO(fbo);
    CHECK_GL_ERROR_NO_INIT
}

GLAPI GLAPIENTRY void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void* data) {
    GLState.Lock();
    ClearTexImage_State(texture, level, format, type, data);
    GLState.Unlock();
}

// --- glPixelStorei: _Backend, _State, Public ---

void PixelStorei_Backend(GLenum pname, GLint param) {
    GLES.glPixelStorei(pname, param);
}

void PixelStorei_State(GLenum pname, GLint param) {
    LOG_D("glPixelStorei, pname = %s, param = %d", glEnumToString(pname), param)
    PixelStorei_Backend(pname, param);
    // Keep CPU-side cache in sync to avoid glGetIntegerv GPU round-trips
    switch (pname) {
        case GL_UNPACK_ALIGNMENT:  GLState.texture.unpackAlignment = param;  break;
        case GL_UNPACK_ROW_LENGTH: GLState.texture.unpackRowLength = param;  break;
        case GL_UNPACK_IMAGE_HEIGHT: GLState.texture.unpackImageHeight = param; break;
        case GL_UNPACK_SKIP_PIXELS: GLState.texture.unpackSkipPixels = param; break;
        case GL_UNPACK_SKIP_ROWS:  GLState.texture.unpackSkipRows = param;  break;
        case GL_UNPACK_SKIP_IMAGES: GLState.texture.unpackSkipImages = param; break;
        case GL_PACK_ALIGNMENT:    GLState.texture.packAlignment = param;    break;
        case GL_PACK_ROW_LENGTH:   GLState.texture.packRowLength = param;    break;
        case GL_PACK_IMAGE_HEIGHT: GLState.texture.packImageHeight = param;  break;
        case GL_PACK_SKIP_PIXELS:  GLState.texture.packSkipPixels = param;   break;
        case GL_PACK_SKIP_ROWS:    GLState.texture.packSkipRows = param;     break;
        case GL_PACK_SKIP_IMAGES:  GLState.texture.packSkipImages = param;   break;
        default: break;
    }
    // Also update RenderState pixel store (new API)
    switch (pname) {
        case GL_UNPACK_ALIGNMENT:    GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackAlignment, param); break;
        case GL_UNPACK_ROW_LENGTH:   GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackRowLength, param); break;
        case GL_UNPACK_IMAGE_HEIGHT: GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackImageHeight, param); break;
        case GL_UNPACK_SKIP_PIXELS:  GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackSkipPixels, param); break;
        case GL_UNPACK_SKIP_ROWS:    GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackSkipRows, param); break;
        case GL_UNPACK_SKIP_IMAGES:  GLState.renderState.SetPixelStoreParam(PixelStoreParam::UnpackSkipImages, param); break;
        case GL_PACK_ALIGNMENT:      GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackAlignment, param); break;
        case GL_PACK_ROW_LENGTH:     GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackRowLength, param); break;
        case GL_PACK_IMAGE_HEIGHT:   GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackImageHeight, param); break;
        case GL_PACK_SKIP_PIXELS:    GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackSkipPixels, param); break;
        case GL_PACK_SKIP_ROWS:      GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackSkipRows, param); break;
        case GL_PACK_SKIP_IMAGES:    GLState.renderState.SetPixelStoreParam(PixelStoreParam::PackSkipImages, param); break;
        default: break;
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glPixelStorei(GLenum pname, GLint param) {
    GLState.Lock();
    PixelStorei_State(pname, param);
    GLState.Unlock();
}

// ============================================================================
// glTexBuffer: _Backend, _State, Public
// ============================================================================

void TexBuffer_Backend(GLenum target, GLenum internalformat, GLuint buffer) {
    GLES.glTexBuffer(target, internalformat, buffer);
}

void TexBuffer_State(GLenum target, GLenum internalformat, GLuint buffer) {
    LOG()
    LOG_D("glTexBuffer, target: %s, internalformat: %s, buffer: %u",
          glEnumToString(target), glEnumToString(internalformat), buffer)
    TexBuffer_Backend(target, internalformat, buffer);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
    GLState.Lock();
    TexBuffer_State(target, internalformat, buffer);
    GLState.Unlock();
}

// ============================================================================
// glTexBufferRange: _Backend, _State, Public
// ============================================================================

void TexBufferRange_Backend(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GLES.glTexBufferRange(target, internalformat, buffer, offset, size);
}

void TexBufferRange_State(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    LOG()
    LOG_D("glTexBufferRange, target: %s, internalformat: %s, buffer: %u, offset: %ld, size: %ld",
          glEnumToString(target), glEnumToString(internalformat), buffer, (long)offset, (long)size)
    TexBufferRange_Backend(target, internalformat, buffer, offset, size);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset,
                                       GLsizeiptr size) {
    GLState.Lock();
    TexBufferRange_State(target, internalformat, buffer, offset, size);
    GLState.Unlock();
}

// ============================================================================
// glGenerateTextureMipmap: _Backend, _State, Public
// ============================================================================

void GenerateTextureMipmap_Backend(GLuint texture) {
    GLES.glGenerateMipmap(GL_TEXTURE_2D);
    // Note: glGenerateTextureMipmap is DSA; ES 3.2 supports glGenerateMipmap only.
    // The DSA wrapper handles binding the texture first.
}

void GenerateTextureMipmap_State(GLuint texture) {
    LOG()
    LOG_D("glGenerateTextureMipmap, texture: %u", texture)
    // DSA: temporarily bind texture, generate mipmap, restore previous binding
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, texture);
    }
    GLES.glGenerateMipmap(GL_TEXTURE_2D);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGenerateTextureMipmap(GLuint texture) {
    GLState.Lock();
    GenerateTextureMipmap_State(texture);
    GLState.Unlock();
}

// ============================================================================
// glTexImage1D: _Backend, _State, Public (not supported in ES 3.2, stub)
// ============================================================================

void TexImage1D_Backend(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border,
                        GLenum format, GLenum type, const GLvoid* pixels) {
    // Not supported in OpenGL ES 3.2
    (void)target; (void)level; (void)internalFormat; (void)width; (void)border;
    (void)format; (void)type; (void)pixels;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glTexImage1D is not supported in OpenGL ES 3.2"));
}

void TexImage1D_State(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border,
                      GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    LOG_D("glTexImage1D, target: %s, level: %d, internalFormat: %s, width: %d, border: %d, format: %s, type: %s",
          glEnumToString(target), level, glEnumToString(internalFormat), width, border, glEnumToString(format),
          glEnumToString(type))
    TexImage1D_Backend(target, level, internalFormat, width, border, format, type, pixels);
}

GLAPI GLAPIENTRY void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border,
                                   GLenum format, GLenum type, const GLvoid* pixels) {
    GLState.Lock();
    TexImage1D_State(target, level, internalFormat, width, border, format, type, pixels);
    GLState.Unlock();
}

// ============================================================================
// glTexSubImage1D: _Backend, _State, Public (not supported in ES 3.2, stub)
// ============================================================================

void TexSubImage1D_Backend(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type,
                           const void* pixels) {
    (void)target; (void)level; (void)xoffset; (void)width; (void)format; (void)type; (void)pixels;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glTexSubImage1D is not supported in OpenGL ES 3.2"));
}

void TexSubImage1D_State(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type,
                         const void* pixels) {
    LOG()
    LOG_D("glTexSubImage1D, target: %s, level: %d, xoffset: %d, width: %d, format: %s, type: %s",
          glEnumToString(target), level, xoffset, width, glEnumToString(format), glEnumToString(type))
    TexSubImage1D_Backend(target, level, xoffset, width, format, type, pixels);
}

GLAPI GLAPIENTRY void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format,
                                      GLenum type, const void* pixels) {
    GLState.Lock();
    TexSubImage1D_State(target, level, xoffset, width, format, type, pixels);
    GLState.Unlock();
}

// ============================================================================
// glTexStorage1D: _Backend, _State, Public (not supported in ES 3.2, stub)
// ============================================================================

void TexStorage1D_Backend(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width) {
    (void)target; (void)levels; (void)internalFormat; (void)width;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glTexStorage1D is not supported in OpenGL ES 3.2"));
}

void TexStorage1D_State(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width) {
    LOG()
    LOG_D("glTexStorage1D, target: %d, levels: %d, internalFormat: %d, width: %d",
          target, levels, internalFormat, width)
    TexStorage1D_Backend(target, levels, internalFormat, width);
}

GLAPI GLAPIENTRY void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width) {
    GLState.Lock();
    TexStorage1D_State(target, levels, internalFormat, width);
    GLState.Unlock();
}

// ============================================================================
// glTexImage2DMultisample: _Backend, _State, Public (stub)
// ============================================================================

void TexImage2DMultisample_Backend(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                   GLsizei height, GLboolean fixedsamplelocations) {
    (void)target; (void)samples; (void)internalformat; (void)width; (void)height; (void)fixedsamplelocations;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glTexImage2DMultisample is not supported in OpenGL ES 3.2"));
}

void TexImage2DMultisample_State(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTexImage2DMultisample, target: %s, samples: %d, internalformat: %s, width: %d, height: %d",
          glEnumToString(target), samples, glEnumToString(internalformat), width, height)
    TexImage2DMultisample_Backend(target, samples, internalformat, width, height, fixedsamplelocations);
}

GLAPI GLAPIENTRY void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                              GLsizei height, GLboolean fixedsamplelocations) {
    GLState.Lock();
    TexImage2DMultisample_State(target, samples, internalformat, width, height, fixedsamplelocations);
    GLState.Unlock();
}

// ============================================================================
// glTexImage3DMultisample: _Backend, _State, Public (stub)
// ============================================================================

void TexImage3DMultisample_Backend(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                   GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    (void)target; (void)samples; (void)internalformat; (void)width; (void)height; (void)depth; (void)fixedsamplelocations;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glTexImage3DMultisample is not supported in OpenGL ES 3.2"));
}

void TexImage3DMultisample_State(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTexImage3DMultisample, target: %s, samples: %d, internalformat: %s, width: %d, height: %d, depth: %d",
          glEnumToString(target), samples, glEnumToString(internalformat), width, height, depth)
    TexImage3DMultisample_Backend(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}

GLAPI GLAPIENTRY void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                              GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GLState.Lock();
    TexImage3DMultisample_State(target, samples, internalformat, width, height, depth, fixedsamplelocations);
    GLState.Unlock();
}

// ============================================================================
// glTexStorage2DMultisample: _Backend, _State, Public
// ============================================================================

void TexStorage2DMultisample_Backend(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                     GLsizei height, GLboolean fixedsamplelocations) {
    GLES.glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}

void TexStorage2DMultisample_State(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                   GLsizei height, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTexStorage2DMultisample, target: %s, samples: %d, internalformat: %s, width: %d, height: %d",
          glEnumToString(target), samples, glEnumToString(internalformat), width, height)
    TexStorage2DMultisample_Backend(target, samples, internalformat, width, height, fixedsamplelocations);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalformat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                                GLsizei height, GLboolean fixedsamplelocations) {
    GLState.Lock();
    TexStorage2DMultisample_State(target, samples, internalformat, width, height, fixedsamplelocations);
    GLState.Unlock();
}

// ============================================================================
// glTexStorage3DMultisample: _Backend, _State, Public
// ============================================================================

void TexStorage3DMultisample_Backend(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                     GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GLES.glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}

void TexStorage3DMultisample_State(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                   GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTexStorage3DMultisample, target: %s, samples: %d, internalformat: %s, width: %d, height: %d, depth: %d",
          glEnumToString(target), samples, glEnumToString(internalformat), width, height, depth)
    TexStorage3DMultisample_Backend(target, samples, internalformat, width, height, depth, fixedsamplelocations);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalformat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
                                                GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GLState.Lock();
    TexStorage3DMultisample_State(target, samples, internalformat, width, height, depth, fixedsamplelocations);
    GLState.Unlock();
}

// ============================================================================
// glBindImageTexture: _Backend, _State, Public
// ============================================================================

void BindImageTexture_Backend(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
                              GLenum access, GLenum format) {
    GLES.glBindImageTexture(unit, texture, level, layered, layer, access, format);
}

void BindImageTexture_State(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
                            GLenum access, GLenum format) {
    LOG()
    LOG_D("glBindImageTexture, unit: %u, texture: %u, level: %d, layered: %d, layer: %d, access: %s, format: %s",
          unit, texture, level, layered, layer, glEnumToString(access), glEnumToString(format))

    BindImageTexture_Backend(unit, texture, level, layered, layer, access, format);

    // Update GLState image unit tracking
    if (unit < MAX_IMAGE_UNITS) {
        GLState.image.imageUnits[unit].texture = texture;
        GLState.image.imageUnits[unit].level = level;
        GLState.image.imageUnits[unit].layered = layered;
        GLState.image.imageUnits[unit].layer = layer;
        GLState.image.imageUnits[unit].access = access;
        GLState.image.imageUnits[unit].format = format;
    }

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
                                         GLenum access, GLenum format) {
    GLState.Lock();
    BindImageTexture_State(unit, texture, level, layered, layer, access, format);
    GLState.Unlock();
}

// ============================================================================
// glBindTextureUnit: _Backend, _State, Public
// ============================================================================

void BindTextureUnit_Backend(GLuint unit, GLuint texture) {
    // ES 3.2 doesn't have glBindTextureUnit; emulate via active texture + bind
    GLint prevUnit = GLState.texture.activeUnit;
    GLES.glActiveTexture(GL_TEXTURE0 + unit);
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glActiveTexture(GL_TEXTURE0 + prevUnit);
}

void BindTextureUnit_State(GLuint unit, GLuint texture) {
    LOG()
    LOG_D("glBindTextureUnit, unit: %u, texture: %u", unit, texture)

    if (unit >= (GLuint)MAX_TEXTURE_UNITS) {
        LOG_E("glBindTextureUnit: Invalid unit %u", unit)
        return;
    }

    BindTextureUnit_Backend(unit, texture);

    // Update GLState tracking
    GLState.texture.texUnits[unit].binding[GL_TEXTURE_2D] = texture;

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glBindTextureUnit(GLuint unit, GLuint texture) {
    GLState.Lock();
    BindTextureUnit_State(unit, texture);
    GLState.Unlock();
}

// ============================================================================
// glCreateTextures: _Backend, _State, Public
// ============================================================================

void CreateTextures_Backend(GLenum target, GLsizei n, GLuint* textures) {
    GLES.glGenTextures(n, textures);
}

void CreateTextures_State(GLenum target, GLsizei n, GLuint* textures) {
    LOG()
    LOG_D("glCreateTextures, target: %s, n: %d", glEnumToString(target), n)

    if (n <= 0 || !textures) {
        LOG_E("glCreateTextures: Invalid parameters")
        return;
    }

    CreateTextures_Backend(target, n, textures);

    for (GLsizei i = 0; i < n; ++i) {
        GLState.texture.textureMap[textures[i]] = textures[i];
        GLState.texture.textureMapReverse[textures[i]] = textures[i];
    }

    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCreateTextures(GLenum target, GLsizei n, GLuint* textures) {
    GLState.Lock();
    CreateTextures_State(target, n, textures);
    GLState.Unlock();
}

// ============================================================================
// glCopyImageSubData: _Backend, _State, Public
// ============================================================================

void CopyImageSubData_Backend(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
                              GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
                              GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    GLES.glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
                            dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
                            srcWidth, srcHeight, srcDepth);
}

void CopyImageSubData_State(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
                            GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
                            GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    LOG()
    LOG_D("glCopyImageSubData, srcName: %u, srcTarget: %s, srcLevel: %d, dstName: %u, dstTarget: %s, dstLevel: %d",
          srcName, glEnumToString(srcTarget), srcLevel, dstName, glEnumToString(dstTarget), dstLevel)

    CopyImageSubData_Backend(srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
                             dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
                             srcWidth, srcHeight, srcDepth);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY,
                                         GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX,
                                         GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight,
                                         GLsizei srcDepth) {
    GLState.Lock();
    CopyImageSubData_State(srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
                           dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
                           srcWidth, srcHeight, srcDepth);
    GLState.Unlock();
}

// ============================================================================
// glGetMultisamplefv: _Backend, _State, Public
// ============================================================================

void GetMultisamplefv_Backend(GLenum pname, GLuint index, GLfloat* val) {
    GLES.glGetMultisamplefv(pname, index, val);
}

void GetMultisamplefv_State(GLenum pname, GLuint index, GLfloat* val) {
    LOG()
    LOG_D("glGetMultisamplefv, pname: %s, index: %u", glEnumToString(pname), index)
    GetMultisamplefv_Backend(pname, index, val);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val) {
    GLState.Lock();
    GetMultisamplefv_State(pname, index, val);
    GLState.Unlock();
}

// ============================================================================
// glGetInternalformativ: _Backend, _State, Public
// ============================================================================

void GetInternalformativ_Backend(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params) {
    GLES.glGetInternalformativ(target, internalformat, pname, bufSize, params);
}

void GetInternalformativ_State(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params) {
    LOG()
    LOG_D("glGetInternalformativ, target: %s, internalformat: %s, pname: %s, bufSize: %d",
          glEnumToString(target), glEnumToString(internalformat), glEnumToString(pname), bufSize)
    GetInternalformativ_Backend(target, internalformat, pname, bufSize, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize,
                                            GLint* params) {
    GLState.Lock();
    GetInternalformativ_State(target, internalformat, pname, bufSize, params);
    GLState.Unlock();
}

// ============================================================================
// glGetCompressedTexImage: _Backend, _State, Public (stub)
// ============================================================================

void GetCompressedTexImage_Backend(GLenum target, GLint level, void* img) {
    (void)target; (void)level; (void)img;
    GLState.errorState.RecordError(ErrorCode::InvalidOperation,
        std::make_unique<GenericErrorInfo>("glGetCompressedTexImage is not supported in OpenGL ES 3.2"));
}

void GetCompressedTexImage_State(GLenum target, GLint level, void* img) {
    LOG()
    LOG_D("glGetCompressedTexImage, target: %s, level: %d", glEnumToString(target), level)
    GetCompressedTexImage_Backend(target, level, img);
}

GLAPI GLAPIENTRY void glGetCompressedTexImage(GLenum target, GLint level, void* img) {
    GLState.Lock();
    GetCompressedTexImage_State(target, level, img);
    GLState.Unlock();
}

// ============================================================================
// glTexParameterIiv: _Backend, _State, Public
// ============================================================================

void TexParameterIiv_Backend(GLenum target, GLenum pname, const GLint* params) {
    GLES.glTexParameterIiv(target, pname, params);
}

void TexParameterIiv_State(GLenum target, GLenum pname, const GLint* params) {
    LOG()
    LOG_D("glTexParameterIiv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    TexParameterIiv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameterIiv(GLenum target, GLenum pname, const GLint* params) {
    GLState.Lock();
    TexParameterIiv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// glTexParameterIuiv: _Backend, _State, Public
// ============================================================================

void TexParameterIuiv_Backend(GLenum target, GLenum pname, const GLuint* params) {
    GLES.glTexParameterIuiv(target, pname, params);
}

void TexParameterIuiv_State(GLenum target, GLenum pname, const GLuint* params) {
    LOG()
    LOG_D("glTexParameterIuiv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    TexParameterIuiv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint* params) {
    GLState.Lock();
    TexParameterIuiv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// glGetTexParameterIiv: _Backend, _State, Public
// ============================================================================

void GetTexParameterIiv_Backend(GLenum target, GLenum pname, GLint* params) {
    GLES.glGetTexParameterIiv(target, pname, params);
}

void GetTexParameterIiv_State(GLenum target, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTexParameterIiv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    GetTexParameterIiv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTexParameterIiv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// glGetTexParameterIuiv: _Backend, _State, Public
// ============================================================================

void GetTexParameterIuiv_Backend(GLenum target, GLenum pname, GLuint* params) {
    GLES.glGetTexParameterIuiv(target, pname, params);
}

void GetTexParameterIuiv_State(GLenum target, GLenum pname, GLuint* params) {
    LOG()
    LOG_D("glGetTexParameterIuiv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    GetTexParameterIuiv_Backend(target, pname, params);
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params) {
    GLState.Lock();
    GetTexParameterIuiv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// DSA glTexture* functions (stubs - emulated via binding/unbinding)
// ============================================================================

// glTextureStorage1D
void TextureStorage1D_State(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    LOG()
    LOG_D("glTextureStorage1D, texture: %u, levels: %d, internalformat: %s, width: %d",
          texture, levels, glEnumToString(internalformat), width)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexStorage2D(GL_TEXTURE_2D, levels, internalformat, width, 1);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    GLState.Lock();
    TextureStorage1D_State(texture, levels, internalformat, width);
    GLState.Unlock();
}

// glTextureStorage2D
void TextureStorage2D_State(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    LOG()
    LOG_D("glTextureStorage2D, texture: %u, levels: %d, internalformat: %s, width: %d, height: %d",
          texture, levels, glEnumToString(internalformat), width, height)
    internal_convert(&internalformat, nullptr, nullptr);
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexStorage2D(GL_TEXTURE_2D, levels, internalformat, width, height);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width,
                                         GLsizei height) {
    GLState.Lock();
    TextureStorage2D_State(texture, levels, internalformat, width, height);
    GLState.Unlock();
}

// glTextureStorage3D
void TextureStorage3D_State(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height,
                            GLsizei depth) {
    LOG()
    LOG_D("glTextureStorage3D, texture: %u, levels: %d, internalformat: %s, width: %d, height: %d, depth: %d",
          texture, levels, glEnumToString(internalformat), width, height, depth)
    internal_convert(&internalformat, nullptr, nullptr);
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_3D, texture);
    GLES.glTexStorage3D(GL_TEXTURE_3D, levels, internalformat, width, height, depth);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width,
                                         GLsizei height, GLsizei depth) {
    GLState.Lock();
    TextureStorage3D_State(texture, levels, internalformat, width, height, depth);
    GLState.Unlock();
}

// glTextureSubImage1D
void TextureSubImage1D_State(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type,
                             const void* pixels) {
    LOG()
    LOG_D("glTextureSubImage1D, texture: %u, level: %d, xoffset: %d, width: %d, format: %s, type: %s",
          texture, level, xoffset, width, glEnumToString(format), glEnumToString(type))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, width, 1, format, type, pixels);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format,
                                          GLenum type, const void* pixels) {
    GLState.Lock();
    TextureSubImage1D_State(texture, level, xoffset, width, format, type, pixels);
    GLState.Unlock();
}

// glTextureSubImage2D
void TextureSubImage2D_State(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                             GLenum format, GLenum type, const void* pixels) {
    LOG()
    LOG_D("glTextureSubImage2D, texture: %u, level: %d, xoffset: %d, yoffset: %d, width: %d, height: %d",
          texture, level, xoffset, yoffset, width, height)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, format, type, pixels);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                          GLsizei height, GLenum format, GLenum type, const void* pixels) {
    GLState.Lock();
    TextureSubImage2D_State(texture, level, xoffset, yoffset, width, height, format, type, pixels);
    GLState.Unlock();
}

// glTextureSubImage3D
void TextureSubImage3D_State(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
                             GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    LOG()
    LOG_D("glTextureSubImage3D, texture: %u, level: %d, xoffset: %d, yoffset: %d, zoffset: %d, width: %d, height: %d, depth: %d",
          texture, level, xoffset, yoffset, zoffset, width, height, depth)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_3D, texture);
    GLES.glTexSubImage3D(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                          const void* pixels) {
    GLState.Lock();
    TextureSubImage3D_State(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    GLState.Unlock();
}

// glTextureParameterf
void TextureParameterf_State(GLuint texture, GLenum pname, GLfloat param) {
    LOG()
    LOG_D("glTextureParameterf, texture: %u, pname: %s, param: %f", texture, glEnumToString(pname), param)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameterf(GL_TEXTURE_2D, pname, param);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
    GLState.Lock();
    TextureParameterf_State(texture, pname, param);
    GLState.Unlock();
}

// glTextureParameteri
void TextureParameteri_State(GLuint texture, GLenum pname, GLint param) {
    LOG()
    LOG_D("glTextureParameteri, texture: %u, pname: %s, param: %d", texture, glEnumToString(pname), param)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameteri(GL_TEXTURE_2D, pname, param);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
    GLState.Lock();
    TextureParameteri_State(texture, pname, param);
    GLState.Unlock();
}

// glTextureParameteriv
void TextureParameteriv_State(GLuint texture, GLenum pname, const GLint* params) {
    LOG()
    LOG_D("glTextureParameteriv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameteriv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params) {
    GLState.Lock();
    TextureParameteriv_State(texture, pname, params);
    GLState.Unlock();
}

// glTextureParameterfv
void TextureParameterfv_State(GLuint texture, GLenum pname, const GLfloat* params) {
    LOG()
    LOG_D("glTextureParameterfv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameterfv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params) {
    GLState.Lock();
    TextureParameterfv_State(texture, pname, params);
    GLState.Unlock();
}

// glTextureParameterIiv
void TextureParameterIiv_State(GLuint texture, GLenum pname, const GLint* params) {
    LOG()
    LOG_D("glTextureParameterIiv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameterIiv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params) {
    GLState.Lock();
    TextureParameterIiv_State(texture, pname, params);
    GLState.Unlock();
}

// glTextureParameterIuiv
void TextureParameterIuiv_State(GLuint texture, GLenum pname, const GLuint* params) {
    LOG()
    LOG_D("glTextureParameterIuiv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexParameterIuiv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params) {
    GLState.Lock();
    TextureParameterIuiv_State(texture, pname, params);
    GLState.Unlock();
}

// glGetTextureImage
void GetTextureImage_State(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
    LOG()
    LOG_D("glGetTextureImage, texture: %u, level: %d, format: %s, type: %s, bufSize: %d",
          texture, level, glEnumToString(format), glEnumToString(type), bufSize)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, level, format, type, pixels);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize,
                                        void* pixels) {
    GLState.Lock();
    GetTextureImage_State(texture, level, format, type, bufSize, pixels);
    GLState.Unlock();
}

// glGetTextureSubImage
void GetTextureSubImage_State(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                              GLsizei bufSize, void* pixels) {
    LOG()
    LOG_D("glGetTextureSubImage, texture: %u, level: %d, xoffset: %d, yoffset: %d, zoffset: %d, width: %d, height: %d, depth: %d",
          texture, level, xoffset, yoffset, zoffset, width, height, depth)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    // Use glReadPixels via FBO for sub-image readback
    GLint prevDrawFBO = GLState.framebuffer.drawFBO;
    GLint prevReadFBO = GLState.framebuffer.readFBO;
    GLuint tempFBO = acquireTempFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, level);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(xoffset, yoffset, width, height, format, type, pixels);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    releaseTempFBO(tempFBO);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                           GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                           GLsizei bufSize, void* pixels) {
    GLState.Lock();
    GetTextureSubImage_State(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
    GLState.Unlock();
}

// glGetTextureParameteriv
void GetTextureParameteriv_State(GLuint texture, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTextureParameteriv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexParameteriv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTextureParameteriv_State(texture, pname, params);
    GLState.Unlock();
}

// glGetTextureParameterfv
void GetTextureParameterfv_State(GLuint texture, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetTextureParameterfv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexParameterfv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params) {
    GLState.Lock();
    GetTextureParameterfv_State(texture, pname, params);
    GLState.Unlock();
}

// glGetTextureParameterIiv
void GetTextureParameterIiv_State(GLuint texture, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTextureParameterIiv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexParameterIiv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTextureParameterIiv_State(texture, pname, params);
    GLState.Unlock();
}

// glGetTextureParameterIuiv
void GetTextureParameterIuiv_State(GLuint texture, GLenum pname, GLuint* params) {
    LOG()
    LOG_D("glGetTextureParameterIuiv, texture: %u, pname: %s", texture, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexParameterIuiv(GL_TEXTURE_2D, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params) {
    GLState.Lock();
    GetTextureParameterIuiv_State(texture, pname, params);
    GLState.Unlock();
}

// glGetTextureLevelParameteriv
void GetTextureLevelParameteriv_State(GLuint texture, GLint level, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTextureLevelParameteriv, texture: %u, level: %d, pname: %s", texture, level, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexLevelParameteriv(GL_TEXTURE_2D, level, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint* params) {
    GLState.Lock();
    GetTextureLevelParameteriv_State(texture, level, pname, params);
    GLState.Unlock();
}

// glGetTextureLevelParameterfv
void GetTextureLevelParameterfv_State(GLuint texture, GLint level, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetTextureLevelParameterfv, texture: %u, level: %d, pname: %s", texture, level, glEnumToString(pname))
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glGetTexLevelParameterfv(GL_TEXTURE_2D, level, pname, params);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat* params) {
    GLState.Lock();
    GetTextureLevelParameterfv_State(texture, level, pname, params);
    GLState.Unlock();
}

// glTextureStorage2DMultisample
void TextureStorage2DMultisample_State(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width,
                                       GLsizei height, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTextureStorage2DMultisample, texture: %u, samples: %d, internalformat: %s, width: %d, height: %d",
          texture, samples, glEnumToString(internalformat), width, height)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_2D, texture);
    GLES.glTexStorage2DMultisample(GL_TEXTURE_2D, samples, internalformat, width, height, fixedsamplelocations);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat,
                                                    GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GLState.Lock();
    TextureStorage2DMultisample_State(texture, samples, internalformat, width, height, fixedsamplelocations);
    GLState.Unlock();
}

// glTextureStorage3DMultisample
void TextureStorage3DMultisample_State(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width,
                                       GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    LOG()
    LOG_D("glTextureStorage3DMultisample, texture: %u, samples: %d, internalformat: %s, width: %d, height: %d, depth: %d",
          texture, samples, glEnumToString(internalformat), width, height, depth)
    GLint prevTex = g_tracked_tex2d_binding[GetCurrentTextureUnitIndex()];
    GLES.glBindTexture(GL_TEXTURE_3D, texture);
    GLES.glTexStorage3DMultisample(GL_TEXTURE_3D, samples, internalformat, width, height, depth, fixedsamplelocations);
    if (prevTex != (GLint)texture) {
        GLES.glBindTexture(GL_TEXTURE_2D, prevTex);
    }
    CHECK_GL_ERROR
}

GLAPI GLAPIENTRY void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat,
                                                    GLsizei width, GLsizei height, GLsizei depth,
                                                    GLboolean fixedsamplelocations) {
    GLState.Lock();
    TextureStorage3DMultisample_State(texture, samples, internalformat, width, height, depth, fixedsamplelocations);
    GLState.Unlock();
}