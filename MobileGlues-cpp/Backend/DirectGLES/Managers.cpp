#include "Managers.h"

namespace MobileGL { namespace MG_Backend {

// Global registries
namespace BufferImpl {
    StateBackendObjectRegistry<BackendBufferObject> g_BufferRegistry;
    GLuint g_CurrentBoundBufferTargets[32] = {0};
}

BackendBufferObject::BackendBufferObject() : backendBufferId(0), target(GL_ARRAY_BUFFER), dirtyBits(0), synced(false) {}
BackendBufferObject::~BackendBufferObject() {}
void BackendBufferObject::Bind(GLenum bindTarget) { target = bindTarget; (void)bindTarget; }

namespace VertexArrayImpl {
    StateBackendObjectRegistry<BackendVertexArrayObject> g_VAORegistry;
    GLuint g_CurrentBoundVAO = 0;
}

BackendVertexArrayObject::BackendVertexArrayObject() : backendVAOId(0), dirtyBits(0), synced(false), hasClientSideData(false) {}
BackendVertexArrayObject::~BackendVertexArrayObject() {}
void BackendVertexArrayObject::Bind() {}

namespace TextureImpl {
    StateBackendObjectRegistry<BackendTextureObject> g_TextureRegistry;
    GLuint g_ActiveTextureUnit = 0;
    GLuint g_CurrentBoundTextures[32] = {0};
}

BackendTextureObject::BackendTextureObject() : backendTextureId(0), target(GL_TEXTURE_2D), dirtyBits(0), synced(false), immutableStorage(false), isImageBindable(false) {}
BackendTextureObject::~BackendTextureObject() {}
void BackendTextureObject::Bind(GLenum bindTarget) { target = bindTarget; (void)bindTarget; }

namespace FramebufferImpl {
    StateBackendObjectRegistry<BackendFramebufferObject> g_FBORegistry;
    GLuint g_CurrentBoundDrawFBO = 0;
    GLuint g_CurrentBoundReadFBO = 0;
}

BackendFramebufferObject::BackendFramebufferObject() : backendFBOId(0), dirtyBits(0), synced(false), isDefaultFBO(false) {}
BackendFramebufferObject::~BackendFramebufferObject() {}
void BackendFramebufferObject::Bind(GLenum target) { (void)target; }

namespace RenderStateImpl {
    bool g_RenderStateDirty = true;
    bool g_BlendStateDirty = true;
    bool g_DepthStencilStateDirty = true;
    bool g_RasterStateDirty = true;
    bool g_ViewportStateDirty = true;
}

namespace PrgramImpl {
    StateBackendObjectRegistry<BackendProgramObjectImpl> g_ProgramRegistry;
    GLuint g_CurrentBoundProgram = 0;
}

BackendProgramObjectImpl::BackendProgramObjectImpl() : backendProgramId(0), dirtyBits(0), synced(false), linked(false), baseInstance(0) {}
BackendProgramObjectImpl::~BackendProgramObjectImpl() {}
void BackendProgramObjectImpl::Use() {}
void BackendProgramObjectImpl::SetBaseInstance(GLuint instance) { baseInstance = static_cast<GLint>(instance); }

namespace SamplerImpl {
    StateBackendObjectRegistry<BackendSamplerObject> g_SamplerRegistry;
}

BackendSamplerObject::BackendSamplerObject() : backendSamplerId(0), dirtyBits(0), synced(false) {}
BackendSamplerObject::~BackendSamplerObject() {}
void BackendSamplerObject::Bind(GLuint unit) { (void)unit; }

namespace RenderbufferImpl {
    StateBackendObjectRegistry<BackendRenderbufferObject> g_RBORegistry;
}

BackendRenderbufferObject::BackendRenderbufferObject() : backendRBOId(0), dirtyBits(0), synced(false) {}
BackendRenderbufferObject::~BackendRenderbufferObject() {}
void BackendRenderbufferObject::Bind() {}

} } // namespace MobileGL::MG_Backend