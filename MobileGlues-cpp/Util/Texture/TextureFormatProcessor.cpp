#include "TextureFormatProcessor.h"
#include "Util/Debug/Log.h"

namespace MobileGL {
namespace Texture {

uint32_t GetApplicablePixelFormatNormalizeOptions() {
    // By default, no normalization options are needed on GLES 3.2
    // since it supports all the formats we use.
    return static_cast<uint32_t>(PixelFormatNormalizeOptionBit::None);
}

NormalizedPixelFormat NormalizePixelFormat(GLenum internalFormat, uint32_t options) {
    NormalizedPixelFormat result;
    result.internalFormat = internalFormat;
    result.format = GL_RGBA;
    result.type = GL_UNSIGNED_BYTE;

    switch (internalFormat) {
        // Single-channel formats
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R8I:
        case GL_R8UI:
            result.format = GL_RED_INTEGER;
            result.type = (internalFormat == GL_R8 || internalFormat == GL_R8_SNORM) ? GL_UNSIGNED_BYTE : GL_BYTE;
            break;
        case GL_R16F:
        case GL_R16I:
        case GL_R16UI:
            result.format = GL_RED_INTEGER;
            result.type = GL_HALF_FLOAT;
            break;
        case GL_R32F:
        case GL_R32I:
        case GL_R32UI:
            result.format = GL_RED_INTEGER;
            result.type = GL_FLOAT;
            break;

        // Two-channel formats
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG8I:
        case GL_RG8UI:
            result.format = GL_RG_INTEGER;
            result.type = GL_UNSIGNED_BYTE;
            break;
        case GL_RG16F:
        case GL_RG16I:
        case GL_RG16UI:
            result.format = GL_RG_INTEGER;
            result.type = GL_HALF_FLOAT;
            break;
        case GL_RG32F:
        case GL_RG32I:
        case GL_RG32UI:
            result.format = GL_RG_INTEGER;
            result.type = GL_FLOAT;
            break;

        // Three-channel formats
        case GL_RGB8:
        case GL_SRGB8:
        case GL_RGB8_SNORM:
            result.format = GL_RGB;
            result.type = GL_UNSIGNED_BYTE;
            break;
        case GL_R11F_G11F_B10F:
            result.format = GL_RGB;
            result.type = GL_UNSIGNED_INT_10F_11F_11F_REV;
            break;
        case GL_RGB9_E5:
            result.format = GL_RGB;
            result.type = GL_UNSIGNED_INT_5_9_9_9_REV;
            break;
        case GL_RGB16F:
            result.format = GL_RGB;
            result.type = GL_HALF_FLOAT;
            break;
        case GL_RGB32F:
            result.format = GL_RGB;
            result.type = GL_FLOAT;
            break;
        case GL_RGB8I:
        case GL_RGB8UI:
            result.format = GL_RGB_INTEGER;
            result.type = GL_UNSIGNED_BYTE;
            break;
        case GL_RGB16I:
        case GL_RGB16UI:
            result.format = GL_RGB_INTEGER;
            result.type = GL_UNSIGNED_SHORT;
            break;
        case GL_RGB32I:
        case GL_RGB32UI:
            result.format = GL_RGB_INTEGER;
            result.type = GL_UNSIGNED_INT;
            break;

        // Four-channel formats
        case GL_RGBA8:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA8_SNORM:
            result.format = GL_RGBA;
            result.type = GL_UNSIGNED_BYTE;
            break;
        case GL_RGB5_A1:
            result.format = GL_RGBA;
            result.type = GL_UNSIGNED_SHORT_5_5_5_1;
            break;
        case GL_RGBA4:
            result.format = GL_RGBA;
            result.type = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case GL_RGB10_A2:
            result.format = GL_RGBA;
            result.type = GL_UNSIGNED_INT_2_10_10_10_REV;
            break;
        case GL_RGB10_A2UI:
            result.format = GL_RGBA_INTEGER;
            result.type = GL_UNSIGNED_INT_2_10_10_10_REV;
            break;
        case GL_RGBA16F:
            result.format = GL_RGBA;
            result.type = GL_HALF_FLOAT;
            break;
        case GL_RGBA32F:
            result.format = GL_RGBA;
            result.type = GL_FLOAT;
            break;
        case GL_RGBA8I:
        case GL_RGBA8UI:
            result.format = GL_RGBA_INTEGER;
            result.type = GL_UNSIGNED_BYTE;
            break;
        case GL_RGBA16I:
        case GL_RGBA16UI:
            result.format = GL_RGBA_INTEGER;
            result.type = GL_UNSIGNED_SHORT;
            break;
        case GL_RGBA32I:
        case GL_RGBA32UI:
            result.format = GL_RGBA_INTEGER;
            result.type = GL_UNSIGNED_INT;
            break;

        // Depth formats
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F:
            result.format = GL_DEPTH_COMPONENT;
            result.type = (internalFormat == GL_DEPTH_COMPONENT32F) ? GL_FLOAT : GL_UNSIGNED_SHORT;
            break;
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH32F_STENCIL8:
            result.format = GL_DEPTH_STENCIL;
            result.type = GL_UNSIGNED_INT_24_8;
            break;
        case GL_STENCIL_INDEX8:
            result.format = 0x1901; // GL_STENCIL_INDEX
            result.type = GL_UNSIGNED_BYTE;
            break;

        default:
            result.format = GL_RGBA;
            result.type = GL_UNSIGNED_BYTE;
            break;
    }

    // Apply normalization options if needed
    if (options & static_cast<uint32_t>(PixelFormatNormalizeOptionBit::NoRgb16)) {
        // Handle RGB16 fallback
    }
    if (options & static_cast<uint32_t>(PixelFormatNormalizeOptionBit::NoSnorm16)) {
        // Handle SNORM16 fallback
    }

    return result;
}

} // namespace Texture
} // namespace MobileGL