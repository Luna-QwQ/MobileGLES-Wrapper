#pragma once

#include "Types.h"

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace MGToGL {

GLenum ConvertBufferUsageToGLEnum(BufferUsage usage);
GLenum ConvertDataTypeToGLEnum(DataType type);

} // namespace MGToGL
} // namespace Converters
} // namespace MobileGL