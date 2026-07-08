#include "BackendObject.h"
#include "BackendObjects.h"
#include "Config.h"
#include "Defines.h"

namespace MobileGL {
namespace MG_Backend {

// ============================================================
// DirectGLES Backend - Minimal Implementation
// ============================================================

class DirectGLESBackend : public BackendObject {
public:
    DirectGLESBackend() {
        m_rendererInfo.backendType = BackendType::DirectGLES;
        m_rendererInfo.backendName = "DirectGLES";
        m_rendererInfo.backendDescription = "Direct GLES 3.2 Backend";
        m_rendererInfo.backendVersion = { PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH };
    }

    ~DirectGLESBackend() override {
        ReleaseEGLResources();
    }

    Bool Initialize() override {
        MGLOG_I("Initializing DirectGLES backend...");

        if (!InitializeEGLDisplay()) {
            MGLOG_E("DirectGLES: Failed to initialize EGL display");
            return false;
        }

        // Create a pbuffer surface for headless initialization
        if (!CreateEGLPbufferSurface(1, 1)) {
            MGLOG_W("DirectGLES: Failed to create headless pbuffer surface, continuing without surface");
        }

        if (!MakeEGLCurrent()) {
            MGLOG_W("DirectGLES: Failed to make EGL context current, continuing...");
        }

        InitEGLFunctions();
        QueryDynamicParameters();
        PopulateFormatCapabilities();
        DetectRendererInfo();

        MGLOG_I("DirectGLES backend initialized successfully");
        MGLOG_I("  Renderer: %s", m_rendererInfo.glInfo.rendererName.c_str());
        MGLOG_I("  Vendor: %s", m_rendererInfo.glInfo.vendorName.c_str());
        MGLOG_I("  Version: %s", m_rendererInfo.glInfo.glVersion.c_str());
        MGLOG_I("  GLSL Version: %s", m_rendererInfo.glInfo.glslVersion.c_str());
        MGLOG_I("  Max Texture Size: %d", m_dynamicParams.maxTextureSize);
        MGLOG_I("  Max Draw Buffers: %d", m_dynamicParams.maxDrawBuffers);
        MGLOG_I("  Max Samples: %d", m_dynamicParams.maxSamples);
        MGLOG_I("  Extensions: %zu", m_dynamicParams.extensions.size());

        return true;
    }

    Bool InitCapabilities() override {
        MGLOG_D("DirectGLES: InitCapabilities");
        PopulateFormatCapabilities();

        // Query additional per-format capabilities from the driver
        for (auto& [format, cache] : m_formatCapabilities) {
            if (cache.caps & FormatCapability::RenderbufferFormatSupported) {
                GLint maxSamples = 0;
                glGetInternalformativ(GL_RENDERBUFFER, format, GL_SAMPLES, 1, &maxSamples);
                cache.maxSamples = maxSamples;
            }

            if (cache.caps & FormatCapability::TextureFormatSupported) {
                GLint maxTexSize = 0;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
                cache.maxSize = maxTexSize;
            }

            if (cache.caps & FormatCapability::RenderbufferFormatSupported) {
                GLint maxRbSize = 0;
                glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRbSize);
                cache.maxSize = std::min(cache.maxSize > 0 ? cache.maxSize : maxRbSize, maxRbSize);
            }
        }

        return true;
    }

    Bool InitWindowSurface() override {
        MGLOG_D("DirectGLES: InitWindowSurface");

        if (!m_eglInitialized) {
            MGLOG_E("DirectGLES: Cannot init window surface, EGL not initialized");
            return false;
        }

        if (m_windowHandle.isValid && m_windowHandle.nativeWindow != nullptr) {
            return CreateEGLWindowSurface(m_windowHandle);
        }

        MGLOG_W("DirectGLES: No valid window handle, using pbuffer surface");
        return CreateEGLPbufferSurface(1, 1);
    }
};

// ============================================================
// Init Function
// ============================================================

void Init() {
    MGLOG_I("Initializing %s backend subsystem...", PROJECT_NAME);

    if (pActiveBackendObject != nullptr) {
        MGLOG_W("Backend already initialized, skipping.");
        return;
    }

    BackendType activeType = Config::ActiveBackendType;

    switch (activeType) {
        case BackendType::DirectGLES: {
            auto* backend = new DirectGLESBackend();
            pActiveBackendObject = backend;

            if (!backend->Initialize()) {
                MGLOG_E("Failed to initialize DirectGLES backend");
                delete backend;
                pActiveBackendObject = nullptr;
                return;
            }

            if (!backend->InitCapabilities()) {
                MGLOG_W("DirectGLES: Failed to initialize capabilities, continuing...");
            }

            MGLOG_I("DirectGLES backend created and initialized");
            break;
        }
        default: {
            MGLOG_E("Unknown backend type: %d", static_cast<Int32>(activeType));
            return;
        }
    }

    // Populate the global backend functions table from the active backend
    g_pGLFunctionsTable = &pActiveBackendObject->GetBackendFunctions();

    if (pActiveBackendObject != nullptr) {
        const auto& funcs = pActiveBackendObject->GetBackendFunctions();

        // Draw calls
        gBackendFunctionsTable.glDrawArrays = funcs.glDrawArrays;
        gBackendFunctionsTable.glDrawElements = funcs.glDrawElements;
        gBackendFunctionsTable.glDrawRangeElements = funcs.glDrawRangeElements;
        gBackendFunctionsTable.glDrawArraysInstanced = funcs.glDrawArraysInstanced;
        gBackendFunctionsTable.glDrawElementsInstanced = funcs.glDrawElementsInstanced;
        gBackendFunctionsTable.glDrawArraysIndirect = funcs.glDrawArraysIndirect;
        gBackendFunctionsTable.glDrawElementsIndirect = funcs.glDrawElementsIndirect;
        gBackendFunctionsTable.glMultiDrawArrays = funcs.glMultiDrawArrays;
        gBackendFunctionsTable.glMultiDrawElements = funcs.glMultiDrawElements;
        gBackendFunctionsTable.glMultiDrawArraysIndirect = funcs.glMultiDrawArraysIndirect;
        gBackendFunctionsTable.glMultiDrawElementsIndirect = funcs.glMultiDrawElementsIndirect;
        gBackendFunctionsTable.glDrawArraysInstancedBaseInstance = funcs.glDrawArraysInstancedBaseInstance;
        gBackendFunctionsTable.glDrawElementsInstancedBaseInstance = funcs.glDrawElementsInstancedBaseInstance;
        gBackendFunctionsTable.glDrawElementsBaseVertex = funcs.glDrawElementsBaseVertex;
        gBackendFunctionsTable.glDrawRangeElementsBaseVertex = funcs.glDrawRangeElementsBaseVertex;
        gBackendFunctionsTable.glDrawElementsInstancedBaseVertex = funcs.glDrawElementsInstancedBaseVertex;
        gBackendFunctionsTable.glDrawElementsInstancedBaseVertexBaseInstance = funcs.glDrawElementsInstancedBaseVertexBaseInstance;

        // Clear
        gBackendFunctionsTable.glClear = funcs.glClear;
        gBackendFunctionsTable.glClearColor = funcs.glClearColor;
        gBackendFunctionsTable.glClearDepthf = funcs.glClearDepthf;
        gBackendFunctionsTable.glClearStencil = funcs.glClearStencil;
        gBackendFunctionsTable.glClearBufferfv = funcs.glClearBufferfv;
        gBackendFunctionsTable.glClearBufferiv = funcs.glClearBufferiv;
        gBackendFunctionsTable.glClearBufferuiv = funcs.glClearBufferuiv;
        gBackendFunctionsTable.glClearBufferfi = funcs.glClearBufferfi;

        // Blit
        gBackendFunctionsTable.glBlitFramebuffer = funcs.glBlitFramebuffer;
        gBackendFunctionsTable.glBlitNamedFramebuffer = funcs.glBlitNamedFramebuffer;

        // Compute
        gBackendFunctionsTable.glDispatchCompute = funcs.glDispatchCompute;
        gBackendFunctionsTable.glDispatchComputeIndirect = funcs.glDispatchComputeIndirect;

        MGLOG_I("Global backend functions table populated");
        MGLOG_I("%s backend subsystem initialized successfully", PROJECT_NAME);
    }
}

void Shutdown() {
    MGLOG_I("Shutting down %s backend subsystem...", PROJECT_NAME);

    if (pActiveBackendObject != nullptr) {
        pActiveBackendObject->ReleaseEGLResources();
        delete pActiveBackendObject;
        pActiveBackendObject = nullptr;
    }

    g_pGLFunctionsTable = nullptr;

    // Zero out the global backend functions table
    gBackendFunctionsTable = GlobalBackendFunctionsTable();

    MGLOG_I("%s backend subsystem shutdown complete.", PROJECT_NAME);
}

} // namespace MG_Backend
} // namespace MobileGL