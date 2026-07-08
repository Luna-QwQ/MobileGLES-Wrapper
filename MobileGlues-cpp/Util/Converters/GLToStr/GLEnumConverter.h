#pragma once

#include <string>

extern "C" {
#include <GLES3/gl3.h>
}

namespace MobileGL {
namespace Converters {
namespace GLToStr {

std::string ConvertGLEnumToString(GLenum value);

} // namespace GLToStr
} // namespace Converters
} // namespace MobileGL