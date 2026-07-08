#include "BufferEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToGL {

GLenum ConvertBufferTargetToGLEnum(BufferTarget target) {
    return static_cast<GLenum>(static_cast<uint32_t>(target));
}

GLenum ConvertBufferUsageToGLEnum(BufferUsage usage) {
    return static_cast<GLenum>(static_cast<uint32_t>(usage));
}

} } } // namespace MobileGL::Converters::MGToGL