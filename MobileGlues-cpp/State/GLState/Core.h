#pragma once
#include "Includes.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// Re-export types from the MobileGL namespace (defined in Types.h)
// ============================================================================

using MobileGL::TextureTarget;
using MobileGL::TextureInternalFormat;
using MobileGL::TextureUploadTarget;
using MobileGL::TextureStorageType;
using MobileGL::TextureSwizzleParam;
using MobileGL::SamplerFilterMode;
using MobileGL::SamplerMipmapMode;
using MobileGL::SamplerWrapMode;
using MobileGL::SamplerCompareMode;
using MobileGL::SamplerCompareFunc;
using MobileGL::BufferTarget;
using MobileGL::BufferUsage;
using MobileGL::BufferChangeBits;
using MobileGL::DataType;
using MobileGL::FramebufferTarget;
using MobileGL::FramebufferAttachmentType;
using MobileGL::ShaderStage;
using MobileGL::GLError;

// ============================================================================
// Forward declarations of all state objects
// ============================================================================

class BufferObject;
class VertexArrayObject;
class ITextureObject;
class FramebufferObject;
class ProgramObject;
class ShaderObject;
class SamplerObject;
class RenderbufferObject;
struct TextureUnit;

// ============================================================================
// Sampler parameters structure
// ============================================================================

struct SamplerParameters {
    SamplerFilterMode minFilter = SamplerFilterMode::Nearest;
    SamplerFilterMode magFilter = SamplerFilterMode::Nearest;
    SamplerMipmapMode mipmapMode = SamplerMipmapMode::None;
    SamplerWrapMode wrapS = SamplerWrapMode::Repeat;
    SamplerWrapMode wrapT = SamplerWrapMode::Repeat;
    SamplerWrapMode wrapR = SamplerWrapMode::Repeat;
    SamplerCompareMode compareMode = SamplerCompareMode::None;
    SamplerCompareFunc compareFunc = SamplerCompareFunc::Less;
    Float minLod = -1000.0f;
    Float maxLod = 1000.0f;
};

// ============================================================================
// Vertex attribute structures
// ============================================================================

struct VertexAttribute {
    Bool Enabled = false;
    Int Size = 4;
    DataType Type = DataType::Float32;
    Bool Normalized = false;
    Int Stride = 0;
    SizeT Offset = 0;
    Int Divisor = 0;
    Bool IsInteger = false;
    SharedPtr<BufferObject> Buffer;
};

struct VertexAttributeVersion {
    Uint16 SwitchVersion = 0;
    Uint16 FormatVersion = 0;
    Uint16 BufferVersion = 0;
};

// ============================================================================
// Render state parameters
// ============================================================================

struct RenderStateParameters {
    Bool DepthTestEnabled = false;
    Bool StencilTestEnabled = false;
    Bool BlendEnabled = false;
    Bool CullFaceEnabled = false;
    Bool ScissorTestEnabled = false;
    Bool PolygonOffsetFillEnabled = false;
    IntVec4<Int> Viewport = IntVec4<Int>{0, 0, 0, 0};
};

// ============================================================================
// GL context (opaque-ish)
// ============================================================================

struct GLContext {
    Uint32 externalContextId = 0;
    Bool isES = true;
    Int majorVersion = 3;
    Int minorVersion = 0;
    String vendorString;
    String rendererString;
    String versionString;
    Vector<String> extensions;
};

extern GLContext* pGLContext;

} } } // namespace MobileGL::MG_State::GLState