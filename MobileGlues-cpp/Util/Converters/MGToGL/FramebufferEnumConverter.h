#pragma once

#include "Types.h"

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace MGToGL {

GLenum ConvertFramebufferTargetToGLEnum(FramebufferTarget target);
GLenum ConvertFramebufferAttachmentTypeToGLEnum(FramebufferAttachmentType type);

} // namespace MGToGL
} // namespace Converters
} // namespace MobileGL