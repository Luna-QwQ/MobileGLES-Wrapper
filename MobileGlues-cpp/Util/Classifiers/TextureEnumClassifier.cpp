#include "TextureEnumClassifier.h"

namespace MobileGL {
namespace Classifiers {

bool IsDepthFormatInternalFormat(TextureInternalFormat format) {
    switch (format) {
        case TextureInternalFormat::DepthComponent16:
        case TextureInternalFormat::DepthComponent24:
        case TextureInternalFormat::DepthComponent32F:
        case TextureInternalFormat::Depth24Stencil8:
        case TextureInternalFormat::Depth32FStencil8:
            return true;
        default:
            return false;
    }
}

bool IsStencilFormatInternalFormat(TextureInternalFormat format) {
    switch (format) {
        case TextureInternalFormat::StencilIndex8:
        case TextureInternalFormat::Depth24Stencil8:
        case TextureInternalFormat::Depth32FStencil8:
            return true;
        default:
            return false;
    }
}

} // namespace Classifiers
} // namespace MobileGL