#include "GLEnumConverter.h"
#include "Includes.h"

namespace MobileGL { namespace Converters { namespace GLToStr {

std::string ConvertGLEnumToString(GLenum glEnum) {
    return "GLEnum_0x" + std::to_string(static_cast<uint32_t>(glEnum));
}

} } } // namespace MobileGL::Converters::GLToStr