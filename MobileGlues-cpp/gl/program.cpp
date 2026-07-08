// MobileGlues - gl/program.cpp
// Program state management: ID mapping, linking, usage tracking.
// Shader compilation and source management is handled by shader.cpp.
//
// Architecture: DirectGLES pattern
//   Public function → _State (validate + state) → _Backend (actual GLES call)
//
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "../includes.h"
#include <GL/gl.h>
#include "glcorearb.h"
#include "log.h"
#include "../gles/loader.h"
#include "mg.h"
#include <GLES3/gl32.h>
#include <cstring>
#include <algorithm>
#include <vector>

#define DEBUG 0

// ============================================================================
// Program Object Lifecycle (ES 3.2 native)
// ============================================================================

// --- glCreateProgram ---

static GLuint CreateProgram_Backend() {
    return GLES.glCreateProgram();
}

static GLuint CreateProgram_State() {
    GLuint program = CreateProgram_Backend();
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::OutOfMemory,
            std::make_unique<GenericErrorInfo>("glCreateProgram: failed to create program object"));
        return 0;
    }
    auto &ss = GLState.shader;
    ss.programMap[program] = program;
    ss.programMapReverse[program] = program;
    ss.programInfo[program] = GLStateManager::ShaderState::ProgramInfo{};
    return program;
}

GLuint glCreateProgram() {
    LOG()
    GLState.Lock();
    GLuint result = CreateProgram_State();
    GLState.Unlock();
    return result;
}

// --- glDeleteProgram ---

static void DeleteProgram_Backend(GLuint program) {
    GLES.glDeleteProgram(program);
}

static void DeleteProgram_State(GLuint program) {
    if (program == 0) return;
    auto &ss = GLState.shader;
    if (ss.currentProgram == program) {
        ss.currentProgram = 0;
        GLState.currentProgram = 0;
    }
    ss.programMap.erase(program);
    ss.programMapReverse.erase(program);
    ss.programInfo.erase(program);
    DeleteProgram_Backend(program);
    CHECK_GL_ERROR
}

void glDeleteProgram(GLuint program) {
    LOG()
    GLState.Lock();
    DeleteProgram_State(program);
    GLState.Unlock();
}

// ============================================================================
// Shader Object Lifecycle
// ============================================================================

// glCreateShader is handled in shader.cpp (GLSL conversion pipeline)
// glShaderSource is handled in shader.cpp
// glCompileShader is handled in shader.cpp

// --- glDeleteShader ---

static void DeleteShader_Backend(GLuint shader) {
    GLES.glDeleteShader(shader);
}

static void DeleteShader_State(GLuint shader) {
    if (shader == 0) return;
    auto &ss = GLState.shader;
    ss.shaderMap.erase(shader);
    ss.shaderMapReverse.erase(shader);
    ss.shaderInfo.erase(shader);
    DeleteShader_Backend(shader);
    CHECK_GL_ERROR
}

void glDeleteShader(GLuint shader) {
    LOG()
    GLState.Lock();
    DeleteShader_State(shader);
    GLState.Unlock();
}

// ============================================================================
// Program Linking & Validation (ES 3.2 native)
// ============================================================================

// --- glAttachShader ---

static void AttachShader_Backend(GLuint program, GLuint shader) {
    GLES.glAttachShader(program, shader);
}

static void AttachShader_State(GLuint program, GLuint shader) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glAttachShader: program is 0"));
        return;
    }
    if (shader == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glAttachShader: shader is 0"));
        return;
    }
    AttachShader_Backend(program, shader);
    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.attachedShaders.push_back(shader);
    } else {
        ss.programInfo[program] = GLStateManager::ShaderState::ProgramInfo{};
        ss.programInfo[program].attachedShaders.push_back(shader);
    }
}

void glAttachShader(GLuint program, GLuint shader) {
    LOG()
    GLState.Lock();
    AttachShader_State(program, shader);
    GLState.Unlock();
}

// --- glDetachShader ---

static void DetachShader_Backend(GLuint program, GLuint shader) {
    GLES.glDetachShader(program, shader);
}

static void DetachShader_State(GLuint program, GLuint shader) {
    if (program == 0 || shader == 0) return;
    DetachShader_Backend(program, shader);
    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        auto &vec = it->second.attachedShaders;
        vec.erase(std::remove(vec.begin(), vec.end(), shader), vec.end());
    }
    CHECK_GL_ERROR
}

void glDetachShader(GLuint program, GLuint shader) {
    LOG()
    GLState.Lock();
    DetachShader_State(program, shader);
    GLState.Unlock();
}

// --- glLinkProgram ---

static void LinkProgram_Backend(GLuint program) {
    GLES.glLinkProgram(program);
}

static void LinkProgram_State(GLuint program) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glLinkProgram: program is 0"));
        return;
    }
    LinkProgram_Backend(program);

    GLint linkStatus = 0;
    GLES.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.linked = (linkStatus == GL_TRUE);
    }

    if (linkStatus == GL_FALSE) {
        GLint infoLogLength = 0;
        GLES.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 1) {
            char* infoLog = new char[infoLogLength];
            GLES.glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog);
            MG_LOG_ERROR("glLinkProgram FAILED: %s", infoLog);
            delete[] infoLog;
        }
    }
}

void glLinkProgram(GLuint program) {
    LOG()
    GLState.Lock();
    LinkProgram_State(program);
    GLState.Unlock();
}

// --- glValidateProgram ---

static void ValidateProgram_Backend(GLuint program) {
    GLES.glValidateProgram(program);
}

static void ValidateProgram_State(GLuint program) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glValidateProgram: program is 0"));
        return;
    }
    ValidateProgram_Backend(program);

    GLint validateStatus = 0;
    GLES.glGetProgramiv(program, GL_VALIDATE_STATUS, &validateStatus);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.validated = (validateStatus == GL_TRUE);
    }

    if (validateStatus == GL_FALSE) {
        GLint infoLogLength = 0;
        GLES.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 1) {
            char* infoLog = new char[infoLogLength];
            GLES.glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog);
            MG_LOG_ERROR("glValidateProgram FAILED: %s", infoLog);
            delete[] infoLog;
        }
    }
}

void glValidateProgram(GLuint program) {
    LOG()
    GLState.Lock();
    ValidateProgram_State(program);
    GLState.Unlock();
}

// ============================================================================
// Program Usage (ES 3.2 native)
// ============================================================================

// --- glUseProgram ---

static void UseProgram_Backend(GLuint program) {
    GLES.glUseProgram(program);
}

static void UseProgram_State(GLuint program) {
    GLState.shader.currentProgram = program;
    GLState.currentProgram = program;
    UseProgram_Backend(program);
}

void glUseProgram(GLuint program) {
    LOG()
    GLState.Lock();
    UseProgram_State(program);
    GLState.Unlock();
}

// ============================================================================
// Program/Shader Queries (ES 3.2 native)
// ============================================================================

// --- glGetProgramiv ---

static void GetProgramiv_Backend(GLuint program, GLenum pname, GLint* params) {
    GLES.glGetProgramiv(program, pname, params);
}

static void GetProgramiv_State(GLuint program, GLenum pname, GLint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramiv: params is NULL"));
        return;
    }
    GetProgramiv_Backend(program, pname, params);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    GLState.Lock();
    GetProgramiv_State(program, pname, params);
    GLState.Unlock();
}

// --- glGetShaderiv ---

static void GetShaderiv_Backend(GLuint shader, GLenum pname, GLint* params) {
    GLES.glGetShaderiv(shader, pname, params);
}

static void GetShaderiv_State(GLuint shader, GLenum pname, GLint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetShaderiv: params is NULL"));
        return;
    }
    GetShaderiv_Backend(shader, pname, params);
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    GLState.Lock();
    GetShaderiv_State(shader, pname, params);
    GLState.Unlock();
}

// --- glGetProgramInfoLog ---

static void GetProgramInfoLog_Backend(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    GLES.glGetProgramInfoLog(program, bufSize, length, infoLog);
}

static void GetProgramInfoLog_State(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramInfoLog: bufSize < 0"));
        return;
    }
    GetProgramInfoLog_Backend(program, bufSize, length, infoLog);
    CHECK_GL_ERROR
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    LOG()
    GLState.Lock();
    GetProgramInfoLog_State(program, bufSize, length, infoLog);
    GLState.Unlock();
}

// --- glGetShaderInfoLog ---

static void GetShaderInfoLog_Backend(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    GLES.glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

static void GetShaderInfoLog_State(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetShaderInfoLog: bufSize < 0"));
        return;
    }
    GetShaderInfoLog_Backend(shader, bufSize, length, infoLog);
    CHECK_GL_ERROR
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
    LOG()
    GLState.Lock();
    GetShaderInfoLog_State(shader, bufSize, length, infoLog);
    GLState.Unlock();
}

// --- glGetAttachedShaders ---

static void GetAttachedShaders_Backend(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders) {
    GLES.glGetAttachedShaders(program, maxCount, count, shaders);
}

static void GetAttachedShaders_State(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders) {
    if (maxCount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetAttachedShaders: maxCount < 0"));
        return;
    }
    GetAttachedShaders_Backend(program, maxCount, count, shaders);
    CHECK_GL_ERROR
}

void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders) {
    LOG()
    GLState.Lock();
    GetAttachedShaders_State(program, maxCount, count, shaders);
    GLState.Unlock();
}

// ============================================================================
// Attribute Location (ES 3.2 native)
// ============================================================================

// --- glBindAttribLocation ---

static void BindAttribLocation_Backend(GLuint program, GLuint index, const GLchar* name) {
    GLES.glBindAttribLocation(program, index, name);
}

static void BindAttribLocation_State(GLuint program, GLuint index, const GLchar* name) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindAttribLocation: program is 0"));
        return;
    }
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindAttribLocation: name is NULL"));
        return;
    }
    BindAttribLocation_Backend(program, index, name);
    CHECK_GL_ERROR
}

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    LOG()
    GLState.Lock();
    BindAttribLocation_State(program, index, name);
    GLState.Unlock();
}

// --- glGetAttribLocation ---

static GLint GetAttribLocation_Backend(GLuint program, const GLchar* name) {
    return GLES.glGetAttribLocation(program, name);
}

static GLint GetAttribLocation_State(GLuint program, const GLchar* name) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetAttribLocation: program is 0"));
        return -1;
    }
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetAttribLocation: name is NULL"));
        return -1;
    }
    GLint result = GetAttribLocation_Backend(program, name);
    CHECK_GL_ERROR
    return result;
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    LOG()
    GLState.Lock();
    GLint result = GetAttribLocation_State(program, name);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Uniform Location (ES 3.2 native)
// ============================================================================

// --- glGetUniformLocation ---

static GLint GetUniformLocation_Backend(GLuint program, const GLchar* name) {
    return GLES.glGetUniformLocation(program, name);
}

static GLint GetUniformLocation_State(GLuint program, const GLchar* name) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformLocation: program is 0"));
        return -1;
    }
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformLocation: name is NULL"));
        return -1;
    }
    GLint result = GetUniformLocation_Backend(program, name);
    CHECK_GL_ERROR
    return result;
}

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    LOG()
    GLState.Lock();
    GLint result = GetUniformLocation_State(program, name);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Active Attribute/Uniform Queries (ES 3.2 native)
// ============================================================================

// --- glGetActiveAttrib ---

static void GetActiveAttrib_Backend(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length,
                                    GLint* size, GLenum* type, GLchar* name) {
    GLES.glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}

static void GetActiveAttrib_State(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length,
                                  GLint* size, GLenum* type, GLchar* name) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetActiveAttrib: bufSize < 0"));
        return;
    }
    GetActiveAttrib_Backend(program, index, bufSize, length, size, type, name);
    CHECK_GL_ERROR
}

void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size,
                       GLenum* type, GLchar* name) {
    LOG()
    GLState.Lock();
    GetActiveAttrib_State(program, index, bufSize, length, size, type, name);
    GLState.Unlock();
}

// --- glGetActiveUniform ---

static void GetActiveUniform_Backend(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length,
                                     GLint* size, GLenum* type, GLchar* name) {
    GLES.glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

static void GetActiveUniform_State(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length,
                                   GLint* size, GLenum* type, GLchar* name) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetActiveUniform: bufSize < 0"));
        return;
    }
    GetActiveUniform_Backend(program, index, bufSize, length, size, type, name);
    CHECK_GL_ERROR
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size,
                        GLenum* type, GLchar* name) {
    LOG()
    GLState.Lock();
    GetActiveUniform_State(program, index, bufSize, length, size, type, name);
    GLState.Unlock();
}

// --- glGetActiveUniformName ---

static void GetActiveUniformName_Backend(GLuint program, GLuint uniformIndex, GLsizei bufSize,
                                         GLsizei* length, GLchar* uniformName) {
    GLES.glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}

static void GetActiveUniformName_State(GLuint program, GLuint uniformIndex, GLsizei bufSize,
                                       GLsizei* length, GLchar* uniformName) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetActiveUniformName: bufSize < 0"));
        return;
    }
    GetActiveUniformName_Backend(program, uniformIndex, bufSize, length, uniformName);
    CHECK_GL_ERROR
}

void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize,
                            GLsizei* length, GLchar* uniformName) {
    LOG()
    GLState.Lock();
    GetActiveUniformName_State(program, uniformIndex, bufSize, length, uniformName);
    GLState.Unlock();
}

// --- glGetUniformIndices ---

static void GetUniformIndices_Backend(GLuint program, GLsizei uniformCount,
                                      const GLchar* const* uniformNames, GLuint* uniformIndices) {
    GLES.glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}

static void GetUniformIndices_State(GLuint program, GLsizei uniformCount,
                                    const GLchar* const* uniformNames, GLuint* uniformIndices) {
    if (uniformCount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformIndices: uniformCount < 0"));
        return;
    }
    if (uniformNames == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformIndices: uniformNames is NULL"));
        return;
    }
    GetUniformIndices_Backend(program, uniformCount, uniformNames, uniformIndices);
    CHECK_GL_ERROR
}

void glGetUniformIndices(GLuint program, GLsizei uniformCount,
                         const GLchar* const* uniformNames, GLuint* uniformIndices) {
    LOG()
    GLState.Lock();
    GetUniformIndices_State(program, uniformCount, uniformNames, uniformIndices);
    GLState.Unlock();
}

// ============================================================================
// Uniform Getters (ES 3.2 native)
// ============================================================================

// --- glGetUniformfv ---

static void GetUniformfv_Backend(GLuint program, GLint location, GLfloat* params) {
    GLES.glGetUniformfv(program, location, params);
}

static void GetUniformfv_State(GLuint program, GLint location, GLfloat* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformfv: params is NULL"));
        return;
    }
    GetUniformfv_Backend(program, location, params);
}

void glGetUniformfv(GLuint program, GLint location, GLfloat* params) {
    GLState.Lock();
    GetUniformfv_State(program, location, params);
    GLState.Unlock();
}

// --- glGetUniformiv ---

static void GetUniformiv_Backend(GLuint program, GLint location, GLint* params) {
    GLES.glGetUniformiv(program, location, params);
}

static void GetUniformiv_State(GLuint program, GLint location, GLint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformiv: params is NULL"));
        return;
    }
    GetUniformiv_Backend(program, location, params);
}

void glGetUniformiv(GLuint program, GLint location, GLint* params) {
    GLState.Lock();
    GetUniformiv_State(program, location, params);
    GLState.Unlock();
}

// --- glGetUniformuiv ---

static void GetUniformuiv_Backend(GLuint program, GLint location, GLuint* params) {
    GLES.glGetUniformuiv(program, location, params);
}

static void GetUniformuiv_State(GLuint program, GLint location, GLuint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformuiv: params is NULL"));
        return;
    }
    GetUniformuiv_Backend(program, location, params);
}

void glGetUniformuiv(GLuint program, GLint location, GLuint* params) {
    GLState.Lock();
    GetUniformuiv_State(program, location, params);
    GLState.Unlock();
}

// ============================================================================
// Uniform Setters - Scalar Float (ES 3.2 native)
// ============================================================================

// --- glUniform1f ---

static void Uniform1f_Backend(GLint location, GLfloat v0) {
    GLES.glUniform1f(location, v0);
}

static void Uniform1f_State(GLint location, GLfloat v0) {
    Uniform1f_Backend(location, v0);
}

void glUniform1f(GLint location, GLfloat v0) {
    GLState.Lock();
    Uniform1f_State(location, v0);
    GLState.Unlock();
}

// --- glUniform2f ---

static void Uniform2f_Backend(GLint location, GLfloat v0, GLfloat v1) {
    GLES.glUniform2f(location, v0, v1);
}

static void Uniform2f_State(GLint location, GLfloat v0, GLfloat v1) {
    Uniform2f_Backend(location, v0, v1);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GLState.Lock();
    Uniform2f_State(location, v0, v1);
    GLState.Unlock();
}

// --- glUniform3f ---

static void Uniform3f_Backend(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLES.glUniform3f(location, v0, v1, v2);
}

static void Uniform3f_State(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    Uniform3f_Backend(location, v0, v1, v2);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLState.Lock();
    Uniform3f_State(location, v0, v1, v2);
    GLState.Unlock();
}

// --- glUniform4f ---

static void Uniform4f_Backend(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLES.glUniform4f(location, v0, v1, v2, v3);
}

static void Uniform4f_State(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    Uniform4f_Backend(location, v0, v1, v2, v3);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLState.Lock();
    Uniform4f_State(location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glUniform1fv ---

static void Uniform1fv_Backend(GLint location, GLsizei count, const GLfloat* value) {
    GLES.glUniform1fv(location, count, value);
}

static void Uniform1fv_State(GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform1fv: count < 0"));
        return;
    }
    Uniform1fv_Backend(location, count, value);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    Uniform1fv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform2fv ---

static void Uniform2fv_Backend(GLint location, GLsizei count, const GLfloat* value) {
    GLES.glUniform2fv(location, count, value);
}

static void Uniform2fv_State(GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform2fv: count < 0"));
        return;
    }
    Uniform2fv_Backend(location, count, value);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    Uniform2fv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform3fv ---

static void Uniform3fv_Backend(GLint location, GLsizei count, const GLfloat* value) {
    GLES.glUniform3fv(location, count, value);
}

static void Uniform3fv_State(GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform3fv: count < 0"));
        return;
    }
    Uniform3fv_Backend(location, count, value);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    Uniform3fv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform4fv ---

static void Uniform4fv_Backend(GLint location, GLsizei count, const GLfloat* value) {
    GLES.glUniform4fv(location, count, value);
}

static void Uniform4fv_State(GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform4fv: count < 0"));
        return;
    }
    Uniform4fv_Backend(location, count, value);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    Uniform4fv_State(location, count, value);
    GLState.Unlock();
}

// ============================================================================
// Uniform Setters - Scalar Int (ES 3.2 native)
// ============================================================================

// --- glUniform1i ---

static void Uniform1i_Backend(GLint location, GLint v0) {
    GLES.glUniform1i(location, v0);
}

static void Uniform1i_State(GLint location, GLint v0) {
    Uniform1i_Backend(location, v0);
}

void glUniform1i(GLint location, GLint v0) {
    GLState.Lock();
    Uniform1i_State(location, v0);
    GLState.Unlock();
}

// --- glUniform2i ---

static void Uniform2i_Backend(GLint location, GLint v0, GLint v1) {
    GLES.glUniform2i(location, v0, v1);
}

static void Uniform2i_State(GLint location, GLint v0, GLint v1) {
    Uniform2i_Backend(location, v0, v1);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
    GLState.Lock();
    Uniform2i_State(location, v0, v1);
    GLState.Unlock();
}

// --- glUniform3i ---

static void Uniform3i_Backend(GLint location, GLint v0, GLint v1, GLint v2) {
    GLES.glUniform3i(location, v0, v1, v2);
}

static void Uniform3i_State(GLint location, GLint v0, GLint v1, GLint v2) {
    Uniform3i_Backend(location, v0, v1, v2);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GLState.Lock();
    Uniform3i_State(location, v0, v1, v2);
    GLState.Unlock();
}

// --- glUniform4i ---

static void Uniform4i_Backend(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLES.glUniform4i(location, v0, v1, v2, v3);
}

static void Uniform4i_State(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    Uniform4i_Backend(location, v0, v1, v2, v3);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLState.Lock();
    Uniform4i_State(location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glUniform1iv ---

static void Uniform1iv_Backend(GLint location, GLsizei count, const GLint* value) {
    GLES.glUniform1iv(location, count, value);
}

static void Uniform1iv_State(GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform1iv: count < 0"));
        return;
    }
    Uniform1iv_Backend(location, count, value);
}

void glUniform1iv(GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    Uniform1iv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform2iv ---

static void Uniform2iv_Backend(GLint location, GLsizei count, const GLint* value) {
    GLES.glUniform2iv(location, count, value);
}

static void Uniform2iv_State(GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform2iv: count < 0"));
        return;
    }
    Uniform2iv_Backend(location, count, value);
}

void glUniform2iv(GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    Uniform2iv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform3iv ---

static void Uniform3iv_Backend(GLint location, GLsizei count, const GLint* value) {
    GLES.glUniform3iv(location, count, value);
}

static void Uniform3iv_State(GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform3iv: count < 0"));
        return;
    }
    Uniform3iv_Backend(location, count, value);
}

void glUniform3iv(GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    Uniform3iv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform4iv ---

static void Uniform4iv_Backend(GLint location, GLsizei count, const GLint* value) {
    GLES.glUniform4iv(location, count, value);
}

static void Uniform4iv_State(GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform4iv: count < 0"));
        return;
    }
    Uniform4iv_Backend(location, count, value);
}

void glUniform4iv(GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    Uniform4iv_State(location, count, value);
    GLState.Unlock();
}

// ============================================================================
// Uniform Setters - Scalar Unsigned Int (ES 3.2 native)
// ============================================================================

// --- glUniform1ui ---

static void Uniform1ui_Backend(GLint location, GLuint v0) {
    GLES.glUniform1ui(location, v0);
}

static void Uniform1ui_State(GLint location, GLuint v0) {
    Uniform1ui_Backend(location, v0);
}

void glUniform1ui(GLint location, GLuint v0) {
    GLState.Lock();
    Uniform1ui_State(location, v0);
    GLState.Unlock();
}

// --- glUniform2ui ---

static void Uniform2ui_Backend(GLint location, GLuint v0, GLuint v1) {
    GLES.glUniform2ui(location, v0, v1);
}

static void Uniform2ui_State(GLint location, GLuint v0, GLuint v1) {
    Uniform2ui_Backend(location, v0, v1);
}

void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    GLState.Lock();
    Uniform2ui_State(location, v0, v1);
    GLState.Unlock();
}

// --- glUniform3ui ---

static void Uniform3ui_Backend(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GLES.glUniform3ui(location, v0, v1, v2);
}

static void Uniform3ui_State(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    Uniform3ui_Backend(location, v0, v1, v2);
}

void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GLState.Lock();
    Uniform3ui_State(location, v0, v1, v2);
    GLState.Unlock();
}

// --- glUniform4ui ---

static void Uniform4ui_Backend(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GLES.glUniform4ui(location, v0, v1, v2, v3);
}

static void Uniform4ui_State(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    Uniform4ui_Backend(location, v0, v1, v2, v3);
}

void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GLState.Lock();
    Uniform4ui_State(location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glUniform1uiv ---

static void Uniform1uiv_Backend(GLint location, GLsizei count, const GLuint* value) {
    GLES.glUniform1uiv(location, count, value);
}

static void Uniform1uiv_State(GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform1uiv: count < 0"));
        return;
    }
    Uniform1uiv_Backend(location, count, value);
}

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    Uniform1uiv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform2uiv ---

static void Uniform2uiv_Backend(GLint location, GLsizei count, const GLuint* value) {
    GLES.glUniform2uiv(location, count, value);
}

static void Uniform2uiv_State(GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform2uiv: count < 0"));
        return;
    }
    Uniform2uiv_Backend(location, count, value);
}

void glUniform2uiv(GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    Uniform2uiv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform3uiv ---

static void Uniform3uiv_Backend(GLint location, GLsizei count, const GLuint* value) {
    GLES.glUniform3uiv(location, count, value);
}

static void Uniform3uiv_State(GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform3uiv: count < 0"));
        return;
    }
    Uniform3uiv_Backend(location, count, value);
}

void glUniform3uiv(GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    Uniform3uiv_State(location, count, value);
    GLState.Unlock();
}

// --- glUniform4uiv ---

static void Uniform4uiv_Backend(GLint location, GLsizei count, const GLuint* value) {
    GLES.glUniform4uiv(location, count, value);
}

static void Uniform4uiv_State(GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniform4uiv: count < 0"));
        return;
    }
    Uniform4uiv_Backend(location, count, value);
}

void glUniform4uiv(GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    Uniform4uiv_State(location, count, value);
    GLState.Unlock();
}

// ============================================================================
// Uniform Setters - Matrix (ES 3.2 native)
// ============================================================================

// --- glUniformMatrix2fv ---

static void UniformMatrix2fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix2fv(location, count, transpose, value);
}

static void UniformMatrix2fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix2fv_Backend(location, count, transpose, value);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix2fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix3fv ---

static void UniformMatrix3fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix3fv(location, count, transpose, value);
}

static void UniformMatrix3fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix3fv_Backend(location, count, transpose, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix3fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix4fv ---

static void UniformMatrix4fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix4fv(location, count, transpose, value);
}

static void UniformMatrix4fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix4fv_Backend(location, count, transpose, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix4fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix2x3fv ---

static void UniformMatrix2x3fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix2x3fv(location, count, transpose, value);
}

static void UniformMatrix2x3fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2x3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2x3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix2x3fv_Backend(location, count, transpose, value);
}

void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix2x3fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix3x2fv ---

static void UniformMatrix3x2fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix3x2fv(location, count, transpose, value);
}

static void UniformMatrix3x2fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3x2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3x2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix3x2fv_Backend(location, count, transpose, value);
}

void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix3x2fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix2x4fv ---

static void UniformMatrix2x4fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix2x4fv(location, count, transpose, value);
}

static void UniformMatrix2x4fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2x4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix2x4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix2x4fv_Backend(location, count, transpose, value);
}

void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix2x4fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix4x2fv ---

static void UniformMatrix4x2fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix4x2fv(location, count, transpose, value);
}

static void UniformMatrix4x2fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4x2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4x2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix4x2fv_Backend(location, count, transpose, value);
}

void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix4x2fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix3x4fv ---

static void UniformMatrix3x4fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix3x4fv(location, count, transpose, value);
}

static void UniformMatrix3x4fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3x4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix3x4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix3x4fv_Backend(location, count, transpose, value);
}

void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix3x4fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// --- glUniformMatrix4x3fv ---

static void UniformMatrix4x3fv_Backend(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLES.glUniformMatrix4x3fv(location, count, transpose, value);
}

static void UniformMatrix4x3fv_State(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4x3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glUniformMatrix4x3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    UniformMatrix4x3fv_Backend(location, count, transpose, value);
}

void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    UniformMatrix4x3fv_State(location, count, transpose, value);
    GLState.Unlock();
}

// ============================================================================
// Program Uniforms - DSA variants (ES 3.1+ native)
// ============================================================================

// --- glProgramUniform1f ---

static void ProgramUniform1f_Backend(GLuint program, GLint location, GLfloat v0) {
    GLES.glProgramUniform1f(program, location, v0);
}

static void ProgramUniform1f_State(GLuint program, GLint location, GLfloat v0) {
    ProgramUniform1f_Backend(program, location, v0);
}

void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    GLState.Lock();
    ProgramUniform1f_State(program, location, v0);
    GLState.Unlock();
}

// --- glProgramUniform2f ---

static void ProgramUniform2f_Backend(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    GLES.glProgramUniform2f(program, location, v0, v1);
}

static void ProgramUniform2f_State(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    ProgramUniform2f_Backend(program, location, v0, v1);
}

void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    GLState.Lock();
    ProgramUniform2f_State(program, location, v0, v1);
    GLState.Unlock();
}

// --- glProgramUniform3f ---

static void ProgramUniform3f_Backend(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLES.glProgramUniform3f(program, location, v0, v1, v2);
}

static void ProgramUniform3f_State(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    ProgramUniform3f_Backend(program, location, v0, v1, v2);
}

void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLState.Lock();
    ProgramUniform3f_State(program, location, v0, v1, v2);
    GLState.Unlock();
}

// --- glProgramUniform4f ---

static void ProgramUniform4f_Backend(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLES.glProgramUniform4f(program, location, v0, v1, v2, v3);
}

static void ProgramUniform4f_State(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    ProgramUniform4f_Backend(program, location, v0, v1, v2, v3);
}

void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLState.Lock();
    ProgramUniform4f_State(program, location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glProgramUniform1i ---

static void ProgramUniform1i_Backend(GLuint program, GLint location, GLint v0) {
    GLES.glProgramUniform1i(program, location, v0);
}

static void ProgramUniform1i_State(GLuint program, GLint location, GLint v0) {
    ProgramUniform1i_Backend(program, location, v0);
}

void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    GLState.Lock();
    ProgramUniform1i_State(program, location, v0);
    GLState.Unlock();
}

// --- glProgramUniform2i ---

static void ProgramUniform2i_Backend(GLuint program, GLint location, GLint v0, GLint v1) {
    GLES.glProgramUniform2i(program, location, v0, v1);
}

static void ProgramUniform2i_State(GLuint program, GLint location, GLint v0, GLint v1) {
    ProgramUniform2i_Backend(program, location, v0, v1);
}

void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    GLState.Lock();
    ProgramUniform2i_State(program, location, v0, v1);
    GLState.Unlock();
}

// --- glProgramUniform3i ---

static void ProgramUniform3i_Backend(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    GLES.glProgramUniform3i(program, location, v0, v1, v2);
}

static void ProgramUniform3i_State(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    ProgramUniform3i_Backend(program, location, v0, v1, v2);
}

void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    GLState.Lock();
    ProgramUniform3i_State(program, location, v0, v1, v2);
    GLState.Unlock();
}

// --- glProgramUniform4i ---

static void ProgramUniform4i_Backend(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLES.glProgramUniform4i(program, location, v0, v1, v2, v3);
}

static void ProgramUniform4i_State(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    ProgramUniform4i_Backend(program, location, v0, v1, v2, v3);
}

void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLState.Lock();
    ProgramUniform4i_State(program, location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glProgramUniform1ui ---

static void ProgramUniform1ui_Backend(GLuint program, GLint location, GLuint v0) {
    GLES.glProgramUniform1ui(program, location, v0);
}

static void ProgramUniform1ui_State(GLuint program, GLint location, GLuint v0) {
    ProgramUniform1ui_Backend(program, location, v0);
}

void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
    GLState.Lock();
    ProgramUniform1ui_State(program, location, v0);
    GLState.Unlock();
}

// --- glProgramUniform2ui ---

static void ProgramUniform2ui_Backend(GLuint program, GLint location, GLuint v0, GLuint v1) {
    GLES.glProgramUniform2ui(program, location, v0, v1);
}

static void ProgramUniform2ui_State(GLuint program, GLint location, GLuint v0, GLuint v1) {
    ProgramUniform2ui_Backend(program, location, v0, v1);
}

void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {
    GLState.Lock();
    ProgramUniform2ui_State(program, location, v0, v1);
    GLState.Unlock();
}

// --- glProgramUniform3ui ---

static void ProgramUniform3ui_Backend(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GLES.glProgramUniform3ui(program, location, v0, v1, v2);
}

static void ProgramUniform3ui_State(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    ProgramUniform3ui_Backend(program, location, v0, v1, v2);
}

void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GLState.Lock();
    ProgramUniform3ui_State(program, location, v0, v1, v2);
    GLState.Unlock();
}

// --- glProgramUniform4ui ---

static void ProgramUniform4ui_Backend(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GLES.glProgramUniform4ui(program, location, v0, v1, v2, v3);
}

static void ProgramUniform4ui_State(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    ProgramUniform4ui_Backend(program, location, v0, v1, v2, v3);
}

void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GLState.Lock();
    ProgramUniform4ui_State(program, location, v0, v1, v2, v3);
    GLState.Unlock();
}

// --- glProgramUniform1fv ---

static void ProgramUniform1fv_Backend(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLES.glProgramUniform1fv(program, location, count, value);
}

static void ProgramUniform1fv_State(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform1fv: count < 0"));
        return;
    }
    ProgramUniform1fv_Backend(program, location, count, value);
}

void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    ProgramUniform1fv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform2fv ---

static void ProgramUniform2fv_Backend(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLES.glProgramUniform2fv(program, location, count, value);
}

static void ProgramUniform2fv_State(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform2fv: count < 0"));
        return;
    }
    ProgramUniform2fv_Backend(program, location, count, value);
}

void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    ProgramUniform2fv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform3fv ---

static void ProgramUniform3fv_Backend(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLES.glProgramUniform3fv(program, location, count, value);
}

static void ProgramUniform3fv_State(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform3fv: count < 0"));
        return;
    }
    ProgramUniform3fv_Backend(program, location, count, value);
}

void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    ProgramUniform3fv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform4fv ---

static void ProgramUniform4fv_Backend(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLES.glProgramUniform4fv(program, location, count, value);
}

static void ProgramUniform4fv_State(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform4fv: count < 0"));
        return;
    }
    ProgramUniform4fv_Backend(program, location, count, value);
}

void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLState.Lock();
    ProgramUniform4fv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform1iv ---

static void ProgramUniform1iv_Backend(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLES.glProgramUniform1iv(program, location, count, value);
}

static void ProgramUniform1iv_State(GLuint program, GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform1iv: count < 0"));
        return;
    }
    ProgramUniform1iv_Backend(program, location, count, value);
}

void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    ProgramUniform1iv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform2iv ---

static void ProgramUniform2iv_Backend(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLES.glProgramUniform2iv(program, location, count, value);
}

static void ProgramUniform2iv_State(GLuint program, GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform2iv: count < 0"));
        return;
    }
    ProgramUniform2iv_Backend(program, location, count, value);
}

void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    ProgramUniform2iv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform3iv ---

static void ProgramUniform3iv_Backend(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLES.glProgramUniform3iv(program, location, count, value);
}

static void ProgramUniform3iv_State(GLuint program, GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform3iv: count < 0"));
        return;
    }
    ProgramUniform3iv_Backend(program, location, count, value);
}

void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    ProgramUniform3iv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform4iv ---

static void ProgramUniform4iv_Backend(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLES.glProgramUniform4iv(program, location, count, value);
}

static void ProgramUniform4iv_State(GLuint program, GLint location, GLsizei count, const GLint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform4iv: count < 0"));
        return;
    }
    ProgramUniform4iv_Backend(program, location, count, value);
}

void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint* value) {
    GLState.Lock();
    ProgramUniform4iv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform1uiv ---

static void ProgramUniform1uiv_Backend(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLES.glProgramUniform1uiv(program, location, count, value);
}

static void ProgramUniform1uiv_State(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform1uiv: count < 0"));
        return;
    }
    ProgramUniform1uiv_Backend(program, location, count, value);
}

void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    ProgramUniform1uiv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform2uiv ---

static void ProgramUniform2uiv_Backend(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLES.glProgramUniform2uiv(program, location, count, value);
}

static void ProgramUniform2uiv_State(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform2uiv: count < 0"));
        return;
    }
    ProgramUniform2uiv_Backend(program, location, count, value);
}

void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    ProgramUniform2uiv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform3uiv ---

static void ProgramUniform3uiv_Backend(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLES.glProgramUniform3uiv(program, location, count, value);
}

static void ProgramUniform3uiv_State(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform3uiv: count < 0"));
        return;
    }
    ProgramUniform3uiv_Backend(program, location, count, value);
}

void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    ProgramUniform3uiv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniform4uiv ---

static void ProgramUniform4uiv_Backend(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLES.glProgramUniform4uiv(program, location, count, value);
}

static void ProgramUniform4uiv_State(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniform4uiv: count < 0"));
        return;
    }
    ProgramUniform4uiv_Backend(program, location, count, value);
}

void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint* value) {
    GLState.Lock();
    ProgramUniform4uiv_State(program, location, count, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix2fv ---

static void ProgramUniformMatrix2fv_Backend(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix2fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix2fv_State(GLuint program, GLint location, GLsizei count,
                                          GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix2fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix2fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix3fv ---

static void ProgramUniformMatrix3fv_Backend(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix3fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix3fv_State(GLuint program, GLint location, GLsizei count,
                                          GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix3fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix3fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix4fv ---

static void ProgramUniformMatrix4fv_Backend(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix4fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix4fv_State(GLuint program, GLint location, GLsizei count,
                                          GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix4fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix4fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix2x3fv ---

static void ProgramUniformMatrix2x3fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix2x3fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2x3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2x3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix2x3fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix2x3fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix3x2fv ---

static void ProgramUniformMatrix3x2fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix3x2fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3x2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3x2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix3x2fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix3x2fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix2x4fv ---

static void ProgramUniformMatrix2x4fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix2x4fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2x4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix2x4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix2x4fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix2x4fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix4x2fv ---

static void ProgramUniformMatrix4x2fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix4x2fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4x2fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4x2fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix4x2fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix4x2fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix3x4fv ---

static void ProgramUniformMatrix3x4fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix3x4fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3x4fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix3x4fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix3x4fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix3x4fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// --- glProgramUniformMatrix4x3fv ---

static void ProgramUniformMatrix4x3fv_Backend(GLuint program, GLint location, GLsizei count,
                                              GLboolean transpose, const GLfloat* value) {
    GLES.glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
}

static void ProgramUniformMatrix4x3fv_State(GLuint program, GLint location, GLsizei count,
                                            GLboolean transpose, const GLfloat* value) {
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4x3fv: count < 0"));
        return;
    }
    if (transpose != GL_FALSE) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glProgramUniformMatrix4x3fv: transpose must be GL_FALSE in GLES"));
        return;
    }
    ProgramUniformMatrix4x3fv_Backend(program, location, count, transpose, value);
}

void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat* value) {
    GLState.Lock();
    ProgramUniformMatrix4x3fv_State(program, location, count, transpose, value);
    GLState.Unlock();
}

// ============================================================================
// Uniform Block Management (ES 3.1+ native)
// ============================================================================

// --- glGetUniformBlockIndex ---

static GLuint GetUniformBlockIndex_Backend(GLuint program, const GLchar* uniformBlockName) {
    return GLES.glGetUniformBlockIndex(program, uniformBlockName);
}

static GLuint GetUniformBlockIndex_State(GLuint program, const GLchar* uniformBlockName) {
    if (uniformBlockName == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetUniformBlockIndex: uniformBlockName is NULL"));
        return GL_INVALID_INDEX;
    }
    return GetUniformBlockIndex_Backend(program, uniformBlockName);
}

GLuint glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) {
    GLState.Lock();
    GLuint result = GetUniformBlockIndex_State(program, uniformBlockName);
    GLState.Unlock();
    return result;
}

// --- glUniformBlockBinding ---

static void UniformBlockBinding_Backend(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    GLES.glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}

static void UniformBlockBinding_State(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    UniformBlockBinding_Backend(program, uniformBlockIndex, uniformBlockBinding);
}

void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    GLState.Lock();
    UniformBlockBinding_State(program, uniformBlockIndex, uniformBlockBinding);
    GLState.Unlock();
}

// --- glGetActiveUniformBlockiv ---

static void GetActiveUniformBlockiv_Backend(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    GLES.glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}

static void GetActiveUniformBlockiv_State(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetActiveUniformBlockiv: params is NULL"));
        return;
    }
    GetActiveUniformBlockiv_Backend(program, uniformBlockIndex, pname, params);
}

void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params) {
    GLState.Lock();
    GetActiveUniformBlockiv_State(program, uniformBlockIndex, pname, params);
    GLState.Unlock();
}

// --- glGetActiveUniformBlockName ---

static void GetActiveUniformBlockName_Backend(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize,
                                              GLsizei* length, GLchar* uniformBlockName) {
    GLES.glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}

static void GetActiveUniformBlockName_State(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize,
                                            GLsizei* length, GLchar* uniformBlockName) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetActiveUniformBlockName: bufSize < 0"));
        return;
    }
    GetActiveUniformBlockName_Backend(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}

void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize,
                                 GLsizei* length, GLchar* uniformBlockName) {
    GLState.Lock();
    GetActiveUniformBlockName_State(program, uniformBlockIndex, bufSize, length, uniformBlockName);
    GLState.Unlock();
}

// ============================================================================
// Frag Data Location (CPU simulation + GLES extension)
// ============================================================================

// CPU-side frag data location storage for glBindFragDataLocation / glGetFragDataLocation
static UnorderedMap<GLuint, UnorderedMap<std::string, GLint>> g_fragDataLocations;

// --- glBindFragDataLocation ---

static void BindFragDataLocation_Backend(GLuint program, GLuint color, const GLchar* name) {
    // Use GLES extension if available, otherwise store in CPU-side map
    if (name != nullptr) {
        g_fragDataLocations[program][std::string(name)] = static_cast<GLint>(color);
    }
    // Try GLES extension as well
    GLES.glBindFragDataLocationEXT(program, color, name);
}

static void BindFragDataLocation_State(GLuint program, GLuint color, const GLchar* name) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindFragDataLocation: program is 0"));
        return;
    }
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindFragDataLocation: name is NULL"));
        return;
    }
    BindFragDataLocation_Backend(program, color, name);
}

void glBindFragDataLocation(GLuint program, GLuint color, const GLchar* name) {
    GLState.Lock();
    BindFragDataLocation_State(program, color, name);
    GLState.Unlock();
}

// --- glGetFragDataLocation ---

static GLint GetFragDataLocation_Backend(GLuint program, const GLchar* name) {
    // Try GLES native first
    GLint result = GLES.glGetFragDataLocation(program, name);
    if (result < 0) {
        // Fallback to CPU-side map
        if (name != nullptr) {
            auto progIt = g_fragDataLocations.find(program);
            if (progIt != g_fragDataLocations.end()) {
                auto nameIt = progIt->second.find(std::string(name));
                if (nameIt != progIt->second.end()) {
                    return nameIt->second;
                }
            }
        }
    }
    return result;
}

static GLint GetFragDataLocation_State(GLuint program, const GLchar* name) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetFragDataLocation: program is 0"));
        return -1;
    }
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetFragDataLocation: name is NULL"));
        return -1;
    }
    return GetFragDataLocation_Backend(program, name);
}

GLint glGetFragDataLocation(GLuint program, const GLchar* name) {
    GLState.Lock();
    GLint result = GetFragDataLocation_State(program, name);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Program Interface Queries (ES 3.1+ native)
// ============================================================================

// --- glGetProgramInterfaceiv ---

static void GetProgramInterfaceiv_Backend(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    GLES.glGetProgramInterfaceiv(program, programInterface, pname, params);
}

static void GetProgramInterfaceiv_State(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    if (params == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramInterfaceiv: params is NULL"));
        return;
    }
    GetProgramInterfaceiv_Backend(program, programInterface, pname, params);
}

void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    GLState.Lock();
    GetProgramInterfaceiv_State(program, programInterface, pname, params);
    GLState.Unlock();
}

// --- glGetProgramResourceIndex ---

static GLuint GetProgramResourceIndex_Backend(GLuint program, GLenum programInterface, const GLchar* name) {
    return GLES.glGetProgramResourceIndex(program, programInterface, name);
}

static GLuint GetProgramResourceIndex_State(GLuint program, GLenum programInterface, const GLchar* name) {
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceIndex: name is NULL"));
        return GL_INVALID_INDEX;
    }
    return GetProgramResourceIndex_Backend(program, programInterface, name);
}

GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar* name) {
    GLState.Lock();
    GLuint result = GetProgramResourceIndex_State(program, programInterface, name);
    GLState.Unlock();
    return result;
}

// --- glGetProgramResourceName ---

static void GetProgramResourceName_Backend(GLuint program, GLenum programInterface, GLuint index,
                                           GLsizei bufSize, GLsizei* length, GLchar* name) {
    GLES.glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}

static void GetProgramResourceName_State(GLuint program, GLenum programInterface, GLuint index,
                                         GLsizei bufSize, GLsizei* length, GLchar* name) {
    if (bufSize < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceName: bufSize < 0"));
        return;
    }
    GetProgramResourceName_Backend(program, programInterface, index, bufSize, length, name);
}

void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index,
                              GLsizei bufSize, GLsizei* length, GLchar* name) {
    GLState.Lock();
    GetProgramResourceName_State(program, programInterface, index, bufSize, length, name);
    GLState.Unlock();
}

// --- glGetProgramResourceiv ---

static void GetProgramResourceiv_Backend(GLuint program, GLenum programInterface, GLuint index,
                                         GLsizei propCount, const GLenum* props, GLsizei bufSize,
                                         GLsizei* length, GLint* params) {
    GLES.glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
}

static void GetProgramResourceiv_State(GLuint program, GLenum programInterface, GLuint index,
                                       GLsizei propCount, const GLenum* props, GLsizei bufSize,
                                       GLsizei* length, GLint* params) {
    if (propCount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceiv: propCount < 0"));
        return;
    }
    if (props == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceiv: props is NULL"));
        return;
    }
    GetProgramResourceiv_Backend(program, programInterface, index, propCount, props, bufSize, length, params);
}

void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index,
                            GLsizei propCount, const GLenum* props, GLsizei bufSize,
                            GLsizei* length, GLint* params) {
    GLState.Lock();
    GetProgramResourceiv_State(program, programInterface, index, propCount, props, bufSize, length, params);
    GLState.Unlock();
}

// --- glGetProgramResourceLocation ---

static GLint GetProgramResourceLocation_Backend(GLuint program, GLenum programInterface, const GLchar* name) {
    return GLES.glGetProgramResourceLocation(program, programInterface, name);
}

static GLint GetProgramResourceLocation_State(GLuint program, GLenum programInterface, const GLchar* name) {
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceLocation: name is NULL"));
        return -1;
    }
    return GetProgramResourceLocation_Backend(program, programInterface, name);
}

GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar* name) {
    GLState.Lock();
    GLint result = GetProgramResourceLocation_State(program, programInterface, name);
    GLState.Unlock();
    return result;
}

// --- glGetProgramResourceLocationIndex ---

// Not available in GLES 3.2; return -1 with appropriate error
static GLint GetProgramResourceLocationIndex_Backend(GLuint program, GLenum programInterface, const GLchar* name) {
    (void)program;
    (void)programInterface;
    (void)name;
    return -1;
}

static GLint GetProgramResourceLocationIndex_State(GLuint program, GLenum programInterface, const GLchar* name) {
    if (name == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetProgramResourceLocationIndex: name is NULL"));
        return -1;
    }
    return GetProgramResourceLocationIndex_Backend(program, programInterface, name);
}

GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar* name) {
    GLState.Lock();
    GLint result = GetProgramResourceLocationIndex_State(program, programInterface, name);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Shader Storage Block Binding (CPU simulation - not in GLES 3.2)
// ============================================================================

// CPU-side shader storage block binding map
static UnorderedMap<GLuint, UnorderedMap<GLuint, GLuint>> g_shaderStorageBlockBindings;

// --- glShaderStorageBlockBinding ---

static void ShaderStorageBlockBinding_Backend(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    g_shaderStorageBlockBindings[program][storageBlockIndex] = storageBlockBinding;
}

static void ShaderStorageBlockBinding_State(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    if (program == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glShaderStorageBlockBinding: program is 0"));
        return;
    }
    ShaderStorageBlockBinding_Backend(program, storageBlockIndex, storageBlockBinding);
}

void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    GLState.Lock();
    ShaderStorageBlockBinding_State(program, storageBlockIndex, storageBlockBinding);
    GLState.Unlock();
}

// ============================================================================
// Validation Queries (ES 3.2 native)
// ============================================================================

// --- glIsProgram ---

static GLboolean IsProgram_Backend(GLuint program) {
    return GLES.glIsProgram(program);
}

static GLboolean IsProgram_State(GLuint program) {
    return IsProgram_Backend(program);
}

GLboolean glIsProgram(GLuint program) {
    GLState.Lock();
    GLboolean result = IsProgram_State(program);
    GLState.Unlock();
    return result;
}

// --- glIsShader ---

static GLboolean IsShader_Backend(GLuint shader) {
    return GLES.glIsShader(shader);
}

static GLboolean IsShader_State(GLuint shader) {
    return IsShader_Backend(shader);
}

GLboolean glIsShader(GLuint shader) {
    GLState.Lock();
    GLboolean result = IsShader_State(shader);
    GLState.Unlock();
    return result;
}