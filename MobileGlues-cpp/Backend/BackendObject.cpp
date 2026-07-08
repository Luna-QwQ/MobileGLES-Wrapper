#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#include "BackendObject.h"
#include "../Defines.h"
#include "BackendObjects.h"

namespace MobileGL {
namespace MG_Backend {

// Global variables
BackendObject* pActiveBackendObject = nullptr;
GlobalBackendFunctionsTable gBackendFunctionsTable;
const GLFunctionsTable* g_pGLFunctionsTable = nullptr;

// ============================================================
// EGL Error Logging
// ============================================================

void BackendObject::LogEGLError(const char* context) {
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        const char* errorStr = "UNKNOWN";
        switch (error) {
            case EGL_NOT_INITIALIZED:       errorStr = "EGL_NOT_INITIALIZED"; break;
            case EGL_BAD_ACCESS:            errorStr = "EGL_BAD_ACCESS"; break;
            case EGL_BAD_ALLOC:             errorStr = "EGL_BAD_ALLOC"; break;
            case EGL_BAD_ATTRIBUTE:         errorStr = "EGL_BAD_ATTRIBUTE"; break;
            case EGL_BAD_CONTEXT:           errorStr = "EGL_BAD_CONTEXT"; break;
            case EGL_BAD_CONFIG:            errorStr = "EGL_BAD_CONFIG"; break;
            case EGL_BAD_CURRENT_SURFACE:   errorStr = "EGL_BAD_CURRENT_SURFACE"; break;
            case EGL_BAD_DISPLAY:           errorStr = "EGL_BAD_DISPLAY"; break;
            case EGL_BAD_SURFACE:           errorStr = "EGL_BAD_SURFACE"; break;
            case EGL_BAD_MATCH:             errorStr = "EGL_BAD_MATCH"; break;
            case EGL_BAD_PARAMETER:         errorStr = "EGL_BAD_PARAMETER"; break;
            case EGL_BAD_NATIVE_PIXMAP:     errorStr = "EGL_BAD_NATIVE_PIXMAP"; break;
            case EGL_BAD_NATIVE_WINDOW:     errorStr = "EGL_BAD_NATIVE_WINDOW"; break;
            case EGL_CONTEXT_LOST:          errorStr = "EGL_CONTEXT_LOST"; break;
            default: break;
        }
        MGLOG_E("[EGL] %s failed: %s (0x%04X)", context, errorStr, error);
    }
}

// ============================================================
// EGL Display Initialization
// ============================================================

Bool BackendObject::InitializeEGLDisplay(EGLNativeDisplayType nativeDisplay) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_eglInitialized) {
        MGLOG_W("EGL display already initialized, skipping.");
        return true;
    }

    m_nativeDisplay = nativeDisplay;

    m_eglDisplay = eglGetDisplay(m_nativeDisplay);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        MGLOG_E("Failed to get EGL display");
        LogEGLError("eglGetDisplay");
        return false;
    }

    EGLint majorVersion = 0, minorVersion = 0;
    if (!eglInitialize(m_eglDisplay, &majorVersion, &minorVersion)) {
        MGLOG_E("Failed to initialize EGL display");
        LogEGLError("eglInitialize");
        return false;
    }

    MGLOG_I("EGL initialized: version %d.%d", majorVersion, minorVersion);

    if (!ChooseEGLConfig()) {
        MGLOG_E("Failed to choose EGL config");
        return false;
    }

    if (!CreateEGLContext()) {
        MGLOG_E("Failed to create EGL context");
        return false;
    }

    m_eglInitialized = true;
    MGLOG_I("EGL display initialized successfully");
    return true;
}

// ============================================================
// EGL Config Selection
// ============================================================

Bool BackendObject::ChooseEGLConfig() {
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         8,
        EGL_DEPTH_SIZE,         24,
        EGL_STENCIL_SIZE,       8,
        EGL_SAMPLE_BUFFERS,     0,
        EGL_SAMPLES,            0,
        EGL_NONE
    };

    EGLint numConfigs = 0;
    if (!eglChooseConfig(m_eglDisplay, configAttribs, &m_eglConfig, 1, &numConfigs)) {
        MGLOG_E("Failed to choose EGL config");
        LogEGLError("eglChooseConfig");
        return false;
    }

    if (numConfigs < 1 || m_eglConfig == nullptr) {
        MGLOG_W("No matching EGL config found with alpha, trying without alpha...");

        const EGLint fallbackAttribs[] = {
            EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,       EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_RED_SIZE,           5,
            EGL_GREEN_SIZE,         6,
            EGL_BLUE_SIZE,          5,
            EGL_DEPTH_SIZE,         16,
            EGL_STENCIL_SIZE,       0,
            EGL_SAMPLE_BUFFERS,     0,
            EGL_SAMPLES,            0,
            EGL_NONE
        };

        if (!eglChooseConfig(m_eglDisplay, fallbackAttribs, &m_eglConfig, 1, &numConfigs) || numConfigs < 1) {
            MGLOG_E("Failed to choose any EGL config");
            return false;
        }
    }

    MGLOG_I("EGL config chosen successfully");
    return true;
}

// ============================================================
// EGL Context Creation
// ============================================================

Bool BackendObject::CreateEGLContext() {
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        MGLOG_E("Failed to create EGL context");
        LogEGLError("eglCreateContext");
        return false;
    }

    MGLOG_I("EGL context created successfully");
    return true;
}

// ============================================================
// Window Surface Creation
// ============================================================

Bool BackendObject::CreateEGLWindowSurface(WindowHandle& windowHandle) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_eglInitialized) {
        MGLOG_E("Cannot create window surface: EGL not initialized");
        return false;
    }

    if (m_eglSurfaceCreated) {
        MGLOG_I("Releasing existing EGL surface before creating new one");
        ReleaseEGLSurface();
    }

    if (!windowHandle.isValid || windowHandle.nativeWindow == nullptr) {
        MGLOG_E("Invalid window handle");
        return false;
    }

    EGLNativeWindowType nativeWindow = reinterpret_cast<EGLNativeWindowType>(windowHandle.nativeWindow);

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, nativeWindow, nullptr);
    if (m_eglSurface == EGL_NO_SURFACE) {
        MGLOG_E("Failed to create EGL window surface");
        LogEGLError("eglCreateWindowSurface");
        return false;
    }

    RegisterEGLWindowSurface(m_eglSurface);
    m_windowHandle = windowHandle;
    m_eglSurfaceCreated = true;

    MGLOG_I("EGL window surface created: %dx%d", windowHandle.width, windowHandle.height);
    return true;
}

// ============================================================
// PBuffer Surface Creation
// ============================================================

Bool BackendObject::CreateEGLPbufferSurface(Int32 width, Int32 height) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_eglInitialized) {
        MGLOG_E("Cannot create pbuffer surface: EGL not initialized");
        return false;
    }

    if (m_eglSurfaceCreated) {
        MGLOG_I("Releasing existing EGL surface before creating new one");
        ReleaseEGLSurface();
    }

    const EGLint pbufferAttribs[] = {
        EGL_WIDTH,  width,
        EGL_HEIGHT, height,
        EGL_NONE
    };

    m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttribs);
    if (m_eglSurface == EGL_NO_SURFACE) {
        MGLOG_E("Failed to create EGL pbuffer surface");
        LogEGLError("eglCreatePbufferSurface");
        return false;
    }

    RegisterEGLPbufferSurface(m_eglSurface);
    m_eglSurfaceCreated = true;

    MGLOG_I("EGL pbuffer surface created: %dx%d", width, height);
    return true;
}

// ============================================================
// Make EGL Current
// ============================================================

Bool BackendObject::MakeEGLCurrent() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_eglInitialized) {
        MGLOG_E("Cannot make EGL current: EGL not initialized");
        return false;
    }

    if (!m_eglSurfaceCreated) {
        MGLOG_E("Cannot make EGL current: no surface created");
        return false;
    }

    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext)) {
        MGLOG_E("Failed to make EGL context current");
        LogEGLError("eglMakeCurrent");
        return false;
    }

    m_eglContextMadeCurrent = true;
    return true;
}

// ============================================================
// Swap EGL Buffers
// ============================================================

Bool BackendObject::SwapEGLBuffers() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_eglInitialized || !m_eglSurfaceCreated) {
        MGLOG_E("Cannot swap buffers: EGL surface not ready");
        return false;
    }

    if (!eglSwapBuffers(m_eglDisplay, m_eglSurface)) {
        MGLOG_E("Failed to swap EGL buffers");
        LogEGLError("eglSwapBuffers");
        return false;
    }

    return true;
}

// ============================================================
// Release EGL Surface
// ============================================================

Bool BackendObject::ReleaseEGLSurface() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        m_eglContextMadeCurrent = false;

        if (m_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(m_eglDisplay, m_eglSurface);
            m_eglSurface = EGL_NO_SURFACE;
        }
    }

    m_eglSurfaceCreated = false;
    m_windowHandle = WindowHandle{};

    MGLOG_I("EGL surface released");
    return true;
}

// ============================================================
// Release EGL Resources
// ============================================================

void BackendObject::ReleaseEGLResources() {
    std::lock_guard<std::mutex> lock(m_mutex);

    ReleaseEGLSurface();

    if (m_eglDisplay != EGL_NO_DISPLAY) {
        if (m_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(m_eglDisplay, m_eglContext);
            m_eglContext = EGL_NO_CONTEXT;
        }

        eglTerminate(m_eglDisplay);
        m_eglDisplay = EGL_NO_DISPLAY;
    }

    m_eglInitialized = false;
    m_eglConfig = nullptr;

    MGLOG_I("EGL resources released");
}

// ============================================================
// Window Handle Management
// ============================================================

void BackendObject::SetWindowHandle(WindowHandle& handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_windowHandle = handle;
}

WindowHandle BackendObject::GetWindowHandle() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_windowHandle;
}

// ============================================================
// EGL State Management
// ============================================================

void BackendObject::ResetEGLRuntimeState() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_eglDisplay = EGL_NO_DISPLAY;
    m_eglContext = EGL_NO_CONTEXT;
    m_eglSurface = EGL_NO_SURFACE;
    m_eglConfig = nullptr;
    m_eglInitialized = false;
    m_eglSurfaceCreated = false;
    m_eglContextMadeCurrent = false;
    m_windowHandle = WindowHandle{};

    MGLOG_I("EGL runtime state reset");
}

void BackendObject::RegisterEGLWindowSurface(EGLSurface surface) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eglSurface = surface;
    m_eglSurfaceCreated = true;
    MGLOG_D("Registered EGL window surface: %p", static_cast<void*>(surface));
}

void BackendObject::RegisterEGLPbufferSurface(EGLSurface surface) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eglSurface = surface;
    m_eglSurfaceCreated = true;
    MGLOG_D("Registered EGL pbuffer surface: %p", static_cast<void*>(surface));
}

// ============================================================
// Renderer Info
// ============================================================

RendererInfo BackendObject::GetRendererInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rendererInfo;
}

String BackendObject::GetBackendAPIVersionString() const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d.%d.%d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
    return String(buf);
}

const GLFunctionsTable& BackendObject::GetBackendFunctions() const {
    return m_functionsTable;
}

const DynamicBackendParameters& BackendObject::GetDynamicParameters() const {
    return m_dynamicParams;
}

BackendType BackendObject::GetBackendType() const {
    return m_rendererInfo.backendType;
}

String BackendObject::GetBackendName() const {
    return m_rendererInfo.backendName;
}

// ============================================================
// Format Capabilities
// ============================================================

HashMap<GLenum, FormatCapabilityCache>& BackendObject::MutableFormatCapabilities() {
    return m_formatCapabilities;
}

const HashMap<GLenum, FormatCapabilityCache>& BackendObject::GetFormatCapabilities() const {
    return m_formatCapabilities;
}

Bool BackendObject::HasFormatCapability(GLenum format, FormatCapability cap) const {
    auto it = m_formatCapabilities.find(format);
    if (it == m_formatCapabilities.end()) {
        return false;
    }
    return (it->second.caps & cap) != 0;
}

FormatCapabilityCache BackendObject::GetFormatCapabilityInfo(GLenum format) const {
    auto it = m_formatCapabilities.find(format);
    if (it != m_formatCapabilities.end()) {
        return it->second;
    }

    FormatCapabilityCache emptyCache;
    emptyCache.glInternalFormat = format;
    emptyCache.name = "UNKNOWN_FORMAT";
    MGLOG_W("Format capability info not found for format 0x%04X", format);
    return emptyCache;
}

Int32 BackendObject::GetFormatCapabilityTargetIndex(GLenum format, FormatCapability cap) const {
    auto it = m_formatCapabilities.find(format);
    if (it == m_formatCapabilities.end()) {
        return -1;
    }
    if (it->second.caps & cap) {
        return static_cast<Int32>(format);
    }
    return -1;
}

Int32 BackendObject::GetRenderbufferFormatCapabilityTargetIndex(GLenum format, FormatCapability cap) const {
    auto it = m_formatCapabilities.find(format);
    if (it == m_formatCapabilities.end()) {
        return -1;
    }
    if (it->second.caps & cap) {
        return static_cast<Int32>(format);
    }
    return -1;
}

void BackendObject::PrintFormatCapabilities() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    MGLOG_I("========== Format Capabilities ==========");
    MGLOG_I("Total formats tracked: %zu", m_formatCapabilities.size());

    for (const auto& [format, cache] : m_formatCapabilities) {
        MGLOG_I("  Format 0x%04X [%s]:", format, cache.name.c_str());
        MGLOG_I("    GL Format: 0x%04X, GL Type: 0x%04X", cache.glFormat, cache.glType);
        MGLOG_I("    Channels: %d, Bits/Channel: %d", cache.numChannels, cache.bitsPerChannel);
        MGLOG_I("    Compressed: %s, SRGB: %s, Float: %s, Integer: %s",
            cache.isCompressed ? "YES" : "NO",
            cache.isSRGB ? "YES" : "NO",
            cache.isFloat ? "YES" : "NO",
            cache.isInteger ? "YES" : "NO");
        MGLOG_I("    Depth: %s, Stencil: %s, DepthStencil: %s",
            cache.isDepth ? "YES" : "NO",
            cache.isStencil ? "YES" : "NO",
            cache.isDepthStencil ? "YES" : "NO");
        MGLOG_I("    MaxSamples: %d, MaxSize: %d", cache.maxSamples, cache.maxSize);

        Uint32 capsValue = static_cast<Uint32>(cache.caps.Value());
        MGLOG_I("    Capabilities: 0x%08X", capsValue);
    }

    MGLOG_I("==========================================");
}

// ============================================================
// Protected Helper Methods (default implementations)
// ============================================================

void BackendObject::PopulateFormatCapabilities() {
    MGLOG_D("BackendObject::PopulateFormatCapabilities - base implementation");

    FormatCapabilityCache rgba8;
    rgba8.glInternalFormat = GL_RGBA8;
    rgba8.glFormat = GL_RGBA;
    rgba8.glType = GL_UNSIGNED_BYTE;
    rgba8.caps = FormatCapability::TextureFormatSupported
               | FormatCapability::TextureFilterSupported
               | FormatCapability::TextureMipmapSupported
               | FormatCapability::FramebufferColorAttachment
               | FormatCapability::RenderbufferFormatSupported
               | FormatCapability::SizedInternalFormat;
    rgba8.numChannels = 4;
    rgba8.bitsPerChannel = 8;
    rgba8.name = "RGBA8";
    m_formatCapabilities[GL_RGBA8] = rgba8;

    FormatCapabilityCache rgba4;
    rgba4.glInternalFormat = GL_RGBA4;
    rgba4.glFormat = GL_RGBA;
    rgba4.glType = GL_UNSIGNED_SHORT_4_4_4_4;
    rgba4.caps = FormatCapability::TextureFormatSupported
               | FormatCapability::TextureFilterSupported
               | FormatCapability::FramebufferColorAttachment
               | FormatCapability::RenderbufferFormatSupported
               | FormatCapability::SizedInternalFormat;
    rgba4.numChannels = 4;
    rgba4.bitsPerChannel = 4;
    rgba4.name = "RGBA4";
    m_formatCapabilities[GL_RGBA4] = rgba4;

    FormatCapabilityCache rgb565;
    rgb565.glInternalFormat = GL_RGB565;
    rgb565.glFormat = GL_RGB;
    rgb565.glType = GL_UNSIGNED_SHORT_5_6_5;
    rgb565.caps = FormatCapability::TextureFormatSupported
                | FormatCapability::TextureFilterSupported
                | FormatCapability::FramebufferColorAttachment
                | FormatCapability::RenderbufferFormatSupported
                | FormatCapability::SizedInternalFormat;
    rgb565.numChannels = 3;
    rgb565.bitsPerChannel = 5;
    rgb565.name = "RGB565";
    m_formatCapabilities[GL_RGB565] = rgb565;

    FormatCapabilityCache rgb5a1;
    rgb5a1.glInternalFormat = GL_RGB5_A1;
    rgb5a1.glFormat = GL_RGBA;
    rgb5a1.glType = GL_UNSIGNED_SHORT_5_5_5_1;
    rgb5a1.caps = FormatCapability::TextureFormatSupported
                | FormatCapability::TextureFilterSupported
                | FormatCapability::FramebufferColorAttachment
                | FormatCapability::RenderbufferFormatSupported
                | FormatCapability::SizedInternalFormat;
    rgb5a1.numChannels = 4;
    rgb5a1.bitsPerChannel = 5;
    rgb5a1.name = "RGB5_A1";
    m_formatCapabilities[GL_RGB5_A1] = rgb5a1;

    FormatCapabilityCache depth16;
    depth16.glInternalFormat = GL_DEPTH_COMPONENT16;
    depth16.glFormat = GL_DEPTH_COMPONENT;
    depth16.glType = GL_UNSIGNED_SHORT;
    depth16.caps = FormatCapability::RenderbufferFormatSupported
                 | FormatCapability::FramebufferDepthAttachment
                 | FormatCapability::SizedInternalFormat;
    depth16.numChannels = 1;
    depth16.bitsPerChannel = 16;
    depth16.isDepth = true;
    depth16.name = "DEPTH16";
    m_formatCapabilities[GL_DEPTH_COMPONENT16] = depth16;

    FormatCapabilityCache depth24;
    depth24.glInternalFormat = GL_DEPTH_COMPONENT24;
    depth24.glFormat = GL_DEPTH_COMPONENT;
    depth24.glType = GL_UNSIGNED_INT;
    depth24.caps = FormatCapability::RenderbufferFormatSupported
                 | FormatCapability::FramebufferDepthAttachment
                 | FormatCapability::SizedInternalFormat;
    depth24.numChannels = 1;
    depth24.bitsPerChannel = 24;
    depth24.isDepth = true;
    depth24.name = "DEPTH24";
    m_formatCapabilities[GL_DEPTH_COMPONENT24] = depth24;

    FormatCapabilityCache stencil8;
    stencil8.glInternalFormat = GL_STENCIL_INDEX8;
    stencil8.glFormat = 0x1901;
    stencil8.glType = GL_UNSIGNED_BYTE;
    stencil8.caps = FormatCapability::RenderbufferFormatSupported
                  | FormatCapability::FramebufferStencilAttachment
                  | FormatCapability::SizedInternalFormat;
    stencil8.numChannels = 1;
    stencil8.bitsPerChannel = 8;
    stencil8.isStencil = true;
    stencil8.name = "STENCIL8";
    m_formatCapabilities[GL_STENCIL_INDEX8] = stencil8;

    FormatCapabilityCache depth24stencil8;
    depth24stencil8.glInternalFormat = GL_DEPTH24_STENCIL8;
    depth24stencil8.glFormat = GL_DEPTH_STENCIL;
    depth24stencil8.glType = GL_UNSIGNED_INT_24_8;
    depth24stencil8.caps = FormatCapability::RenderbufferFormatSupported
                         | FormatCapability::FramebufferDepthAttachment
                         | FormatCapability::FramebufferStencilAttachment
                         | FormatCapability::SizedInternalFormat;
    depth24stencil8.numChannels = 2;
    depth24stencil8.bitsPerChannel = 24;
    depth24stencil8.isDepth = true;
    depth24stencil8.isStencil = true;
    depth24stencil8.isDepthStencil = true;
    depth24stencil8.name = "DEPTH24_STENCIL8";
    m_formatCapabilities[GL_DEPTH24_STENCIL8] = depth24stencil8;

    FormatCapabilityCache srgb8;
    srgb8.glInternalFormat = GL_SRGB8;
    srgb8.glFormat = GL_RGB;
    srgb8.glType = GL_UNSIGNED_BYTE;
    srgb8.caps = FormatCapability::TextureFormatSupported
               | FormatCapability::SizedInternalFormat;
    srgb8.numChannels = 3;
    srgb8.bitsPerChannel = 8;
    srgb8.isSRGB = true;
    srgb8.name = "SRGB8";
    m_formatCapabilities[GL_SRGB8] = srgb8;

    FormatCapabilityCache srgb8Alpha8;
    srgb8Alpha8.glInternalFormat = GL_SRGB8_ALPHA8;
    srgb8Alpha8.glFormat = GL_RGBA;
    srgb8Alpha8.glType = GL_UNSIGNED_BYTE;
    srgb8Alpha8.caps = FormatCapability::TextureFormatSupported
                     | FormatCapability::FramebufferColorAttachment
                     | FormatCapability::SizedInternalFormat;
    srgb8Alpha8.numChannels = 4;
    srgb8Alpha8.bitsPerChannel = 8;
    srgb8Alpha8.isSRGB = true;
    srgb8Alpha8.name = "SRGB8_ALPHA8";
    m_formatCapabilities[GL_SRGB8_ALPHA8] = srgb8Alpha8;

    MGLOG_D("Populated %zu base format capabilities", m_formatCapabilities.size());
}

void BackendObject::QueryDynamicParameters() {
    MGLOG_D("BackendObject::QueryDynamicParameters - base implementation");

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_dynamicParams.maxTextureSize);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_dynamicParams.max3DTextureSize);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &m_dynamicParams.maxCubeMapTextureSize);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_dynamicParams.maxArrayTextureLayers);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &m_dynamicParams.maxRenderbufferSize);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &m_dynamicParams.maxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_dynamicParams.maxColorAttachments);
    glGetIntegerv(GL_MAX_SAMPLES, &m_dynamicParams.maxSamples);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_dynamicParams.maxVertexAttribs);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &m_dynamicParams.maxVertexUniformComponents);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &m_dynamicParams.maxFragmentUniformComponents);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &m_dynamicParams.maxVaryingVectors);
    glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &m_dynamicParams.maxVaryingComponents);
    glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &m_dynamicParams.maxVertexOutputComponents);
    glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &m_dynamicParams.maxFragmentInputComponents);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &m_dynamicParams.maxVertexTextureImageUnits);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_dynamicParams.maxTextureImageUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_dynamicParams.maxCombinedTextureImageUnits);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &m_dynamicParams.maxUniformBufferBindings);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &m_dynamicParams.maxUniformBlockSize);
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &m_dynamicParams.maxTransformFeedbackInterleavedComponents);
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &m_dynamicParams.maxTransformFeedbackSeparateAttribs);
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &m_dynamicParams.maxTransformFeedbackSeparateComponents);
    glGetIntegerv(GL_SUBPIXEL_BITS, &m_dynamicParams.subPixelBits);

    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, m_dynamicParams.maxViewportDims);

    GLfloat aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    m_dynamicParams.maxAnisotropy = aniso;

    GLfloat lodBias = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &lodBias);
    m_dynamicParams.maxTextureLodBias = lodBias;

    GLfloat lineWidthRange[2] = {0.0f, 0.0f};
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    m_dynamicParams.minLineWidth = lineWidthRange[0];
    m_dynamicParams.maxLineWidth = lineWidthRange[1];

    GLfloat pointSizeRange[2] = {0.0f, 0.0f};
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange);
    m_dynamicParams.aliasedPointSizeRange[0] = pointSizeRange[0];
    m_dynamicParams.aliasedPointSizeRange[1] = pointSizeRange[1];

    const GLubyte* rendererStr = glGetString(GL_RENDERER);
    const GLubyte* vendorStr = glGetString(GL_VENDOR);
    const GLubyte* versionStr = glGetString(GL_VERSION);
    const GLubyte* glslVersionStr = glGetString(GL_SHADING_LANGUAGE_VERSION);

    m_dynamicParams.rendererString = rendererStr ? reinterpret_cast<const char*>(rendererStr) : "Unknown";
    m_dynamicParams.vendorString = vendorStr ? reinterpret_cast<const char*>(vendorStr) : "Unknown";
    m_dynamicParams.versionString = versionStr ? reinterpret_cast<const char*>(versionStr) : "Unknown";
    m_dynamicParams.glslVersionString = glslVersionStr ? reinterpret_cast<const char*>(glslVersionStr) : "Unknown";

    if (versionStr) {
        String verStr = reinterpret_cast<const char*>(versionStr);
        if (sscanf(verStr.c_str(), "OpenGL ES %d.%d", &m_dynamicParams.majorVersion, &m_dynamicParams.minorVersion) < 2) {
            sscanf(verStr.c_str(), "%d.%d", &m_dynamicParams.majorVersion, &m_dynamicParams.minorVersion);
        }
    }

    if (glslVersionStr) {
        String glslStr = reinterpret_cast<const char*>(glslVersionStr);
        sscanf(glslStr.c_str(), "OpenGL ES GLSL ES %d.%d", &m_dynamicParams.glslMajorVersion, &m_dynamicParams.glslMinorVersion);
    }

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    m_dynamicParams.extensions.clear();
    m_dynamicParams.extensions.reserve(numExtensions);

    for (GLint i = 0; i < numExtensions; ++i) {
        const GLubyte* ext = glGetStringi(GL_EXTENSIONS, i);
        if (ext) {
            String extStr = reinterpret_cast<const char*>(ext);
            m_dynamicParams.extensions.push_back(extStr);

            if (extStr == "GL_EXT_texture_filter_anisotropic") {
                m_dynamicParams.supportsTextureFilterAnisotropic = true;
            }
            if (extStr == "GL_OES_texture_float_linear") {
                m_dynamicParams.supportsTextureFloatLinear = true;
            }
            if (extStr == "GL_EXT_texture_norm16" || extStr == "GL_OES_texture_float") {
                m_dynamicParams.supportsTextureNPOT = true;
            }
            if (extStr == "GL_EXT_sRGB" || extStr == "GL_EXT_sRGB_write_control") {
                m_dynamicParams.supportsSRGBFramebuffer = true;
            }
            if (extStr == "GL_KHR_debug") {
                m_dynamicParams.supportsDebugOutput = true;
            }
            if (extStr == "GL_OES_geometry_shader" || extStr == "GL_EXT_geometry_shader") {
                m_dynamicParams.supportsGeometryShaders = true;
            }
            if (extStr == "GL_OES_tessellation_shader" || extStr == "GL_EXT_tessellation_shader") {
                m_dynamicParams.supportsTessellationShaders = true;
            }
            if (extStr == "GL_OES_texture_buffer" || extStr == "GL_EXT_texture_buffer") {
                m_dynamicParams.supportsTextureBuffer = true;
            }
            if (extStr == "GL_OES_texture_storage_multisample_2d_array" || extStr == "GL_EXT_multisampled_render_to_texture") {
                m_dynamicParams.supportsTextureMultisample = true;
            }
            if (extStr == "GL_EXT_base_instance" || extStr == "GL_EXT_draw_instanced") {
                m_dynamicParams.supportsBaseInstance = true;
            }
            if (extStr == "GL_EXT_draw_elements_base_vertex" || extStr == "GL_OES_draw_elements_base_vertex") {
                m_dynamicParams.supportsBaseVertex = true;
            }
            if (extStr == "GL_EXT_draw_buffers_indexed") {
                m_dynamicParams.supportsDrawBuffersBlend = true;
            }
            if (extStr == "GL_EXT_polygon_offset_clamp" || extStr == "GL_NV_polygon_mode") {
                m_dynamicParams.supportsPolygonMode = true;
            }
            if (extStr == "GL_EXT_depth_clamp" || extStr == "GL_NV_depth_clamp") {
                m_dynamicParams.supportsDepthClamp = true;
            }
            if (extStr == "GL_OES_texture_cube_map_array") {
                m_dynamicParams.supportsSeamlessCubeMap = true;
            }
            if (extStr == "GL_OES_get_program_binary") {
                m_dynamicParams.supportsProgramBinary = true;
            }
            if (extStr == "GL_EXT_shader_image_load_store" || extStr == "GL_OES_shader_image_atomic") {
                m_dynamicParams.supportsShaderImageLoadStore = true;
            }
            if (extStr == "GL_EXT_shader_atomic_counters") {
                m_dynamicParams.supportsShaderAtomicCounters = true;
            }
        }
    }

    if (m_dynamicParams.majorVersion >= 3) {
        m_dynamicParams.supportsDrawIndirect = true;
        m_dynamicParams.supportsMultiDrawIndirect = true;
        m_dynamicParams.supportsTextureNPOT = true;
        m_dynamicParams.supportsPrimitiveRestartFixedIndex = true;
    }

    if (m_dynamicParams.majorVersion >= 3 && m_dynamicParams.minorVersion >= 1) {
        m_dynamicParams.supportsComputeShaders = true;
        m_dynamicParams.supportsTextureMultisample = true;
    }

    MGLOG_D("Dynamic parameters queried successfully");
}

void BackendObject::DetectRendererInfo() {
    MGLOG_D("BackendObject::DetectRendererInfo - base implementation");

    m_rendererInfo.backendType = BackendType::DirectGLES;
    m_rendererInfo.backendName = "DirectGLES";
    m_rendererInfo.backendDescription = "Direct GLES3.2 Backend";
    m_rendererInfo.backendVersion = { PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH };

    m_rendererInfo.glInfo.rendererName = m_dynamicParams.rendererString;
    m_rendererInfo.glInfo.vendorName = m_dynamicParams.vendorString;
    m_rendererInfo.glInfo.glVersion = m_dynamicParams.versionString;
    m_rendererInfo.glInfo.glslVersion = m_dynamicParams.glslVersionString;
    m_rendererInfo.glInfo.apiVersion = { m_dynamicParams.majorVersion, m_dynamicParams.minorVersion, 0 };
    m_rendererInfo.glInfo.extensions = m_dynamicParams.extensions;

    String rendererLower = m_dynamicParams.rendererString;
    String vendorLower = m_dynamicParams.vendorString;
    std::transform(rendererLower.begin(), rendererLower.end(), rendererLower.begin(), ::tolower);
    std::transform(vendorLower.begin(), vendorLower.end(), vendorLower.begin(), ::tolower);

    m_rendererInfo.glInfo.isHardwareAccelerated = true;
    m_rendererInfo.glInfo.isANGLE = (rendererLower.find("angle") != String::npos);
    m_rendererInfo.glInfo.isAdreno = (rendererLower.find("adreno") != String::npos);
    m_rendererInfo.glInfo.isMali = (rendererLower.find("mali") != String::npos);
    m_rendererInfo.glInfo.isPowerVR = (rendererLower.find("powervr") != String::npos);
    m_rendererInfo.glInfo.isAppleGPU = (rendererLower.find("apple") != String::npos);
    m_rendererInfo.glInfo.isIntel = (rendererLower.find("intel") != String::npos);
    m_rendererInfo.glInfo.isNVIDIA = (rendererLower.find("nvidia") != String::npos || rendererLower.find("tegra") != String::npos);
    m_rendererInfo.glInfo.isAMD = (rendererLower.find("amd") != String::npos || rendererLower.find("radeon") != String::npos);
    m_rendererInfo.glInfo.isMesa = (rendererLower.find("mesa") != String::npos);
    m_rendererInfo.glInfo.isSwiftShader = (rendererLower.find("swiftshader") != String::npos);
    m_rendererInfo.glInfo.isZink = (rendererLower.find("zink") != String::npos);
    m_rendererInfo.glInfo.isVirGL = (rendererLower.find("virgl") != String::npos);

    if (m_rendererInfo.glInfo.isSwiftShader || m_rendererInfo.glInfo.isZink || m_rendererInfo.glInfo.isVirGL) {
        m_rendererInfo.glInfo.isHardwareAccelerated = false;
    }

    MGLOG_D("Renderer info detected: %s", m_rendererInfo.glInfo.rendererName.c_str());
}

void BackendObject::InitEGLFunctions() {
    MGLOG_D("BackendObject::InitEGLFunctions - base implementation");

    m_functionsTable.glGetIntegerv = reinterpret_cast<decltype(m_functionsTable.glGetIntegerv)>(eglGetProcAddress("glGetIntegerv"));
    m_functionsTable.glGetFloatv = reinterpret_cast<decltype(m_functionsTable.glGetFloatv)>(eglGetProcAddress("glGetFloatv"));
    m_functionsTable.glGetBooleanv = reinterpret_cast<decltype(m_functionsTable.glGetBooleanv)>(eglGetProcAddress("glGetBooleanv"));
    m_functionsTable.glGetString = reinterpret_cast<decltype(m_functionsTable.glGetString)>(eglGetProcAddress("glGetString"));
    m_functionsTable.glGetStringi = reinterpret_cast<decltype(m_functionsTable.glGetStringi)>(eglGetProcAddress("glGetStringi"));
    m_functionsTable.glGetError = reinterpret_cast<decltype(m_functionsTable.glGetError)>(eglGetProcAddress("glGetError"));

    m_functionsTable.glDrawArrays = reinterpret_cast<decltype(m_functionsTable.glDrawArrays)>(eglGetProcAddress("glDrawArrays"));
    m_functionsTable.glDrawElements = reinterpret_cast<decltype(m_functionsTable.glDrawElements)>(eglGetProcAddress("glDrawElements"));
    m_functionsTable.glDrawRangeElements = reinterpret_cast<decltype(m_functionsTable.glDrawRangeElements)>(eglGetProcAddress("glDrawRangeElements"));
    m_functionsTable.glDrawArraysInstanced = reinterpret_cast<decltype(m_functionsTable.glDrawArraysInstanced)>(eglGetProcAddress("glDrawArraysInstanced"));
    m_functionsTable.glDrawElementsInstanced = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstanced)>(eglGetProcAddress("glDrawElementsInstanced"));

    m_functionsTable.glClear = reinterpret_cast<decltype(m_functionsTable.glClear)>(eglGetProcAddress("glClear"));
    m_functionsTable.glClearColor = reinterpret_cast<decltype(m_functionsTable.glClearColor)>(eglGetProcAddress("glClearColor"));
    m_functionsTable.glClearDepthf = reinterpret_cast<decltype(m_functionsTable.glClearDepthf)>(eglGetProcAddress("glClearDepthf"));
    m_functionsTable.glClearStencil = reinterpret_cast<decltype(m_functionsTable.glClearStencil)>(eglGetProcAddress("glClearStencil"));
    m_functionsTable.glClearBufferfv = reinterpret_cast<decltype(m_functionsTable.glClearBufferfv)>(eglGetProcAddress("glClearBufferfv"));
    m_functionsTable.glClearBufferiv = reinterpret_cast<decltype(m_functionsTable.glClearBufferiv)>(eglGetProcAddress("glClearBufferiv"));
    m_functionsTable.glClearBufferuiv = reinterpret_cast<decltype(m_functionsTable.glClearBufferuiv)>(eglGetProcAddress("glClearBufferuiv"));
    m_functionsTable.glClearBufferfi = reinterpret_cast<decltype(m_functionsTable.glClearBufferfi)>(eglGetProcAddress("glClearBufferfi"));

    m_functionsTable.glBlitFramebuffer = reinterpret_cast<decltype(m_functionsTable.glBlitFramebuffer)>(eglGetProcAddress("glBlitFramebuffer"));

    m_functionsTable.glGenBuffers = reinterpret_cast<decltype(m_functionsTable.glGenBuffers)>(eglGetProcAddress("glGenBuffers"));
    m_functionsTable.glDeleteBuffers = reinterpret_cast<decltype(m_functionsTable.glDeleteBuffers)>(eglGetProcAddress("glDeleteBuffers"));
    m_functionsTable.glBindBuffer = reinterpret_cast<decltype(m_functionsTable.glBindBuffer)>(eglGetProcAddress("glBindBuffer"));
    m_functionsTable.glBufferData = reinterpret_cast<decltype(m_functionsTable.glBufferData)>(eglGetProcAddress("glBufferData"));
    m_functionsTable.glBufferSubData = reinterpret_cast<decltype(m_functionsTable.glBufferSubData)>(eglGetProcAddress("glBufferSubData"));
    m_functionsTable.glBindBufferBase = reinterpret_cast<decltype(m_functionsTable.glBindBufferBase)>(eglGetProcAddress("glBindBufferBase"));
    m_functionsTable.glBindBufferRange = reinterpret_cast<decltype(m_functionsTable.glBindBufferRange)>(eglGetProcAddress("glBindBufferRange"));
    m_functionsTable.glMapBufferRange = reinterpret_cast<decltype(m_functionsTable.glMapBufferRange)>(eglGetProcAddress("glMapBufferRange"));
    m_functionsTable.glUnmapBuffer = reinterpret_cast<decltype(m_functionsTable.glUnmapBuffer)>(eglGetProcAddress("glUnmapBuffer"));

    m_functionsTable.glGenTextures = reinterpret_cast<decltype(m_functionsTable.glGenTextures)>(eglGetProcAddress("glGenTextures"));
    m_functionsTable.glDeleteTextures = reinterpret_cast<decltype(m_functionsTable.glDeleteTextures)>(eglGetProcAddress("glDeleteTextures"));
    m_functionsTable.glBindTexture = reinterpret_cast<decltype(m_functionsTable.glBindTexture)>(eglGetProcAddress("glBindTexture"));
    m_functionsTable.glTexImage2D = reinterpret_cast<decltype(m_functionsTable.glTexImage2D)>(eglGetProcAddress("glTexImage2D"));
    m_functionsTable.glTexImage3D = reinterpret_cast<decltype(m_functionsTable.glTexImage3D)>(eglGetProcAddress("glTexImage3D"));
    m_functionsTable.glTexSubImage2D = reinterpret_cast<decltype(m_functionsTable.glTexSubImage2D)>(eglGetProcAddress("glTexSubImage2D"));
    m_functionsTable.glTexSubImage3D = reinterpret_cast<decltype(m_functionsTable.glTexSubImage3D)>(eglGetProcAddress("glTexSubImage3D"));
    m_functionsTable.glTexStorage2D = reinterpret_cast<decltype(m_functionsTable.glTexStorage2D)>(eglGetProcAddress("glTexStorage2D"));
    m_functionsTable.glTexStorage3D = reinterpret_cast<decltype(m_functionsTable.glTexStorage3D)>(eglGetProcAddress("glTexStorage3D"));
    m_functionsTable.glTexParameteri = reinterpret_cast<decltype(m_functionsTable.glTexParameteri)>(eglGetProcAddress("glTexParameteri"));
    m_functionsTable.glTexParameterf = reinterpret_cast<decltype(m_functionsTable.glTexParameterf)>(eglGetProcAddress("glTexParameterf"));
    m_functionsTable.glTexParameteriv = reinterpret_cast<decltype(m_functionsTable.glTexParameteriv)>(eglGetProcAddress("glTexParameteriv"));
    m_functionsTable.glTexParameterfv = reinterpret_cast<decltype(m_functionsTable.glTexParameterfv)>(eglGetProcAddress("glTexParameterfv"));
    m_functionsTable.glGenerateMipmap = reinterpret_cast<decltype(m_functionsTable.glGenerateMipmap)>(eglGetProcAddress("glGenerateMipmap"));
    m_functionsTable.glActiveTexture = reinterpret_cast<decltype(m_functionsTable.glActiveTexture)>(eglGetProcAddress("glActiveTexture"));

    m_functionsTable.glGenFramebuffers = reinterpret_cast<decltype(m_functionsTable.glGenFramebuffers)>(eglGetProcAddress("glGenFramebuffers"));
    m_functionsTable.glDeleteFramebuffers = reinterpret_cast<decltype(m_functionsTable.glDeleteFramebuffers)>(eglGetProcAddress("glDeleteFramebuffers"));
    m_functionsTable.glBindFramebuffer = reinterpret_cast<decltype(m_functionsTable.glBindFramebuffer)>(eglGetProcAddress("glBindFramebuffer"));
    m_functionsTable.glFramebufferTexture2D = reinterpret_cast<decltype(m_functionsTable.glFramebufferTexture2D)>(eglGetProcAddress("glFramebufferTexture2D"));
    m_functionsTable.glFramebufferTextureLayer = reinterpret_cast<decltype(m_functionsTable.glFramebufferTextureLayer)>(eglGetProcAddress("glFramebufferTextureLayer"));
    m_functionsTable.glFramebufferRenderbuffer = reinterpret_cast<decltype(m_functionsTable.glFramebufferRenderbuffer)>(eglGetProcAddress("glFramebufferRenderbuffer"));
    m_functionsTable.glCheckFramebufferStatus = reinterpret_cast<decltype(m_functionsTable.glCheckFramebufferStatus)>(eglGetProcAddress("glCheckFramebufferStatus"));
    m_functionsTable.glReadBuffer = reinterpret_cast<decltype(m_functionsTable.glReadBuffer)>(eglGetProcAddress("glReadBuffer"));
    m_functionsTable.glDrawBuffers = reinterpret_cast<decltype(m_functionsTable.glDrawBuffers)>(eglGetProcAddress("glDrawBuffers"));
    m_functionsTable.glReadPixels = reinterpret_cast<decltype(m_functionsTable.glReadPixels)>(eglGetProcAddress("glReadPixels"));

    m_functionsTable.glGenRenderbuffers = reinterpret_cast<decltype(m_functionsTable.glGenRenderbuffers)>(eglGetProcAddress("glGenRenderbuffers"));
    m_functionsTable.glDeleteRenderbuffers = reinterpret_cast<decltype(m_functionsTable.glDeleteRenderbuffers)>(eglGetProcAddress("glDeleteRenderbuffers"));
    m_functionsTable.glBindRenderbuffer = reinterpret_cast<decltype(m_functionsTable.glBindRenderbuffer)>(eglGetProcAddress("glBindRenderbuffer"));
    m_functionsTable.glRenderbufferStorage = reinterpret_cast<decltype(m_functionsTable.glRenderbufferStorage)>(eglGetProcAddress("glRenderbufferStorage"));
    m_functionsTable.glRenderbufferStorageMultisample = reinterpret_cast<decltype(m_functionsTable.glRenderbufferStorageMultisample)>(eglGetProcAddress("glRenderbufferStorageMultisample"));

    m_functionsTable.glCreateShader = reinterpret_cast<decltype(m_functionsTable.glCreateShader)>(eglGetProcAddress("glCreateShader"));
    m_functionsTable.glDeleteShader = reinterpret_cast<decltype(m_functionsTable.glDeleteShader)>(eglGetProcAddress("glDeleteShader"));
    m_functionsTable.glShaderSource = reinterpret_cast<decltype(m_functionsTable.glShaderSource)>(eglGetProcAddress("glShaderSource"));
    m_functionsTable.glCompileShader = reinterpret_cast<decltype(m_functionsTable.glCompileShader)>(eglGetProcAddress("glCompileShader"));
    m_functionsTable.glCreateProgram = reinterpret_cast<decltype(m_functionsTable.glCreateProgram)>(eglGetProcAddress("glCreateProgram"));
    m_functionsTable.glDeleteProgram = reinterpret_cast<decltype(m_functionsTable.glDeleteProgram)>(eglGetProcAddress("glDeleteProgram"));
    m_functionsTable.glAttachShader = reinterpret_cast<decltype(m_functionsTable.glAttachShader)>(eglGetProcAddress("glAttachShader"));
    m_functionsTable.glDetachShader = reinterpret_cast<decltype(m_functionsTable.glDetachShader)>(eglGetProcAddress("glDetachShader"));
    m_functionsTable.glLinkProgram = reinterpret_cast<decltype(m_functionsTable.glLinkProgram)>(eglGetProcAddress("glLinkProgram"));
    m_functionsTable.glUseProgram = reinterpret_cast<decltype(m_functionsTable.glUseProgram)>(eglGetProcAddress("glUseProgram"));
    m_functionsTable.glGetShaderiv = reinterpret_cast<decltype(m_functionsTable.glGetShaderiv)>(eglGetProcAddress("glGetShaderiv"));
    m_functionsTable.glGetProgramiv = reinterpret_cast<decltype(m_functionsTable.glGetProgramiv)>(eglGetProcAddress("glGetProgramiv"));
    m_functionsTable.glGetShaderInfoLog = reinterpret_cast<decltype(m_functionsTable.glGetShaderInfoLog)>(eglGetProcAddress("glGetShaderInfoLog"));
    m_functionsTable.glGetProgramInfoLog = reinterpret_cast<decltype(m_functionsTable.glGetProgramInfoLog)>(eglGetProcAddress("glGetProgramInfoLog"));
    m_functionsTable.glGetUniformLocation = reinterpret_cast<decltype(m_functionsTable.glGetUniformLocation)>(eglGetProcAddress("glGetUniformLocation"));
    m_functionsTable.glGetAttribLocation = reinterpret_cast<decltype(m_functionsTable.glGetAttribLocation)>(eglGetProcAddress("glGetAttribLocation"));
    m_functionsTable.glBindAttribLocation = reinterpret_cast<decltype(m_functionsTable.glBindAttribLocation)>(eglGetProcAddress("glBindAttribLocation"));
    m_functionsTable.glUniform1i = reinterpret_cast<decltype(m_functionsTable.glUniform1i)>(eglGetProcAddress("glUniform1i"));
    m_functionsTable.glUniform1f = reinterpret_cast<decltype(m_functionsTable.glUniform1f)>(eglGetProcAddress("glUniform1f"));
    m_functionsTable.glUniform2i = reinterpret_cast<decltype(m_functionsTable.glUniform2i)>(eglGetProcAddress("glUniform2i"));
    m_functionsTable.glUniform2f = reinterpret_cast<decltype(m_functionsTable.glUniform2f)>(eglGetProcAddress("glUniform2f"));
    m_functionsTable.glUniform3i = reinterpret_cast<decltype(m_functionsTable.glUniform3i)>(eglGetProcAddress("glUniform3i"));
    m_functionsTable.glUniform3f = reinterpret_cast<decltype(m_functionsTable.glUniform3f)>(eglGetProcAddress("glUniform3f"));
    m_functionsTable.glUniform4i = reinterpret_cast<decltype(m_functionsTable.glUniform4i)>(eglGetProcAddress("glUniform4i"));
    m_functionsTable.glUniform4f = reinterpret_cast<decltype(m_functionsTable.glUniform4f)>(eglGetProcAddress("glUniform4f"));
    m_functionsTable.glUniformMatrix2fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix2fv)>(eglGetProcAddress("glUniformMatrix2fv"));
    m_functionsTable.glUniformMatrix3fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix3fv)>(eglGetProcAddress("glUniformMatrix3fv"));
    m_functionsTable.glUniformMatrix4fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix4fv)>(eglGetProcAddress("glUniformMatrix4fv"));
    m_functionsTable.glUniform1iv = reinterpret_cast<decltype(m_functionsTable.glUniform1iv)>(eglGetProcAddress("glUniform1iv"));
    m_functionsTable.glUniform1fv = reinterpret_cast<decltype(m_functionsTable.glUniform1fv)>(eglGetProcAddress("glUniform1fv"));

    m_functionsTable.glGenVertexArrays = reinterpret_cast<decltype(m_functionsTable.glGenVertexArrays)>(eglGetProcAddress("glGenVertexArrays"));
    m_functionsTable.glDeleteVertexArrays = reinterpret_cast<decltype(m_functionsTable.glDeleteVertexArrays)>(eglGetProcAddress("glDeleteVertexArrays"));
    m_functionsTable.glBindVertexArray = reinterpret_cast<decltype(m_functionsTable.glBindVertexArray)>(eglGetProcAddress("glBindVertexArray"));
    m_functionsTable.glEnableVertexAttribArray = reinterpret_cast<decltype(m_functionsTable.glEnableVertexAttribArray)>(eglGetProcAddress("glEnableVertexAttribArray"));
    m_functionsTable.glDisableVertexAttribArray = reinterpret_cast<decltype(m_functionsTable.glDisableVertexAttribArray)>(eglGetProcAddress("glDisableVertexAttribArray"));
    m_functionsTable.glVertexAttribPointer = reinterpret_cast<decltype(m_functionsTable.glVertexAttribPointer)>(eglGetProcAddress("glVertexAttribPointer"));

    m_functionsTable.glGenSamplers = reinterpret_cast<decltype(m_functionsTable.glGenSamplers)>(eglGetProcAddress("glGenSamplers"));
    m_functionsTable.glDeleteSamplers = reinterpret_cast<decltype(m_functionsTable.glDeleteSamplers)>(eglGetProcAddress("glDeleteSamplers"));
    m_functionsTable.glBindSampler = reinterpret_cast<decltype(m_functionsTable.glBindSampler)>(eglGetProcAddress("glBindSampler"));
    m_functionsTable.glSamplerParameteri = reinterpret_cast<decltype(m_functionsTable.glSamplerParameteri)>(eglGetProcAddress("glSamplerParameteri"));
    m_functionsTable.glSamplerParameterf = reinterpret_cast<decltype(m_functionsTable.glSamplerParameterf)>(eglGetProcAddress("glSamplerParameterf"));

    m_functionsTable.glEnable = reinterpret_cast<decltype(m_functionsTable.glEnable)>(eglGetProcAddress("glEnable"));
    m_functionsTable.glDisable = reinterpret_cast<decltype(m_functionsTable.glDisable)>(eglGetProcAddress("glDisable"));
    m_functionsTable.glIsEnabled = reinterpret_cast<decltype(m_functionsTable.glIsEnabled)>(eglGetProcAddress("glIsEnabled"));
    m_functionsTable.glDepthFunc = reinterpret_cast<decltype(m_functionsTable.glDepthFunc)>(eglGetProcAddress("glDepthFunc"));
    m_functionsTable.glDepthMask = reinterpret_cast<decltype(m_functionsTable.glDepthMask)>(eglGetProcAddress("glDepthMask"));
    m_functionsTable.glDepthRangef = reinterpret_cast<decltype(m_functionsTable.glDepthRangef)>(eglGetProcAddress("glDepthRangef"));
    m_functionsTable.glStencilFunc = reinterpret_cast<decltype(m_functionsTable.glStencilFunc)>(eglGetProcAddress("glStencilFunc"));
    m_functionsTable.glStencilFuncSeparate = reinterpret_cast<decltype(m_functionsTable.glStencilFuncSeparate)>(eglGetProcAddress("glStencilFuncSeparate"));
    m_functionsTable.glStencilOp = reinterpret_cast<decltype(m_functionsTable.glStencilOp)>(eglGetProcAddress("glStencilOp"));
    m_functionsTable.glStencilOpSeparate = reinterpret_cast<decltype(m_functionsTable.glStencilOpSeparate)>(eglGetProcAddress("glStencilOpSeparate"));
    m_functionsTable.glStencilMask = reinterpret_cast<decltype(m_functionsTable.glStencilMask)>(eglGetProcAddress("glStencilMask"));
    m_functionsTable.glStencilMaskSeparate = reinterpret_cast<decltype(m_functionsTable.glStencilMaskSeparate)>(eglGetProcAddress("glStencilMaskSeparate"));
    m_functionsTable.glBlendFunc = reinterpret_cast<decltype(m_functionsTable.glBlendFunc)>(eglGetProcAddress("glBlendFunc"));
    m_functionsTable.glBlendFuncSeparate = reinterpret_cast<decltype(m_functionsTable.glBlendFuncSeparate)>(eglGetProcAddress("glBlendFuncSeparate"));
    m_functionsTable.glBlendEquation = reinterpret_cast<decltype(m_functionsTable.glBlendEquation)>(eglGetProcAddress("glBlendEquation"));
    m_functionsTable.glBlendEquationSeparate = reinterpret_cast<decltype(m_functionsTable.glBlendEquationSeparate)>(eglGetProcAddress("glBlendEquationSeparate"));
    m_functionsTable.glBlendColor = reinterpret_cast<decltype(m_functionsTable.glBlendColor)>(eglGetProcAddress("glBlendColor"));
    m_functionsTable.glColorMask = reinterpret_cast<decltype(m_functionsTable.glColorMask)>(eglGetProcAddress("glColorMask"));
    m_functionsTable.glCullFace = reinterpret_cast<decltype(m_functionsTable.glCullFace)>(eglGetProcAddress("glCullFace"));
    m_functionsTable.glFrontFace = reinterpret_cast<decltype(m_functionsTable.glFrontFace)>(eglGetProcAddress("glFrontFace"));
    m_functionsTable.glPolygonOffset = reinterpret_cast<decltype(m_functionsTable.glPolygonOffset)>(eglGetProcAddress("glPolygonOffset"));
    m_functionsTable.glLineWidth = reinterpret_cast<decltype(m_functionsTable.glLineWidth)>(eglGetProcAddress("glLineWidth"));
    m_functionsTable.glScissor = reinterpret_cast<decltype(m_functionsTable.glScissor)>(eglGetProcAddress("glScissor"));
    m_functionsTable.glViewport = reinterpret_cast<decltype(m_functionsTable.glViewport)>(eglGetProcAddress("glViewport"));
    m_functionsTable.glPixelStorei = reinterpret_cast<decltype(m_functionsTable.glPixelStorei)>(eglGetProcAddress("glPixelStorei"));
    m_functionsTable.glFlush = reinterpret_cast<decltype(m_functionsTable.glFlush)>(eglGetProcAddress("glFlush"));
    m_functionsTable.glFinish = reinterpret_cast<decltype(m_functionsTable.glFinish)>(eglGetProcAddress("glFinish"));
    m_functionsTable.glHint = reinterpret_cast<decltype(m_functionsTable.glHint)>(eglGetProcAddress("glHint"));
    m_functionsTable.glGetInternalformativ = reinterpret_cast<decltype(m_functionsTable.glGetInternalformativ)>(eglGetProcAddress("glGetInternalformativ"));

    m_functionsTable.glDrawArraysIndirect = reinterpret_cast<decltype(m_functionsTable.glDrawArraysIndirect)>(eglGetProcAddress("glDrawArraysIndirect"));
    m_functionsTable.glDrawElementsIndirect = reinterpret_cast<decltype(m_functionsTable.glDrawElementsIndirect)>(eglGetProcAddress("glDrawElementsIndirect"));
    m_functionsTable.glMultiDrawArrays = reinterpret_cast<decltype(m_functionsTable.glMultiDrawArrays)>(eglGetProcAddress("glMultiDrawArraysEXT"));
    if (!m_functionsTable.glMultiDrawArrays) {
        m_functionsTable.glMultiDrawArrays = reinterpret_cast<decltype(m_functionsTable.glMultiDrawArrays)>(eglGetProcAddress("glMultiDrawArrays"));
    }
    m_functionsTable.glMultiDrawElements = reinterpret_cast<decltype(m_functionsTable.glMultiDrawElements)>(eglGetProcAddress("glMultiDrawElementsEXT"));
    if (!m_functionsTable.glMultiDrawElements) {
        m_functionsTable.glMultiDrawElements = reinterpret_cast<decltype(m_functionsTable.glMultiDrawElements)>(eglGetProcAddress("glMultiDrawElements"));
    }
    m_functionsTable.glMultiDrawArraysIndirect = reinterpret_cast<decltype(m_functionsTable.glMultiDrawArraysIndirect)>(eglGetProcAddress("glMultiDrawArraysIndirect"));
    m_functionsTable.glMultiDrawElementsIndirect = reinterpret_cast<decltype(m_functionsTable.glMultiDrawElementsIndirect)>(eglGetProcAddress("glMultiDrawElementsIndirect"));
    m_functionsTable.glDrawArraysInstancedBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawArraysInstancedBaseInstance)>(eglGetProcAddress("glDrawArraysInstancedBaseInstanceEXT"));
    if (!m_functionsTable.glDrawArraysInstancedBaseInstance) {
        m_functionsTable.glDrawArraysInstancedBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawArraysInstancedBaseInstance)>(eglGetProcAddress("glDrawArraysInstancedBaseInstance"));
    }
    m_functionsTable.glDrawElementsInstancedBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseInstance)>(eglGetProcAddress("glDrawElementsInstancedBaseInstanceEXT"));
    if (!m_functionsTable.glDrawElementsInstancedBaseInstance) {
        m_functionsTable.glDrawElementsInstancedBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseInstance)>(eglGetProcAddress("glDrawElementsInstancedBaseInstance"));
    }
    m_functionsTable.glDrawElementsBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawElementsBaseVertex)>(eglGetProcAddress("glDrawElementsBaseVertexEXT"));
    if (!m_functionsTable.glDrawElementsBaseVertex) {
        m_functionsTable.glDrawElementsBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawElementsBaseVertex)>(eglGetProcAddress("glDrawElementsBaseVertex"));
    }
    m_functionsTable.glDrawRangeElementsBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawRangeElementsBaseVertex)>(eglGetProcAddress("glDrawRangeElementsBaseVertexEXT"));
    if (!m_functionsTable.glDrawRangeElementsBaseVertex) {
        m_functionsTable.glDrawRangeElementsBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawRangeElementsBaseVertex)>(eglGetProcAddress("glDrawRangeElementsBaseVertex"));
    }
    m_functionsTable.glDrawElementsInstancedBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseVertex)>(eglGetProcAddress("glDrawElementsInstancedBaseVertexEXT"));
    if (!m_functionsTable.glDrawElementsInstancedBaseVertex) {
        m_functionsTable.glDrawElementsInstancedBaseVertex = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseVertex)>(eglGetProcAddress("glDrawElementsInstancedBaseVertex"));
    }
    m_functionsTable.glDrawElementsInstancedBaseVertexBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseVertexBaseInstance)>(eglGetProcAddress("glDrawElementsInstancedBaseVertexBaseInstanceEXT"));
    if (!m_functionsTable.glDrawElementsInstancedBaseVertexBaseInstance) {
        m_functionsTable.glDrawElementsInstancedBaseVertexBaseInstance = reinterpret_cast<decltype(m_functionsTable.glDrawElementsInstancedBaseVertexBaseInstance)>(eglGetProcAddress("glDrawElementsInstancedBaseVertexBaseInstance"));
    }
    m_functionsTable.glDispatchCompute = reinterpret_cast<decltype(m_functionsTable.glDispatchCompute)>(eglGetProcAddress("glDispatchCompute"));
    m_functionsTable.glDispatchComputeIndirect = reinterpret_cast<decltype(m_functionsTable.glDispatchComputeIndirect)>(eglGetProcAddress("glDispatchComputeIndirect"));
    m_functionsTable.glBlitNamedFramebuffer = reinterpret_cast<decltype(m_functionsTable.glBlitNamedFramebuffer)>(eglGetProcAddress("glBlitNamedFramebuffer"));
    m_functionsTable.glCopyBufferSubData = reinterpret_cast<decltype(m_functionsTable.glCopyBufferSubData)>(eglGetProcAddress("glCopyBufferSubData"));
    m_functionsTable.glCompressedTexImage2D = reinterpret_cast<decltype(m_functionsTable.glCompressedTexImage2D)>(eglGetProcAddress("glCompressedTexImage2D"));
    m_functionsTable.glCompressedTexImage3D = reinterpret_cast<decltype(m_functionsTable.glCompressedTexImage3D)>(eglGetProcAddress("glCompressedTexImage3D"));
    m_functionsTable.glCompressedTexSubImage2D = reinterpret_cast<decltype(m_functionsTable.glCompressedTexSubImage2D)>(eglGetProcAddress("glCompressedTexSubImage2D"));
    m_functionsTable.glCompressedTexSubImage3D = reinterpret_cast<decltype(m_functionsTable.glCompressedTexSubImage3D)>(eglGetProcAddress("glCompressedTexSubImage3D"));
    m_functionsTable.glTexStorage2DMultisample = reinterpret_cast<decltype(m_functionsTable.glTexStorage2DMultisample)>(eglGetProcAddress("glTexStorage2DMultisample"));
    m_functionsTable.glTexStorage3DMultisample = reinterpret_cast<decltype(m_functionsTable.glTexStorage3DMultisample)>(eglGetProcAddress("glTexStorage3DMultisample"));
    m_functionsTable.glVertexAttribIPointer = reinterpret_cast<decltype(m_functionsTable.glVertexAttribIPointer)>(eglGetProcAddress("glVertexAttribIPointer"));
    m_functionsTable.glVertexAttribDivisor = reinterpret_cast<decltype(m_functionsTable.glVertexAttribDivisor)>(eglGetProcAddress("glVertexAttribDivisor"));
    m_functionsTable.glVertexAttrib1f = reinterpret_cast<decltype(m_functionsTable.glVertexAttrib1f)>(eglGetProcAddress("glVertexAttrib1f"));
    m_functionsTable.glVertexAttrib2f = reinterpret_cast<decltype(m_functionsTable.glVertexAttrib2f)>(eglGetProcAddress("glVertexAttrib2f"));
    m_functionsTable.glVertexAttrib3f = reinterpret_cast<decltype(m_functionsTable.glVertexAttrib3f)>(eglGetProcAddress("glVertexAttrib3f"));
    m_functionsTable.glVertexAttrib4f = reinterpret_cast<decltype(m_functionsTable.glVertexAttrib4f)>(eglGetProcAddress("glVertexAttrib4f"));
    m_functionsTable.glVertexAttribI4i = reinterpret_cast<decltype(m_functionsTable.glVertexAttribI4i)>(eglGetProcAddress("glVertexAttribI4i"));
    m_functionsTable.glVertexAttribI4ui = reinterpret_cast<decltype(m_functionsTable.glVertexAttribI4ui)>(eglGetProcAddress("glVertexAttribI4ui"));
    m_functionsTable.glVertexAttribBinding = reinterpret_cast<decltype(m_functionsTable.glVertexAttribBinding)>(eglGetProcAddress("glVertexAttribBinding"));
    m_functionsTable.glVertexBindingDivisor = reinterpret_cast<decltype(m_functionsTable.glVertexBindingDivisor)>(eglGetProcAddress("glVertexBindingDivisor"));
    m_functionsTable.glGetQueryObjecti64v = reinterpret_cast<decltype(m_functionsTable.glGetQueryObjecti64v)>(eglGetProcAddress("glGetQueryObjecti64v"));
    m_functionsTable.glPauseTransformFeedback = reinterpret_cast<decltype(m_functionsTable.glPauseTransformFeedback)>(eglGetProcAddress("glPauseTransformFeedback"));
    m_functionsTable.glResumeTransformFeedback = reinterpret_cast<decltype(m_functionsTable.glResumeTransformFeedback)>(eglGetProcAddress("glResumeTransformFeedback"));
    m_functionsTable.glMapBuffer = reinterpret_cast<decltype(m_functionsTable.glMapBuffer)>(eglGetProcAddress("glMapBufferOES"));
    m_functionsTable.glFlushMappedBufferRange = reinterpret_cast<decltype(m_functionsTable.glFlushMappedBufferRange)>(eglGetProcAddress("glFlushMappedBufferRange"));
    m_functionsTable.glGetInteger64v = reinterpret_cast<decltype(m_functionsTable.glGetInteger64v)>(eglGetProcAddress("glGetInteger64v"));
    m_functionsTable.glGetDoublev = reinterpret_cast<decltype(m_functionsTable.glGetDoublev)>(eglGetProcAddress("glGetDoublev"));
    m_functionsTable.glUniform2iv = reinterpret_cast<decltype(m_functionsTable.glUniform2iv)>(eglGetProcAddress("glUniform2iv"));
    m_functionsTable.glUniform2fv = reinterpret_cast<decltype(m_functionsTable.glUniform2fv)>(eglGetProcAddress("glUniform2fv"));
    m_functionsTable.glUniform3iv = reinterpret_cast<decltype(m_functionsTable.glUniform3iv)>(eglGetProcAddress("glUniform3iv"));
    m_functionsTable.glUniform3fv = reinterpret_cast<decltype(m_functionsTable.glUniform3fv)>(eglGetProcAddress("glUniform3fv"));
    m_functionsTable.glUniform4iv = reinterpret_cast<decltype(m_functionsTable.glUniform4iv)>(eglGetProcAddress("glUniform4iv"));
    m_functionsTable.glUniform4fv = reinterpret_cast<decltype(m_functionsTable.glUniform4fv)>(eglGetProcAddress("glUniform4fv"));
    m_functionsTable.glUniformMatrix2x3fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix2x3fv)>(eglGetProcAddress("glUniformMatrix2x3fv"));
    m_functionsTable.glUniformMatrix3x2fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix3x2fv)>(eglGetProcAddress("glUniformMatrix3x2fv"));
    m_functionsTable.glUniformMatrix2x4fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix2x4fv)>(eglGetProcAddress("glUniformMatrix2x4fv"));
    m_functionsTable.glUniformMatrix4x2fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix4x2fv)>(eglGetProcAddress("glUniformMatrix4x2fv"));
    m_functionsTable.glUniformMatrix3x4fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix3x4fv)>(eglGetProcAddress("glUniformMatrix3x4fv"));
    m_functionsTable.glUniformMatrix4x3fv = reinterpret_cast<decltype(m_functionsTable.glUniformMatrix4x3fv)>(eglGetProcAddress("glUniformMatrix4x3fv"));
    m_functionsTable.glUniform1ui = reinterpret_cast<decltype(m_functionsTable.glUniform1ui)>(eglGetProcAddress("glUniform1ui"));
    m_functionsTable.glUniform2ui = reinterpret_cast<decltype(m_functionsTable.glUniform2ui)>(eglGetProcAddress("glUniform2ui"));
    m_functionsTable.glUniform3ui = reinterpret_cast<decltype(m_functionsTable.glUniform3ui)>(eglGetProcAddress("glUniform3ui"));
    m_functionsTable.glUniform4ui = reinterpret_cast<decltype(m_functionsTable.glUniform4ui)>(eglGetProcAddress("glUniform4ui"));
    m_functionsTable.glUniform1uiv = reinterpret_cast<decltype(m_functionsTable.glUniform1uiv)>(eglGetProcAddress("glUniform1uiv"));
    m_functionsTable.glUniform2uiv = reinterpret_cast<decltype(m_functionsTable.glUniform2uiv)>(eglGetProcAddress("glUniform2uiv"));
    m_functionsTable.glUniform3uiv = reinterpret_cast<decltype(m_functionsTable.glUniform3uiv)>(eglGetProcAddress("glUniform3uiv"));
    m_functionsTable.glUniform4uiv = reinterpret_cast<decltype(m_functionsTable.glUniform4uiv)>(eglGetProcAddress("glUniform4uiv"));
    m_functionsTable.glBindFragDataLocation = reinterpret_cast<decltype(m_functionsTable.glBindFragDataLocation)>(eglGetProcAddress("glBindFragDataLocation"));
    m_functionsTable.glBindFragDataLocationIndexed = reinterpret_cast<decltype(m_functionsTable.glBindFragDataLocationIndexed)>(eglGetProcAddress("glBindFragDataLocationIndexed"));
    m_functionsTable.glGetFragDataIndex = reinterpret_cast<decltype(m_functionsTable.glGetFragDataIndex)>(eglGetProcAddress("glGetFragDataIndex"));
    m_functionsTable.glGetFragDataLocation = reinterpret_cast<decltype(m_functionsTable.glGetFragDataLocation)>(eglGetProcAddress("glGetFragDataLocation"));
    m_functionsTable.glProgramUniform1i = reinterpret_cast<decltype(m_functionsTable.glProgramUniform1i)>(eglGetProcAddress("glProgramUniform1i"));
    m_functionsTable.glProgramUniform1f = reinterpret_cast<decltype(m_functionsTable.glProgramUniform1f)>(eglGetProcAddress("glProgramUniform1f"));
    m_functionsTable.glProgramUniform2i = reinterpret_cast<decltype(m_functionsTable.glProgramUniform2i)>(eglGetProcAddress("glProgramUniform2i"));
    m_functionsTable.glProgramUniform2f = reinterpret_cast<decltype(m_functionsTable.glProgramUniform2f)>(eglGetProcAddress("glProgramUniform2f"));
    m_functionsTable.glProgramUniform3i = reinterpret_cast<decltype(m_functionsTable.glProgramUniform3i)>(eglGetProcAddress("glProgramUniform3i"));
    m_functionsTable.glProgramUniform3f = reinterpret_cast<decltype(m_functionsTable.glProgramUniform3f)>(eglGetProcAddress("glProgramUniform3f"));
    m_functionsTable.glProgramUniform4i = reinterpret_cast<decltype(m_functionsTable.glProgramUniform4i)>(eglGetProcAddress("glProgramUniform4i"));
    m_functionsTable.glProgramUniform4f = reinterpret_cast<decltype(m_functionsTable.glProgramUniform4f)>(eglGetProcAddress("glProgramUniform4f"));
    m_functionsTable.glProgramUniformMatrix2fv = reinterpret_cast<decltype(m_functionsTable.glProgramUniformMatrix2fv)>(eglGetProcAddress("glProgramUniformMatrix2fv"));
    m_functionsTable.glProgramUniformMatrix3fv = reinterpret_cast<decltype(m_functionsTable.glProgramUniformMatrix3fv)>(eglGetProcAddress("glProgramUniformMatrix3fv"));
    m_functionsTable.glProgramUniformMatrix4fv = reinterpret_cast<decltype(m_functionsTable.glProgramUniformMatrix4fv)>(eglGetProcAddress("glProgramUniformMatrix4fv"));
    m_functionsTable.glTransformFeedbackVaryings = reinterpret_cast<decltype(m_functionsTable.glTransformFeedbackVaryings)>(eglGetProcAddress("glTransformFeedbackVaryings"));
    m_functionsTable.glGetTransformFeedbackVarying = reinterpret_cast<decltype(m_functionsTable.glGetTransformFeedbackVarying)>(eglGetProcAddress("glGetTransformFeedbackVarying"));
    m_functionsTable.glSamplerParameteriv = reinterpret_cast<decltype(m_functionsTable.glSamplerParameteriv)>(eglGetProcAddress("glSamplerParameteriv"));
    m_functionsTable.glSamplerParameterfv = reinterpret_cast<decltype(m_functionsTable.glSamplerParameterfv)>(eglGetProcAddress("glSamplerParameterfv"));

    MGLOG_D("EGL functions initialized");
}

} // namespace MG_Backend
} // namespace MobileGL