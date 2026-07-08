#pragma once

#include "Types.h"

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace MGToGL {

GLenum ConvertShaderStageToGLEnum(ShaderStage stage);
GLenum ConvertSamplerFilterModeToGLEnum(SamplerFilterMode filter, SamplerMipmapMode mipmap);
GLenum ConvertSamplerWrapModeToGLEnum(SamplerWrapMode wrap);
GLenum ConvertSamplerCompareModeToGLEnum(SamplerCompareMode mode);
GLenum ConvertSamplerCompareFuncToGLEnum(SamplerCompareFunc func);

} // namespace MGToGL
} // namespace Converters
} // namespace MobileGL