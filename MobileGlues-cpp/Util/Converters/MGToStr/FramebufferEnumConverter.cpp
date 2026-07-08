#include "FramebufferEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace MGToStr {

std::string ConvertFramebufferTargetToString(FramebufferTarget target) {
    return "FramebufferTarget_" + std::to_string(static_cast<uint32_t>(target));
}

} } } // namespace MobileGL::Converters::MGToStr