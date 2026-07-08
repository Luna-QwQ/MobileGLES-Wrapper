/*
 * MobileGL-DirectGLES Branch
 * Copyright (C) 2025 MobileGL Contributors
 *
 * BackendObject_DirectGLES.h - BackendObject subclass for DirectGLES backend.
 * Handles EGL context lifecycle, capability probing, and backend function
 * table population.
 */

#pragma once

#include <string>
#include <EGL/egl.h>
#include <GLES3/gl32.h>

// ---------------------------------------------------------------------------
// Abstract base class for backend objects
// ---------------------------------------------------------------------------

class BackendObject {
public:
    virtual ~BackendObject() = default;

    virtual bool Initialize() = 0;
    virtual void InitCapabilities() = 0;
    virtual void* InitWindowSurface(void* nativeWindow) = 0;
    virtual void* InitPbufferSurface(GLint width, GLint height) = 0;
    virtual bool MakeCurrent(void* surface) = 0;
    virtual bool ReleaseCurrent(void* surface) = 0;
    virtual bool Present(void* surface) = 0;
    virtual std::string GetRendererInfo() const = 0;
    virtual std::string GetBackendAPIVersionString() const = 0;
    virtual void GetBackendFunctions(void* outTable) = 0;
    virtual void UpdateDynamicBackendParameters() = 0;
    virtual void DestroyEGLContext() = 0;
};

// ---------------------------------------------------------------------------
// BackendObject_DirectGLES
// ---------------------------------------------------------------------------

class BackendObject_DirectGLES : public BackendObject {
public:
    BackendObject_DirectGLES();
    ~BackendObject_DirectGLES() override;

    // Initialize the backend: load EGL/GLES function pointers, create EGL context
    bool Initialize() override;

    // Probe GLES capabilities (extensions, limits, format support)
    void InitCapabilities() override;

    // Create a window surface for rendering
    void* InitWindowSurface(void* nativeWindow) override;

    // Create a pbuffer surface for off-screen rendering
    void* InitPbufferSurface(GLint width, GLint height) override;

    // Make the EGL context current for the given surface
    bool MakeCurrent(void* surface) override;

    // Release the current EGL context
    bool ReleaseCurrent(void* surface) override;

    // Present the rendered frame (eglSwapBuffers)
    bool Present(void* surface) override;

    // Get renderer info string (e.g., "Espryt")
    std::string GetRendererInfo() const override;

    // Get backend API version string (e.g., "Direct (OpenGL ES) 3.2")
    std::string GetBackendAPIVersionString() const override;

    // Populate the GlobalBackendFunctionsTable with DirectGLES function pointers
    void GetBackendFunctions(void* outTable) override;

    // Update dynamic backend parameters from GLES capabilities
    void UpdateDynamicBackendParameters() override;

    // Destroy the EGL context and clean up
    void DestroyEGLContext() override;

    // EGL lifecycle methods
    void SetEGLFuncsTable(void* table);
    void SetGLESFuncsTable(void* table);
    void SetGLESCapabilities(void* caps);

private:
    // Load EGL function pointers
    bool LoadEGLFunctions();

    // Load GLES function pointers
    bool LoadGLESFunctions();

    // Probe extension support
    void ProbeExtensions();

    // Probe format capabilities
    void ProbeFormatCapabilities();

    // Probe system limits
    void ProbeLimits();

    // Create the EGL context
    bool CreateEGLContext();

    // Choose an EGL config
    bool ChooseEGLConfig(EGLConfig& outConfig);

    // EGL display
    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    EGLConfig  m_eglConfig;

    // Current surface
    EGLSurface m_currentDrawSurface;
    EGLSurface m_currentReadSurface;

    // Whether initialized
    bool m_initialized;
    bool m_contextCreated;

    // Backend info
    std::string m_rendererInfo;
    std::string m_versionString;
    int m_glESMajorVersion;
    int m_glESMinorVersion;
};