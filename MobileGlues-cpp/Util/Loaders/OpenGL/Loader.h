#pragma once

#include "Includes.h"

extern "C" {
#include <GLES3/gl3.h>
#include <EGL/egl.h>
}

namespace MobileGL {
namespace MG_External {

// ========== GLES Function Table ==========
// Direct function pointers to GLES3 API
struct GLESFunctionsTable {
    // Drawing
    void (*DrawArrays)(GLenum mode, GLint first, GLsizei count) = nullptr;
    void (*DrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*DrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) = nullptr;
    void (*DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;
    void (*DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*DrawArraysIndirect)(GLenum mode, const void* indirect) = nullptr;
    void (*DrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect) = nullptr;
    void (*MultiDrawArrays)(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (*MultiDrawElements)(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (*MultiDrawArraysIndirect)(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*MultiDrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*DrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*DrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*DrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex) = nullptr;
    void (*MultiDrawElementsBaseVertex)(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex) = nullptr;
    void (*DrawElementsInstancedBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*DrawArraysInstancedBaseInstance)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*DrawElementsInstancedBaseVertexBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;

    // State
    void (*Enable)(GLenum cap) = nullptr;
    void (*Disable)(GLenum cap) = nullptr;
    GLboolean (*IsEnabled)(GLenum cap) = nullptr;
    void (*GetBooleanv)(GLenum pname, GLboolean* data) = nullptr;
    void (*GetFloatv)(GLenum pname, GLfloat* data) = nullptr;
    void (*GetIntegerv)(GLenum pname, GLint* data) = nullptr;
    void (*GetInteger64v)(GLenum pname, GLint64* data) = nullptr;
    void (*GetDoublev)(GLenum pname, GLdouble* data) = nullptr;
    void (*GetIntegeri_v)(GLenum target, GLuint index, GLint* data) = nullptr;
    void (*GetInteger64i_v)(GLenum target, GLuint index, GLint64* data) = nullptr;
    const GLubyte* (*GetString)(GLenum name) = nullptr;
    GLenum (*GetError)() = nullptr;

    // Viewport/Scissor
    void (*Viewport)(GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void (*Scissor)(GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;

    // Clear
    void (*Clear)(GLbitfield mask) = nullptr;
    void (*ClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void (*ClearDepthf)(GLfloat depth) = nullptr;
    void (*ClearStencil)(GLint s) = nullptr;
    void (*ClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void (*ClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void (*ClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void (*ClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;

    // Blending
    void (*BlendFunc)(GLenum sfactor, GLenum dfactor) = nullptr;
    void (*BlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) = nullptr;
    void (*BlendEquation)(GLenum mode) = nullptr;
    void (*BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha) = nullptr;
    void (*BlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;

    // Culling
    void (*CullFace)(GLenum mode) = nullptr;
    void (*FrontFace)(GLenum mode) = nullptr;

    // Depth
    void (*DepthFunc)(GLenum func) = nullptr;
    void (*DepthMask)(GLboolean flag) = nullptr;
    void (*DepthRangef)(GLfloat n, GLfloat f) = nullptr;

    // Stencil
    void (*StencilFunc)(GLenum func, GLint ref, GLuint mask) = nullptr;
    void (*StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask) = nullptr;
    void (*StencilOp)(GLenum fail, GLenum zfail, GLenum zpass) = nullptr;
    void (*StencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) = nullptr;
    void (*StencilMask)(GLuint mask) = nullptr;
    void (*StencilMaskSeparate)(GLenum face, GLuint mask) = nullptr;

    // Polygon
    void (*PolygonOffset)(GLfloat factor, GLfloat units) = nullptr;
    void (*LineWidth)(GLfloat width) = nullptr;

    // Pixel Store
    void (*PixelStorei)(GLenum pname, GLint param) = nullptr;

    // Textures
    void (*GenTextures)(GLsizei n, GLuint* textures) = nullptr;
    void (*DeleteTextures)(GLsizei n, const GLuint* textures) = nullptr;
    void (*BindTexture)(GLenum target, GLuint texture) = nullptr;
    void (*TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) = nullptr;
    void (*CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void (*CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (*CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (*TexParameteri)(GLenum target, GLenum pname, GLint param) = nullptr;
    void (*TexParameterf)(GLenum target, GLenum pname, GLfloat param) = nullptr;
    void (*TexParameteriv)(GLenum target, GLenum pname, const GLint* params) = nullptr;
    void (*TexParameterfv)(GLenum target, GLenum pname, const GLfloat* params) = nullptr;
    void (*GetTexParameteriv)(GLenum target, GLenum pname, GLint* params) = nullptr;
    void (*GetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params) = nullptr;
    void (*GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint* params) = nullptr;
    void (*GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat* params) = nullptr;
    void (*GenerateMipmap)(GLenum target) = nullptr;
    void (*ActiveTexture)(GLenum texture) = nullptr;
    void (*TexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (*TexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (*GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, void* pixels) = nullptr;
    void (*GetCompressedTexImage)(GLenum target, GLint level, void* img) = nullptr;
    void (*TexBuffer)(GLenum target, GLenum internalformat, GLuint buffer) = nullptr;

    // Buffers
    void (*GenBuffers)(GLsizei n, GLuint* buffers) = nullptr;
    void (*DeleteBuffers)(GLsizei n, const GLuint* buffers) = nullptr;
    void (*BindBuffer)(GLenum target, GLuint buffer) = nullptr;
    void (*BufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void (*BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void* (*MapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) = nullptr;
    GLboolean (*UnmapBuffer)(GLenum target) = nullptr;
    void (*FlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length) = nullptr;
    void (*BindBufferBase)(GLenum target, GLuint index, GLuint buffer) = nullptr;
    void (*BindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void (*CopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) = nullptr;

    // Vertex Arrays
    void (*GenVertexArrays)(GLsizei n, GLuint* arrays) = nullptr;
    void (*DeleteVertexArrays)(GLsizei n, const GLuint* arrays) = nullptr;
    void (*BindVertexArray)(GLuint array) = nullptr;
    void (*VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (*VertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (*EnableVertexAttribArray)(GLuint index) = nullptr;
    void (*DisableVertexAttribArray)(GLuint index) = nullptr;
    void (*VertexAttribDivisor)(GLuint index, GLuint divisor) = nullptr;

    // Framebuffers
    void (*GenFramebuffers)(GLsizei n, GLuint* framebuffers) = nullptr;
    void (*DeleteFramebuffers)(GLsizei n, const GLuint* framebuffers) = nullptr;
    void (*BindFramebuffer)(GLenum target, GLuint framebuffer) = nullptr;
    void (*FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void (*FramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void (*FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    GLenum (*CheckFramebufferStatus)(GLenum target) = nullptr;
    void (*DrawBuffers)(GLsizei n, const GLenum* bufs) = nullptr;
    void (*ReadBuffer)(GLenum src) = nullptr;
    void (*BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;

    // Renderbuffers
    void (*GenRenderbuffers)(GLsizei n, GLuint* renderbuffers) = nullptr;
    void (*DeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers) = nullptr;
    void (*BindRenderbuffer)(GLenum target, GLuint renderbuffer) = nullptr;
    void (*RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (*RenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;

    // Programs/Shaders
    GLuint (*CreateProgram)() = nullptr;
    void (*DeleteProgram)(GLuint program) = nullptr;
    void (*UseProgram)(GLuint program) = nullptr;
    GLuint (*CreateShader)(GLenum type) = nullptr;
    void (*DeleteShader)(GLuint shader) = nullptr;
    void (*ShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;
    void (*CompileShader)(GLuint shader) = nullptr;
    void (*AttachShader)(GLuint program, GLuint shader) = nullptr;
    void (*DetachShader)(GLuint program, GLuint shader) = nullptr;
    void (*LinkProgram)(GLuint program) = nullptr;
    void (*GetShaderiv)(GLuint shader, GLenum pname, GLint* params) = nullptr;
    void (*GetProgramiv)(GLuint program, GLenum pname, GLint* params) = nullptr;
    void (*GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void (*GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void (*ValidateProgram)(GLuint program) = nullptr;
    GLint (*GetUniformLocation)(GLuint program, const GLchar* name) = nullptr;
    GLint (*GetAttribLocation)(GLuint program, const GLchar* name) = nullptr;
    void (*GetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) = nullptr;
    void (*GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) = nullptr;
    void (*BindAttribLocation)(GLuint program, GLuint index, const GLchar* name) = nullptr;

    // Uniforms
    void (*Uniform1f)(GLint location, GLfloat v0) = nullptr;
    void (*Uniform2f)(GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void (*Uniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void (*Uniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void (*Uniform1i)(GLint location, GLint v0) = nullptr;
    void (*Uniform2i)(GLint location, GLint v0, GLint v1) = nullptr;
    void (*Uniform3i)(GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void (*Uniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void (*Uniform1ui)(GLint location, GLuint v0) = nullptr;
    void (*Uniform2ui)(GLint location, GLuint v0, GLuint v1) = nullptr;
    void (*Uniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2) = nullptr;
    void (*Uniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) = nullptr;
    void (*Uniform1fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*Uniform2fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*Uniform3fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*Uniform4fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*Uniform1iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*Uniform2iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*Uniform3iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*Uniform4iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*Uniform1uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*Uniform2uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*Uniform3uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*Uniform4uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*UniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;

    // Program Resources
    void (*GetProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint* params) = nullptr;
    GLuint (*GetProgramResourceIndex)(GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    void (*GetProgramResourceName)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name) = nullptr;
    void (*GetProgramResourceiv)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params) = nullptr;
    GLint (*GetProgramResourceLocation)(GLuint program, GLenum programInterface, const GLchar* name) = nullptr;
    void (*ShaderStorageBlockBinding)(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) = nullptr;

    // Compute
    void (*DispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void (*DispatchComputeIndirect)(GLintptr indirect) = nullptr;

    // Memory Barriers
    void (*MemoryBarrier)(GLbitfield barriers) = nullptr;
    void (*MemoryBarrierByRegion)(GLbitfield barriers) = nullptr;

    // Samplers
    void (*GenSamplers)(GLsizei count, GLuint* samplers) = nullptr;
    void (*DeleteSamplers)(GLsizei count, const GLuint* samplers) = nullptr;
    void (*BindSampler)(GLuint unit, GLuint sampler) = nullptr;
    void (*SamplerParameteri)(GLuint sampler, GLenum pname, GLint param) = nullptr;
    void (*SamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param) = nullptr;

    // Image Load/Store
    void (*BindImageTexture)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) = nullptr;

    // ReadPixels
    void (*ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) = nullptr;

    // CopyImageSubData
    void (*CopyImageSubData)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) = nullptr;

    // Flush/Finish
    void (*Flush)() = nullptr;
    void (*Finish)() = nullptr;
};

// ========== EGL Function Table (External) ==========
struct EGLFunctionsTable {
    EGLDisplay (*GetDisplay)(EGLNativeDisplayType display_id) = nullptr;
    EGLBoolean (*Initialize)(EGLDisplay dpy, EGLint* major, EGLint* minor) = nullptr;
    EGLBoolean (*Terminate)(EGLDisplay dpy) = nullptr;
    EGLBoolean (*ChooseConfig)(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config) = nullptr;
    EGLBoolean (*GetConfigs)(EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config) = nullptr;
    EGLBoolean (*GetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value) = nullptr;
    EGLSurface (*CreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list) = nullptr;
    EGLSurface (*CreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list) = nullptr;
    EGLBoolean (*DestroySurface)(EGLDisplay dpy, EGLSurface surface) = nullptr;
    EGLContext (*CreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list) = nullptr;
    EGLBoolean (*DestroyContext)(EGLDisplay dpy, EGLContext ctx) = nullptr;
    EGLBoolean (*MakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) = nullptr;
    EGLBoolean (*SwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;
    EGLBoolean (*QuerySurface)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value) = nullptr;
    EGLint (*GetError)() = nullptr;
    EGLBoolean (*ReleaseThread)() = nullptr;
    void* (*GetProcAddress)(const char* procname) = nullptr;
};

// ========== GLES Capabilities ==========
struct GLESCapabilities {
    int maxTextureSize = 0;
    int max3DTextureSize = 0;
    int maxCubeMapTextureSize = 0;
    int maxRenderbufferSize = 0;
    int maxDrawBuffers = 0;
    int maxColorAttachments = 0;
    int maxSamples = 0;
    int maxVertexAttribs = 0;
    int maxUniformBufferBindings = 0;
    int maxShaderStorageBufferBindings = 0;
    int maxComputeWorkGroupInvocations = 0;
    bool hasTextureBuffer = false;
    bool hasTextureMultisample = false;
    bool hasGeometryShader = false;
    bool hasTessellationShader = false;
    bool hasComputeShader = false;
    bool hasDrawIndirect = false;
    bool hasMultiDrawIndirect = false;
    bool hasBaseVertex = false;
    bool hasBaseInstance = false;
    bool hasCopyImage = false;
    bool hasDebugOutput = false;
};

// ========== Loader Functions ==========
bool AcquireEGLFunctions(EGLFunctionsTable& table);
bool AcquireGLESFunctions(GLESFunctionsTable& table);
void FillInGLESCapabilities(GLESCapabilities& caps);

} // namespace MG_External
} // namespace MobileGL