// MobileGlues - gl/backend/Init.cpp
// Backend initialization entry point
//
// Architecture: "CPU-GPU 平权共生 Context"
//   CPU和GPU在GLContext中完全平等。Initialize()必须同时接收两者，
//   Shutdown()同时释放两者。不存在"先GPU后CPU"或"GPU优先CPU附属"的模式。
//   全局pActiveBackendObject保留为向后兼容的非拥有引用。
//
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "BackendObject.h"
#include "DirectGLES/DirectGLES.h"
#include "DirectGLES/BackendObject_DirectGLES.h"
#include "../state/Core.h"

namespace MobileGL::MG_Backend {

// Legacy global: 向后兼容的非拥有引用，实际所有权在GLContext中
GlobalBackendFunctionsTable gBackendFunctionsTable = {};
UniquePtr<BackendObject> pActiveBackendObject = nullptr;

} // namespace MobileGL::MG_Backend

namespace MobileGL::backend::DirectGLES {

EGLFunctionsTable g_EGLFuncs = {};
GLESFunctionsTable g_GLESFuncs = {};
GLESCapabilities g_GLESCapabilities = {};

// Helper: 从平权共生Context获取后端
static MG_Backend::BackendObject* GetActiveBackend() {
    auto& ctx = MG_State::GLState::GLContext::Get();
    return ctx.GetBackend();
}

// Backend object registry instances
namespace BufferImpl {
StateBackendObjectRegistry<MG_State::GLState::BufferObject, BackendBufferObject> g_backendBufferObjects;
}

namespace VertexArrayImpl {
StateBackendObjectRegistry<MG_State::GLState::VertexArrayObject, BackendVertexArrayObject>
    g_backendVertexArrayObjects;
}

namespace TextureImpl {
StateBackendObjectRegistry<MG_State::GLState::ITextureObject, BackendTextureObject>
    g_backendTextureObjects;
Array<Array<BackendTextureObject*, (SizeT)TextureTarget::TextureTargetCount>,
      MG_State::GLState::TextureState::MAX_TEXTURE_IMAGE_UNITS>
    g_boundTexturesCache = {};
Uint g_activeTextureUnit = 0;
}

namespace FramebufferImpl {
StateBackendObjectRegistry<MG_State::GLState::FramebufferObject, BackendFramebufferObject>
    g_backendFramebufferObjects;
Array<Uint16, SizeT(FramebufferTarget::FramebufferTargetCount)> g_fboBindVersions = {};
}

namespace ProgramImpl {
Uint32 g_snormFallbackClampOutputMask = 0;
Uint32 g_unormFallbackClampOutputMask = 0;
StateBackendObjectRegistry<MG_State::GLState::ProgramObject, BackendProgramObjectImpl>
    g_backendProgramObjects;
}

namespace SamplerImpl {
Array<BackendSamplerObject*, MG_State::GLState::TextureState::MAX_TEXTURE_IMAGE_UNITS>
    g_boundSamplersCache = {};
StateBackendObjectRegistry<MG_State::GLState::SamplerObject, BackendSamplerObject>
    g_backendSamplerObjects;
}

namespace RenderbufferImpl {
StateBackendObjectRegistry<MG_State::GLState::RenderbufferObject, BackendRenderbufferObject>
    g_backendRenderbufferObjects;
}

// =============================================================================
// 平权共生初始化：CPU和GPU同时创建，缺一不可
// =============================================================================

Bool InitWindowSurface(NativeWindowType window) {
    auto& ctx = MG_State::GLState::GLContext::Get();

    // 如果尚未初始化，CPU和GPU同时创建
    if (!ctx.IsInitialized()) {
        auto backend = UniquePtr<BackendObject_DirectGLES>(new BackendObject_DirectGLES());
        // Initialize() 同时初始化CPU状态和GPU后端——两者平权，不可分离
        ctx.Initialize(std::move(backend));
    }

    auto* backend = ctx.GetBackend();
    if (!backend) return false;

    // 向后兼容：更新全局引用（非拥有）
    MG_Backend::pActiveBackendObject.reset(backend);

    backend->SetWindowHandle(
        MG_Backend::WindowHandle{
            .Backend = MG_Backend::WindowBackend::Android,
            .Handle = window,
            .Width = 0,
            .Height = 0
        });
    return backend->InitWindowSurface();
}

Bool InitPbufferSurface(EGLint width, EGLint height) {
    auto& ctx = MG_State::GLState::GLContext::Get();

    if (!ctx.IsInitialized()) {
        auto backend = UniquePtr<BackendObject_DirectGLES>(new BackendObject_DirectGLES());
        ctx.Initialize(std::move(backend));
    }

    auto* backend = ctx.GetBackend();
    if (!backend) return false;

    MG_Backend::pActiveBackendObject.reset(backend);
    return backend->InitPbufferSurface(width, height);
}

Bool MakeCurrent() {
    auto* backend = GetActiveBackend();
    if (!backend) return false;
    return backend->MakeEGLCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
}

Bool ReleaseCurrent() {
    auto* backend = GetActiveBackend();
    if (!backend) return false;
    return backend->MakeEGLCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void Present() {
    auto* backend = GetActiveBackend();
    if (backend) {
        backend->SwapEGLBuffers(m_eglDisplay, m_eglSurface);
    }
}

void SetEGLFuncsTable(const EGLFunctionsTable& eglFuncs) { g_EGLFuncs = eglFuncs; }
void SetGLESFuncsTable(const GLESFunctionsTable& glesFuncs) { g_GLESFuncs = glesFuncs; }
void SetGLESCapabilities(const GLESCapabilities& capabilities) { g_GLESCapabilities = capabilities; }

void DestroyEGLContext() {
    // 平权共生销毁：CPU和GPU同时释放
    auto& ctx = MG_State::GLState::GLContext::Get();
    ctx.Shutdown();

    // 清除向后兼容的全局引用
    MG_Backend::pActiveBackendObject.release();
}

} // namespace MobileGL::backend::DirectGLES