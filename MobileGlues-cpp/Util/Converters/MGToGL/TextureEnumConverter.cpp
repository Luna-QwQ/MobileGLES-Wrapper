#include "TextureEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToGL {

GLenum ConvertTextureTargetToGLEnum(TextureTarget target) {
    return static_cast<GLenum>(static_cast<uint32_t>(target));
}

GLenum ConvertTextureUploadTargetToGLEnum(TextureUploadTarget target) {
    return static_cast<GLenum>(static_cast<uint32_t>(target));
}

GLenum ConvertTextureInternalFormatToGLEnum(TextureInternalFormat format) {
    return static_cast<GLenum>(static_cast<uint32_t>(format));
}

} } } // namespace MobileGL::Converters::MGToGL