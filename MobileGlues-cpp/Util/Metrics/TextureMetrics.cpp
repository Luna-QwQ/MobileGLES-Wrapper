#include "TextureMetrics.h"

#include <algorithm>
#include <cmath>

namespace MobileGL {
namespace Metrics {

uint32_t GetBytesPerPixel(GLenum internalFormat) {
    switch (internalFormat) {
        // 8-bit single channel
        case GL_R8: case GL_R8_SNORM: case GL_R8I: case GL_R8UI:
            return 1;
        // 16-bit single channel
        case GL_R16F: case GL_R16I: case GL_R16UI:
            return 2;
        // 32-bit single channel
        case GL_R32F: case GL_R32I: case GL_R32UI:
            return 4;

        // 8-bit dual channel
        case GL_RG8: case GL_RG8_SNORM: case GL_RG8I: case GL_RG8UI:
            return 2;
        // 16-bit dual channel
        case GL_RG16F: case GL_RG16I: case GL_RG16UI:
            return 4;
        // 32-bit dual channel
        case GL_RG32F: case GL_RG32I: case GL_RG32UI:
            return 8;

        // 8-bit RGB
        case GL_RGB8: case GL_SRGB8: case GL_RGB8_SNORM:
            return 3;
        // Special RGB formats
        case GL_R11F_G11F_B10F: return 4;
        case GL_RGB9_E5:        return 4;
        // 16-bit RGB
        case GL_RGB16F: case GL_RGB16I: case GL_RGB16UI:
            return 6;
        // 32-bit RGB
        case GL_RGB32F: case GL_RGB32I: case GL_RGB32UI:
            return 12;

        // 8-bit RGBA
        case GL_RGBA8: case GL_SRGB8_ALPHA8: case GL_RGBA8_SNORM:
            return 4;
        case GL_RGB5_A1: case GL_RGBA4:
            return 2;
        case GL_RGB10_A2: case GL_RGB10_A2UI:
            return 4;
        // 16-bit RGBA
        case GL_RGBA16F: case GL_RGBA16I: case GL_RGBA16UI:
            return 8;
        // 32-bit RGBA
        case GL_RGBA32F: case GL_RGBA32I: case GL_RGBA32UI:
            return 16;

        // Depth
        case GL_DEPTH_COMPONENT16: return 2;
        case GL_DEPTH_COMPONENT24: return 3;
        case GL_DEPTH_COMPONENT32F: return 4;
        case GL_DEPTH24_STENCIL8: return 4;
        case GL_DEPTH32F_STENCIL8: return 8;
        case GL_STENCIL_INDEX8: return 1;

        default: return 4; // Default to RGBA8
    }
}

int GetComponentCount(GLenum internalFormat) {
    switch (internalFormat) {
        case GL_R8: case GL_R8_SNORM: case GL_R16F: case GL_R32F:
        case GL_R8I: case GL_R8UI: case GL_R16I: case GL_R16UI:
        case GL_R32I: case GL_R32UI:
        case GL_DEPTH_COMPONENT16: case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F: case GL_STENCIL_INDEX8:
            return 1;

        case GL_RG8: case GL_RG8_SNORM: case GL_RG16F: case GL_RG32F:
        case GL_RG8I: case GL_RG8UI: case GL_RG16I: case GL_RG16UI:
        case GL_RG32I: case GL_RG32UI:
        case GL_DEPTH24_STENCIL8: case GL_DEPTH32F_STENCIL8:
            return 2;

        case GL_RGB8: case GL_SRGB8: case GL_RGB8_SNORM:
        case GL_R11F_G11F_B10F: case GL_RGB9_E5:
        case GL_RGB16F: case GL_RGB32F:
        case GL_RGB8I: case GL_RGB8UI: case GL_RGB16I: case GL_RGB16UI:
        case GL_RGB32I: case GL_RGB32UI:
            return 3;

        case GL_RGBA8: case GL_SRGB8_ALPHA8: case GL_RGBA8_SNORM:
        case GL_RGB5_A1: case GL_RGBA4: case GL_RGB10_A2: case GL_RGB10_A2UI:
        case GL_RGBA16F: case GL_RGBA32F:
        case GL_RGBA8I: case GL_RGBA8UI: case GL_RGBA16I: case GL_RGBA16UI:
        case GL_RGBA32I: case GL_RGBA32UI:
            return 4;

        default: return 4;
    }
}

uint32_t CalculateTextureLevelSize(GLenum internalFormat, int width, int height, int depth) {
    uint32_t bpp = GetBytesPerPixel(internalFormat);
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (depth < 1) depth = 1;
    return static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * static_cast<uint32_t>(depth) * bpp;
}

uint32_t CalculateTextureTotalSize(GLenum internalFormat, int width, int height, int depth, int levels) {
    uint32_t totalSize = 0;
    int w = width;
    int h = height;
    int d = depth;

    for (int level = 0; level < levels; level++) {
        totalSize += CalculateTextureLevelSize(internalFormat, w, h, d);
        w = std::max(w / 2, 1);
        h = std::max(h / 2, 1);
        d = std::max(d / 2, 1);
    }

    return totalSize;
}

uint32_t GetAlignedRowSize(int width, uint32_t bytesPerPixel, int alignment) {
    if (alignment <= 0) alignment = 1;
    uint32_t rowSize = static_cast<uint32_t>(width) * bytesPerPixel;
    return ((rowSize + alignment - 1) / alignment) * alignment;
}

} // namespace Metrics
} // namespace MobileGL