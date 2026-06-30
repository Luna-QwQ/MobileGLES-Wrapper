// MobileGlues - gl/buffer.cpp
// Buffer state management: ID mapping, VAO mapping, binding tracking,
// atomic counter buffer emulation, and texture buffer emulation.
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
// All buffer state is tracked CPU-side via GLStateManager to avoid GPU round-trips.
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

#define DEBUG 0

// ============================================================================
// Buffer ID generation and management
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGenBuffers, GLsizei n, GLuint *buffers)
{
    GLuint *real = (GLuint *)alloca(sizeof(GLuint) * n);
    _native(n, real);

    auto &bs = GLState.buffer;
    for (GLsizei i = 0; i < n; i++) {
        bs.bufferMap[buffers[i]] = real[i];
        bs.bufferMapReverse[real[i]] = buffers[i];
        BufferObject info;
        info.id = real[i];
        bs.bufferInfo[buffers[i]] = info;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGenBuffers, n, buffers)

NATIVE_FUNCTION_HEAD(void, glDeleteBuffers, GLsizei n, const GLuint *buffers)
{
    GLuint *real = (GLuint *)alloca(sizeof(GLuint) * n);
    for (GLsizei i = 0; i < n; i++) {
        real[i] = GLState.GetRealBuffer(buffers[i]);
    }

    auto &bs = GLState.buffer;
    for (GLsizei i = 0; i < n; i++) {
        GLuint vid = buffers[i];
        if (vid == 0) continue;

        // Clean up texture buffer emulation
        auto tbIt = bs.texBuffers.find(vid);
        if (tbIt != bs.texBuffers.end()) {
            if (tbIt->second.texture) {
                glDeleteTextures(1, &tbIt->second.texture);
            }
            bs.texBuffers.erase(tbIt);
        }

        // Clean up buffer maps
        bs.bufferMaps.erase(vid);

        // Clean up buffer info
        bs.bufferInfo.erase(vid);

        // Remove from binding tracking
        for (auto &pair : bs.boundBuffer) {
            if (pair.second == vid) pair.second = 0;
        }

        // Remove from uniform buffer bases
        for (int j = 0; j < MAX_UNIFORM_BUFFER_BINDINGS; j++) {
            if (bs.uniformBufferBases[j] == vid) {
                bs.uniformBufferBases[j] = 0;
                bs.uniformBufferOffsets[j] = 0;
                bs.uniformBufferSizes[j] = 0;
            }
        }

        // Remove from shader storage bases
        for (int j = 0; j < MAX_SHADER_STORAGE_BUFFER_BINDINGS; j++) {
            if (bs.shaderStorageBases[j] == vid) bs.shaderStorageBases[j] = 0;
        }

        // Remove from atomic counter bases
        for (int j = 0; j < MAX_ATOMIC_COUNTER_BUFFER_BINDINGS; j++) {
            if (bs.atomicCounterBases[j] == vid) bs.atomicCounterBases[j] = 0;
        }

        // Remove from transform feedback bindings
        for (int j = 0; j < MAX_TRANSFORM_FEEDBACK_BUFFERS; j++) {
            if (bs.transformFeedbackBuffers[j] == vid) bs.transformFeedbackBuffers[j] = 0;
        }

        // Remove from ID mappings
        bs.bufferMap.erase(vid);
        if (real[i]) bs.bufferMapReverse.erase(real[i]);
    }

    _native(n, real);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteBuffers, n, buffers)

NATIVE_FUNCTION_HEAD(GLboolean, glIsBuffer, GLuint buffer)
{
    if (buffer == 0) return GL_FALSE;
    return GLState.GetRealBuffer(buffer) != 0 ? GL_TRUE : GL_FALSE;
}
NATIVE_FUNCTION_END(GLboolean, glIsBuffer, buffer)

// ============================================================================
// Buffer binding and data management
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindBuffer, GLenum target, GLuint buffer)
{
    GLuint realId = GLState.GetRealBuffer(buffer);
    _native(target, realId);

    // Track binding
    GLState.buffer.boundBuffer[target] = buffer;
    STATE_LOG("glBindBuffer: target=0x%X, virtual=%u, real=%u", target, buffer, realId);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindBuffer, target, buffer)

NATIVE_FUNCTION_HEAD(void, glBufferData, GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    _native(target, size, data, usage);

    // Update buffer info
    if (boundVirt) {
        auto it = GLState.buffer.bufferInfo.find(boundVirt);
        if (it != GLState.buffer.bufferInfo.end()) {
            it->second.size = size;
            it->second.target = target;
        }
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBufferData, target, size, data, usage)

NATIVE_FUNCTION_HEAD(void, glBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    _native(target, offset, size, data);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBufferSubData, target, offset, size, data)

NATIVE_FUNCTION_HEAD(void, glCopyBufferSubData, GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    GLuint readVirt = GLState.buffer.boundBuffer[readTarget];
    GLuint writeVirt = GLState.buffer.boundBuffer[writeTarget];
    GLuint readReal = GLState.GetRealBuffer(readVirt);
    GLuint writeReal = GLState.GetRealBuffer(writeVirt);
    _native(GL_COPY_READ_BUFFER, readOffset, GL_COPY_WRITE_BUFFER, writeOffset, size);

    // If copying involves atomic counter buffer, update CPU-side data
    if (GLState.buffer.atomicCounterBufferBinding == readVirt) {
        // Read-back would be needed but typically not used this way
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glCopyBufferSubData, readTarget, writeTarget, readOffset, writeOffset, size)

// ============================================================================
// Buffer mapping
// ============================================================================

NATIVE_FUNCTION_HEAD(void*, glMapBufferRange, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    void *ptr = _native(target, offset, length, access);

    if (ptr && boundVirt) {
        GLState.buffer.bufferMaps[boundVirt] = ptr;
        auto it = GLState.buffer.bufferInfo.find(boundVirt);
        if (it != GLState.buffer.bufferInfo.end()) {
            it->second.mappedPtr = ptr;
            it->second.access = access;
            it->second.isMapped = true;
        }
    }

    // If this is the atomic counter buffer, sync CPU data
    if (boundVirt == GLState.buffer.atomicCounterBufferBinding && ptr) {
        // Keep the mapped pointer for atomic counter emulation
    }

    return ptr;
}
NATIVE_FUNCTION_END(void*, glMapBufferRange, target, offset, length, access)

NATIVE_FUNCTION_HEAD(GLboolean, glUnmapBuffer, GLenum target)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    GLboolean result = _native(target);

    if (boundVirt) {
        // If this was the atomic counter buffer and we have CPU-side data,
        // sync it back to the GPU
        if (boundVirt == GLState.buffer.atomicCounterBufferBinding &&
            !GLState.buffer.atomicCounterData.empty()) {
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realId);
            glBufferSubData(GL_ATOMIC_COUNTER_BUFFER,
                           GLState.buffer.atomicCounterBufferOffset,
                           GLState.buffer.atomicCounterBufferSize,
                           GLState.buffer.atomicCounterData.data());
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realId);
        }

        GLState.buffer.bufferMaps.erase(boundVirt);
        auto it = GLState.buffer.bufferInfo.find(boundVirt);
        if (it != GLState.buffer.bufferInfo.end()) {
            it->second.mappedPtr = nullptr;
            it->second.isMapped = false;
        }
    }

    return result;
}
NATIVE_FUNCTION_END(GLboolean, glUnmapBuffer, target)

NATIVE_FUNCTION_HEAD(void, glFlushMappedBufferRange, GLenum target, GLintptr offset, GLsizeiptr length)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    _native(target, offset, length);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glFlushMappedBufferRange, target, offset, length)

// ============================================================================
// Buffer parameter queries
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGetBufferParameteriv, GLenum target, GLenum pname, GLint *params)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);

    _native(target, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetBufferParameteriv, target, pname, params)

NATIVE_FUNCTION_HEAD(void, glGetBufferParameteri64v, GLenum target, GLenum pname, GLint64 *params)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    _native(target, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetBufferParameteri64v, target, pname, params)

NATIVE_FUNCTION_HEAD(void, glGetBufferPointerv, GLenum target, GLenum pname, void **params)
{
    GLuint boundVirt = GLState.buffer.boundBuffer[target];
    GLuint realId = GLState.GetRealBuffer(boundVirt);
    _native(target, pname, params);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGetBufferPointerv, target, pname, params)

// ============================================================================
// Uniform buffer binding
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindBufferRange, GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    GLuint realId = GLState.GetRealBuffer(buffer);
    _native(target, index, realId, offset, size);

    auto &bs = GLState.buffer;
    if (target == GL_UNIFORM_BUFFER) {
        bs.uniformBufferBases[index] = buffer;
        bs.uniformBufferOffsets[index] = offset;
        bs.uniformBufferSizes[index] = size;
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        bs.shaderStorageBases[index] = buffer;
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        bs.atomicCounterBases[index] = buffer;
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        if (index < MAX_TRANSFORM_FEEDBACK_BUFFERS) {
            bs.transformFeedbackBuffers[index] = buffer;
        }
    }

    STATE_LOG("glBindBufferRange: target=0x%X, index=%u, virtual=%u, real=%u",
              target, index, buffer, realId);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindBufferRange, target, index, buffer, offset, size)

NATIVE_FUNCTION_HEAD(void, glBindBufferBase, GLenum target, GLuint index, GLuint buffer)
{
    GLuint realId = GLState.GetRealBuffer(buffer);
    _native(target, index, realId);

    auto &bs = GLState.buffer;
    if (target == GL_UNIFORM_BUFFER) {
        bs.uniformBufferBases[index] = buffer;
        bs.uniformBufferOffsets[index] = 0;
        bs.uniformBufferSizes[index] = 0;
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        bs.shaderStorageBases[index] = buffer;
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        bs.atomicCounterBases[index] = buffer;
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        if (index < MAX_TRANSFORM_FEEDBACK_BUFFERS) {
            bs.transformFeedbackBuffers[index] = buffer;
        }
    }

    STATE_LOG("glBindBufferBase: target=0x%X, index=%u, virtual=%u, real=%u",
              target, index, buffer, realId);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindBufferBase, target, index, buffer)

// ============================================================================
// VAO management
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glGenVertexArrays, GLsizei n, GLuint *arrays)
{
    GLuint *real = (GLuint *)alloca(sizeof(GLuint) * n);
    _native(n, real);

    auto &bs = GLState.buffer;
    for (GLsizei i = 0; i < n; i++) {
        bs.vaoMap[arrays[i]] = real[i];
        bs.vaoMapReverse[real[i]] = arrays[i];
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glGenVertexArrays, n, arrays)

NATIVE_FUNCTION_HEAD(void, glDeleteVertexArrays, GLsizei n, const GLuint *arrays)
{
    GLuint *real = (GLuint *)alloca(sizeof(GLuint) * n);
    for (GLsizei i = 0; i < n; i++) {
        real[i] = GLState.GetRealVAO(arrays[i]);
    }
    _native(n, real);

    auto &bs = GLState.buffer;
    for (GLsizei i = 0; i < n; i++) {
        if (GLState.vertexArray.currentVAO == arrays[i]) {
            GLState.vertexArray.currentVAO = 0;
        }
        bs.vaoMap.erase(arrays[i]);
        if (real[i]) bs.vaoMapReverse.erase(real[i]);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDeleteVertexArrays, n, arrays)

NATIVE_FUNCTION_HEAD(void, glBindVertexArray, GLuint array)
{
    GLuint realId = GLState.GetRealVAO(array);
    _native(realId);
    GLState.vertexArray.currentVAO = array;
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindVertexArray, array)

NATIVE_FUNCTION_HEAD(GLboolean, glIsVertexArray, GLuint array)
{
    if (array == 0) return GL_FALSE;
    return GLState.GetRealVAO(array) != 0 ? GL_TRUE : GL_FALSE;
}
NATIVE_FUNCTION_END(GLboolean, glIsVertexArray, array)

// ============================================================================
// Texture buffer emulation
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glTexBuffer, GLenum target, GLenum internalformat, GLuint buffer)
{
    GLuint realBuffer = GLState.GetRealBuffer(buffer);

    if (GLState.emulateTextureBuffer) {
        // Emulate texture buffer using a regular 2D texture + sampler
        auto &bs = GLState.buffer;
        auto &slot = bs.texBuffers[buffer];
        slot.buffer = realBuffer;
        slot.internalFormat = internalformat;

        // Create or reuse the emulation texture
        if (!slot.texture) {
            glGenTextures(1, &slot.texture);
        }

        GLint prevTexUnit;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &prevTexUnit);
        glActiveTexture(GL_TEXTURE0 + GLState.currentTexUnit);
        glBindTexture(GL_TEXTURE_2D, slot.texture);

        // Get buffer size to determine texture dimensions
        GLint bufSize = 0;
        GLint prevBinding = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &prevBinding);
        glBindBuffer(GL_TEXTURE_BUFFER, realBuffer);
        glGetBufferParameteriv(GL_TEXTURE_BUFFER, GL_BUFFER_SIZE, &bufSize);
        glBindBuffer(GL_TEXTURE_BUFFER, prevBinding);

        GLsizei texWidth = bufSize / 16; // 4 floats per pixel (RGBA32F)
        if (texWidth < 1) texWidth = 1;
        if (texWidth > 4096) texWidth = 4096;

        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, texWidth, 1, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(prevTexUnit);
    } else {
        _native(GL_TEXTURE_BUFFER, internalformat, realBuffer);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexBuffer, target, internalformat, buffer)

NATIVE_FUNCTION_HEAD(void, glTexBufferRange, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    GLuint realBuffer = GLState.GetRealBuffer(buffer);

    if (GLState.emulateTextureBuffer) {
        auto &bs = GLState.buffer;
        auto &slot = bs.texBuffers[buffer];
        slot.buffer = realBuffer;
        slot.internalFormat = internalformat;

        if (!slot.texture) {
            glGenTextures(1, &slot.texture);
        }

        GLint prevTexUnit;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &prevTexUnit);
        glActiveTexture(GL_TEXTURE0 + GLState.currentTexUnit);
        glBindTexture(GL_TEXTURE_2D, slot.texture);

        GLsizei texWidth = (GLsizei)(size / 16);
        if (texWidth < 1) texWidth = 1;
        if (texWidth > 4096) texWidth = 4096;

        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, texWidth, 1, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(prevTexUnit);
    } else {
        _native(GL_TEXTURE_BUFFER, internalformat, realBuffer, offset, size);
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glTexBufferRange, target, internalformat, buffer, offset, size)

// ============================================================================
// Vertex buffer binding (for DSA-style buffer binding)
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindVertexBuffer, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
{
    GLuint realId = GLState.GetRealBuffer(buffer);
    _native(bindingindex, realId, offset, stride);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindVertexBuffer, bindingindex, buffer, offset, stride)

// ============================================================================
// Map initialization helpers (for backward compatibility)
// ============================================================================

void InitBufferMap(size_t expectedSize) {
    auto &bs = GLState.buffer;
    bs.bufferMap.reserve(expectedSize);
    bs.bufferMapReverse.reserve(expectedSize);
    bs.bufferInfo.reserve(expectedSize);
}

void InitVertexArrayMap(size_t expectedSize) {
    auto &bs = GLState.buffer;
    bs.vaoMap.reserve(expectedSize);
    bs.vaoMapReverse.reserve(expectedSize);
}