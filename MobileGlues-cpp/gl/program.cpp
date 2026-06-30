// MobileGlues - gl/program.cpp
// Program state management: ID mapping, linking, usage tracking.
// Shader compilation and source management is handled by shader.cpp.
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
#include <string>
#include <vector>
#include <cstring>

#define DEBUG 0

// ============================================================================
// Program ID management
// ============================================================================

NATIVE_FUNCTION_HEAD(GLuint, glCreateProgram)
{
    GLuint realId = _native();

    auto &ss = GLState.shader;
    ss.programMap[realId] = realId;
    ss.programMapReverse[realId] = realId;

    ss.programInfo[realId] = GLStateManager::ShaderState::ProgramInfo{};

    return realId;
}
NATIVE_FUNCTION_END(GLuint, glCreateProgram)

NATIVE_FUNCTION_HEAD(void, glDeleteProgram, GLuint program)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId);

    auto &ss = GLState.shader;
    if (ss.currentProgram == program) {
        ss.currentProgram = 0;
        GLState.currentProgram = 0;
    }
    ss.programMap.erase(program);
    ss.programMapReverse.erase(realId);
    ss.programInfo.erase(program);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteProgram, program)

// ============================================================================
// Program linking and usage
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glAttachShader, GLuint program, GLuint shader)
{
    GLuint realProg = GLState.GetRealProgram(program);
    GLuint realShader = GLState.GetRealShader(shader);
    _native(realProg, realShader);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.attachedShaders.push_back(shader);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glAttachShader, program, shader)

NATIVE_FUNCTION_HEAD(void, glDetachShader, GLuint program, GLuint shader)
{
    GLuint realProg = GLState.GetRealProgram(program);
    GLuint realShader = GLState.GetRealShader(shader);
    _native(realProg, realShader);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        auto &vec = it->second.attachedShaders;
        vec.erase(std::remove(vec.begin(), vec.end(), shader), vec.end());
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDetachShader, program, shader)

NATIVE_FUNCTION_HEAD(void, glLinkProgram, GLuint program)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId);

    GLint linked = 0;
    glGetProgramiv(realId, GL_LINK_STATUS, &linked);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.linked = (linked == GL_TRUE);
    }

    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(realId, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 1) {
            std::vector<char> logBuf(logLen);
            glGetProgramInfoLog(realId, logLen, nullptr, logBuf.data());
            MG_LOG_ERROR("Program %u link error: %s", program, logBuf.data());
        }
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glLinkProgram, program)

NATIVE_FUNCTION_HEAD(void, glUseProgram, GLuint program)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId);

    GLState.shader.currentProgram = program;
    GLState.currentProgram = program;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glUseProgram, program)

NATIVE_FUNCTION_HEAD(void, glBindAttribLocation, GLuint program, GLuint index, const GLchar *name)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, index, name);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindAttribLocation, program, index, name)

NATIVE_FUNCTION_HEAD(void, glValidateProgram, GLuint program)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId);

    GLint validated = 0;
    glGetProgramiv(realId, GL_VALIDATE_STATUS, &validated);

    auto &ss = GLState.shader;
    auto it = ss.programInfo.find(program);
    if (it != ss.programInfo.end()) {
        it->second.validated = (validated == GL_TRUE);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glValidateProgram, program)

// ============================================================================
// Program queries
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetProgramiv, GLuint program, GLenum pname, GLint *params)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetProgramiv, program, pname, params)

NATIVE_FUNCTION_HEAD(void, glGetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, bufSize, length, infoLog);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetProgramInfoLog, program, bufSize, length, infoLog)

NATIVE_FUNCTION_HEAD(void, glGetAttachedShaders, GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, maxCount, count, shaders);

    // Convert real shader IDs back to virtual IDs
    for (GLsizei i = 0; i < (count ? *count : 0); i++) {
        shaders[i] = GLState.GetVirtualShader(shaders[i]);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetAttachedShaders, program, maxCount, count, shaders)

NATIVE_FUNCTION_HEAD(GLint, glGetAttribLocation, GLuint program, const GLchar *name)
{
    GLuint realId = GLState.GetRealProgram(program);
    return _native(realId, name);
}
NATIVE_FUNCTION_END(GLint, glGetAttribLocation, program, name)

NATIVE_FUNCTION_HEAD(GLint, glGetUniformLocation, GLuint program, const GLchar *name)
{
    GLuint realId = GLState.GetRealProgram(program);
    return _native(realId, name);
}
NATIVE_FUNCTION_END(GLint, glGetUniformLocation, program, name)

NATIVE_FUNCTION_HEAD(void, glGetActiveAttrib, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, index, bufSize, length, size, type, name);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetActiveAttrib, program, index, bufSize, length, size, type, name)

NATIVE_FUNCTION_HEAD(void, glGetActiveUniform, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    GLuint realId = GLState.GetRealProgram(program);
    _native(realId, index, bufSize, length, size, type, name);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetActiveUniform, program, index, bufSize, length, size, type, name)

// ============================================================================
// Helper: translate real shader ID back to virtual
// ============================================================================

// Defined in state.h

// ============================================================================
// glGetString - return custom renderer string
// ============================================================================

NATIVE_FUNCTION_HEAD(const GLubyte*, glGetString, GLenum name)
{
    const GLubyte *result = _native(name);
    if (name == GL_RENDERER) {
        return (const GLubyte *)"MobileGlues";
    }
    if (name == GL_VENDOR) {
        return (const GLubyte *)"MobileGL-Dev";
    }
    return result;
}
NATIVE_FUNCTION_END(const GLubyte*, glGetString, name)