#pragma once

#include "Types.h"

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace MGToGL {

GLenum ConvertTextureTargetToGLEnum(TextureTarget target);
GLenum ConvertTextureUploadTargetToGLEnum(TextureUploadTarget target);
GLenum ConvertTextureInternalFormatToGLEnum(TextureInternalFormat format);
GLenum ConvertTextureSwizzleParamToGLEnum(TextureSwizzleParam param);

} // namespace MGToGL
} // namespace Converters
} // namespace MobileGL