#include "Includes.h"
#include "Backend/BackendObjects.h"

using namespace MobileGL::MG_Backend;

#include <dlfcn.h>

extern "C" {
#include <EGL/egl.h>
#include <EGL/eglext.h>
}

// ========================================================================
// EGL functions delegate to the real EGL implementation via dlsym.
// We use RTLD_NEXT to find the next occurrence of each symbol in the
// dynamic linker's search order, avoiding infinite recursion.
//
// For eglSwapBuffers, we delegate to the BackendObject's method so that
// the backend can track frame state.
// ========================================================================

// Helper macro to declare a cached real function pointer and call it
#define EGL_REAL_FUNC(name, ret, ...)                              \
    static ret (*real_##name)(__VA_ARGS__) = nullptr;              \
    if (!real_##name) {                                            \
        real_##name = (ret(*)(__VA_ARGS__))dlsym(RTLD_NEXT, #name); \
    }                                                              \
    if (real_##name) return real_##name(__VA_ARGS__);              \
    return (ret)0

// ========================================================================
// EGL Display Functions
// ========================================================================

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id) {
    typedef EGLDisplay (*fn_t)(EGLNativeDisplayType);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetDisplay");
    if (real_fn) return real_fn(display_id);
    return EGL_NO_DISPLAY;
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLint*, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglInitialize");
    if (real_fn) return real_fn(dpy, major, minor);
    return EGL_FALSE;
}

EGLBoolean eglTerminate(EGLDisplay dpy) {
    typedef EGLBoolean (*fn_t)(EGLDisplay);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglTerminate");
    if (real_fn) return real_fn(dpy);
    return EGL_FALSE;
}

EGLBoolean eglBindAPI(EGLenum api) {
    typedef EGLBoolean (*fn_t)(EGLenum);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglBindAPI");
    if (real_fn) return real_fn(api);
    return EGL_FALSE;
}

EGLenum eglQueryAPI() {
    typedef EGLenum (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglQueryAPI");
    if (real_fn) return real_fn();
    return EGL_OPENGL_ES_API;
}

const char* eglQueryString(EGLDisplay dpy, EGLint name) {
    typedef const char* (*fn_t)(EGLDisplay, EGLint);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglQueryString");
    if (real_fn) return real_fn(dpy, name);
    return nullptr;
}

// ========================================================================
// EGL Configuration Functions
// ========================================================================

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglChooseConfig");
    if (real_fn) return real_fn(dpy, attrib_list, configs, config_size, num_config);
    return EGL_FALSE;
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLConfig*, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetConfigs");
    if (real_fn) return real_fn(dpy, configs, config_size, num_config);
    return EGL_FALSE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLConfig, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetConfigAttrib");
    if (real_fn) return real_fn(dpy, config, attribute, value);
    return EGL_FALSE;
}

// ========================================================================
// EGL Surface Functions
// ========================================================================

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list) {
    typedef EGLSurface (*fn_t)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreateWindowSurface");
    if (real_fn) return real_fn(dpy, config, win, attrib_list);
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list) {
    typedef EGLSurface (*fn_t)(EGLDisplay, EGLConfig, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreatePbufferSurface");
    if (real_fn) return real_fn(dpy, config, attrib_list);
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint* attrib_list) {
    typedef EGLSurface (*fn_t)(EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreatePixmapSurface");
    if (real_fn) return real_fn(dpy, config, pixmap, attrib_list);
    return EGL_NO_SURFACE;
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSurface);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglDestroySurface");
    if (real_fn) return real_fn(dpy, surface);
    return EGL_FALSE;
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSurface, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglQuerySurface");
    if (real_fn) return real_fn(dpy, surface, attribute, value);
    return EGL_FALSE;
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSurface, EGLint, EGLint);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglSurfaceAttrib");
    if (real_fn) return real_fn(dpy, surface, attribute, value);
    return EGL_FALSE;
}

// ========================================================================
// EGL Context Functions
// ========================================================================

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list) {
    typedef EGLContext (*fn_t)(EGLDisplay, EGLConfig, EGLContext, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreateContext");
    if (real_fn) return real_fn(dpy, config, share_context, attrib_list);
    return EGL_NO_CONTEXT;
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLContext);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglDestroyContext");
    if (real_fn) return real_fn(dpy, ctx);
    return EGL_FALSE;
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglMakeCurrent");
    if (real_fn) return real_fn(dpy, draw, read, ctx);
    return EGL_FALSE;
}

EGLContext eglGetCurrentContext() {
    typedef EGLContext (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetCurrentContext");
    if (real_fn) return real_fn();
    return EGL_NO_CONTEXT;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw) {
    typedef EGLSurface (*fn_t)(EGLint);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetCurrentSurface");
    if (real_fn) return real_fn(readdraw);
    return EGL_NO_SURFACE;
}

EGLDisplay eglGetCurrentDisplay() {
    typedef EGLDisplay (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetCurrentDisplay");
    if (real_fn) return real_fn();
    return EGL_NO_DISPLAY;
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLContext, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglQueryContext");
    if (real_fn) return real_fn(dpy, ctx, attribute, value);
    return EGL_FALSE;
}

// ========================================================================
// EGL Swap Buffers
// ========================================================================

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // Delegate to the backend's SwapEGLBuffers if available, so the
    // backend can track frame state and perform any necessary
    // post-processing.
    if (pActiveBackendObject != nullptr) {
        return pActiveBackendObject->SwapEGLBuffers() ? EGL_TRUE : EGL_FALSE;
    }

    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSurface);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglSwapBuffers");
    if (real_fn) return real_fn(dpy, surface);
    return EGL_FALSE;
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLint);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglSwapInterval");
    if (real_fn) return real_fn(dpy, interval);
    return EGL_FALSE;
}

// ========================================================================
// EGL Utility Functions
// ========================================================================

EGLint eglGetError() {
    typedef EGLint (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetError");
    if (real_fn) return real_fn();
    return EGL_SUCCESS;
}

EGLBoolean eglReleaseThread() {
    typedef EGLBoolean (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglReleaseThread");
    if (real_fn) return real_fn();
    return EGL_FALSE;
}

void (*eglGetProcAddress(const char* procname))() {
    typedef void (*return_t)();
    typedef return_t (*fn_t)(const char*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetProcAddress");
    if (real_fn) return real_fn(procname);
    return nullptr;
}

EGLBoolean eglWaitClient() {
    typedef EGLBoolean (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglWaitClient");
    if (real_fn) return real_fn();
    return EGL_FALSE;
}

EGLBoolean eglWaitGL() {
    typedef EGLBoolean (*fn_t)();
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglWaitGL");
    if (real_fn) return real_fn();
    return EGL_FALSE;
}

EGLBoolean eglWaitNative(EGLint engine) {
    typedef EGLBoolean (*fn_t)(EGLint);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglWaitNative");
    if (real_fn) return real_fn(engine);
    return EGL_FALSE;
}

// ========================================================================
// EGL Image Functions (EGL 1.4 / KHR_image_base)
// ========================================================================

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint* attrib_list) {
    typedef EGLImageKHR (*fn_t)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreateImageKHR");
    if (real_fn) return real_fn(dpy, ctx, target, buffer, attrib_list);
    return EGL_NO_IMAGE_KHR;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLImageKHR);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglDestroyImageKHR");
    if (real_fn) return real_fn(dpy, image);
    return EGL_FALSE;
}

// ========================================================================
// EGL Sync Functions (KHR_fence_sync)
// ========================================================================

EGLSyncKHR eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint* attrib_list) {
    typedef EGLSyncKHR (*fn_t)(EGLDisplay, EGLenum, const EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglCreateSyncKHR");
    if (real_fn) return real_fn(dpy, type, attrib_list);
    return EGL_NO_SYNC_KHR;
}

EGLBoolean eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSyncKHR);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglDestroySyncKHR");
    if (real_fn) return real_fn(dpy, sync);
    return EGL_FALSE;
}

EGLint eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) {
    typedef EGLint (*fn_t)(EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglClientWaitSyncKHR");
    if (real_fn) return real_fn(dpy, sync, flags, timeout);
    return EGL_FALSE;
}

EGLBoolean eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint* value) {
    typedef EGLBoolean (*fn_t)(EGLDisplay, EGLSyncKHR, EGLint, EGLint*);
    static fn_t real_fn = nullptr;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "eglGetSyncAttribKHR");
    if (real_fn) return real_fn(dpy, sync, attribute, value);
    return EGL_FALSE;
}