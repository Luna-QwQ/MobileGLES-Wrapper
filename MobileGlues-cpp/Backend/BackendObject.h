#pragma once
#include "Includes.h"
#include "Config.h"

namespace MobileGL {
namespace MG_Backend {

// ============================================================
// Format Capability Types
// ============================================================

enum class FormatCapability : Uint32 {
    // Texture format capabilities
    TextureFormatSupported        = 1 << 0,
    TextureFilterSupported        = 1 << 1,
    TextureMipmapSupported        = 1 << 2,
    TextureNPOTSupported          = 1 << 3,
    TextureCompressedSupported    = 1 << 4,
    TextureDepthSupported         = 1 << 5,
    TextureStencilSupported       = 1 << 6,
    TextureSRGBSupported          = 1 << 7,
    TextureFloatSupported         = 1 << 8,
    TextureIntegerSupported       = 1 << 9,
    TextureArraySupported         = 1 << 10,
    Texture3DSupported            = 1 << 11,
    TextureCubeMapSupported       = 1 << 12,
    TextureMultisampleSupported   = 1 << 13,
    TextureBufferSupported        = 1 << 14,

    // Renderbuffer format capabilities
    RenderbufferFormatSupported   = 1 << 15,
    RenderbufferMSAASupported     = 1 << 16,

    // Framebuffer attachment capabilities
    FramebufferColorAttachment    = 1 << 17,
    FramebufferDepthAttachment    = 1 << 18,
    FramebufferStencilAttachment  = 1 << 19,
    FramebufferBlitSupported      = 1 << 20,

    // Internal format capabilities
    SizedInternalFormat           = 1 << 21,
    UnsizedInternalFormat         = 1 << 22,

    // Misc
    FormatRequiresSwizzle         = 1 << 23,
    FormatRequiresConversion      = 1 << 24,
};

using FormatCapabilityFlags = Flags<FormatCapability>;

inline FormatCapabilityFlags operator|(FormatCapability a, FormatCapability b) {
    return FormatCapabilityFlags(a) | FormatCapabilityFlags(b);
}

inline FormatCapabilityFlags operator&(FormatCapability a, FormatCapability b) {
    return FormatCapabilityFlags(a) & FormatCapabilityFlags(b);
}

struct FormatCapabilityCache {
    // Per-format capability record
    GLenum glInternalFormat = 0;
    GLenum glFormat = 0;
    GLenum glType = 0;
    FormatCapabilityFlags caps;
    Int32 maxSamples = 0;
    Int32 maxSize = 0;
    Int32 numChannels = 0;
    Int32 bitsPerChannel = 0;
    Bool isCompressed = false;
    Bool isSRGB = false;
    Bool isFloat = false;
    Bool isInteger = false;
    Bool isDepth = false;
    Bool isStencil = false;
    Bool isDepthStencil = false;
    String name;
};

// ============================================================
// GL Functions Table
// ============================================================

struct GLFunctionsTable {
    // State queries
    void (*glGetIntegerv)(GLenum pname, GLint* data) = nullptr;
    void (*glGetFloatv)(GLenum pname, GLfloat* data) = nullptr;
    void (*glGetBooleanv)(GLenum pname, GLboolean* data) = nullptr;
    void (*glGetInteger64v)(GLenum pname, GLint64* data) = nullptr;
    void (*glGetDoublev)(GLenum pname, GLdouble* data) = nullptr;
    const GLubyte* (*glGetString)(GLenum name) = nullptr;
    const GLubyte* (*glGetStringi)(GLenum name, GLuint index) = nullptr;
    GLenum (*glGetError)(void) = nullptr;

    // Draw calls
    void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count) = nullptr;
    void (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*glDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) = nullptr;
    void (*glDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;
    void (*glDrawArraysIndirect)(GLenum mode, const void* indirect) = nullptr;
    void (*glDrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect) = nullptr;
    void (*glMultiDrawArrays)(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (*glMultiDrawElements)(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (*glMultiDrawArraysIndirect)(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*glMultiDrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*glDrawArraysInstancedBaseInstance)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*glDrawElementsInstancedBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*glDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*glDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*glDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex) = nullptr;
    void (*glDrawElementsInstancedBaseVertexBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;

    // Clear
    void (*glClear)(GLbitfield mask) = nullptr;
    void (*glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void (*glClearDepthf)(GLfloat d) = nullptr;
    void (*glClearStencil)(GLint s) = nullptr;
    void (*glClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void (*glClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void (*glClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void (*glClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;

    // Blit
    void (*glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void (*glBlitNamedFramebuffer)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;

    // Compute
    void (*glDispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void (*glDispatchComputeIndirect)(GLintptr indirect) = nullptr;

    // Buffer management
    void (*glGenBuffers)(GLsizei n, GLuint* buffers) = nullptr;
    void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers) = nullptr;
    void (*glBindBuffer)(GLenum target, GLuint buffer) = nullptr;
    void (*glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage) = nullptr;
    void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = nullptr;
    void (*glBindBufferBase)(GLenum target, GLuint index, GLuint buffer) = nullptr;
    void (*glBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) = nullptr;
    void (*glMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) = nullptr;
    GLboolean (*glUnmapBuffer)(GLenum target) = nullptr;
    void (*glCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) = nullptr;
    void* (*glMapBuffer)(GLenum target, GLenum access) = nullptr;
    void (*glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length) = nullptr;

    // Texture management
    void (*glGenTextures)(GLsizei n, GLuint* textures) = nullptr;
    void (*glDeleteTextures)(GLsizei n, const GLuint* textures) = nullptr;
    void (*glBindTexture)(GLenum target, GLuint texture) = nullptr;
    void (*glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*glTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*glTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = nullptr;
    void (*glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (*glCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) = nullptr;
    void (*glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (*glCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) = nullptr;
    void (*glTexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (*glTexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = nullptr;
    void (*glTexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) = nullptr;
    void (*glTexStorage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) = nullptr;
    void (*glTexParameteri)(GLenum target, GLenum pname, GLint param) = nullptr;
    void (*glTexParameterf)(GLenum target, GLenum pname, GLfloat param) = nullptr;
    void (*glTexParameteriv)(GLenum target, GLenum pname, const GLint* params) = nullptr;
    void (*glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params) = nullptr;
    void (*glGenerateMipmap)(GLenum target) = nullptr;
    void (*glActiveTexture)(GLenum texture) = nullptr;

    // Framebuffer management
    void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers) = nullptr;
    void (*glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers) = nullptr;
    void (*glBindFramebuffer)(GLenum target, GLuint framebuffer) = nullptr;
    void (*glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = nullptr;
    void (*glFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) = nullptr;
    void (*glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = nullptr;
    GLenum (*glCheckFramebufferStatus)(GLenum target) = nullptr;
    void (*glReadBuffer)(GLenum src) = nullptr;
    void (*glDrawBuffers)(GLsizei n, const GLenum* bufs) = nullptr;
    void (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) = nullptr;

    // Renderbuffer management
    void (*glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers) = nullptr;
    void (*glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers) = nullptr;
    void (*glBindRenderbuffer)(GLenum target, GLuint renderbuffer) = nullptr;
    void (*glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;
    void (*glRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) = nullptr;

    // Shader/Program
    GLuint (*glCreateShader)(GLenum type) = nullptr;
    void (*glDeleteShader)(GLuint shader) = nullptr;
    void (*glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;
    void (*glCompileShader)(GLuint shader) = nullptr;
    GLuint (*glCreateProgram)(void) = nullptr;
    void (*glDeleteProgram)(GLuint program) = nullptr;
    void (*glAttachShader)(GLuint program, GLuint shader) = nullptr;
    void (*glDetachShader)(GLuint program, GLuint shader) = nullptr;
    void (*glLinkProgram)(GLuint program) = nullptr;
    void (*glUseProgram)(GLuint program) = nullptr;
    void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint* params) = nullptr;
    void (*glGetProgramiv)(GLuint program, GLenum pname, GLint* params) = nullptr;
    void (*glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    void (*glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) = nullptr;
    GLint (*glGetUniformLocation)(GLuint program, const GLchar* name) = nullptr;
    GLint (*glGetAttribLocation)(GLuint program, const GLchar* name) = nullptr;
    void (*glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name) = nullptr;
    void (*glUniform1i)(GLint location, GLint v0) = nullptr;
    void (*glUniform1f)(GLint location, GLfloat v0) = nullptr;
    void (*glUniform2i)(GLint location, GLint v0, GLint v1) = nullptr;
    void (*glUniform2f)(GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void (*glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void (*glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void (*glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void (*glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void (*glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniform1iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*glUniform1fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*glUniform2iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*glUniform2fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*glUniform3iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*glUniform3fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*glUniform4iv)(GLint location, GLsizei count, const GLint* value) = nullptr;
    void (*glUniform4fv)(GLint location, GLsizei count, const GLfloat* value) = nullptr;
    void (*glUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glUniform1ui)(GLint location, GLuint v0) = nullptr;
    void (*glUniform2ui)(GLint location, GLuint v0, GLuint v1) = nullptr;
    void (*glUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2) = nullptr;
    void (*glUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) = nullptr;
    void (*glUniform1uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*glUniform2uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*glUniform3uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*glUniform4uiv)(GLint location, GLsizei count, const GLuint* value) = nullptr;
    void (*glBindFragDataLocation)(GLuint program, GLuint color, const GLchar* name) = nullptr;
    void (*glBindFragDataLocationIndexed)(GLuint program, GLuint colorNumber, GLuint index, const GLchar* name) = nullptr;
    GLint (*glGetFragDataIndex)(GLuint program, const GLchar* name) = nullptr;
    GLint (*glGetFragDataLocation)(GLuint program, const GLchar* name) = nullptr;
    void (*glProgramUniform1i)(GLuint program, GLint location, GLint v0) = nullptr;
    void (*glProgramUniform1f)(GLuint program, GLint location, GLfloat v0) = nullptr;
    void (*glProgramUniform2i)(GLuint program, GLint location, GLint v0, GLint v1) = nullptr;
    void (*glProgramUniform2f)(GLuint program, GLint location, GLfloat v0, GLfloat v1) = nullptr;
    void (*glProgramUniform3i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) = nullptr;
    void (*glProgramUniform3f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) = nullptr;
    void (*glProgramUniform4i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = nullptr;
    void (*glProgramUniform4f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) = nullptr;
    void (*glProgramUniformMatrix2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glProgramUniformMatrix3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glProgramUniformMatrix4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = nullptr;
    void (*glTransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode) = nullptr;
    void (*glGetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name) = nullptr;

    // Vertex Array
    void (*glGenVertexArrays)(GLsizei n, GLuint* arrays) = nullptr;
    void (*glDeleteVertexArrays)(GLsizei n, const GLuint* arrays) = nullptr;
    void (*glBindVertexArray)(GLuint array) = nullptr;
    void (*glEnableVertexAttribArray)(GLuint index) = nullptr;
    void (*glDisableVertexAttribArray)(GLuint index) = nullptr;
    void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = nullptr;
    void (*glVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) = nullptr;
    void (*glVertexAttribDivisor)(GLuint index, GLuint divisor) = nullptr;
    void (*glVertexAttrib1f)(GLuint index, GLfloat x) = nullptr;
    void (*glVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y) = nullptr;
    void (*glVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z) = nullptr;
    void (*glVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = nullptr;
    void (*glVertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w) = nullptr;
    void (*glVertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) = nullptr;
    void (*glVertexAttribBinding)(GLuint attribindex, GLuint bindingindex) = nullptr;
    void (*glVertexBindingDivisor)(GLuint bindingindex, GLuint divisor) = nullptr;

    // Sampler
    void (*glGenSamplers)(GLsizei count, GLuint* samplers) = nullptr;
    void (*glDeleteSamplers)(GLsizei count, const GLuint* samplers) = nullptr;
    void (*glBindSampler)(GLuint unit, GLuint sampler) = nullptr;
    void (*glSamplerParameteri)(GLuint sampler, GLenum pname, GLint param) = nullptr;
    void (*glSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param) = nullptr;
    void (*glSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint* param) = nullptr;
    void (*glSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat* param) = nullptr;

    // Sync / Fence
    GLsync (*glFenceSync)(GLenum condition, GLbitfield flags) = nullptr;
    GLenum (*glClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void (*glWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) = nullptr;
    void (*glDeleteSync)(GLsync sync) = nullptr;

    // Query
    void (*glGenQueries)(GLsizei n, GLuint* ids) = nullptr;
    void (*glDeleteQueries)(GLsizei n, const GLuint* ids) = nullptr;
    void (*glBeginQuery)(GLenum target, GLuint id) = nullptr;
    void (*glEndQuery)(GLenum target) = nullptr;
    void (*glGetQueryObjectuiv)(GLuint id, GLenum pname, GLuint* params) = nullptr;
    void (*glGetQueryObjecti64v)(GLuint id, GLenum pname, GLint64* params) = nullptr;

    // Transform Feedback
    void (*glGenTransformFeedbacks)(GLsizei n, GLuint* ids) = nullptr;
    void (*glDeleteTransformFeedbacks)(GLsizei n, const GLuint* ids) = nullptr;
    void (*glBindTransformFeedback)(GLenum target, GLuint id) = nullptr;
    void (*glBeginTransformFeedback)(GLenum primitiveMode) = nullptr;
    void (*glEndTransformFeedback)(void) = nullptr;
    void (*glPauseTransformFeedback)(void) = nullptr;
    void (*glResumeTransformFeedback)(void) = nullptr;

    // State management
    void (*glEnable)(GLenum cap) = nullptr;
    void (*glDisable)(GLenum cap) = nullptr;
    GLboolean (*glIsEnabled)(GLenum cap) = nullptr;
    void (*glDepthFunc)(GLenum func) = nullptr;
    void (*glDepthMask)(GLboolean flag) = nullptr;
    void (*glDepthRangef)(GLfloat n, GLfloat f) = nullptr;
    void (*glStencilFunc)(GLenum func, GLint ref, GLuint mask) = nullptr;
    void (*glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask) = nullptr;
    void (*glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass) = nullptr;
    void (*glStencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) = nullptr;
    void (*glStencilMask)(GLuint mask) = nullptr;
    void (*glStencilMaskSeparate)(GLenum face, GLuint mask) = nullptr;
    void (*glBlendFunc)(GLenum sfactor, GLenum dfactor) = nullptr;
    void (*glBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) = nullptr;
    void (*glBlendEquation)(GLenum mode) = nullptr;
    void (*glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha) = nullptr;
    void (*glBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void (*glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = nullptr;
    void (*glCullFace)(GLenum mode) = nullptr;
    void (*glFrontFace)(GLenum mode) = nullptr;
    void (*glPolygonOffset)(GLfloat factor, GLfloat units) = nullptr;
    void (*glLineWidth)(GLfloat width) = nullptr;
    void (*glScissor)(GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;
    void (*glViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = nullptr;

    // Pixel pack/unpack
    void (*glPixelStorei)(GLenum pname, GLint param) = nullptr;

    // Misc
    void (*glFlush)(void) = nullptr;
    void (*glFinish)(void) = nullptr;
    void (*glHint)(GLenum target, GLenum mode) = nullptr;
    void (*glGetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint* params) = nullptr;
};

// ============================================================
// Global Backend Functions Table (delegates to active backend)
// ============================================================

struct GlobalBackendFunctionsTable {
    // Draw calls
    void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count) = nullptr;
    void (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*glDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) = nullptr;
    void (*glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) = nullptr;
    void (*glDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) = nullptr;
    void (*glDrawArraysIndirect)(GLenum mode, const void* indirect) = nullptr;
    void (*glDrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect) = nullptr;
    void (*glMultiDrawArrays)(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;
    void (*glMultiDrawElements)(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) = nullptr;
    void (*glMultiDrawArraysIndirect)(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*glMultiDrawElementsIndirect)(GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;
    void (*glDrawArraysInstancedBaseInstance)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*glDrawElementsInstancedBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) = nullptr;
    void (*glDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*glDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices, GLint basevertex) = nullptr;
    void (*glDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex) = nullptr;
    void (*glDrawElementsInstancedBaseVertexBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) = nullptr;

    // Clear
    void (*glClear)(GLbitfield mask) = nullptr;
    void (*glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = nullptr;
    void (*glClearDepthf)(GLfloat d) = nullptr;
    void (*glClearStencil)(GLint s) = nullptr;
    void (*glClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat* value) = nullptr;
    void (*glClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint* value) = nullptr;
    void (*glClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint* value) = nullptr;
    void (*glClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = nullptr;

    // Blit
    void (*glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;
    void (*glBlitNamedFramebuffer)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = nullptr;

    // Compute
    void (*glDispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) = nullptr;
    void (*glDispatchComputeIndirect)(GLintptr indirect) = nullptr;
};

// ============================================================
// Dynamic Backend Parameters
// ============================================================

struct DynamicBackendParameters {
    Int32 maxTextureSize = 0;
    Int32 max3DTextureSize = 0;
    Int32 maxCubeMapTextureSize = 0;
    Int32 maxArrayTextureLayers = 0;
    Int32 maxRenderbufferSize = 0;
    Int32 maxDrawBuffers = 0;
    Int32 maxColorAttachments = 0;
    Int32 maxSamples = 0;
    Int32 maxVertexAttribs = 0;
    Int32 maxVertexUniformComponents = 0;
    Int32 maxFragmentUniformComponents = 0;
    Int32 maxGeometryUniformComponents = 0;
    Int32 maxComputeUniformComponents = 0;
    Int32 maxVaryingVectors = 0;
    Int32 maxVaryingComponents = 0;
    Int32 maxVertexOutputComponents = 0;
    Int32 maxFragmentInputComponents = 0;
    Int32 maxVertexTextureImageUnits = 0;
    Int32 maxFragmentTextureImageUnits = 0;
    Int32 maxCombinedTextureImageUnits = 0;
    Int32 maxTextureImageUnits = 0;
    Int32 maxUniformBufferBindings = 0;
    Int32 maxUniformBlockSize = 0;
    Int32 maxShaderStorageBufferBindings = 0;
    Int32 maxShaderStorageBlockSize = 0;
    Int32 maxTransformFeedbackInterleavedComponents = 0;
    Int32 maxTransformFeedbackSeparateAttribs = 0;
    Int32 maxTransformFeedbackSeparateComponents = 0;
    Int32 maxAtomicCounterBufferBindings = 0;
    Int32 maxImageUnits = 0;

    Int32 subPixelBits = 0;
    Int32 maxViewportDims[2] = {0, 0};
    Int32 maxViewports = 0;
    Float maxAnisotropy = 0.0f;
    Float maxTextureLodBias = 0.0f;
    Float minLineWidth = 0.0f;
    Float maxLineWidth = 0.0f;
    Float aliasedPointSizeRange[2] = {0.0f, 0.0f};
    Int32 maxClipDistances = 0;
    Int32 maxDualSourceDrawBuffers = 0;

    Int32 majorVersion = 0;
    Int32 minorVersion = 0;
    Int32 glslMajorVersion = 0;
    Int32 glslMinorVersion = 0;

    Bool supportsComputeShaders = false;
    Bool supportsGeometryShaders = false;
    Bool supportsTessellationShaders = false;
    Bool supportsTextureMultisample = false;
    Bool supportsTextureBuffer = false;
    Bool supportsDrawIndirect = false;
    Bool supportsMultiDrawIndirect = false;
    Bool supportsBaseVertex = false;
    Bool supportsBaseInstance = false;
    Bool supportsDrawBuffersBlend = false;
    Bool supportsPolygonMode = false;
    Bool supportsDepthClamp = false;
    Bool supportsSeamlessCubeMap = false;
    Bool supportsTextureFilterAnisotropic = false;
    Bool supportsTextureNPOT = false;
    Bool supportsTextureFloatLinear = false;
    Bool supportsSRGBFramebuffer = false;
    Bool supportsDebugOutput = false;
    Bool supportsPrimitiveRestartFixedIndex = false;
    Bool supportsProgramBinary = false;
    Bool supportsShaderImageLoadStore = false;
    Bool supportsShaderAtomicCounters = false;

    String rendererString;
    String vendorString;
    String versionString;
    String glslVersionString;
    Vector<String> extensions;
};

// ============================================================
// Window Handle
// ============================================================

enum class WindowBackend : Int32 {
    None = 0,
    ANativeWindow = 1,
    X11 = 2,
    Wayland = 3,
    Win32 = 4,
    Custom = 5,
};

struct WindowHandle {
    WindowBackend backend = WindowBackend::None;
    void* nativeWindow = nullptr;
    Int32 width = 0;
    Int32 height = 0;
    Int32 format = 0;
    Bool isValid = false;
};

// ============================================================
// Renderer Info
// ============================================================

struct RendererInfo {
    struct Version {
        Int32 major = 0;
        Int32 minor = 0;
        Int32 patch = 0;
        String toString() const {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d.%d.%d", major, minor, patch);
            return String(buf);
        }
    };

    struct RendererGLInfo {
        String rendererName;
        String vendorName;
        String glVersion;
        String glslVersion;
        Version apiVersion;
        Vector<String> extensions;
        Bool isHardwareAccelerated = false;
        Bool isANGLE = false;
        Bool isAdreno = false;
        Bool isMali = false;
        Bool isPowerVR = false;
        Bool isAppleGPU = false;
        Bool isIntel = false;
        Bool isNVIDIA = false;
        Bool isAMD = false;
        Bool isMesa = false;
        Bool isSwiftShader = false;
        Bool isZink = false;
        Bool isVirGL = false;
    };

    Version backendVersion;
    RendererGLInfo glInfo;
    BackendType backendType = BackendType::DirectGLES;
    String backendName;
    String backendDescription;
};

// ============================================================
// BackendObject Base Class
// ============================================================

class BackendObject {
public:
    BackendObject() = default;
    virtual ~BackendObject() = default;

    // Non-copyable, non-movable
    BackendObject(const BackendObject&) = delete;
    BackendObject& operator=(const BackendObject&) = delete;
    BackendObject(BackendObject&&) = delete;
    BackendObject& operator=(BackendObject&&) = delete;

    // ============================================================
    // Initialization
    // ============================================================

    virtual Bool Initialize() = 0;
    virtual Bool InitCapabilities() = 0;
    virtual Bool InitWindowSurface() = 0;

    // ============================================================
    // EGL Lifecycle
    // ============================================================

    virtual Bool InitializeEGLDisplay(EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY);
    virtual Bool CreateEGLWindowSurface(WindowHandle& windowHandle);
    virtual Bool CreateEGLPbufferSurface(Int32 width, Int32 height);
    virtual Bool MakeEGLCurrent();
    virtual Bool SwapEGLBuffers();
    virtual Bool ReleaseEGLSurface();
    virtual void ReleaseEGLResources();

    // ============================================================
    // Window Handle Management
    // ============================================================

    virtual void SetWindowHandle(WindowHandle& handle);
    virtual WindowHandle GetWindowHandle() const;

    // ============================================================
    // EGL State Management
    // ============================================================

    virtual void ResetEGLRuntimeState();
    virtual void RegisterEGLWindowSurface(EGLSurface surface);
    virtual void RegisterEGLPbufferSurface(EGLSurface surface);

    // ============================================================
    // Renderer Info
    // ============================================================

    virtual RendererInfo GetRendererInfo() const;
    virtual String GetBackendAPIVersionString() const;
    virtual const GLFunctionsTable& GetBackendFunctions() const;
    virtual const DynamicBackendParameters& GetDynamicParameters() const;
    virtual BackendType GetBackendType() const;
    virtual String GetBackendName() const;

    // ============================================================
    // Format Capabilities
    // ============================================================

    virtual HashMap<GLenum, FormatCapabilityCache>& MutableFormatCapabilities();
    virtual const HashMap<GLenum, FormatCapabilityCache>& GetFormatCapabilities() const;
    virtual Bool HasFormatCapability(GLenum format, FormatCapability cap) const;
    virtual FormatCapabilityCache GetFormatCapabilityInfo(GLenum format) const;
    virtual Int32 GetFormatCapabilityTargetIndex(GLenum format, FormatCapability cap) const;
    virtual Int32 GetRenderbufferFormatCapabilityTargetIndex(GLenum format, FormatCapability cap) const;
    virtual void PrintFormatCapabilities() const;

    // ============================================================
    // Direct Accessors
    // ============================================================

    EGLDisplay GetEGLDisplay() const { return m_eglDisplay; }
    EGLContext GetEGLContext() const { return m_eglContext; }
    EGLSurface GetEGLSurface() const { return m_eglSurface; }
    EGLConfig GetEGLConfig() const { return m_eglConfig; }
    Bool IsEGLInitialized() const { return m_eglInitialized; }
    Bool IsSurfaceCreated() const { return m_eglSurfaceCreated; }

protected:
    // ============================================================
    // EGL State
    // ============================================================

    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLContext m_eglContext = EGL_NO_CONTEXT;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLConfig m_eglConfig = nullptr;
    EGLNativeDisplayType m_nativeDisplay = EGL_DEFAULT_DISPLAY;

    Bool m_eglInitialized = false;
    Bool m_eglSurfaceCreated = false;
    Bool m_eglContextMadeCurrent = false;

    WindowHandle m_windowHandle;

    // ============================================================
    // Synchronization
    // ============================================================

    mutable std::mutex m_mutex;

    // ============================================================
    // Format Capabilities
    // ============================================================

    HashMap<GLenum, FormatCapabilityCache> m_formatCapabilities;

    // ============================================================
    // Backend Info
    // ============================================================

    GLFunctionsTable m_functionsTable;
    DynamicBackendParameters m_dynamicParams;
    RendererInfo m_rendererInfo;

    // ============================================================
    // Helper Methods
    // ============================================================

    virtual void PopulateFormatCapabilities();
    virtual void QueryDynamicParameters();
    virtual void DetectRendererInfo();
    virtual Bool ChooseEGLConfig();
    virtual Bool CreateEGLContext();
    virtual void InitEGLFunctions();

    static void LogEGLError(const char* context);
};

} // namespace MG_Backend
} // namespace MobileGL