#pragma once

#include <cstdint>

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Texture {

// Simple pixel store state management
struct PixelStoreState {
    int unpackAlignment = 4;
    int packAlignment = 4;
    int unpackRowLength = 0;
    int unpackImageHeight = 0;
    int unpackSkipPixels = 0;
    int unpackSkipRows = 0;
    int unpackSkipImages = 0;
    int packRowLength = 0;
    int packSkipPixels = 0;
    int packSkipRows = 0;
};

class PixelStoreProcessor {
public:
    PixelStoreProcessor() = default;
    ~PixelStoreProcessor() = default;

    void SetPixelStorei(GLenum pname, GLint param);
    GLint GetPixelStorei(GLenum pname) const;

    const PixelStoreState& GetState() const { return m_state; }
    void Reset();

private:
    PixelStoreState m_state;
};

} // namespace Texture
} // namespace MobileGL