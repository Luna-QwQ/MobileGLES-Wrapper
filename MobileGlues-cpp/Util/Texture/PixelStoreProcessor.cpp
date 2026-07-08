#include "PixelStoreProcessor.h"

namespace MobileGL {
namespace Texture {

void PixelStoreProcessor::SetPixelStorei(GLenum pname, GLint param) {
    switch (pname) {
        case GL_UNPACK_ALIGNMENT:
            m_state.unpackAlignment = param;
            break;
        case GL_PACK_ALIGNMENT:
            m_state.packAlignment = param;
            break;
        case GL_UNPACK_ROW_LENGTH:
            m_state.unpackRowLength = param;
            break;
        case GL_UNPACK_IMAGE_HEIGHT:
            m_state.unpackImageHeight = param;
            break;
        case GL_UNPACK_SKIP_PIXELS:
            m_state.unpackSkipPixels = param;
            break;
        case GL_UNPACK_SKIP_ROWS:
            m_state.unpackSkipRows = param;
            break;
        case GL_UNPACK_SKIP_IMAGES:
            m_state.unpackSkipImages = param;
            break;
        case GL_PACK_ROW_LENGTH:
            m_state.packRowLength = param;
            break;
        case GL_PACK_SKIP_PIXELS:
            m_state.packSkipPixels = param;
            break;
        case GL_PACK_SKIP_ROWS:
            m_state.packSkipRows = param;
            break;
        default:
            break;
    }
}

GLint PixelStoreProcessor::GetPixelStorei(GLenum pname) const {
    switch (pname) {
        case GL_UNPACK_ALIGNMENT:     return m_state.unpackAlignment;
        case GL_PACK_ALIGNMENT:       return m_state.packAlignment;
        case GL_UNPACK_ROW_LENGTH:    return m_state.unpackRowLength;
        case GL_UNPACK_IMAGE_HEIGHT:  return m_state.unpackImageHeight;
        case GL_UNPACK_SKIP_PIXELS:   return m_state.unpackSkipPixels;
        case GL_UNPACK_SKIP_ROWS:     return m_state.unpackSkipRows;
        case GL_UNPACK_SKIP_IMAGES:   return m_state.unpackSkipImages;
        case GL_PACK_ROW_LENGTH:      return m_state.packRowLength;
        case GL_PACK_SKIP_PIXELS:     return m_state.packSkipPixels;
        case GL_PACK_SKIP_ROWS:       return m_state.packSkipRows;
        default:                      return 0;
    }
}

void PixelStoreProcessor::Reset() {
    m_state = PixelStoreState{};
}

} // namespace Texture
} // namespace MobileGL