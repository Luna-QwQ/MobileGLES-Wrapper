#pragma once

#include "Types.h"

namespace MobileGL {
namespace Classifiers {

bool IsDepthFormatInternalFormat(TextureInternalFormat format);
bool IsStencilFormatInternalFormat(TextureInternalFormat format);

} // namespace Classifiers
} // namespace MobileGL