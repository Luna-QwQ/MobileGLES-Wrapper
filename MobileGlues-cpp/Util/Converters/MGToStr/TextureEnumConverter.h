#pragma once

#include "Types.h"

#include <string>

namespace MobileGL {
namespace Converters {
namespace MGToStr {

std::string ConvertTextureTargetToString(TextureTarget target);
std::string ConvertTextureInternalFormatToString(TextureInternalFormat format);
std::string ConvertTextureUploadTargetToString(TextureUploadTarget target);

} // namespace MGToStr
} // namespace Converters
} // namespace MobileGL