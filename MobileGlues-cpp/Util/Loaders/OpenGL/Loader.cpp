#include "Loader.h"
#include "Util/Debug/Log.h"

#include <cstring>
#include <dlfcn.h>

#ifndef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#endif
#ifndef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#endif

namespace MobileGL {
namespace MG_External {

// Helper to load a function symbol from the current process or a specified library
static void* LoadFunction(const char* name) {
    // Try to get the symbol from the global symbol table first
    void* func = dlsym(RTLD_DEFAULT, name);
    if (!func) {
        // Try to load from libGLESv2.so
        static void* libGLES = dlopen("libGLESv2.so", RTLD_NOW | RTLD_GLOBAL);
        if (libGLES) {
            func = dlsym(libGLES, name);
        }
    }
    if (!func) {
        // Try libGLESv3.so
        static void* libGLES3 = dlopen("libGLESv3.so", RTLD_NOW | RTLD_GLOBAL);
        if (libGLES3) {
            func = dlsym(libGLES3, name);
        }
    }
    return func;
}

static void* LoadEGLFunction(const char* name) {
    void* func = dlsym(RTLD_DEFAULT, name);
    if (!func) {
        static void* libEGL = dlopen("libEGL.so", RTLD_NOW | RTLD_GLOBAL);
        if (libEGL) {
            func = dlsym(libEGL, name);
        }
    }
    return func;
}

#define LOAD_FUNC(table, name) table.name = reinterpret_cast<decltype(table.name)>(LoadFunction(#name))
#define LOAD_EGL_FUNC(table, name) table.name = reinterpret_cast<decltype(table.name)>(LoadEGLFunction(#name))

bool AcquireEGLFunctions(EGLFunctionsTable& table) {
    memset(&table, 0, sizeof(table));

    LOAD_EGL_FUNC(table, GetDisplay);
    LOAD_EGL_FUNC(table, Initialize);
    LOAD_EGL_FUNC(table, Terminate);
    LOAD_EGL_FUNC(table, ChooseConfig);
    LOAD_EGL_FUNC(table, GetConfigs);
    LOAD_EGL_FUNC(table, GetConfigAttrib);
    LOAD_EGL_FUNC(table, CreateWindowSurface);
    LOAD_EGL_FUNC(table, CreatePbufferSurface);
    LOAD_EGL_FUNC(table, DestroySurface);
    LOAD_EGL_FUNC(table, CreateContext);
    LOAD_EGL_FUNC(table, DestroyContext);
    LOAD_EGL_FUNC(table, MakeCurrent);
    LOAD_EGL_FUNC(table, SwapBuffers);
    LOAD_EGL_FUNC(table, QuerySurface);
    LOAD_EGL_FUNC(table, GetError);
    LOAD_EGL_FUNC(table, ReleaseThread);
    LOAD_EGL_FUNC(table, GetProcAddress);

    // Verify critical functions
    if (!table.GetDisplay || !table.Initialize || !table.ChooseConfig ||
        !table.CreateContext || !table.MakeCurrent || !table.SwapBuffers) {
        MGLOG_E("AcquireEGLFunctions: Failed to load critical EGL functions!");
        return false;
    }

    MGLOG_I("AcquireEGLFunctions: All EGL functions loaded successfully.");
    return true;
}

bool AcquireGLESFunctions(GLESFunctionsTable& table) {
    memset(&table, 0, sizeof(table));

    // Drawing
    LOAD_FUNC(table, DrawArrays);
    LOAD_FUNC(table, DrawElements);
    LOAD_FUNC(table, DrawArraysInstanced);
    LOAD_FUNC(table, DrawElementsInstanced);
    LOAD_FUNC(table, DrawRangeElements);
    LOAD_FUNC(table, DrawArraysIndirect);
    LOAD_FUNC(table, DrawElementsIndirect);
    LOAD_FUNC(table, MultiDrawArrays);
    LOAD_FUNC(table, MultiDrawElements);
    LOAD_FUNC(table, MultiDrawArraysIndirect);
    LOAD_FUNC(table, MultiDrawElementsIndirect);
    LOAD_FUNC(table, DrawElementsBaseVertex);
    LOAD_FUNC(table, DrawRangeElementsBaseVertex);
    LOAD_FUNC(table, DrawElementsInstancedBaseVertex);
    LOAD_FUNC(table, MultiDrawElementsBaseVertex);
    LOAD_FUNC(table, DrawElementsInstancedBaseInstance);
    LOAD_FUNC(table, DrawArraysInstancedBaseInstance);
    LOAD_FUNC(table, DrawElementsInstancedBaseVertexBaseInstance);

    // State
    LOAD_FUNC(table, Enable);
    LOAD_FUNC(table, Disable);
    LOAD_FUNC(table, IsEnabled);
    LOAD_FUNC(table, GetBooleanv);
    LOAD_FUNC(table, GetFloatv);
    LOAD_FUNC(table, GetIntegerv);
    LOAD_FUNC(table, GetInteger64v);
    LOAD_FUNC(table, GetDoublev);
    LOAD_FUNC(table, GetIntegeri_v);
    LOAD_FUNC(table, GetInteger64i_v);
    LOAD_FUNC(table, GetString);
    LOAD_FUNC(table, GetError);

    // Viewport/Scissor
    LOAD_FUNC(table, Viewport);
    LOAD_FUNC(table, Scissor);

    // Clear
    LOAD_FUNC(table, Clear);
    LOAD_FUNC(table, ClearColor);
    LOAD_FUNC(table, ClearDepthf);
    LOAD_FUNC(table, ClearStencil);
    LOAD_FUNC(table, ClearBufferfv);
    LOAD_FUNC(table, ClearBufferiv);
    LOAD_FUNC(table, ClearBufferuiv);
    LOAD_FUNC(table, ClearBufferfi);

    // Blending
    LOAD_FUNC(table, BlendFunc);
    LOAD_FUNC(table, BlendFuncSeparate);
    LOAD_FUNC(table, BlendEquation);
    LOAD_FUNC(table, BlendEquationSeparate);
    LOAD_FUNC(table, BlendColor);

    // Culling
    LOAD_FUNC(table, CullFace);
    LOAD_FUNC(table, FrontFace);

    // Depth
    LOAD_FUNC(table, DepthFunc);
    LOAD_FUNC(table, DepthMask);
    LOAD_FUNC(table, DepthRangef);

    // Stencil
    LOAD_FUNC(table, StencilFunc);
    LOAD_FUNC(table, StencilFuncSeparate);
    LOAD_FUNC(table, StencilOp);
    LOAD_FUNC(table, StencilOpSeparate);
    LOAD_FUNC(table, StencilMask);
    LOAD_FUNC(table, StencilMaskSeparate);

    // Polygon
    LOAD_FUNC(table, PolygonOffset);
    LOAD_FUNC(table, LineWidth);

    // Pixel Store
    LOAD_FUNC(table, PixelStorei);

    // Textures
    LOAD_FUNC(table, GenTextures);
    LOAD_FUNC(table, DeleteTextures);
    LOAD_FUNC(table, BindTexture);
    LOAD_FUNC(table, TexImage2D);
    LOAD_FUNC(table, TexImage3D);
    LOAD_FUNC(table, TexSubImage2D);
    LOAD_FUNC(table, TexSubImage3D);
    LOAD_FUNC(table, CopyTexImage2D);
    LOAD_FUNC(table, CopyTexSubImage2D);
    LOAD_FUNC(table, CompressedTexImage2D);
    LOAD_FUNC(table, CompressedTexSubImage2D);
    LOAD_FUNC(table, TexParameteri);
    LOAD_FUNC(table, TexParameterf);
    LOAD_FUNC(table, TexParameteriv);
    LOAD_FUNC(table, TexParameterfv);
    LOAD_FUNC(table, GetTexParameteriv);
    LOAD_FUNC(table, GetTexParameterfv);
    LOAD_FUNC(table, GetTexLevelParameteriv);
    LOAD_FUNC(table, GetTexLevelParameterfv);
    LOAD_FUNC(table, GenerateMipmap);
    LOAD_FUNC(table, ActiveTexture);
    LOAD_FUNC(table, TexStorage2D);
    LOAD_FUNC(table, TexStorage3D);
    LOAD_FUNC(table, GetTexImage);
    LOAD_FUNC(table, GetCompressedTexImage);
    LOAD_FUNC(table, TexBuffer);

    // Buffers
    LOAD_FUNC(table, GenBuffers);
    LOAD_FUNC(table, DeleteBuffers);
    LOAD_FUNC(table, BindBuffer);
    LOAD_FUNC(table, BufferData);
    LOAD_FUNC(table, BufferSubData);
    LOAD_FUNC(table, MapBufferRange);
    LOAD_FUNC(table, UnmapBuffer);
    LOAD_FUNC(table, FlushMappedBufferRange);
    LOAD_FUNC(table, BindBufferBase);
    LOAD_FUNC(table, BindBufferRange);
    LOAD_FUNC(table, CopyBufferSubData);

    // Vertex Arrays
    LOAD_FUNC(table, GenVertexArrays);
    LOAD_FUNC(table, DeleteVertexArrays);
    LOAD_FUNC(table, BindVertexArray);
    LOAD_FUNC(table, VertexAttribPointer);
    LOAD_FUNC(table, VertexAttribIPointer);
    LOAD_FUNC(table, EnableVertexAttribArray);
    LOAD_FUNC(table, DisableVertexAttribArray);
    LOAD_FUNC(table, VertexAttribDivisor);

    // Framebuffers
    LOAD_FUNC(table, GenFramebuffers);
    LOAD_FUNC(table, DeleteFramebuffers);
    LOAD_FUNC(table, BindFramebuffer);
    LOAD_FUNC(table, FramebufferTexture2D);
    LOAD_FUNC(table, FramebufferTextureLayer);
    LOAD_FUNC(table, FramebufferRenderbuffer);
    LOAD_FUNC(table, CheckFramebufferStatus);
    LOAD_FUNC(table, DrawBuffers);
    LOAD_FUNC(table, ReadBuffer);
    LOAD_FUNC(table, BlitFramebuffer);

    // Renderbuffers
    LOAD_FUNC(table, GenRenderbuffers);
    LOAD_FUNC(table, DeleteRenderbuffers);
    LOAD_FUNC(table, BindRenderbuffer);
    LOAD_FUNC(table, RenderbufferStorage);
    LOAD_FUNC(table, RenderbufferStorageMultisample);

    // Programs/Shaders
    LOAD_FUNC(table, CreateProgram);
    LOAD_FUNC(table, DeleteProgram);
    LOAD_FUNC(table, UseProgram);
    LOAD_FUNC(table, CreateShader);
    LOAD_FUNC(table, DeleteShader);
    LOAD_FUNC(table, ShaderSource);
    LOAD_FUNC(table, CompileShader);
    LOAD_FUNC(table, AttachShader);
    LOAD_FUNC(table, DetachShader);
    LOAD_FUNC(table, LinkProgram);
    LOAD_FUNC(table, GetShaderiv);
    LOAD_FUNC(table, GetProgramiv);
    LOAD_FUNC(table, GetShaderInfoLog);
    LOAD_FUNC(table, GetProgramInfoLog);
    LOAD_FUNC(table, ValidateProgram);
    LOAD_FUNC(table, GetUniformLocation);
    LOAD_FUNC(table, GetAttribLocation);
    LOAD_FUNC(table, GetActiveUniform);
    LOAD_FUNC(table, GetActiveAttrib);
    LOAD_FUNC(table, BindAttribLocation);

    // Uniforms
    LOAD_FUNC(table, Uniform1f);
    LOAD_FUNC(table, Uniform2f);
    LOAD_FUNC(table, Uniform3f);
    LOAD_FUNC(table, Uniform4f);
    LOAD_FUNC(table, Uniform1i);
    LOAD_FUNC(table, Uniform2i);
    LOAD_FUNC(table, Uniform3i);
    LOAD_FUNC(table, Uniform4i);
    LOAD_FUNC(table, Uniform1ui);
    LOAD_FUNC(table, Uniform2ui);
    LOAD_FUNC(table, Uniform3ui);
    LOAD_FUNC(table, Uniform4ui);
    LOAD_FUNC(table, Uniform1fv);
    LOAD_FUNC(table, Uniform2fv);
    LOAD_FUNC(table, Uniform3fv);
    LOAD_FUNC(table, Uniform4fv);
    LOAD_FUNC(table, Uniform1iv);
    LOAD_FUNC(table, Uniform2iv);
    LOAD_FUNC(table, Uniform3iv);
    LOAD_FUNC(table, Uniform4iv);
    LOAD_FUNC(table, Uniform1uiv);
    LOAD_FUNC(table, Uniform2uiv);
    LOAD_FUNC(table, Uniform3uiv);
    LOAD_FUNC(table, Uniform4uiv);
    LOAD_FUNC(table, UniformMatrix2fv);
    LOAD_FUNC(table, UniformMatrix3fv);
    LOAD_FUNC(table, UniformMatrix4fv);
    LOAD_FUNC(table, UniformMatrix2x3fv);
    LOAD_FUNC(table, UniformMatrix3x2fv);
    LOAD_FUNC(table, UniformMatrix2x4fv);
    LOAD_FUNC(table, UniformMatrix4x2fv);
    LOAD_FUNC(table, UniformMatrix3x4fv);
    LOAD_FUNC(table, UniformMatrix4x3fv);

    // Program Resources
    LOAD_FUNC(table, GetProgramInterfaceiv);
    LOAD_FUNC(table, GetProgramResourceIndex);
    LOAD_FUNC(table, GetProgramResourceName);
    LOAD_FUNC(table, GetProgramResourceiv);
    LOAD_FUNC(table, GetProgramResourceLocation);
    LOAD_FUNC(table, ShaderStorageBlockBinding);

    // Compute
    LOAD_FUNC(table, DispatchCompute);
    LOAD_FUNC(table, DispatchComputeIndirect);

    // Memory Barriers
    LOAD_FUNC(table, MemoryBarrier);
    LOAD_FUNC(table, MemoryBarrierByRegion);

    // Samplers
    LOAD_FUNC(table, GenSamplers);
    LOAD_FUNC(table, DeleteSamplers);
    LOAD_FUNC(table, BindSampler);
    LOAD_FUNC(table, SamplerParameteri);
    LOAD_FUNC(table, SamplerParameterf);

    // Image Load/Store
    LOAD_FUNC(table, BindImageTexture);

    // ReadPixels
    LOAD_FUNC(table, ReadPixels);

    // CopyImageSubData
    LOAD_FUNC(table, CopyImageSubData);

    // Flush/Finish
    LOAD_FUNC(table, Flush);
    LOAD_FUNC(table, Finish);

    // Verify critical functions
    if (!table.DrawArrays || !table.DrawElements || !table.Clear ||
        !table.GenTextures || !table.GenBuffers || !table.GenFramebuffers ||
        !table.CreateProgram || !table.CreateShader) {
        MGLOG_E("AcquireGLESFunctions: Failed to load critical GLES functions!");
        return false;
    }

    MGLOG_I("AcquireGLESFunctions: All GLES functions loaded successfully.");
    return true;
}

void FillInGLESCapabilities(GLESCapabilities& caps) {
    GLint value;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    caps.maxTextureSize = value;

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
    caps.max3DTextureSize = value;

    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &value);
    caps.maxCubeMapTextureSize = value;

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &value);
    caps.maxRenderbufferSize = value;

    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &value);
    caps.maxDrawBuffers = value;

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &value);
    caps.maxColorAttachments = value;

    glGetIntegerv(GL_MAX_SAMPLES, &value);
    caps.maxSamples = value;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
    caps.maxVertexAttribs = value;

    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &value);
    caps.maxUniformBufferBindings = value;

    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &value);
    caps.maxShaderStorageBufferBindings = value;

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
    caps.maxComputeWorkGroupInvocations = value;

    // Check extensions
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions) {
        caps.hasTextureBuffer = strstr(extensions, "GL_EXT_texture_buffer") != nullptr;
        caps.hasTextureMultisample = strstr(extensions, "GL_EXT_multisampled_render_to_texture") != nullptr;
        caps.hasGeometryShader = strstr(extensions, "GL_EXT_geometry_shader") != nullptr;
        caps.hasTessellationShader = strstr(extensions, "GL_EXT_tessellation_shader") != nullptr;
        caps.hasComputeShader = true; // Core in ES 3.1+
        caps.hasDrawIndirect = true; // Core in ES 3.1+
        caps.hasMultiDrawIndirect = strstr(extensions, "GL_EXT_multi_draw_indirect") != nullptr;
        caps.hasBaseVertex = strstr(extensions, "GL_EXT_draw_elements_base_vertex") != nullptr;
        caps.hasBaseInstance = strstr(extensions, "GL_EXT_base_instance") != nullptr;
        caps.hasCopyImage = strstr(extensions, "GL_EXT_copy_image") != nullptr;
        caps.hasDebugOutput = strstr(extensions, "GL_KHR_debug") != nullptr;
    }

    MGLOG_I("FillInGLESCapabilities: maxTextureSize=%d, maxSamples=%d", caps.maxTextureSize, caps.maxSamples);
}

#undef LOAD_FUNC
#undef LOAD_EGL_FUNC

} // namespace MG_External
} // namespace MobileGL