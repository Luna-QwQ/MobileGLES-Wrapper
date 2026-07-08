#include "TextureEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace GLToMG {

TextureTarget ConvertGLEnumToTextureTarget(GLenum glEnum) {
    return static_cast<TextureTarget>(static_cast<uint32_t>(glEnum));
}

TextureInternalFormat ConvertGLEnumToTextureInternalFormat(GLenum glEnum) {
    return static_cast<TextureInternalFormat>(static_cast<uint32_t>(glEnum));
}

} } } // namespace MobileGL::Converters::GLToMG