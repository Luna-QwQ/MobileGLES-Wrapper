#pragma once

#include "Types.h"

#include <cstdint>

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Texture {

// Pixel format normalization options
enum class PixelFormatNormalizeOptionBit : uint32_t {
    None = 0,
    NoRgb16 = 1 << 0,
    NoSnorm16 = 1 << 1,
    NoSnorm8 = 1 << 2,
    NoNorm16 = 1 << 3,
    NoRGBA8Snorm = 1 << 4,
    NoRGB16Snorm = 1 << 5,
    NoDepthComponent32 = 1 << 6,
};

// Result of pixel format normalization
struct NormalizedPixelFormat {
    GLenum internalFormat;  // Normalized internal format
    GLenum format;          // Format for pixel transfer
    GLenum type;            // Data type for pixel transfer
};

// Get applicable normalization options based on GLES capabilities
uint32_t GetApplicablePixelFormatNormalizeOptions();

// Normalize a pixel format: given an internal format, returns the best
// compatible format/type for GLES3
NormalizedPixelFormat NormalizePixelFormat(GLenum internalFormat, uint32_t options);

} // namespace Texture
} // namespace MobileGL