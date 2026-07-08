#pragma once

#include <cstdint>

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Metrics {

// Texture size calculation utilities
struct TextureSize {
    int width = 0;
    int height = 0;
    int depth = 0;
};

// Calculate the size in bytes of a texture level
uint32_t CalculateTextureLevelSize(GLenum internalFormat, int width, int height, int depth);

// Calculate the total size of all mipmap levels
uint32_t CalculateTextureTotalSize(GLenum internalFormat, int width, int height, int depth, int levels);

// Get the number of bytes per pixel for a given internal format
uint32_t GetBytesPerPixel(GLenum internalFormat);

// Get the number of components for a given internal format
int GetComponentCount(GLenum internalFormat);

// Calculate aligned row size
uint32_t GetAlignedRowSize(int width, uint32_t bytesPerPixel, int alignment);

} // namespace Metrics
} // namespace MobileGL