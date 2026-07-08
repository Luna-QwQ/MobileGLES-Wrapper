#include "FramebufferEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToGL {

GLenum ConvertFramebufferTargetToGLEnum(FramebufferTarget target) {
    return static_cast<GLenum>(static_cast<uint32_t>(target));
}

GLenum ConvertFramebufferAttachmentTypeToGLEnum(FramebufferAttachmentType type) {
    return static_cast<GLenum>(static_cast<uint32_t>(type));
}

} } } // namespace MobileGL::Converters::MGToGL