#pragma once

#include <cstdint>
#include <GLES3/gl3.h>
#include <EGL/egl.h>

namespace MobileGL {

// ========== Texture Types ==========
enum class TextureTarget : uint32_t {
    Texture1D = 0x0DE0,                    // GL_TEXTURE_1D
    Texture2D = GL_TEXTURE_2D,
    Texture3D = GL_TEXTURE_3D,
    TextureCubeMap = GL_TEXTURE_CUBE_MAP,
    Texture1DArray = 0x8C18,               // GL_TEXTURE_1D_ARRAY
    Texture2DArray = GL_TEXTURE_2D_ARRAY,
    TextureCubeMapArray = 0x9009,          // GL_TEXTURE_CUBE_MAP_ARRAY
    TextureRectangle = 0x84F5,             // GL_TEXTURE_RECTANGLE
    Texture2DMultisample = 0x9100,         // GL_TEXTURE_2D_MULTISAMPLE
    Texture2DMultisampleArray = 0x9102,    // GL_TEXTURE_2D_MULTISAMPLE_ARRAY
    TextureBuffer = 0x8C2A,                // GL_TEXTURE_BUFFER
    TextureTargetCount,
    Unknown = 0xFFFF
};

enum class TextureUploadTarget : uint32_t {
    Texture2D = GL_TEXTURE_2D,
    TextureCubeMapPositiveX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    TextureCubeMapNegativeX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    TextureCubeMapPositiveY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    TextureCubeMapNegativeY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    TextureCubeMapPositiveZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    TextureCubeMapNegativeZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    Texture3D = GL_TEXTURE_3D,
    Texture2DArray = GL_TEXTURE_2D_ARRAY,
    TextureCubeMapArray = 0x9009,          // GL_TEXTURE_CUBE_MAP_ARRAY
    Texture2DMultisample = 0x9100,         // GL_TEXTURE_2D_MULTISAMPLE
    Texture2DMultisampleArray = 0x9102,    // GL_TEXTURE_2D_MULTISAMPLE_ARRAY
    Unknown = 0xFFFF
};

enum class TextureStorageType : uint32_t {
    Mipmap = 0,
    Buffer = 1,
    Unknown = 0xFFFF
};

enum class TextureInternalFormat : uint32_t {
    Unknown = 0,
    // Base formats
    R8 = GL_R8,
    R8Snorm = GL_R8_SNORM,
    R16F = GL_R16F,
    R32F = GL_R32F,
    R8I = GL_R8I,
    R8UI = GL_R8UI,
    R16I = GL_R16I,
    R16UI = GL_R16UI,
    R32I = GL_R32I,
    R32UI = GL_R32UI,

    RG8 = GL_RG8,
    RG8Snorm = GL_RG8_SNORM,
    RG16F = GL_RG16F,
    RG32F = GL_RG32F,
    RG8I = GL_RG8I,
    RG8UI = GL_RG8UI,
    RG16I = GL_RG16I,
    RG16UI = GL_RG16UI,
    RG32I = GL_RG32I,
    RG32UI = GL_RG32UI,

    RGB8 = GL_RGB8,
    SRGB8 = GL_SRGB8,
    RGB8Snorm = GL_RGB8_SNORM,
    RGB16F = GL_RGB16F,
    RGB32F = GL_RGB32F,
    RGB8I = GL_RGB8I,
    RGB8UI = GL_RGB8UI,
    RGB16I = GL_RGB16I,
    RGB16UI = GL_RGB16UI,
    RGB32I = GL_RGB32I,
    RGB32UI = GL_RGB32UI,

    RGBA8 = GL_RGBA8,
    SRGB8Alpha8 = GL_SRGB8_ALPHA8,
    RGBA8Snorm = GL_RGBA8_SNORM,
    RGBA16F = GL_RGBA16F,
    RGBA32F = GL_RGBA32F,
    RGBA8I = GL_RGBA8I,
    RGBA8UI = GL_RGBA8UI,
    RGBA16I = GL_RGBA16I,
    RGBA16UI = GL_RGBA16UI,
    RGBA32I = GL_RGBA32I,
    RGBA32UI = GL_RGBA32UI,

    R11FG11FB10F = GL_R11F_G11F_B10F,
    RGB10A2 = GL_RGB10_A2,
    RGB10A2UI = GL_RGB10_A2UI,

    // Depth/Stencil
    DepthComponent16 = GL_DEPTH_COMPONENT16,
    DepthComponent24 = GL_DEPTH_COMPONENT24,
    DepthComponent32F = GL_DEPTH_COMPONENT32F,
    Depth24Stencil8 = GL_DEPTH24_STENCIL8,
    Depth32FStencil8 = GL_DEPTH32F_STENCIL8,
    StencilIndex8 = GL_STENCIL_INDEX8,

    // Desktop GL formats (use raw values)
    R16 = 0x822A,               // GL_R16
    R16Snorm = 0x8F98,          // GL_R16_SNORM
    RG16 = 0x822C,              // GL_RG16
    RG16Snorm = 0x8F99,         // GL_RG16_SNORM
    RGB16 = 0x8054,             // GL_RGB16
    RGB16Snorm = 0x8F9A,        // GL_RGB16_SNORM
    RGBA16 = 0x805B,            // GL_RGBA16
    RGBA16Snorm = 0x8F9B,       // GL_RGBA16_SNORM
    DepthComponent32 = 0x81A7,  // GL_DEPTH_COMPONENT32

    TextureInternalFormatCount,
};

enum class TextureSwizzleParam : uint32_t {
    Red = GL_RED,
    Green = GL_GREEN,
    Blue = GL_BLUE,
    Alpha = GL_ALPHA,
    Zero = GL_ZERO,
    One = GL_ONE,
    Unknown = 0xFFFF
};

// ========== Buffer Types ==========
enum class BufferTarget : uint32_t {
    Array = GL_ARRAY_BUFFER,
    AtomicCounter = 0x92C0,         // GL_ATOMIC_COUNTER_BUFFER
    CopyRead = GL_COPY_READ_BUFFER,
    CopyWrite = GL_COPY_WRITE_BUFFER,
    DispatchIndirect = 0x90EE,      // GL_DISPATCH_INDIRECT_BUFFER
    DrawIndirect = 0x8F3F,          // GL_DRAW_INDIRECT_BUFFER
    ElementArray = GL_ELEMENT_ARRAY_BUFFER,
    PixelPack = GL_PIXEL_PACK_BUFFER,
    PixelUnpack = GL_PIXEL_UNPACK_BUFFER,
    Query = 0x9192,                 // GL_QUERY_BUFFER
    ShaderStorage = 0x90D2,         // GL_SHADER_STORAGE_BUFFER
    Texture = 0x8C2A,               // GL_TEXTURE_BUFFER
    TransformFeedback = GL_TRANSFORM_FEEDBACK_BUFFER,
    Uniform = GL_UNIFORM_BUFFER,
    BufferTargetCount,
    Unknown = 0xFFFF
};

enum class BufferUsage : uint32_t {
    StreamDraw = GL_STREAM_DRAW,
    StreamRead = GL_STREAM_READ,
    StreamCopy = GL_STREAM_COPY,
    StaticDraw = GL_STATIC_DRAW,
    StaticRead = GL_STATIC_READ,
    StaticCopy = GL_STATIC_COPY,
    DynamicDraw = GL_DYNAMIC_DRAW,
    DynamicRead = GL_DYNAMIC_READ,
    DynamicCopy = GL_DYNAMIC_COPY,
    Unknown = 0xFFFF
};

enum class BufferChangeBits : uint32_t {
    None = 0,
    DirtyBit = 1 << 0,
    PreferReallocationBit = 1 << 1,
    ForbidInvalidationBit = 1 << 2,
    ForbidUnsynchronizationBit = 1 << 3
};

inline BufferChangeBits operator|(BufferChangeBits a, BufferChangeBits b) {
    return static_cast<BufferChangeBits>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferChangeBits& operator|=(BufferChangeBits& a, BufferChangeBits b) {
    a = a | b;
    return a;
}

inline BufferChangeBits operator&(BufferChangeBits a, BufferChangeBits b) {
    return static_cast<BufferChangeBits>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool operator!(BufferChangeBits a) {
    return static_cast<uint32_t>(a) == 0;
}

// ========== Data Types ==========
enum class DataType : uint32_t {
    Int8 = GL_BYTE,
    Uint8 = GL_UNSIGNED_BYTE,
    Int16 = GL_SHORT,
    Uint16 = GL_UNSIGNED_SHORT,
    Int32 = GL_INT,
    Uint32 = GL_UNSIGNED_INT,
    Float16 = GL_HALF_FLOAT,
    Float32 = GL_FLOAT,
    Float64 = 0x140A,               // GL_DOUBLE (desktop)
    Fixed32 = GL_FIXED,
    Unknown = 0xFFFF
};

// ========== Framebuffer Types ==========
enum class FramebufferTarget : uint32_t {
    Read = GL_READ_FRAMEBUFFER,
    Draw = GL_DRAW_FRAMEBUFFER,
    FramebufferTargetCount,
    Unknown = 0xFFFF
};

enum class FramebufferAttachmentType : uint32_t {
    None = 0,
    Color0 = GL_COLOR_ATTACHMENT0,
    Color1 = GL_COLOR_ATTACHMENT1,
    Color2 = GL_COLOR_ATTACHMENT2,
    Color3 = GL_COLOR_ATTACHMENT3,
    Color4 = GL_COLOR_ATTACHMENT4,
    Color5 = GL_COLOR_ATTACHMENT5,
    Color6 = GL_COLOR_ATTACHMENT6,
    Color7 = GL_COLOR_ATTACHMENT7,
    Color8 = GL_COLOR_ATTACHMENT8,
    Color9 = GL_COLOR_ATTACHMENT9,
    Color10 = GL_COLOR_ATTACHMENT10,
    Color11 = GL_COLOR_ATTACHMENT11,
    Color12 = GL_COLOR_ATTACHMENT12,
    Color13 = GL_COLOR_ATTACHMENT13,
    Color14 = GL_COLOR_ATTACHMENT14,
    Color15 = GL_COLOR_ATTACHMENT15,
    Color16 = 0x8CF0,               // GL_COLOR_ATTACHMENT16
    Color17 = 0x8CF1,               // GL_COLOR_ATTACHMENT17
    Color18 = 0x8CF2,               // GL_COLOR_ATTACHMENT18
    Color19 = 0x8CF3,               // GL_COLOR_ATTACHMENT19
    Color20 = 0x8CF4,               // GL_COLOR_ATTACHMENT20
    Color21 = 0x8CF5,               // GL_COLOR_ATTACHMENT21
    Color22 = 0x8CF6,               // GL_COLOR_ATTACHMENT22
    Color23 = 0x8CF7,               // GL_COLOR_ATTACHMENT23
    Color24 = 0x8CF8,               // GL_COLOR_ATTACHMENT24
    Color25 = 0x8CF9,               // GL_COLOR_ATTACHMENT25
    Color26 = 0x8CFA,               // GL_COLOR_ATTACHMENT26
    Color27 = 0x8CFB,               // GL_COLOR_ATTACHMENT27
    Color28 = 0x8CFC,               // GL_COLOR_ATTACHMENT28
    Color29 = 0x8CFD,               // GL_COLOR_ATTACHMENT29
    Color30 = 0x8CFE,               // GL_COLOR_ATTACHMENT30
    Color31 = 0x8CFF,               // GL_COLOR_ATTACHMENT31
    Depth = GL_DEPTH_ATTACHMENT,
    Stencil = GL_STENCIL_ATTACHMENT,
    DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
    FrontLeft = 0x0400,             // GL_FRONT_LEFT
    FrontRight = 0x0401,            // GL_FRONT_RIGHT
    BackLeft = 0x0402,              // GL_BACK_LEFT
    BackRight = 0x0403,             // GL_BACK_RIGHT
    Unknown = 0xFFFF
};

// ========== Shader/Program Types ==========
enum class ShaderStage : uint32_t {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = 0x8DD9,              // GL_GEOMETRY_SHADER (desktop GL)
    Compute = 0x91B9,               // GL_COMPUTE_SHADER
    TessControl = 0x8E88,           // GL_TESS_CONTROL_SHADER
    TessEvaluation = 0x8E87,        // GL_TESS_EVALUATION_SHADER
    Unknown = 0xFFFF
};

// ========== Sampler Types ==========
enum class SamplerFilterMode : uint32_t {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR,
    Unknown = 0xFFFF
};

enum class SamplerMipmapMode : uint32_t {
    None = 0,
    Nearest = GL_NEAREST_MIPMAP_NEAREST,
    Linear = GL_LINEAR_MIPMAP_LINEAR,
    Unknown = 0xFFFF
};

enum class SamplerWrapMode : uint32_t {
    ClampToEdge = GL_CLAMP_TO_EDGE,
    ClampToBorder = 0x812D,         // GL_CLAMP_TO_BORDER
    Repeat = GL_REPEAT,
    MirroredRepeat = GL_MIRRORED_REPEAT,
    MirrorClampToEdge = 0x8743,     // GL_MIRROR_CLAMP_TO_EDGE
    Unknown = 0xFFFF
};

enum class SamplerCompareMode : uint32_t {
    None = GL_NONE,
    CompareRefToTexture = GL_COMPARE_REF_TO_TEXTURE,
    Unknown = 0xFFFF
};

enum class SamplerCompareFunc : uint32_t {
    Never = GL_NEVER,
    Less = GL_LESS,
    Equal = GL_EQUAL,
    Lequal = GL_LEQUAL,
    Greater = GL_GREATER,
    Notequal = GL_NOTEQUAL,
    Gequal = GL_GEQUAL,
    Always = GL_ALWAYS,
    Unknown = 0xFFFF
};

// ========== Error Codes ==========
enum class GLError : uint32_t {
    NoError = GL_NO_ERROR,
    InvalidEnum = GL_INVALID_ENUM,
    InvalidValue = GL_INVALID_VALUE,
    InvalidOperation = GL_INVALID_OPERATION,
    StackOverflow = 0x0503,             // GL_STACK_OVERFLOW
    StackUnderflow = 0x0504,            // GL_STACK_UNDERFLOW
    OutOfMemory = GL_OUT_OF_MEMORY,
    InvalidFramebufferOperation = GL_INVALID_FRAMEBUFFER_OPERATION,
    ContextLost = 0x0507                // GL_CONTEXT_LOST
};

} // namespace MobileGL