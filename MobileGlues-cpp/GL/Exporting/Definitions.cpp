#include "Includes.h"
#include "Backend/BackendObjects.h"

using namespace MobileGL::MG_Backend;

extern "C" {
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
}

// ========================================================================
// All GL functions delegate to the backend's GLFunctionsTable via
// the global g_pGLFunctionsTable pointer, set during Init().
// ========================================================================

// ========================================================================
// State Query Functions
// ========================================================================

void glGetIntegerv(GLenum pname, GLint* data) {
    g_pGLFunctionsTable->glGetIntegerv(pname, data);
}

void glGetFloatv(GLenum pname, GLfloat* data) {
    g_pGLFunctionsTable->glGetFloatv(pname, data);
}

void glGetBooleanv(GLenum pname, GLboolean* data) {
    g_pGLFunctionsTable->glGetBooleanv(pname, data);
}

void glGetInteger64v(GLenum pname, GLint64* data) {
    g_pGLFunctionsTable->glGetInteger64v(pname, data);
}

void glGetDoublev(GLenum pname, GLdouble* data) {
    g_pGLFunctionsTable->glGetDoublev(pname, data);
}

const GLubyte* glGetString(GLenum name) {
    return g_pGLFunctionsTable->glGetString(name);
}

const GLubyte* glGetStringi(GLenum name, GLuint index) {
    return g_pGLFunctionsTable->glGetStringi(name, index);
}

GLenum glGetError() {
    return g_pGLFunctionsTable->glGetError();
}

// ========================================================================
// Drawing Functions
// ========================================================================

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    g_pGLFunctionsTable->glDrawArrays(mode, first, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    g_pGLFunctionsTable->glDrawElements(mode, count, type, indices);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) {
    g_pGLFunctionsTable->glDrawRangeElements(mode, start, end, count, type, indices);
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    g_pGLFunctionsTable->glDrawArraysInstanced(mode, first, count, instancecount);
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {
    g_pGLFunctionsTable->glDrawElementsInstanced(mode, count, type, indices, instancecount);
}

void glDrawArraysIndirect(GLenum mode, const void* indirect) {
    g_pGLFunctionsTable->glDrawArraysIndirect(mode, indirect);
}

void glDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect) {
    g_pGLFunctionsTable->glDrawElementsIndirect(mode, type, indirect);
}

void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
    g_pGLFunctionsTable->glMultiDrawArrays(mode, first, count, drawcount);
}

void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount) {
    g_pGLFunctionsTable->glMultiDrawElements(mode, count, type, indices, drawcount);
}

void glMultiDrawArraysIndirect(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) {
    g_pGLFunctionsTable->glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}

void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) {
    g_pGLFunctionsTable->glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
    g_pGLFunctionsTable->glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance);
}

void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance) {
    g_pGLFunctionsTable->glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) {
    g_pGLFunctionsTable->glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}

void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices, GLint basevertex) {
    g_pGLFunctionsTable->glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}

void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex) {
    g_pGLFunctionsTable->glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}

void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
    g_pGLFunctionsTable->glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}

// ========================================================================
// Clear Functions
// ========================================================================

void glClear(GLbitfield mask) {
    g_pGLFunctionsTable->glClear(mask);
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    g_pGLFunctionsTable->glClearColor(red, green, blue, alpha);
}

void glClearDepthf(GLfloat depth) {
    g_pGLFunctionsTable->glClearDepthf(depth);
}

void glClearStencil(GLint s) {
    g_pGLFunctionsTable->glClearStencil(s);
}

void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value) {
    g_pGLFunctionsTable->glClearBufferfv(buffer, drawbuffer, value);
}

void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value) {
    g_pGLFunctionsTable->glClearBufferiv(buffer, drawbuffer, value);
}

void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value) {
    g_pGLFunctionsTable->glClearBufferuiv(buffer, drawbuffer, value);
}

void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    g_pGLFunctionsTable->glClearBufferfi(buffer, drawbuffer, depth, stencil);
}

// ========================================================================
// Blit Functions
// ========================================================================

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    g_pGLFunctionsTable->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    g_pGLFunctionsTable->glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

// ========================================================================
// Compute Functions
// ========================================================================

void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    g_pGLFunctionsTable->glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

void glDispatchComputeIndirect(GLintptr indirect) {
    g_pGLFunctionsTable->glDispatchComputeIndirect(indirect);
}

// ========================================================================
// Buffer Management Functions
// ========================================================================

void glGenBuffers(GLsizei n, GLuint* buffers) {
    g_pGLFunctionsTable->glGenBuffers(n, buffers);
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    g_pGLFunctionsTable->glDeleteBuffers(n, buffers);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    g_pGLFunctionsTable->glBindBuffer(target, buffer);
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    g_pGLFunctionsTable->glBufferData(target, size, data, usage);
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    g_pGLFunctionsTable->glBufferSubData(target, offset, size, data);
}

void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    g_pGLFunctionsTable->glBindBufferBase(target, index, buffer);
}

void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    g_pGLFunctionsTable->glBindBufferRange(target, index, buffer, offset, size);
}

void* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    // The GLFunctionsTable declares glMapBufferRange as returning void,
    // but the actual GLES function returns void*. We use a reinterpret_cast
    // to work around this mismatch in the auto-generated table.
    typedef void* (*MapBufferRangeFn)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    auto fn = reinterpret_cast<MapBufferRangeFn>(
        reinterpret_cast<void*>(g_pGLFunctionsTable->glMapBufferRange));
    return fn(target, offset, length, access);
}

GLboolean glUnmapBuffer(GLenum target) {
    return g_pGLFunctionsTable->glUnmapBuffer(target);
}

void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    g_pGLFunctionsTable->glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}

void* glMapBuffer(GLenum target, GLenum access) {
    return g_pGLFunctionsTable->glMapBuffer(target, access);
}

void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    g_pGLFunctionsTable->glFlushMappedBufferRange(target, offset, length);
}

// ========================================================================
// Texture Management Functions
// ========================================================================

void glGenTextures(GLsizei n, GLuint* textures) {
    g_pGLFunctionsTable->glGenTextures(n, textures);
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    g_pGLFunctionsTable->glDeleteTextures(n, textures);
}

void glBindTexture(GLenum target, GLuint texture) {
    g_pGLFunctionsTable->glBindTexture(target, texture);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
    g_pGLFunctionsTable->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) {
    g_pGLFunctionsTable->glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
    g_pGLFunctionsTable->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    g_pGLFunctionsTable->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) {
    g_pGLFunctionsTable->glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data) {
    g_pGLFunctionsTable->glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
    g_pGLFunctionsTable->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) {
    g_pGLFunctionsTable->glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    g_pGLFunctionsTable->glTexStorage2D(target, levels, internalformat, width, height);
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    g_pGLFunctionsTable->glTexStorage3D(target, levels, internalformat, width, height, depth);
}

void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    g_pGLFunctionsTable->glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}

void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    g_pGLFunctionsTable->glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    g_pGLFunctionsTable->glTexParameteri(target, pname, param);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    g_pGLFunctionsTable->glTexParameterf(target, pname, param);
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {
    g_pGLFunctionsTable->glTexParameteriv(target, pname, params);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    g_pGLFunctionsTable->glTexParameterfv(target, pname, params);
}

void glGenerateMipmap(GLenum target) {
    g_pGLFunctionsTable->glGenerateMipmap(target);
}

void glActiveTexture(GLenum texture) {
    g_pGLFunctionsTable->glActiveTexture(texture);
}

// ========================================================================
// Framebuffer Management Functions
// ========================================================================

void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    g_pGLFunctionsTable->glGenFramebuffers(n, framebuffers);
}

void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    g_pGLFunctionsTable->glDeleteFramebuffers(n, framebuffers);
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    g_pGLFunctionsTable->glBindFramebuffer(target, framebuffer);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    g_pGLFunctionsTable->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    g_pGLFunctionsTable->glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    g_pGLFunctionsTable->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

GLenum glCheckFramebufferStatus(GLenum target) {
    return g_pGLFunctionsTable->glCheckFramebufferStatus(target);
}

void glReadBuffer(GLenum src) {
    g_pGLFunctionsTable->glReadBuffer(src);
}

void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    g_pGLFunctionsTable->glDrawBuffers(n, bufs);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
    g_pGLFunctionsTable->glReadPixels(x, y, width, height, format, type, pixels);
}

// ========================================================================
// Renderbuffer Management Functions
// ========================================================================

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    g_pGLFunctionsTable->glGenRenderbuffers(n, renderbuffers);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    g_pGLFunctionsTable->glDeleteRenderbuffers(n, renderbuffers);
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    g_pGLFunctionsTable->glBindRenderbuffer(target, renderbuffer);
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    g_pGLFunctionsTable->glRenderbufferStorage(target, internalformat, width, height);
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    g_pGLFunctionsTable->glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

// ========================================================================
// Shader / Program Functions
// ========================================================================

GLuint glCreateShader(GLenum type) {
    return g_pGLFunctionsTable->glCreateShader(type);
}

void glDeleteShader(GLuint shader) {
    g_pGLFunctionsTable->glDeleteShader(shader);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    g_pGLFunctionsTable->glShaderSource(shader, count, string, length);
}

void glCompileShader(GLuint shader) {
    g_pGLFunctionsTable->glCompileShader(shader);
}

GLuint glCreateProgram() {
    return g_pGLFunctionsTable->glCreateProgram();
}

void glDeleteProgram(GLuint program) {
    g_pGLFunctionsTable->glDeleteProgram(program);
}

void glAttachShader(GLuint program, GLuint shader) {
    g_pGLFunctionsTable->glAttachShader(program, shader);
}

void glDetachShader(GLuint program, GLuint shader) {
    g_pGLFunctionsTable->glDetachShader(program, shader);
}

void glLinkProgram(GLuint program) {
    g_pGLFunctionsTable->glLinkProgram(program);
}

void glUseProgram(GLuint program) {
    g_pGLFunctionsTable->glUseProgram(program);
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    g_pGLFunctionsTable->glGetShaderiv(shader, pname, params);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    g_pGLFunctionsTable->glGetProgramiv(program, pname, params);
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    g_pGLFunctionsTable->glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    g_pGLFunctionsTable->glGetProgramInfoLog(program, bufSize, length, infoLog);
}

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    return g_pGLFunctionsTable->glGetUniformLocation(program, name);
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    return g_pGLFunctionsTable->glGetAttribLocation(program, name);
}

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    g_pGLFunctionsTable->glBindAttribLocation(program, index, name);
}

// ========================================================================
// Uniform Functions
// ========================================================================

void glUniform1i(GLint location, GLint v0) {
    g_pGLFunctionsTable->glUniform1i(location, v0);
}

void glUniform1f(GLint location, GLfloat v0) {
    g_pGLFunctionsTable->glUniform1f(location, v0);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
    g_pGLFunctionsTable->glUniform2i(location, v0, v1);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    g_pGLFunctionsTable->glUniform2f(location, v0, v1);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    g_pGLFunctionsTable->glUniform3i(location, v0, v1, v2);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    g_pGLFunctionsTable->glUniform3f(location, v0, v1, v2);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    g_pGLFunctionsTable->glUniform4i(location, v0, v1, v2, v3);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    g_pGLFunctionsTable->glUniform4f(location, v0, v1, v2, v3);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix2fv(location, count, transpose, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix3fv(location, count, transpose, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix4fv(location, count, transpose, value);
}

void glUniform1iv(GLint location, GLsizei count, const GLint* value) {
    g_pGLFunctionsTable->glUniform1iv(location, count, value);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) {
    g_pGLFunctionsTable->glUniform1fv(location, count, value);
}

void glUniform2iv(GLint location, GLsizei count, const GLint* value) {
    g_pGLFunctionsTable->glUniform2iv(location, count, value);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) {
    g_pGLFunctionsTable->glUniform2fv(location, count, value);
}

void glUniform3iv(GLint location, GLsizei count, const GLint* value) {
    g_pGLFunctionsTable->glUniform3iv(location, count, value);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) {
    g_pGLFunctionsTable->glUniform3fv(location, count, value);
}

void glUniform4iv(GLint location, GLsizei count, const GLint* value) {
    g_pGLFunctionsTable->glUniform4iv(location, count, value);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) {
    g_pGLFunctionsTable->glUniform4fv(location, count, value);
}

void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix2x3fv(location, count, transpose, value);
}

void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix3x2fv(location, count, transpose, value);
}

void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix2x4fv(location, count, transpose, value);
}

void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix4x2fv(location, count, transpose, value);
}

void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix3x4fv(location, count, transpose, value);
}

void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glUniformMatrix4x3fv(location, count, transpose, value);
}

void glUniform1ui(GLint location, GLuint v0) {
    g_pGLFunctionsTable->glUniform1ui(location, v0);
}

void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    g_pGLFunctionsTable->glUniform2ui(location, v0, v1);
}

void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    g_pGLFunctionsTable->glUniform3ui(location, v0, v1, v2);
}

void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    g_pGLFunctionsTable->glUniform4ui(location, v0, v1, v2, v3);
}

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value) {
    g_pGLFunctionsTable->glUniform1uiv(location, count, value);
}

void glUniform2uiv(GLint location, GLsizei count, const GLuint* value) {
    g_pGLFunctionsTable->glUniform2uiv(location, count, value);
}

void glUniform3uiv(GLint location, GLsizei count, const GLuint* value) {
    g_pGLFunctionsTable->glUniform3uiv(location, count, value);
}

void glUniform4uiv(GLint location, GLsizei count, const GLuint* value) {
    g_pGLFunctionsTable->glUniform4uiv(location, count, value);
}

// ========================================================================
// Program Uniform Functions (GL 4.1 / GLES 3.1)
// ========================================================================

void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    g_pGLFunctionsTable->glProgramUniform1i(program, location, v0);
}

void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    g_pGLFunctionsTable->glProgramUniform1f(program, location, v0);
}

void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    g_pGLFunctionsTable->glProgramUniform2i(program, location, v0, v1);
}

void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    g_pGLFunctionsTable->glProgramUniform2f(program, location, v0, v1);
}

void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    g_pGLFunctionsTable->glProgramUniform3i(program, location, v0, v1, v2);
}

void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    g_pGLFunctionsTable->glProgramUniform3f(program, location, v0, v1, v2);
}

void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    g_pGLFunctionsTable->glProgramUniform4i(program, location, v0, v1, v2, v3);
}

void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    g_pGLFunctionsTable->glProgramUniform4f(program, location, v0, v1, v2, v3);
}

void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glProgramUniformMatrix2fv(program, location, count, transpose, value);
}

void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glProgramUniformMatrix3fv(program, location, count, transpose, value);
}

void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_pGLFunctionsTable->glProgramUniformMatrix4fv(program, location, count, transpose, value);
}

// ========================================================================
// Frag Data / Transform Feedback
// ========================================================================

void glBindFragDataLocation(GLuint program, GLuint color, const GLchar* name) {
    g_pGLFunctionsTable->glBindFragDataLocation(program, color, name);
}

void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar* name) {
    g_pGLFunctionsTable->glBindFragDataLocationIndexed(program, colorNumber, index, name);
}

GLint glGetFragDataIndex(GLuint program, const GLchar* name) {
    return g_pGLFunctionsTable->glGetFragDataIndex(program, name);
}

GLint glGetFragDataLocation(GLuint program, const GLchar* name) {
    return g_pGLFunctionsTable->glGetFragDataLocation(program, name);
}

void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode) {
    g_pGLFunctionsTable->glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}

void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name) {
    g_pGLFunctionsTable->glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}

// ========================================================================
// Vertex Array Functions
// ========================================================================

void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    g_pGLFunctionsTable->glGenVertexArrays(n, arrays);
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    g_pGLFunctionsTable->glDeleteVertexArrays(n, arrays);
}

void glBindVertexArray(GLuint array) {
    g_pGLFunctionsTable->glBindVertexArray(array);
}

void glEnableVertexAttribArray(GLuint index) {
    g_pGLFunctionsTable->glEnableVertexAttribArray(index);
}

void glDisableVertexAttribArray(GLuint index) {
    g_pGLFunctionsTable->glDisableVertexAttribArray(index);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    g_pGLFunctionsTable->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    g_pGLFunctionsTable->glVertexAttribIPointer(index, size, type, stride, pointer);
}

void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    g_pGLFunctionsTable->glVertexAttribDivisor(index, divisor);
}

void glVertexAttrib1f(GLuint index, GLfloat x) {
    g_pGLFunctionsTable->glVertexAttrib1f(index, x);
}

void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    g_pGLFunctionsTable->glVertexAttrib2f(index, x, y);
}

void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    g_pGLFunctionsTable->glVertexAttrib3f(index, x, y, z);
}

void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    g_pGLFunctionsTable->glVertexAttrib4f(index, x, y, z, w);
}

void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    g_pGLFunctionsTable->glVertexAttribI4i(index, x, y, z, w);
}

void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    g_pGLFunctionsTable->glVertexAttribI4ui(index, x, y, z, w);
}

void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    g_pGLFunctionsTable->glVertexAttribBinding(attribindex, bindingindex);
}

void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    g_pGLFunctionsTable->glVertexBindingDivisor(bindingindex, divisor);
}

// ========================================================================
// Sampler Functions
// ========================================================================

void glGenSamplers(GLsizei count, GLuint* samplers) {
    g_pGLFunctionsTable->glGenSamplers(count, samplers);
}

void glDeleteSamplers(GLsizei count, const GLuint* samplers) {
    g_pGLFunctionsTable->glDeleteSamplers(count, samplers);
}

void glBindSampler(GLuint unit, GLuint sampler) {
    g_pGLFunctionsTable->glBindSampler(unit, sampler);
}

void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    g_pGLFunctionsTable->glSamplerParameteri(sampler, pname, param);
}

void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    g_pGLFunctionsTable->glSamplerParameterf(sampler, pname, param);
}

void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint* param) {
    g_pGLFunctionsTable->glSamplerParameteriv(sampler, pname, param);
}

void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* param) {
    g_pGLFunctionsTable->glSamplerParameterfv(sampler, pname, param);
}

// ========================================================================
// Sync / Fence Functions
// ========================================================================

GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    return g_pGLFunctionsTable->glFenceSync(condition, flags);
}

GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    return g_pGLFunctionsTable->glClientWaitSync(sync, flags, timeout);
}

void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    g_pGLFunctionsTable->glWaitSync(sync, flags, timeout);
}

void glDeleteSync(GLsync sync) {
    g_pGLFunctionsTable->glDeleteSync(sync);
}

// ========================================================================
// Query Functions
// ========================================================================

void glGenQueries(GLsizei n, GLuint* ids) {
    g_pGLFunctionsTable->glGenQueries(n, ids);
}

void glDeleteQueries(GLsizei n, const GLuint* ids) {
    g_pGLFunctionsTable->glDeleteQueries(n, ids);
}

void glBeginQuery(GLenum target, GLuint id) {
    g_pGLFunctionsTable->glBeginQuery(target, id);
}

void glEndQuery(GLenum target) {
    g_pGLFunctionsTable->glEndQuery(target);
}

void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
    g_pGLFunctionsTable->glGetQueryObjectuiv(id, pname, params);
}

void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    g_pGLFunctionsTable->glGetQueryObjecti64v(id, pname, params);
}

// ========================================================================
// Transform Feedback Functions
// ========================================================================

void glGenTransformFeedbacks(GLsizei n, GLuint* ids) {
    g_pGLFunctionsTable->glGenTransformFeedbacks(n, ids);
}

void glDeleteTransformFeedbacks(GLsizei n, const GLuint* ids) {
    g_pGLFunctionsTable->glDeleteTransformFeedbacks(n, ids);
}

void glBindTransformFeedback(GLenum target, GLuint id) {
    g_pGLFunctionsTable->glBindTransformFeedback(target, id);
}

void glBeginTransformFeedback(GLenum primitiveMode) {
    g_pGLFunctionsTable->glBeginTransformFeedback(primitiveMode);
}

void glEndTransformFeedback() {
    g_pGLFunctionsTable->glEndTransformFeedback();
}

void glPauseTransformFeedback() {
    g_pGLFunctionsTable->glPauseTransformFeedback();
}

void glResumeTransformFeedback() {
    g_pGLFunctionsTable->glResumeTransformFeedback();
}

// ========================================================================
// State Management Functions
// ========================================================================

void glEnable(GLenum cap) {
    g_pGLFunctionsTable->glEnable(cap);
}

void glDisable(GLenum cap) {
    g_pGLFunctionsTable->glDisable(cap);
}

GLboolean glIsEnabled(GLenum cap) {
    return g_pGLFunctionsTable->glIsEnabled(cap);
}

void glDepthFunc(GLenum func) {
    g_pGLFunctionsTable->glDepthFunc(func);
}

void glDepthMask(GLboolean flag) {
    g_pGLFunctionsTable->glDepthMask(flag);
}

void glDepthRangef(GLfloat n, GLfloat f) {
    g_pGLFunctionsTable->glDepthRangef(n, f);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    g_pGLFunctionsTable->glStencilFunc(func, ref, mask);
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    g_pGLFunctionsTable->glStencilFuncSeparate(face, func, ref, mask);
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    g_pGLFunctionsTable->glStencilOp(fail, zfail, zpass);
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    g_pGLFunctionsTable->glStencilOpSeparate(face, sfail, dpfail, dppass);
}

void glStencilMask(GLuint mask) {
    g_pGLFunctionsTable->glStencilMask(mask);
}

void glStencilMaskSeparate(GLenum face, GLuint mask) {
    g_pGLFunctionsTable->glStencilMaskSeparate(face, mask);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    g_pGLFunctionsTable->glBlendFunc(sfactor, dfactor);
}

void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    g_pGLFunctionsTable->glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

void glBlendEquation(GLenum mode) {
    g_pGLFunctionsTable->glBlendEquation(mode);
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    g_pGLFunctionsTable->glBlendEquationSeparate(modeRGB, modeAlpha);
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    g_pGLFunctionsTable->glBlendColor(red, green, blue, alpha);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    g_pGLFunctionsTable->glColorMask(red, green, blue, alpha);
}

void glCullFace(GLenum mode) {
    g_pGLFunctionsTable->glCullFace(mode);
}

void glFrontFace(GLenum mode) {
    g_pGLFunctionsTable->glFrontFace(mode);
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    g_pGLFunctionsTable->glPolygonOffset(factor, units);
}

void glLineWidth(GLfloat width) {
    g_pGLFunctionsTable->glLineWidth(width);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    g_pGLFunctionsTable->glScissor(x, y, width, height);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    g_pGLFunctionsTable->glViewport(x, y, width, height);
}

// ========================================================================
// Pixel Store Functions
// ========================================================================

void glPixelStorei(GLenum pname, GLint param) {
    g_pGLFunctionsTable->glPixelStorei(pname, param);
}

// ========================================================================
// Misc Functions
// ========================================================================

void glFlush() {
    g_pGLFunctionsTable->glFlush();
}

void glFinish() {
    g_pGLFunctionsTable->glFinish();
}

void glHint(GLenum target, GLenum mode) {
    g_pGLFunctionsTable->glHint(target, mode);
}

void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint* params) {
    g_pGLFunctionsTable->glGetInternalformativ(target, internalformat, pname, count, params);
}