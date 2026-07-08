#pragma once
#include "DirectGLES.h"
#include <GLES3/gl3.h>

namespace MobileGL { namespace MG_Backend {

// Backend buffer object
struct BackendBufferObject {
    GLuint backendBufferId;
    GLenum target;
    Uint32 dirtyBits;
    Bool synced;

    BackendBufferObject();
    ~BackendBufferObject();
    void SyncToBackend(const struct BufferState& state);
    void Bind(GLenum bindTarget);
};

#define BUFFER_CHANGE_NONE 0
#define BUFFER_CHANGE_ALL 0xFFFFFFFF
#define BUFFER_CHANGE_DATA (1 << 0)
#define BUFFER_CHANGE_MAP (1 << 1)
#define BUFFER_CHANGE_USAGE (1 << 2)

// Backend vertex array object
struct BackendVertexArrayObject {
    GLuint backendVAOId;
    Uint32 dirtyBits;
    Bool synced;
    Bool hasClientSideData;

    BackendVertexArrayObject();
    ~BackendVertexArrayObject();
    void SyncToBackend(const struct VertexArrayState& state);
    void SyncClientSideAttributesForDrawArrays(const struct VertexArrayState& state);
    void Bind();
};

#define VAO_CHANGE_NONE 0
#define VAO_CHANGE_ALL 0xFFFFFFFF
#define VAO_CHANGE_ATTRIB_FORMAT (1 << 0)
#define VAO_CHANGE_ATTRIB_BINDING (1 << 1)
#define VAO_CHANGE_ATTRIB_ENABLE (1 << 2)
#define VAO_CHANGE_ELEMENT_BUFFER (1 << 3)

// Backend texture object
struct BackendTextureObject {
    GLuint backendTextureId;
    GLenum target;
    Uint32 dirtyBits;
    Bool synced;
    Bool immutableStorage;
    Bool isImageBindable;

    BackendTextureObject();
    ~BackendTextureObject();
    void SyncToBackend(const struct TextureState& state);
    void SyncMipmapsToBackend(const struct TextureState& state);
    void SyncBuiltinSamplerToBackend(const struct TextureState& state);
    void SyncTextureParamsToBackend(const struct TextureState& state);
    void RequireImageBindableStorage(const struct TextureState& state);
    void Bind(GLenum bindTarget);
};

#define TEXTURE_CHANGE_NONE 0
#define TEXTURE_CHANGE_ALL 0xFFFFFFFF
#define TEXTURE_CHANGE_IMAGE_DATA (1 << 0)
#define TEXTURE_CHANGE_MIPMAPS (1 << 1)
#define TEXTURE_CHANGE_PARAMETERS (1 << 2)
#define TEXTURE_CHANGE_SAMPLER_OBJECT (1 << 3)
#define TEXTURE_CHANGE_LEVEL_RANGE (1 << 4)
#define TEXTURE_CHANGE_SWIZZLE (1 << 5)
#define TEXTURE_CHANGE_BORDER_COLOR (1 << 6)
#define TEXTURE_CHANGE_BUFFER_TEXTURE (1 << 7)

// Backend framebuffer object
struct BackendFramebufferObject {
    GLuint backendFBOId;
    Uint32 dirtyBits;
    Bool synced;
    Bool isDefaultFBO;

    BackendFramebufferObject();
    ~BackendFramebufferObject();
    void SyncToBackend(const struct FramebufferState& state);
    void InvalidateSyncedState();
    void Bind(GLenum target);
};

enum BackendAttachmentType {
    ATTACHMENT_NONE = 0,
    ATTACHMENT_TEXTURE = 1,
    ATTACHMENT_RENDERBUFFER = 2
};

#define FBO_CHANGE_NONE 0
#define FBO_CHANGE_ALL 0xFFFFFFFF
#define FBO_CHANGE_ATTACHMENTS (1 << 0)
#define FBO_CHANGE_DRAW_BUFFERS (1 << 1)
#define FBO_CHANGE_READ_BUFFER (1 << 2)
#define FBO_CHANGE_PARAMETERS (1 << 3)

// Backend program object
struct BackendProgramObjectImpl {
    GLuint backendProgramId;
    Uint32 dirtyBits;
    Bool synced;
    Bool linked;
    GLint baseInstance;

    BackendProgramObjectImpl();
    ~BackendProgramObjectImpl();
    void SyncToBackend(const struct ProgramState& state);
    void Use();
    void SetBaseInstance(GLuint instance);
};

#define PROGRAM_CHANGE_NONE 0
#define PROGRAM_CHANGE_ALL 0xFFFFFFFF
#define PROGRAM_CHANGE_SHADER_SOURCE (1 << 0)
#define PROGRAM_CHANGE_LINK (1 << 1)
#define PROGRAM_CHANGE_UNIFORM_BLOCKS (1 << 2)

// Backend sampler object
struct BackendSamplerObject {
    GLuint backendSamplerId;
    Uint32 dirtyBits;
    Bool synced;

    BackendSamplerObject();
    ~BackendSamplerObject();
    void SyncToBackend(const struct SamplerState& state);
    void Bind(GLuint unit);
};

#define SAMPLER_CHANGE_NONE 0
#define SAMPLER_CHANGE_ALL 0xFFFFFFFF

// Backend renderbuffer object
struct BackendRenderbufferObject {
    GLuint backendRBOId;
    Uint32 dirtyBits;
    Bool synced;

    BackendRenderbufferObject();
    ~BackendRenderbufferObject();
    void SyncToBackend(const struct RenderbufferState& state);
    void Bind();
};

#define RBO_CHANGE_NONE 0
#define RBO_CHANGE_ALL 0xFFFFFFFF

// State backend object registry
template <typename T>
struct StateBackendObjectRegistry {
    HashMap<GLuint, T*> registry;
    T* Get(GLuint id) { auto it = registry.find(id); return it != registry.end() ? it->second : nullptr; }
    void Set(GLuint id, T* obj) { registry[id] = obj; }
};

// GLES capabilities
struct GLESCapabilities {
    Bool hasTextureFilterAnisotropic = false;
    Bool hasTextureBorderClamp = false;
    Bool hasFramebufferMultisample = false;
    Int maxImageUnits = 0;
    Uint8 textureFormatSupport[256] = {};
    Uint8 renderableFormatSupport[256] = {};
};

extern GLESCapabilities g_GLESCapabilities;

// Forward declarations of state types used by backend
struct BufferState;
struct VertexArrayState;
struct TextureState;
struct FramebufferState;
struct ProgramState;
struct SamplerState;
struct RenderbufferState;

} } // namespace MobileGL::MG_Backend