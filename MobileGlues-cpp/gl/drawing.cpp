// MobileGlues - gl/drawing.cpp
// Draw call dispatch, render state management, and state validation.
// Follows the DirectGLES architecture pattern:
//   Public function → _State (validate + track) → _Backend (actual GLES call)
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

// ---------------------------------------------------------------------------
// Module-level state: tracked 2D texture bindings per unit
// (referenced from texture.cpp, drawing.cpp, and FSR1/FSR1.cpp)
// ---------------------------------------------------------------------------

GLuint g_tracked_tex2d_binding[32] = {0};

// ============================================================================
// Internal helper: GLenum → strongly-typed enum conversions
// ============================================================================

static Capability GLEnumToCapability(GLenum cap) {
    switch (cap) {
    case GL_BLEND:                      return Capability::Blend;
    case GL_CULL_FACE:                  return Capability::CullFace;
    case GL_DEPTH_TEST:                 return Capability::DepthTest;
    case GL_DITHER:                     return Capability::Dither;
    case GL_POLYGON_OFFSET_FILL:        return Capability::PolygonOffsetFill;
    case GL_POLYGON_OFFSET_LINE:        return Capability::PolygonOffsetLine;
    case GL_POLYGON_OFFSET_POINT:       return Capability::PolygonOffsetPoint;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:   return Capability::SampleAlphaToCoverage;
    case GL_SAMPLE_ALPHA_TO_ONE:        return Capability::SampleAlphaToOne;
    case GL_SAMPLE_COVERAGE:            return Capability::SampleCoverage;
    case GL_SAMPLE_MASK:                return Capability::SampleMask;
    case GL_SCISSOR_TEST:               return Capability::ScissorTest;
    case GL_STENCIL_TEST:               return Capability::StencilTest;
    case GL_PRIMITIVE_RESTART:          return Capability::PrimitiveRestart;
    case GL_PRIMITIVE_RESTART_FIXED_INDEX: return Capability::PrimitiveRestartFixedIndex;
    case GL_RASTERIZER_DISCARD:         return Capability::RasterizerDiscard;
    case GL_FRAMEBUFFER_SRGB:           return Capability::FramebufferSrgb;
    case GL_PROGRAM_POINT_SIZE:         return Capability::ProgramPointSize;
    case GL_COLOR_LOGIC_OP:             return Capability::ColorLogicOp;
    case GL_DEBUG_OUTPUT:               return Capability::DebugOutput;
    case GL_DEBUG_OUTPUT_SYNCHRONOUS:   return Capability::DebugOutputSynchronous;
    case GL_DEPTH_CLAMP:                return Capability::DepthClamp;
    case GL_LINE_SMOOTH:                return Capability::LineSmooth;
    case GL_MULTISAMPLE:                return Capability::Multisample;
    case GL_POLYGON_SMOOTH:             return Capability::PolygonSmooth;
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:  return Capability::TextureCubeMapSeamless;
    default:                            return Capability::Unknown;
    }
}

static BlendFactor GLEnumToBlendFactor(GLenum factor) {
    switch (factor) {
    case GL_ZERO:                     return BlendFactor::Zero;
    case GL_ONE:                      return BlendFactor::One;
    case GL_SRC_COLOR:                return BlendFactor::SrcColor;
    case GL_ONE_MINUS_SRC_COLOR:      return BlendFactor::OneMinusSrcColor;
    case GL_DST_COLOR:                return BlendFactor::DstColor;
    case GL_ONE_MINUS_DST_COLOR:      return BlendFactor::OneMinusDstColor;
    case GL_SRC_ALPHA:                return BlendFactor::SrcAlpha;
    case GL_ONE_MINUS_SRC_ALPHA:      return BlendFactor::OneMinusSrcAlpha;
    case GL_DST_ALPHA:                return BlendFactor::DstAlpha;
    case GL_ONE_MINUS_DST_ALPHA:      return BlendFactor::OneMinusDstAlpha;
    case GL_CONSTANT_COLOR:           return BlendFactor::ConstantColor;
    case GL_ONE_MINUS_CONSTANT_COLOR: return BlendFactor::OneMinusConstantColor;
    case GL_CONSTANT_ALPHA:           return BlendFactor::ConstantAlpha;
    case GL_ONE_MINUS_CONSTANT_ALPHA: return BlendFactor::OneMinusConstantAlpha;
    default:                          return BlendFactor::Unknown;
    }
}

static BlendEquation GLEnumToBlendEquation(GLenum eq) {
    switch (eq) {
    case GL_FUNC_ADD:              return BlendEquation::Add;
    case GL_FUNC_SUBTRACT:         return BlendEquation::Subtract;
    case GL_FUNC_REVERSE_SUBTRACT: return BlendEquation::ReverseSubtract;
    case GL_MIN:                   return BlendEquation::Min;
    case GL_MAX:                   return BlendEquation::Max;
    default:                       return BlendEquation::Unknown;
    }
}

static DepthTestFunc GLEnumToDepthFunc(GLenum func) {
    switch (func) {
    case GL_NEVER:    return DepthTestFunc::Never;
    case GL_LESS:     return DepthTestFunc::Less;
    case GL_EQUAL:    return DepthTestFunc::Equal;
    case GL_LEQUAL:   return DepthTestFunc::LessEqual;
    case GL_GREATER:  return DepthTestFunc::Greater;
    case GL_NOTEQUAL: return DepthTestFunc::NotEqual;
    case GL_GEQUAL:   return DepthTestFunc::GreaterEqual;
    case GL_ALWAYS:   return DepthTestFunc::Always;
    default:          return DepthTestFunc::Unknown;
    }
}

static StencilOperation GLEnumToStencilOp(GLenum op) {
    switch (op) {
    case GL_KEEP:      return StencilOperation::Keep;
    case GL_ZERO:      return StencilOperation::Zero;
    case GL_REPLACE:   return StencilOperation::Replace;
    case GL_INCR:      return StencilOperation::IncrementClamp;
    case GL_DECR:      return StencilOperation::DecrementClamp;
    case GL_INVERT:    return StencilOperation::Invert;
    case GL_INCR_WRAP: return StencilOperation::IncrementWrap;
    case GL_DECR_WRAP: return StencilOperation::DecrementWrap;
    default:           return StencilOperation::Unknown;
    }
}

static CullFaceMode GLEnumToCullFace(GLenum mode) {
    switch (mode) {
    case GL_FRONT:           return CullFaceMode::Front;
    case GL_BACK:            return CullFaceMode::Back;
    case GL_FRONT_AND_BACK:  return CullFaceMode::FrontAndBack;
    default:                 return CullFaceMode::Unknown;
    }
}

static FrontFaceMode GLEnumToFrontFace(GLenum mode) {
    switch (mode) {
    case GL_CW:  return FrontFaceMode::Clockwise;
    case GL_CCW: return FrontFaceMode::CounterClockwise;
    default:     return FrontFaceMode::Unknown;
    }
}

// ============================================================================
// Primitive mode validation: desktop GL modes not in ES 3.2
// ============================================================================

static bool IsValidPrimitiveMode(GLenum mode) {
    switch (mode) {
    case GL_POINTS:
    case GL_LINES:
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_LINES_ADJACENCY:
    case GL_LINE_STRIP_ADJACENCY:
    case GL_TRIANGLES_ADJACENCY:
    case GL_TRIANGLE_STRIP_ADJACENCY:
    case GL_PATCHES:
        return true;
    // Desktop GL only — not supported in ES 3.2
    case GL_QUADS:
    case GL_QUAD_STRIP:
    case GL_POLYGON:
    default:
        return false;
    }
}

// ============================================================================
// Atomic counter buffer emulation
// ============================================================================

static void syncAtomicCounters() {
    auto &bs = GLState.buffer;
    if (bs.atomicCounterBufferBinding == 0) return;
    if (bs.atomicCounterData.empty()) return;

    GLuint realBuf = GLState.GetRealBuffer(bs.atomicCounterBufferBinding);
    if (realBuf) {
        GLES.glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realBuf);
        GLES.glBufferSubData(GL_ATOMIC_COUNTER_BUFFER,
                             bs.atomicCounterBufferOffset,
                             bs.atomicCounterBufferSize,
                             bs.atomicCounterData.data());
    }
}

static void readbackAtomicCounters() {
    auto &bs = GLState.buffer;
    if (bs.atomicCounterBufferBinding == 0) return;

    GLuint realBuf = GLState.GetRealBuffer(bs.atomicCounterBufferBinding);
    if (realBuf) {
        GLES.glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, realBuf);
        void *ptr = GLES.glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                          bs.atomicCounterBufferOffset,
                                          bs.atomicCounterBufferSize,
                                          GL_MAP_READ_BIT);
        if (ptr) {
            memcpy(bs.atomicCounterData.data(), ptr, (size_t)bs.atomicCounterBufferSize);
            GLES.glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        }
    }
}

// ============================================================================
// Draw call Backend functions (actual GLES calls)
// ============================================================================

static void DrawArrays_Backend(GLenum mode, GLint first, GLsizei count) {
    GLES.glDrawArrays(mode, first, count);
}

static void DrawElements_Backend(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    GLES.glDrawElements(mode, count, type, indices);
}

static void DrawArraysInstanced_Backend(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    GLES.glDrawArraysInstanced(mode, first, count, instancecount);
}

static void DrawElementsInstanced_Backend(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    GLES.glDrawElementsInstanced(mode, count, type, indices, instancecount);
}

static void DrawRangeElements_Backend(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    GLES.glDrawRangeElements(mode, start, end, count, type, indices);
}

static void DrawElementsBaseVertex_Backend(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GLES.glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}

static void DrawArraysIndirect_Backend(GLenum mode, const void *indirect) {
    GLES.glDrawArraysIndirect(mode, indirect);
}

static void DrawElementsIndirect_Backend(GLenum mode, GLenum type, const void *indirect) {
    GLES.glDrawElementsIndirect(mode, type, indirect);
}

static void DispatchCompute_Backend(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    GLES.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

// ============================================================================
// Draw call State functions (validation + state management)
// ============================================================================

static void DrawArrays_State(GLenum mode, GLint first, GLsizei count) {
    // Validate primitive mode
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawArrays: invalid primitive mode"));
        return;
    }
    // Validate count
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawArrays: count is negative"));
        return;
    }
    // Check current program is set
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawArrays: no program bound"));
        return;
    }

    // FSR proxy rendering: check if we're in proxy mode
    if (GLState.proxyWidth > 0 && GLState.proxyHeight > 0) {
        GLState.isDrawCall = true;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawArrays_Backend(mode, first, count);
    GLState.isDrawCall = false;
}

static void DrawElements_State(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawElements: invalid primitive mode"));
        return;
    }
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawElements: count is negative"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawElements: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawElements_Backend(mode, count, type, indices);
    GLState.isDrawCall = false;
}

static void DrawArraysInstanced_State(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawArraysInstanced: invalid primitive mode"));
        return;
    }
    if (count < 0 || instancecount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawArraysInstanced: negative count or instancecount"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawArraysInstanced: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawArraysInstanced_Backend(mode, first, count, instancecount);
    GLState.isDrawCall = false;
}

static void DrawElementsInstanced_State(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawElementsInstanced: invalid primitive mode"));
        return;
    }
    if (count < 0 || instancecount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawElementsInstanced: negative count or instancecount"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawElementsInstanced: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawElementsInstanced_Backend(mode, count, type, indices, instancecount);
    GLState.isDrawCall = false;
}

static void DrawRangeElements_State(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawRangeElements: invalid primitive mode"));
        return;
    }
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawRangeElements: count is negative"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawRangeElements: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawRangeElements_Backend(mode, start, end, count, type, indices);
    GLState.isDrawCall = false;
}

static void DrawElementsBaseVertex_State(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawElementsBaseVertex: invalid primitive mode"));
        return;
    }
    if (count < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawElementsBaseVertex: count is negative"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawElementsBaseVertex: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawElementsBaseVertex_Backend(mode, count, type, indices, basevertex);
    GLState.isDrawCall = false;
}

static void DrawArraysIndirect_State(GLenum mode, const void *indirect) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawArraysIndirect: invalid primitive mode"));
        return;
    }
    if (indirect == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawArraysIndirect: indirect is null"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawArraysIndirect: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawArraysIndirect_Backend(mode, indirect);
    GLState.isDrawCall = false;
}

static void DrawElementsIndirect_State(GLenum mode, GLenum type, const void *indirect) {
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDrawElementsIndirect: invalid primitive mode"));
        return;
    }
    if (indirect == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawElementsIndirect: indirect is null"));
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDrawElementsIndirect: no program bound"));
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    DrawElementsIndirect_Backend(mode, type, indirect);
    GLState.isDrawCall = false;
}

// ============================================================================
// Draw call Public functions (thin wrappers)
// ============================================================================

extern "C" GLAPI GLAPIENTRY void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    GLState.Lock();
    DrawArrays_State(mode, first, count);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    GLState.Lock();
    DrawElements_State(mode, count, type, indices);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    GLState.Lock();
    DrawArraysInstanced_State(mode, first, count, instancecount);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    GLState.Lock();
    DrawElementsInstanced_State(mode, count, type, indices, instancecount);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    GLState.Lock();
    DrawRangeElements_State(mode, start, end, count, type, indices);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GLState.Lock();
    DrawElementsBaseVertex_State(mode, count, type, indices, basevertex);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawArraysIndirect(GLenum mode, const void *indirect) {
    GLState.Lock();
    DrawArraysIndirect_State(mode, indirect);
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {
    GLState.Lock();
    DrawElementsIndirect_State(mode, type, indirect);
    GLState.Unlock();
}

// ============================================================================
// glMultiDrawArrays / glMultiDrawElements — CPU simulation
// ============================================================================

extern "C" GLAPI GLAPIENTRY void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount) {
    GLState.Lock();
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glMultiDrawArrays: invalid primitive mode"));
        GLState.Unlock();
        return;
    }
    if (primcount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMultiDrawArrays: primcount is negative"));
        GLState.Unlock();
        return;
    }
    if (first == nullptr || count == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMultiDrawArrays: null pointer"));
        GLState.Unlock();
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glMultiDrawArrays: no program bound"));
        GLState.Unlock();
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    for (GLsizei i = 0; i < primcount; ++i) {
        if (count[i] > 0) {
            GLES.glDrawArrays(mode, first[i], count[i]);
        }
    }
    GLState.isDrawCall = false;
    GLState.Unlock();
}

extern "C" GLAPI GLAPIENTRY void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    GLState.Lock();
    if (!IsValidPrimitiveMode(mode)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glMultiDrawElements: invalid primitive mode"));
        GLState.Unlock();
        return;
    }
    if (primcount < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMultiDrawElements: primcount is negative"));
        GLState.Unlock();
        return;
    }
    if (count == nullptr || indices == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMultiDrawElements: null pointer"));
        GLState.Unlock();
        return;
    }
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glMultiDrawElements: no program bound"));
        GLState.Unlock();
        return;
    }

    GLState.isDrawCall = true;
    syncAtomicCounters();
    for (GLsizei i = 0; i < primcount; ++i) {
        if (count[i] > 0) {
            GLES.glDrawElements(mode, count[i], type, indices[i]);
        }
    }
    GLState.isDrawCall = false;
    GLState.Unlock();
}

// ============================================================================
// Compute shader dispatch
// ============================================================================

static void DispatchCompute_State(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    if (GLState.currentProgram == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidOperation,
            std::make_unique<GenericErrorInfo>("glDispatchCompute: no program bound"));
        return;
    }
    GLState.isDrawCall = true;
    syncAtomicCounters();
    DispatchCompute_Backend(num_groups_x, num_groups_y, num_groups_z);
    readbackAtomicCounters();
    GLState.isDrawCall = false;
}

extern "C" GLAPI GLAPIENTRY void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    GLState.Lock();
    DispatchCompute_State(num_groups_x, num_groups_y, num_groups_z);
    GLState.Unlock();
}

// ============================================================================
// glClear / glClearColor / glClearDepthf / glClearStencil
// ============================================================================

static void Clear_Backend(GLbitfield mask) {
    GLES.glClear(mask);
}

static void Clear_State(GLbitfield mask) {
    // GL_ACCUM_BUFFER_BIT is not supported in ES, strip it
    mask &= ~0x00000200;
    Clear_Backend(mask);
}

extern "C" GLAPI GLAPIENTRY void glClear(GLbitfield mask) {
    GLState.Lock();
    Clear_State(mask);
    GLState.Unlock();
}

static void ClearColor_Backend(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLES.glClearColor(red, green, blue, alpha);
}

static void ClearColor_State(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLState.renderState.SetClearColor(red, green, blue, alpha);
    ClearColor_Backend(red, green, blue, alpha);
}

extern "C" GLAPI GLAPIENTRY void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLState.Lock();
    ClearColor_State(red, green, blue, alpha);
    GLState.Unlock();
}

static void ClearDepthf_Backend(GLfloat d) {
    GLES.glClearDepthf(d);
}

static void ClearDepthf_State(GLfloat d) {
    GLState.renderState.SetClearDepth(d);
    ClearDepthf_Backend(d);
}

extern "C" GLAPI GLAPIENTRY void glClearDepthf(GLfloat d) {
    GLState.Lock();
    ClearDepthf_State(d);
    GLState.Unlock();
}

static void ClearStencil_Backend(GLint s) {
    GLES.glClearStencil(s);
}

static void ClearStencil_State(GLint s) {
    GLState.renderState.SetClearStencil(s);
    ClearStencil_Backend(s);
}

extern "C" GLAPI GLAPIENTRY void glClearStencil(GLint s) {
    GLState.Lock();
    ClearStencil_State(s);
    GLState.Unlock();
}

// ============================================================================
// glEnable / glDisable
// ============================================================================

static void Enable_Backend(GLenum cap) {
    GLES.glEnable(cap);
}

static void Enable_State(GLenum cap) {
    Capability c = GLEnumToCapability(cap);
    if (c == Capability::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glEnable: invalid capability"));
        return;
    }
    GLState.renderState.SetCapability(c, true);
    Enable_Backend(cap);
}

extern "C" GLAPI GLAPIENTRY void glEnable(GLenum cap) {
    GLState.Lock();
    Enable_State(cap);
    GLState.Unlock();
}

static void Disable_Backend(GLenum cap) {
    GLES.glDisable(cap);
}

static void Disable_State(GLenum cap) {
    Capability c = GLEnumToCapability(cap);
    if (c == Capability::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDisable: invalid capability"));
        return;
    }
    GLState.renderState.SetCapability(c, false);
    Disable_Backend(cap);
}

extern "C" GLAPI GLAPIENTRY void glDisable(GLenum cap) {
    GLState.Lock();
    Disable_State(cap);
    GLState.Unlock();
}

// ============================================================================
// glEnablei / glDisablei
// ============================================================================

static void Enablei_Backend(GLenum target, GLuint index) {
    GLES.glEnablei(target, index);
}

static void Enablei_State(GLenum target, GLuint index) {
    Capability c = GLEnumToCapability(target);
    if (c == Capability::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glEnablei: invalid capability"));
        return;
    }
    GLState.renderState.SetCapabilityIndexed(c, index, true);
    Enablei_Backend(target, index);
}

extern "C" GLAPI GLAPIENTRY void glEnablei(GLenum target, GLuint index) {
    GLState.Lock();
    Enablei_State(target, index);
    GLState.Unlock();
}

static void Disablei_Backend(GLenum target, GLuint index) {
    GLES.glDisablei(target, index);
}

static void Disablei_State(GLenum target, GLuint index) {
    Capability c = GLEnumToCapability(target);
    if (c == Capability::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDisablei: invalid capability"));
        return;
    }
    GLState.renderState.SetCapabilityIndexed(c, index, false);
    Disablei_Backend(target, index);
}

extern "C" GLAPI GLAPIENTRY void glDisablei(GLenum target, GLuint index) {
    GLState.Lock();
    Disablei_State(target, index);
    GLState.Unlock();
}

// ============================================================================
// glBlendFunc / glBlendFuncSeparate
// ============================================================================

static void BlendFunc_Backend(GLenum sfactor, GLenum dfactor) {
    GLES.glBlendFunc(sfactor, dfactor);
}

static void BlendFunc_State(GLenum sfactor, GLenum dfactor) {
    BlendFactor src = GLEnumToBlendFactor(sfactor);
    BlendFactor dst = GLEnumToBlendFactor(dfactor);
    if (src == BlendFactor::Unknown || dst == BlendFactor::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendFunc: invalid blend factor"));
        return;
    }
    GLState.renderState.SetBlendFunc(src, dst, src, dst);
    BlendFunc_Backend(sfactor, dfactor);
}

extern "C" GLAPI GLAPIENTRY void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GLState.Lock();
    BlendFunc_State(sfactor, dfactor);
    GLState.Unlock();
}

static void BlendFuncSeparate_Backend(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    GLES.glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static void BlendFuncSeparate_State(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    BlendFactor srcRGB = GLEnumToBlendFactor(sfactorRGB);
    BlendFactor dstRGB = GLEnumToBlendFactor(dfactorRGB);
    BlendFactor srcAlpha = GLEnumToBlendFactor(sfactorAlpha);
    BlendFactor dstAlpha = GLEnumToBlendFactor(dfactorAlpha);
    if (srcRGB == BlendFactor::Unknown || dstRGB == BlendFactor::Unknown ||
        srcAlpha == BlendFactor::Unknown || dstAlpha == BlendFactor::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendFuncSeparate: invalid blend factor"));
        return;
    }
    GLState.renderState.SetBlendFunc(srcRGB, dstRGB, srcAlpha, dstAlpha);
    BlendFuncSeparate_Backend(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

extern "C" GLAPI GLAPIENTRY void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    GLState.Lock();
    BlendFuncSeparate_State(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    GLState.Unlock();
}

// ============================================================================
// glBlendEquation / glBlendEquationSeparate
// ============================================================================

static void BlendEquation_Backend(GLenum mode) {
    GLES.glBlendEquation(mode);
}

static void BlendEquation_State(GLenum mode) {
    BlendEquation eq = GLEnumToBlendEquation(mode);
    if (eq == BlendEquation::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendEquation: invalid blend equation"));
        return;
    }
    GLState.renderState.SetBlendEquation(eq, eq);
    BlendEquation_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glBlendEquation(GLenum mode) {
    GLState.Lock();
    BlendEquation_State(mode);
    GLState.Unlock();
}

static void BlendEquationSeparate_Backend(GLenum modeRGB, GLenum modeAlpha) {
    GLES.glBlendEquationSeparate(modeRGB, modeAlpha);
}

static void BlendEquationSeparate_State(GLenum modeRGB, GLenum modeAlpha) {
    BlendEquation eqRGB = GLEnumToBlendEquation(modeRGB);
    BlendEquation eqAlpha = GLEnumToBlendEquation(modeAlpha);
    if (eqRGB == BlendEquation::Unknown || eqAlpha == BlendEquation::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendEquationSeparate: invalid blend equation"));
        return;
    }
    GLState.renderState.SetBlendEquation(eqRGB, eqAlpha);
    BlendEquationSeparate_Backend(modeRGB, modeAlpha);
}

extern "C" GLAPI GLAPIENTRY void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    GLState.Lock();
    BlendEquationSeparate_State(modeRGB, modeAlpha);
    GLState.Unlock();
}

// ============================================================================
// glBlendColor
// ============================================================================

static void BlendColor_Backend(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLES.glBlendColor(red, green, blue, alpha);
}

static void BlendColor_State(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLState.renderState.SetBlendColor(red, green, blue, alpha);
    BlendColor_Backend(red, green, blue, alpha);
}

extern "C" GLAPI GLAPIENTRY void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLState.Lock();
    BlendColor_State(red, green, blue, alpha);
    GLState.Unlock();
}

// ============================================================================
// glBlendFunci / glBlendFuncSeparatei
// ============================================================================

static void BlendFunci_Backend(GLuint buf, GLenum src, GLenum dst) {
    GLES.glBlendFunci(buf, src, dst);
}

static void BlendFunci_State(GLuint buf, GLenum src, GLenum dst) {
    BlendFactor srcFactor = GLEnumToBlendFactor(src);
    BlendFactor dstFactor = GLEnumToBlendFactor(dst);
    if (srcFactor == BlendFactor::Unknown || dstFactor == BlendFactor::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendFunci: invalid blend factor"));
        return;
    }
    GLState.renderState.SetBlendFuncIndexed(buf, srcFactor, dstFactor, srcFactor, dstFactor);
    BlendFunci_Backend(buf, src, dst);
}

extern "C" GLAPI GLAPIENTRY void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {
    GLState.Lock();
    BlendFunci_State(buf, src, dst);
    GLState.Unlock();
}

static void BlendFuncSeparatei_Backend(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    GLES.glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

static void BlendFuncSeparatei_State(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    BlendFactor sRGB = GLEnumToBlendFactor(srcRGB);
    BlendFactor dRGB = GLEnumToBlendFactor(dstRGB);
    BlendFactor sA = GLEnumToBlendFactor(srcAlpha);
    BlendFactor dA = GLEnumToBlendFactor(dstAlpha);
    if (sRGB == BlendFactor::Unknown || dRGB == BlendFactor::Unknown ||
        sA == BlendFactor::Unknown || dA == BlendFactor::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendFuncSeparatei: invalid blend factor"));
        return;
    }
    GLState.renderState.SetBlendFuncIndexed(buf, sRGB, dRGB, sA, dA);
    BlendFuncSeparatei_Backend(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

extern "C" GLAPI GLAPIENTRY void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    GLState.Lock();
    BlendFuncSeparatei_State(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
    GLState.Unlock();
}

// ============================================================================
// glBlendEquationi / glBlendEquationSeparatei
// ============================================================================

static void BlendEquationi_Backend(GLuint buf, GLenum mode) {
    GLES.glBlendEquationi(buf, mode);
}

static void BlendEquationi_State(GLuint buf, GLenum mode) {
    BlendEquation eq = GLEnumToBlendEquation(mode);
    if (eq == BlendEquation::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendEquationi: invalid blend equation"));
        return;
    }
    GLState.renderState.SetBlendEquationIndexed(buf, eq, eq);
    BlendEquationi_Backend(buf, mode);
}

extern "C" GLAPI GLAPIENTRY void glBlendEquationi(GLuint buf, GLenum mode) {
    GLState.Lock();
    BlendEquationi_State(buf, mode);
    GLState.Unlock();
}

static void BlendEquationSeparatei_Backend(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    GLES.glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}

static void BlendEquationSeparatei_State(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    BlendEquation eqRGB = GLEnumToBlendEquation(modeRGB);
    BlendEquation eqAlpha = GLEnumToBlendEquation(modeAlpha);
    if (eqRGB == BlendEquation::Unknown || eqAlpha == BlendEquation::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBlendEquationSeparatei: invalid blend equation"));
        return;
    }
    GLState.renderState.SetBlendEquationIndexed(buf, eqRGB, eqAlpha);
    BlendEquationSeparatei_Backend(buf, modeRGB, modeAlpha);
}

extern "C" GLAPI GLAPIENTRY void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    GLState.Lock();
    BlendEquationSeparatei_State(buf, modeRGB, modeAlpha);
    GLState.Unlock();
}

// ============================================================================
// glDepthFunc
// ============================================================================

static void DepthFunc_Backend(GLenum func) {
    GLES.glDepthFunc(func);
}

static void DepthFunc_State(GLenum func) {
    DepthTestFunc df = GLEnumToDepthFunc(func);
    if (df == DepthTestFunc::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glDepthFunc: invalid depth function"));
        return;
    }
    GLState.renderState.SetDepthFunc(df);
    DepthFunc_Backend(func);
}

extern "C" GLAPI GLAPIENTRY void glDepthFunc(GLenum func) {
    GLState.Lock();
    DepthFunc_State(func);
    GLState.Unlock();
}

// ============================================================================
// glDepthMask
// ============================================================================

static void DepthMask_Backend(GLboolean flag) {
    GLES.glDepthMask(flag);
}

static void DepthMask_State(GLboolean flag) {
    GLState.renderState.SetDepthMask(flag == GL_TRUE);
    DepthMask_Backend(flag);
}

extern "C" GLAPI GLAPIENTRY void glDepthMask(GLboolean flag) {
    GLState.Lock();
    DepthMask_State(flag);
    GLState.Unlock();
}

// ============================================================================
// glDepthRangef
// ============================================================================

static void DepthRangef_Backend(GLfloat n, GLfloat f) {
    GLES.glDepthRangef(n, f);
}

static void DepthRangef_State(GLfloat n, GLfloat f) {
    GLState.renderState.SetDepthRange(n, f);
    DepthRangef_Backend(n, f);
}

extern "C" GLAPI GLAPIENTRY void glDepthRangef(GLfloat n, GLfloat f) {
    GLState.Lock();
    DepthRangef_State(n, f);
    GLState.Unlock();
}

// ============================================================================
// glStencilFunc
// ============================================================================

static void StencilFunc_Backend(GLenum func, GLint ref, GLuint mask) {
    GLES.glStencilFunc(func, ref, mask);
}

static void StencilFunc_State(GLenum func, GLint ref, GLuint mask) {
    DepthTestFunc df = GLEnumToDepthFunc(func);
    if (df == DepthTestFunc::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glStencilFunc: invalid stencil function"));
        return;
    }
    GLState.renderState.SetStencilFunc(0, df, ref, mask);
    GLState.renderState.SetStencilFunc(1, df, ref, mask);
    StencilFunc_Backend(func, ref, mask);
}

extern "C" GLAPI GLAPIENTRY void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    GLState.Lock();
    StencilFunc_State(func, ref, mask);
    GLState.Unlock();
}

// ============================================================================
// glStencilOp
// ============================================================================

static void StencilOp_Backend(GLenum fail, GLenum zfail, GLenum zpass) {
    GLES.glStencilOp(fail, zfail, zpass);
}

static void StencilOp_State(GLenum fail, GLenum zfail, GLenum zpass) {
    StencilOperation sfail = GLEnumToStencilOp(fail);
    StencilOperation szfail = GLEnumToStencilOp(zfail);
    StencilOperation szpass = GLEnumToStencilOp(zpass);
    if (sfail == StencilOperation::Unknown || szfail == StencilOperation::Unknown || szpass == StencilOperation::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glStencilOp: invalid stencil operation"));
        return;
    }
    GLState.renderState.SetStencilOp(0, sfail, szfail, szpass);
    GLState.renderState.SetStencilOp(1, sfail, szfail, szpass);
    StencilOp_Backend(fail, zfail, zpass);
}

extern "C" GLAPI GLAPIENTRY void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    GLState.Lock();
    StencilOp_State(fail, zfail, zpass);
    GLState.Unlock();
}

// ============================================================================
// glStencilMask
// ============================================================================

static void StencilMask_Backend(GLuint mask) {
    GLES.glStencilMask(mask);
}

static void StencilMask_State(GLuint mask) {
    GLState.renderState.SetStencilMask(0, mask);
    GLState.renderState.SetStencilMask(1, mask);
    StencilMask_Backend(mask);
}

extern "C" GLAPI GLAPIENTRY void glStencilMask(GLuint mask) {
    GLState.Lock();
    StencilMask_State(mask);
    GLState.Unlock();
}

// ============================================================================
// glCullFace
// ============================================================================

static void CullFace_Backend(GLenum mode) {
    GLES.glCullFace(mode);
}

static void CullFace_State(GLenum mode) {
    CullFaceMode cm = GLEnumToCullFace(mode);
    if (cm == CullFaceMode::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glCullFace: invalid cull face mode"));
        return;
    }
    GLState.renderState.SetCullFaceMode(cm);
    CullFace_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glCullFace(GLenum mode) {
    GLState.Lock();
    CullFace_State(mode);
    GLState.Unlock();
}

// ============================================================================
// glFrontFace
// ============================================================================

static void FrontFace_Backend(GLenum mode) {
    GLES.glFrontFace(mode);
}

static void FrontFace_State(GLenum mode) {
    FrontFaceMode fm = GLEnumToFrontFace(mode);
    if (fm == FrontFaceMode::Unknown) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFrontFace: invalid front face mode"));
        return;
    }
    GLState.renderState.SetFrontFaceMode(fm);
    FrontFace_Backend(mode);
}

extern "C" GLAPI GLAPIENTRY void glFrontFace(GLenum mode) {
    GLState.Lock();
    FrontFace_State(mode);
    GLState.Unlock();
}

// ============================================================================
// glViewport
// ============================================================================

static void Viewport_Backend(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLES.glViewport(x, y, width, height);
}

static void Viewport_State(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (width < 0 || height < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glViewport: negative width or height"));
        return;
    }
    GLState.renderState.SetViewport(x, y, width, height);
    Viewport_Backend(x, y, width, height);
}

extern "C" GLAPI GLAPIENTRY void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLState.Lock();
    Viewport_State(x, y, width, height);
    GLState.Unlock();
}

// ============================================================================
// glScissor
// ============================================================================

static void Scissor_Backend(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLES.glScissor(x, y, width, height);
}

static void Scissor_State(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (width < 0 || height < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glScissor: negative width or height"));
        return;
    }
    GLState.renderState.SetScissorBox(x, y, width, height);
    Scissor_Backend(x, y, width, height);
}

extern "C" GLAPI GLAPIENTRY void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLState.Lock();
    Scissor_State(x, y, width, height);
    GLState.Unlock();
}

// ============================================================================
// glLineWidth
// ============================================================================

static void LineWidth_Backend(GLfloat width) {
    GLES.glLineWidth(width);
}

static void LineWidth_State(GLfloat width) {
    if (width <= 0.0f) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glLineWidth: width must be greater than zero"));
        return;
    }
    GLState.renderState.SetLineWidth(width);
    LineWidth_Backend(width);
}

extern "C" GLAPI GLAPIENTRY void glLineWidth(GLfloat width) {
    GLState.Lock();
    LineWidth_State(width);
    GLState.Unlock();
}

// ============================================================================
// glPolygonOffset
// ============================================================================

static void PolygonOffset_Backend(GLfloat factor, GLfloat units) {
    GLES.glPolygonOffset(factor, units);
}

static void PolygonOffset_State(GLfloat factor, GLfloat units) {
    GLState.renderState.SetPolygonOffset(factor, units);
    PolygonOffset_Backend(factor, units);
}

extern "C" GLAPI GLAPIENTRY void glPolygonOffset(GLfloat factor, GLfloat units) {
    GLState.Lock();
    PolygonOffset_State(factor, units);
    GLState.Unlock();
}

// ============================================================================
// glColorMask
// ============================================================================

static void ColorMask_Backend(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GLES.glColorMask(red, green, blue, alpha);
}

static void ColorMask_State(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GLState.renderState.SetColorMask(red == GL_TRUE, green == GL_TRUE, blue == GL_TRUE, alpha == GL_TRUE);
    ColorMask_Backend(red, green, blue, alpha);
}

extern "C" GLAPI GLAPIENTRY void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GLState.Lock();
    ColorMask_State(red, green, blue, alpha);
    GLState.Unlock();
}

// ============================================================================
// glSampleCoverage
// ============================================================================

static void SampleCoverage_Backend(GLfloat value, GLboolean invert) {
    GLES.glSampleCoverage(value, invert);
}

static void SampleCoverage_State(GLfloat value, GLboolean invert) {
    GLState.renderState.SetSampleCoverage(value, invert == GL_TRUE);
    SampleCoverage_Backend(value, invert);
}

extern "C" GLAPI GLAPIENTRY void glSampleCoverage(GLfloat value, GLboolean invert) {
    GLState.Lock();
    SampleCoverage_State(value, invert);
    GLState.Unlock();
}

// ============================================================================
// glSampleMaski
// ============================================================================

static void SampleMaski_Backend(GLuint maskNumber, GLbitfield mask) {
    GLES.glSampleMaski(maskNumber, mask);
}

static void SampleMaski_State(GLuint maskNumber, GLbitfield mask) {
    GLState.renderState.SetSampleMaskValue(mask);
    SampleMaski_Backend(maskNumber, mask);
}

extern "C" GLAPI GLAPIENTRY void glSampleMaski(GLuint maskNumber, GLbitfield mask) {
    GLState.Lock();
    SampleMaski_State(maskNumber, mask);
    GLState.Unlock();
}

// ============================================================================
// glClearDepth - desktop uses double, ES uses float
// ============================================================================

extern "C" GLAPI GLAPIENTRY void glClearDepth(GLclampd depth) {
    GLState.Lock();
    GLState.renderState.SetClearDepth((GLfloat)depth);
    GLES.glClearDepthf((float)depth);
    GLState.Unlock();
}

// ============================================================================
// glFinish / glFlush
// ============================================================================

extern "C" GLAPI GLAPIENTRY void glFinishARB(void) __attribute__((alias("glFinish")));
extern "C" GLAPI GLAPIENTRY void glFinish(void) {
    GLES.glFinish();
}

extern "C" GLAPI GLAPIENTRY void glFlushARB(void) __attribute__((alias("glFlush")));
extern "C" GLAPI GLAPIENTRY void glFlush(void) {
    GLES.glFlush();
}

// ============================================================================
// glMemoryBarrier - handle atomic counter buffer sync
// ============================================================================

// Narrow GL_ALL_BARRIER_BITS to only the barriers commonly needed for
// compute shader work. This reduces the probability of driver-forced
// global synchronization, which is expensive on mobile GPUs.
// The full GL_ALL_BARRIER_BITS (0xFFFFFFFF) causes the driver to flush
// all caches; narrowing to the actually relevant bits avoids this.
#define MG_COMPUTE_BARRIER_MASK (GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | \
                                 GL_ELEMENT_ARRAY_BARRIER_BIT |       \
                                 GL_UNIFORM_BARRIER_BIT |             \
                                 GL_TEXTURE_FETCH_BARRIER_BIT |       \
                                 GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | \
                                 GL_COMMAND_BARRIER_BIT |             \
                                 GL_PIXEL_BUFFER_BARRIER_BIT |        \
                                 GL_TEXTURE_UPDATE_BARRIER_BIT |      \
                                 GL_BUFFER_UPDATE_BARRIER_BIT |       \
                                 GL_FRAMEBUFFER_BARRIER_BIT |         \
                                 GL_TRANSFORM_FEEDBACK_BARRIER_BIT |  \
                                 GL_ATOMIC_COUNTER_BARRIER_BIT |      \
                                 GL_SHADER_STORAGE_BARRIER_BIT)

static void MemoryBarrier_Backend(GLbitfield barriers) {
    GLES.glMemoryBarrier(barriers);
}

static void MemoryBarrier_State(GLbitfield barriers) {
    if (barriers & GL_ATOMIC_COUNTER_BARRIER_BIT) {
        syncAtomicCounters();
    }
    // Narrow GL_ALL_BARRIER_BITS to known barrier bits only.
    // Some drivers treat unknown bits as a full pipeline flush.
    if (barriers == GL_ALL_BARRIER_BITS) {
        barriers &= MG_COMPUTE_BARRIER_MASK;
    }
    MemoryBarrier_Backend(barriers);
}

extern "C" GLAPI GLAPIENTRY void glMemoryBarrier(GLbitfield barriers) {
    GLState.Lock();
    MemoryBarrier_State(barriers);
    GLState.Unlock();
}

// ============================================================================
// glDrawBuffers - track draw buffer state
// ============================================================================

static void DrawBuffers_Backend(GLsizei n, const GLenum *bufs) {
    GLES.glDrawBuffers(n, bufs);
}

static void DrawBuffers_State(GLsizei n, const GLenum *bufs) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawBuffers: n is negative"));
        return;
    }
    if (n > 0 && bufs == nullptr) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDrawBuffers: bufs is null"));
        return;
    }

    auto &fb = GLState.framebuffer;
    fb.drawBufferCount = n;
    for (GLsizei i = 0; i < n && i < MAX_DRAW_BUFFERS; i++) {
        fb.drawBuffers[i] = bufs[i];
    }

    DrawBuffers_Backend(n, bufs);
}

extern "C" GLAPI GLAPIENTRY void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    GLState.Lock();
    DrawBuffers_State(n, bufs);
    GLState.Unlock();
}

// ============================================================================
// glBindImageTexture - track image unit state
// ============================================================================

static void BindImageTexture_Backend(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    GLES.glBindImageTexture(unit, texture, level, layered, layer, access, format);
}

static void BindImageTexture_State(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    if (unit < MAX_IMAGE_UNITS) {
        auto &img = GLState.image.imageUnits[unit];
        img.texture = texture;
        img.level = level;
        img.layered = layered;
        img.layer = layer;
        img.access = access;
        img.format = format;
    }
    BindImageTexture_Backend(unit, texture, level, layered, layer, access, format);
}

extern "C" GLAPI GLAPIENTRY void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    GLState.Lock();
    BindImageTexture_State(unit, texture, level, layered, layer, access, format);
    GLState.Unlock();
}