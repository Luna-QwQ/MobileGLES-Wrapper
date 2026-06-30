// MobileGlues - gl/drawing.cpp
// Draw call preparation: sampler buffer emulation, atomic counter handling,
// and draw call dispatch.
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
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

#define DEBUG 0

// ============================================================================
// Atomic counter buffer emulation
// ============================================================================

// Update CPU-side atomic counter data before a draw call
static void syncAtomicCounters()
{
    auto &bs = GLState.buffer;
    if (bs.atomicCounterBufferBinding == 0) return;
    if (bs.atomicCounterData.empty()) return;

    // Upload CPU-side atomic counter data to the GPU buffer
    GLuint realBuf = GLState.GetRealBuffer(bs.atomicCounterBufferBinding);
    if (realBuf) {
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realBuf);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER,
                       bs.atomicCounterBufferOffset,
                       bs.atomicCounterBufferSize,
                       bs.atomicCounterData.data());
    }
}

// Read back atomic counter data after a draw call (for compute shaders)
static void readbackAtomicCounters()
{
    auto &bs = GLState.buffer;
    if (bs.atomicCounterBufferBinding == 0) return;

    GLuint realBuf = GLState.GetRealBuffer(bs.atomicCounterBufferBinding);
    if (realBuf) {
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realBuf);
        void *ptr = glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                     bs.atomicCounterBufferOffset,
                                     bs.atomicCounterBufferSize,
                                     GL_MAP_READ_BIT);
        if (ptr) {
            memcpy(bs.atomicCounterData.data(), ptr, (size_t)bs.atomicCounterBufferSize);
            glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        }
    }
}

// ============================================================================
// Draw call dispatch
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glDrawArrays, GLenum mode, GLint first, GLsizei count)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, first, count);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawArrays, mode, first, count)

NATIVE_FUNCTION_HEAD(void, glDrawElements, GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, count, type, indices);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawElements, mode, count, type, indices)

NATIVE_FUNCTION_HEAD(void, glDrawArraysInstanced, GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, first, count, instancecount);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawArraysInstanced, mode, first, count, instancecount)

NATIVE_FUNCTION_HEAD(void, glDrawElementsInstanced, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, count, type, indices, instancecount);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawElementsInstanced, mode, count, type, indices, instancecount)

NATIVE_FUNCTION_HEAD(void, glDrawRangeElements, GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, start, end, count, type, indices);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawRangeElements, mode, start, end, count, type, indices)

NATIVE_FUNCTION_HEAD(void, glDrawElementsBaseVertex, GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, count, type, indices, basevertex);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawElementsBaseVertex, mode, count, type, indices, basevertex)

NATIVE_FUNCTION_HEAD(void, glDrawArraysIndirect, GLenum mode, const void *indirect)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, indirect);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawArraysIndirect, mode, indirect)

NATIVE_FUNCTION_HEAD(void, glDrawElementsIndirect, GLenum mode, GLenum type, const void *indirect)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(mode, type, indirect);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawElementsIndirect, mode, type, indirect)

// ============================================================================
// Compute shader dispatch
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glDispatchCompute, GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
{
    PREPARE_FOR_DRAW();
    syncAtomicCounters();
    _native(num_groups_x, num_groups_y, num_groups_z);
    readbackAtomicCounters();
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDispatchCompute, num_groups_x, num_groups_y, num_groups_z)

// ============================================================================
// glDrawBuffers - track draw buffer state
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glDrawBuffers, GLsizei n, const GLenum *bufs)
{
    _native(n, bufs);

    auto &fb = GLState.framebuffer;
    fb.drawBufferCount = n;
    for (GLsizei i = 0; i < n && i < MAX_DRAW_BUFFERS; i++) {
        fb.drawBuffers[i] = bufs[i];
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glDrawBuffers, n, bufs)

// ============================================================================
// glBindImageTexture - track image unit state
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glBindImageTexture, GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
    GLuint realTex = GLState.GetRealTexture(texture);
    _native(unit, realTex, level, layered, layer, access, format);

    if (unit < MAX_IMAGE_UNITS) {
        auto &img = GLState.image.imageUnits[unit];
        img.texture = texture;
        img.level = level;
        img.layered = layered;
        img.layer = layer;
        img.access = access;
        img.format = format;
    }
}
NATIVE_FUNCTION_END_NO_RETURN(void, glBindImageTexture, unit, texture, level, layered, layer, access, format)

// ============================================================================
// glMemoryBarrier - handle atomic counter buffer sync
// ============================================================================

NATIVE_FUNCTION_HEAD(void, glMemoryBarrier, GLbitfield barriers)
{
    // Before barrier, sync atomic counter data if needed
    if (barriers & GL_ATOMIC_COUNTER_BARRIER_BIT) {
        syncAtomicCounters();
    }
    _native(barriers);
}
NATIVE_FUNCTION_END_NO_RETURN(void, glMemoryBarrier, barriers)