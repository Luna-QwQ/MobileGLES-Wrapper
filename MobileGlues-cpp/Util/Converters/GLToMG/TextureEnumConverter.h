#pragma once

#include "Types.h"

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace GLToMG {

TextureTarget ConvertGLEnumToTextureTarget(GLenum value);
TextureInternalFormat ConvertGLEnumToTextureInternalFormat(GLenum value);

} // namespace GLToMG
} // namespace Converters
} // namespace MobileGL