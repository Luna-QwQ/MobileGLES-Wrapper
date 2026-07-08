#include "TextureEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToStr {

std::string ConvertTextureTargetToString(TextureTarget target) {
    return "TextureTarget_" + std::to_string(static_cast<uint32_t>(target));
}

std::string ConvertTextureInternalFormatToString(TextureInternalFormat format) {
    return "TextureInternalFormat_" + std::to_string(static_cast<uint32_t>(format));
}

} } } // namespace MobileGL::Converters::MGToStr