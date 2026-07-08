#include "ProgramEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToGL {

GLenum ConvertShaderStageToGLEnum(ShaderStage stage) {
    return static_cast<GLenum>(static_cast<uint32_t>(stage));
}

} } } // namespace MobileGL::Converters::MGToGL