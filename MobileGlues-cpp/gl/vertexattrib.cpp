// MobileGlues - gl/vertexattrib.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

// ============================================================================
// Vertex Attribute Architecture
//
// Principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
// Architecture: DirectGLES — public wrappers → _State (validation + state) → _Backend (GLES call)
//
// ES 3.2 natively supports:
//   - glVertexAttribI4ui, glVertexAttribI4i (integer vertex attributes with 4 components)
//   - glVertexAttribPointer, glVertexAttribIPointer
//   - glVertexAttribFormat, glVertexAttribIFormat
//   - glVertexAttribBinding, glVertexBindingDivisor, glVertexAttribDivisor
//   - glEnableVertexAttribArray, glDisableVertexAttribArray
//   - glVertexAttrib1f/2f/3f/4f, glVertexAttrib1fv/2fv/3fv/4fv
//   - glGetVertexAttribfv/iv/Iiv/Iuiv, glGetVertexAttribPointerv
// ============================================================================

#include "vertexattrib.h"
#include "buffer.h"

#define DEBUG 0

// ============================================================================
// Helper: validate vertex attribute index
// ============================================================================

static inline bool isValidAttribIndex(GLuint index) {
    return index < MAX_VERTEX_ATTRIBS;
}

// ============================================================================
// Helper: validate attribute size
// ============================================================================

static inline bool isValidAttribSize(GLint size) {
    return (size >= 1 && size <= 4) || size == GL_BGRA;
}

// ============================================================================
// Helper: validate attribute type for glVertexAttribPointer
// ============================================================================

static inline bool isValidAttribPointerType(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_HALF_FLOAT:
    case GL_FLOAT:
    case GL_DOUBLE:
    case GL_FIXED:
    case GL_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
        return true;
    default:
        return false;
    }
}

// ============================================================================
// Helper: validate integer attribute type for glVertexAttribIPointer
// ============================================================================

static inline bool isValidAttribIPointerType(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return true;
    default:
        return false;
    }
}

// ============================================================================
// Helper: validate attrib format type
// ============================================================================

static inline bool isValidAttribFormatType(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_HALF_FLOAT:
    case GL_FLOAT:
    case GL_DOUBLE:
    case GL_FIXED:
    case GL_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
        return true;
    default:
        return false;
    }
}

// ============================================================================
// Helper: validate integer attrib format type
// ============================================================================

static inline bool isValidAttribIFormatType(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return true;
    default:
        return false;
    }
}

// ============================================================================
// Helper: validate getter pname for glGetVertexAttrib*
// ============================================================================

static inline bool isValidVertexAttribPname(GLenum pname) {
    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
    case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
    case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
    case GL_VERTEX_ATTRIB_ARRAY_LONG:
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
    case GL_CURRENT_VERTEX_ATTRIB:
        return true;
    default:
        return false;
    }
}

// ============================================================================
// SECTION 1: Attribute Value Setters (glVertexAttrib{1,2,3,4}{f,fv})
// ============================================================================

// --- glVertexAttrib1f ---

static void VertexAttrib1f_Backend(GLuint index, GLfloat x) {
    GLES.glVertexAttrib1f(index, x);
    CHECK_GL_ERROR
}

static void VertexAttrib1f_State(GLuint index, GLfloat x) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1f: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib1f_Backend(index, x);
}

GLAPI GLAPIENTRY void glVertexAttrib1f(GLuint index, GLfloat x) {
    LOG()
    LOG_D("glVertexAttrib1f(%u, %f)", index, x)
    GLState.Lock();
    VertexAttrib1f_State(index, x);
    GLState.Unlock();
}

// --- glVertexAttrib1fv ---

static void VertexAttrib1fv_Backend(GLuint index, const GLfloat* v) {
    GLES.glVertexAttrib1fv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttrib1fv_State(GLuint index, const GLfloat* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1fv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1fv: v is null"));
        return;
    }
    VertexAttrib1fv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib1fv(GLuint index, const GLfloat* v) {
    LOG()
    LOG_D("glVertexAttrib1fv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib1fv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib2f ---

static void VertexAttrib2f_Backend(GLuint index, GLfloat x, GLfloat y) {
    GLES.glVertexAttrib2f(index, x, y);
    CHECK_GL_ERROR
}

static void VertexAttrib2f_State(GLuint index, GLfloat x, GLfloat y) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2f: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib2f_Backend(index, x, y);
}

GLAPI GLAPIENTRY void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    LOG()
    LOG_D("glVertexAttrib2f(%u, %f, %f)", index, x, y)
    GLState.Lock();
    VertexAttrib2f_State(index, x, y);
    GLState.Unlock();
}

// --- glVertexAttrib2fv ---

static void VertexAttrib2fv_Backend(GLuint index, const GLfloat* v) {
    GLES.glVertexAttrib2fv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttrib2fv_State(GLuint index, const GLfloat* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2fv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2fv: v is null"));
        return;
    }
    VertexAttrib2fv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib2fv(GLuint index, const GLfloat* v) {
    LOG()
    LOG_D("glVertexAttrib2fv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib2fv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib3f ---

static void VertexAttrib3f_Backend(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    GLES.glVertexAttrib3f(index, x, y, z);
    CHECK_GL_ERROR
}

static void VertexAttrib3f_State(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3f: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib3f_Backend(index, x, y, z);
}

GLAPI GLAPIENTRY void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    LOG()
    LOG_D("glVertexAttrib3f(%u, %f, %f, %f)", index, x, y, z)
    GLState.Lock();
    VertexAttrib3f_State(index, x, y, z);
    GLState.Unlock();
}

// --- glVertexAttrib3fv ---

static void VertexAttrib3fv_Backend(GLuint index, const GLfloat* v) {
    GLES.glVertexAttrib3fv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttrib3fv_State(GLuint index, const GLfloat* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3fv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3fv: v is null"));
        return;
    }
    VertexAttrib3fv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib3fv(GLuint index, const GLfloat* v) {
    LOG()
    LOG_D("glVertexAttrib3fv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib3fv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib4f ---

static void VertexAttrib4f_Backend(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLES.glVertexAttrib4f(index, x, y, z, w);
    CHECK_GL_ERROR
}

static void VertexAttrib4f_State(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4f: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib4f_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    LOG()
    LOG_D("glVertexAttrib4f(%u, %f, %f, %f, %f)", index, x, y, z, w)
    GLState.Lock();
    VertexAttrib4f_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttrib4fv ---

static void VertexAttrib4fv_Backend(GLuint index, const GLfloat* v) {
    GLES.glVertexAttrib4fv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttrib4fv_State(GLuint index, const GLfloat* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4fv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4fv: v is null"));
        return;
    }
    VertexAttrib4fv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib4fv(GLuint index, const GLfloat* v) {
    LOG()
    LOG_D("glVertexAttrib4fv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib4fv_State(index, v);
    GLState.Unlock();
}

// ============================================================================
// SECTION 2: Integer Attribute Value Setters (glVertexAttribI4{i,ui,iv,uiv})
// ============================================================================

// --- glVertexAttribI4i ---

static void VertexAttribI4i_Backend(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    GLES.glVertexAttribI4i(index, x, y, z, w);
    CHECK_GL_ERROR
}

static void VertexAttribI4i_State(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4i: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribI4i_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    LOG()
    LOG_D("glVertexAttribI4i(%u, %d, %d, %d, %d)", index, x, y, z, w)
    GLState.Lock();
    VertexAttribI4i_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttribI4ui ---

static void VertexAttribI4ui_Backend(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    GLES.glVertexAttribI4ui(index, x, y, z, w);
    CHECK_GL_ERROR
}

static void VertexAttribI4ui_State(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4ui: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribI4ui_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    LOG()
    LOG_D("glVertexAttribI4ui(%u, %u, %u, %u, %u)", index, x, y, z, w)
    GLState.Lock();
    VertexAttribI4ui_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttribI4iv ---

static void VertexAttribI4iv_Backend(GLuint index, const GLint* v) {
    GLES.glVertexAttribI4iv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttribI4iv_State(GLuint index, const GLint* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4iv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4iv: v is null"));
        return;
    }
    VertexAttribI4iv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribI4iv(GLuint index, const GLint* v) {
    LOG()
    LOG_D("glVertexAttribI4iv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribI4iv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttribI4uiv ---

static void VertexAttribI4uiv_Backend(GLuint index, const GLuint* v) {
    GLES.glVertexAttribI4uiv(index, v);
    CHECK_GL_ERROR
}

static void VertexAttribI4uiv_State(GLuint index, const GLuint* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4uiv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribI4uiv: v is null"));
        return;
    }
    VertexAttribI4uiv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribI4uiv(GLuint index, const GLuint* v) {
    LOG()
    LOG_D("glVertexAttribI4uiv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribI4uiv_State(index, v);
    GLState.Unlock();
}

// ============================================================================
// SECTION 3: Attribute Pointer Setup (glVertexAttribPointer, glVertexAttribIPointer)
// ============================================================================

// --- glVertexAttribPointer ---

static void VertexAttribPointer_Backend(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    GLES.glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    CHECK_GL_ERROR
}

static void VertexAttribPointer_State(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribPointer: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribPointer: invalid size"));
        return;
    }
    if (!isValidAttribPointerType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexAttribPointer: invalid type"));
        return;
    }
    if (stride < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribPointer: stride < 0"));
        return;
    }
    VertexAttribPointer_Backend(index, size, type, normalized, stride, pointer);
}

GLAPI GLAPIENTRY void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    LOG()
    LOG_D("glVertexAttribPointer(%u, %d, %s, %d, %d, %p)", index, size, glEnumToString(type), normalized, stride, pointer)
    GLState.Lock();
    VertexAttribPointer_State(index, size, type, normalized, stride, pointer);
    GLState.Unlock();
}

// --- glVertexAttribIPointer ---

static void VertexAttribIPointer_Backend(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    GLES.glVertexAttribIPointer(index, size, type, stride, pointer);
    CHECK_GL_ERROR
}

static void VertexAttribIPointer_State(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribIPointer: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribIPointer: invalid size"));
        return;
    }
    if (!isValidAttribIPointerType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexAttribIPointer: invalid type"));
        return;
    }
    if (stride < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribIPointer: stride < 0"));
        return;
    }
    VertexAttribIPointer_Backend(index, size, type, stride, pointer);
}

GLAPI GLAPIENTRY void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    LOG()
    LOG_D("glVertexAttribIPointer(%u, %d, %s, %d, %p)", index, size, glEnumToString(type), stride, pointer)
    GLState.Lock();
    VertexAttribIPointer_State(index, size, type, stride, pointer);
    GLState.Unlock();
}

// ============================================================================
// SECTION 4: Attribute Enable/Disable
// ============================================================================

// --- glEnableVertexAttribArray ---

static void EnableVertexAttribArray_Backend(GLuint index) {
    GLES.glEnableVertexAttribArray(index);
    CHECK_GL_ERROR
}

static void EnableVertexAttribArray_State(GLuint index) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glEnableVertexAttribArray: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    EnableVertexAttribArray_Backend(index);
}

GLAPI GLAPIENTRY void glEnableVertexAttribArray(GLuint index) {
    LOG()
    LOG_D("glEnableVertexAttribArray(%u)", index)
    GLState.Lock();
    EnableVertexAttribArray_State(index);
    GLState.Unlock();
}

// --- glDisableVertexAttribArray ---

static void DisableVertexAttribArray_Backend(GLuint index) {
    GLES.glDisableVertexAttribArray(index);
    CHECK_GL_ERROR
}

static void DisableVertexAttribArray_State(GLuint index) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDisableVertexAttribArray: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    DisableVertexAttribArray_Backend(index);
}

GLAPI GLAPIENTRY void glDisableVertexAttribArray(GLuint index) {
    LOG()
    LOG_D("glDisableVertexAttribArray(%u)", index)
    GLState.Lock();
    DisableVertexAttribArray_State(index);
    GLState.Unlock();
}

// ============================================================================
// SECTION 5: Attribute Format (glVertexAttribFormat, glVertexAttribIFormat)
// ============================================================================

// --- glVertexAttribFormat ---

static void VertexAttribFormat_Backend(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GLES.glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
    CHECK_GL_ERROR
}

static void VertexAttribFormat_State(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    if (!isValidAttribIndex(attribindex)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribFormat: attribindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribFormat: invalid size"));
        return;
    }
    if (!isValidAttribFormatType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexAttribFormat: invalid type"));
        return;
    }
    VertexAttribFormat_Backend(attribindex, size, type, normalized, relativeoffset);
}

GLAPI GLAPIENTRY void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    LOG()
    LOG_D("glVertexAttribFormat(%u, %d, %s, %d, %u)", attribindex, size, glEnumToString(type), normalized, relativeoffset)
    GLState.Lock();
    VertexAttribFormat_State(attribindex, size, type, normalized, relativeoffset);
    GLState.Unlock();
}

// --- glVertexAttribIFormat ---

static void VertexAttribIFormat_Backend(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GLES.glVertexAttribIFormat(attribindex, size, type, relativeoffset);
    CHECK_GL_ERROR
}

static void VertexAttribIFormat_State(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    if (!isValidAttribIndex(attribindex)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribIFormat: attribindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribIFormat: invalid size"));
        return;
    }
    if (!isValidAttribIFormatType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexAttribIFormat: invalid type"));
        return;
    }
    VertexAttribIFormat_Backend(attribindex, size, type, relativeoffset);
}

GLAPI GLAPIENTRY void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    LOG()
    LOG_D("glVertexAttribIFormat(%u, %d, %s, %u)", attribindex, size, glEnumToString(type), relativeoffset)
    GLState.Lock();
    VertexAttribIFormat_State(attribindex, size, type, relativeoffset);
    GLState.Unlock();
}

// ============================================================================
// SECTION 6: Attribute Binding & Divisor
// ============================================================================

// --- glVertexAttribBinding ---

static void VertexAttribBinding_Backend(GLuint attribindex, GLuint bindingindex) {
    GLES.glVertexAttribBinding(attribindex, bindingindex);
    CHECK_GL_ERROR
}

static void VertexAttribBinding_State(GLuint attribindex, GLuint bindingindex) {
    if (!isValidAttribIndex(attribindex)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribBinding: attribindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (bindingindex >= MAX_VERTEX_ATTRIBS) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribBinding: bindingindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribBinding_Backend(attribindex, bindingindex);
}

GLAPI GLAPIENTRY void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    LOG()
    LOG_D("glVertexAttribBinding(%u, %u)", attribindex, bindingindex)
    GLState.Lock();
    VertexAttribBinding_State(attribindex, bindingindex);
    GLState.Unlock();
}

// --- glVertexBindingDivisor ---

static void VertexBindingDivisor_Backend(GLuint bindingindex, GLuint divisor) {
    GLES.glVertexBindingDivisor(bindingindex, divisor);
    CHECK_GL_ERROR
}

static void VertexBindingDivisor_State(GLuint bindingindex, GLuint divisor) {
    if (bindingindex >= MAX_VERTEX_ATTRIBS) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexBindingDivisor: bindingindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexBindingDivisor_Backend(bindingindex, divisor);
}

GLAPI GLAPIENTRY void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    LOG()
    LOG_D("glVertexBindingDivisor(%u, %u)", bindingindex, divisor)
    GLState.Lock();
    VertexBindingDivisor_State(bindingindex, divisor);
    GLState.Unlock();
}

// --- glVertexAttribDivisor ---

static void VertexAttribDivisor_Backend(GLuint index, GLuint divisor) {
    GLES.glVertexAttribDivisor(index, divisor);
    CHECK_GL_ERROR
}

static void VertexAttribDivisor_State(GLuint index, GLuint divisor) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribDivisor: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribDivisor_Backend(index, divisor);
}

GLAPI GLAPIENTRY void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    LOG()
    LOG_D("glVertexAttribDivisor(%u, %u)", index, divisor)
    GLState.Lock();
    VertexAttribDivisor_State(index, divisor);
    GLState.Unlock();
}

// ============================================================================
// SECTION 7: Attribute Getters
// ============================================================================

// --- glGetVertexAttribfv ---

static void GetVertexAttribfv_Backend(GLuint index, GLenum pname, GLfloat* params) {
    GLES.glGetVertexAttribfv(index, pname, params);
    CHECK_GL_ERROR
}

static void GetVertexAttribfv_State(GLuint index, GLenum pname, GLfloat* params) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribfv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidVertexAttribPname(pname)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribfv: invalid pname"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribfv: params is null"));
        return;
    }
    GetVertexAttribfv_Backend(index, pname, params);
}

GLAPI GLAPIENTRY void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetVertexAttribfv(%u, %s, %p)", index, glEnumToString(pname), params)
    GLState.Lock();
    GetVertexAttribfv_State(index, pname, params);
    GLState.Unlock();
}

// --- glGetVertexAttribiv ---

static void GetVertexAttribiv_Backend(GLuint index, GLenum pname, GLint* params) {
    GLES.glGetVertexAttribiv(index, pname, params);
    CHECK_GL_ERROR
}

static void GetVertexAttribiv_State(GLuint index, GLenum pname, GLint* params) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribiv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidVertexAttribPname(pname)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribiv: invalid pname"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribiv: params is null"));
        return;
    }
    GetVertexAttribiv_Backend(index, pname, params);
}

GLAPI GLAPIENTRY void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetVertexAttribiv(%u, %s, %p)", index, glEnumToString(pname), params)
    GLState.Lock();
    GetVertexAttribiv_State(index, pname, params);
    GLState.Unlock();
}

// --- glGetVertexAttribIiv ---

static void GetVertexAttribIiv_Backend(GLuint index, GLenum pname, GLint* params) {
    GLES.glGetVertexAttribIiv(index, pname, params);
    CHECK_GL_ERROR
}

static void GetVertexAttribIiv_State(GLuint index, GLenum pname, GLint* params) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIiv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidVertexAttribPname(pname)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIiv: invalid pname"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIiv: params is null"));
        return;
    }
    GetVertexAttribIiv_Backend(index, pname, params);
}

GLAPI GLAPIENTRY void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetVertexAttribIiv(%u, %s, %p)", index, glEnumToString(pname), params)
    GLState.Lock();
    GetVertexAttribIiv_State(index, pname, params);
    GLState.Unlock();
}

// --- glGetVertexAttribIuiv ---

static void GetVertexAttribIuiv_Backend(GLuint index, GLenum pname, GLuint* params) {
    GLES.glGetVertexAttribIuiv(index, pname, params);
    CHECK_GL_ERROR
}

static void GetVertexAttribIuiv_State(GLuint index, GLenum pname, GLuint* params) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIuiv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidVertexAttribPname(pname)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIuiv: invalid pname"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribIuiv: params is null"));
        return;
    }
    GetVertexAttribIuiv_Backend(index, pname, params);
}

GLAPI GLAPIENTRY void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params) {
    LOG()
    LOG_D("glGetVertexAttribIuiv(%u, %s, %p)", index, glEnumToString(pname), params)
    GLState.Lock();
    GetVertexAttribIuiv_State(index, pname, params);
    GLState.Unlock();
}

// --- glGetVertexAttribPointerv ---

static void GetVertexAttribPointerv_Backend(GLuint index, GLenum pname, void** pointer) {
    GLES.glGetVertexAttribPointerv(index, pname, pointer);
    CHECK_GL_ERROR
}

static void GetVertexAttribPointerv_State(GLuint index, GLenum pname, void** pointer) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribPointerv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribPointerv: invalid pname"));
        return;
    }
    if (!pointer) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetVertexAttribPointerv: pointer is null"));
        return;
    }
    GetVertexAttribPointerv_Backend(index, pname, pointer);
}

GLAPI GLAPIENTRY void glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer) {
    LOG()
    LOG_D("glGetVertexAttribPointerv(%u, %s, %p)", index, glEnumToString(pname), pointer)
    GLState.Lock();
    GetVertexAttribPointerv_State(index, pname, pointer);
    GLState.Unlock();
}

// ============================================================================
// SECTION 8: Stub Functions — Non-ES 3.2 Native (CPU simulation / no-op)
// ============================================================================

// --- glVertexAttrib4Nub ---

static void VertexAttrib4Nub_Backend(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    // CPU simulation: normalized unsigned byte → float
    // glVertexAttrib4Nub sets the attribute to (x/255, y/255, z/255, w/255)
    GLES.glVertexAttrib4f(index, (GLfloat)x / 255.0f, (GLfloat)y / 255.0f, (GLfloat)z / 255.0f, (GLfloat)w / 255.0f);
    CHECK_GL_ERROR
}

static void VertexAttrib4Nub_State(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4Nub: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib4Nub_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    LOG()
    LOG_D("glVertexAttrib4Nub(%u, %u, %u, %u, %u)", index, x, y, z, w)
    GLState.Lock();
    VertexAttrib4Nub_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttrib4Nubv ---

static void VertexAttrib4Nubv_Backend(GLuint index, const GLubyte* v) {
    GLES.glVertexAttrib4f(index, (GLfloat)v[0] / 255.0f, (GLfloat)v[1] / 255.0f, (GLfloat)v[2] / 255.0f, (GLfloat)v[3] / 255.0f);
    CHECK_GL_ERROR
}

static void VertexAttrib4Nubv_State(GLuint index, const GLubyte* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4Nubv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4Nubv: v is null"));
        return;
    }
    VertexAttrib4Nubv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib4Nubv(GLuint index, const GLubyte* v) {
    LOG()
    LOG_D("glVertexAttrib4Nubv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib4Nubv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib4ubv (CPU simulation: unsigned byte → float) ---

static void VertexAttrib4ubv_Backend(GLuint index, const GLubyte* v) {
    GLES.glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
    CHECK_GL_ERROR
}

static void VertexAttrib4ubv_State(GLuint index, const GLubyte* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4ubv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4ubv: v is null"));
        return;
    }
    VertexAttrib4ubv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib4ubv(GLuint index, const GLubyte* v) {
    LOG()
    LOG_D("glVertexAttrib4ubv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib4ubv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib1d (CPU simulation: double → float) ---

static void VertexAttrib1d_Backend(GLuint index, GLdouble x) {
    GLES.glVertexAttrib1f(index, (GLfloat)x);
    CHECK_GL_ERROR
}

static void VertexAttrib1d_State(GLuint index, GLdouble x) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib1d_Backend(index, x);
}

GLAPI GLAPIENTRY void glVertexAttrib1d(GLuint index, GLdouble x) {
    LOG()
    LOG_D("glVertexAttrib1d(%u, %lf)", index, x)
    GLState.Lock();
    VertexAttrib1d_State(index, x);
    GLState.Unlock();
}

// --- glVertexAttrib1dv ---

static void VertexAttrib1dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib1f(index, (GLfloat)v[0]);
    CHECK_GL_ERROR
}

static void VertexAttrib1dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib1dv: v is null"));
        return;
    }
    VertexAttrib1dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib1dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttrib1dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib1dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib2d ---

static void VertexAttrib2d_Backend(GLuint index, GLdouble x, GLdouble y) {
    GLES.glVertexAttrib2f(index, (GLfloat)x, (GLfloat)y);
    CHECK_GL_ERROR
}

static void VertexAttrib2d_State(GLuint index, GLdouble x, GLdouble y) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib2d_Backend(index, x, y);
}

GLAPI GLAPIENTRY void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {
    LOG()
    LOG_D("glVertexAttrib2d(%u, %lf, %lf)", index, x, y)
    GLState.Lock();
    VertexAttrib2d_State(index, x, y);
    GLState.Unlock();
}

// --- glVertexAttrib2dv ---

static void VertexAttrib2dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib2f(index, (GLfloat)v[0], (GLfloat)v[1]);
    CHECK_GL_ERROR
}

static void VertexAttrib2dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib2dv: v is null"));
        return;
    }
    VertexAttrib2dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib2dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttrib2dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib2dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib3d ---

static void VertexAttrib3d_Backend(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GLES.glVertexAttrib3f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z);
    CHECK_GL_ERROR
}

static void VertexAttrib3d_State(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib3d_Backend(index, x, y, z);
}

GLAPI GLAPIENTRY void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    LOG()
    LOG_D("glVertexAttrib3d(%u, %lf, %lf, %lf)", index, x, y, z)
    GLState.Lock();
    VertexAttrib3d_State(index, x, y, z);
    GLState.Unlock();
}

// --- glVertexAttrib3dv ---

static void VertexAttrib3dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib3f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
    CHECK_GL_ERROR
}

static void VertexAttrib3dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib3dv: v is null"));
        return;
    }
    VertexAttrib3dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib3dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttrib3dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib3dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttrib4d ---

static void VertexAttrib4d_Backend(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GLES.glVertexAttrib4f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
    CHECK_GL_ERROR
}

static void VertexAttrib4d_State(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttrib4d_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    LOG()
    LOG_D("glVertexAttrib4d(%u, %lf, %lf, %lf, %lf)", index, x, y, z, w)
    GLState.Lock();
    VertexAttrib4d_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttrib4dv ---

static void VertexAttrib4dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
    CHECK_GL_ERROR
}

static void VertexAttrib4dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttrib4dv: v is null"));
        return;
    }
    VertexAttrib4dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttrib4dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttrib4dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttrib4dv_State(index, v);
    GLState.Unlock();
}

// ============================================================================
// SECTION 9: L-type (double-precision) Attribute Functions (CPU simulation)
// ============================================================================

// --- glVertexAttribL1d ---

static void VertexAttribL1d_Backend(GLuint index, GLdouble x) {
    // L-type attributes are double-precision; GLES 3.2 doesn't support them.
    // Emulate by passing double as float (precision loss is acceptable for targeting).
    GLES.glVertexAttrib1f(index, (GLfloat)x);
    CHECK_GL_ERROR
}

static void VertexAttribL1d_State(GLuint index, GLdouble x) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL1d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribL1d_Backend(index, x);
}

GLAPI GLAPIENTRY void glVertexAttribL1d(GLuint index, GLdouble x) {
    LOG()
    LOG_D("glVertexAttribL1d(%u, %lf)", index, x)
    GLState.Lock();
    VertexAttribL1d_State(index, x);
    GLState.Unlock();
}

// --- glVertexAttribL2d ---

static void VertexAttribL2d_Backend(GLuint index, GLdouble x, GLdouble y) {
    GLES.glVertexAttrib2f(index, (GLfloat)x, (GLfloat)y);
    CHECK_GL_ERROR
}

static void VertexAttribL2d_State(GLuint index, GLdouble x, GLdouble y) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL2d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribL2d_Backend(index, x, y);
}

GLAPI GLAPIENTRY void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {
    LOG()
    LOG_D("glVertexAttribL2d(%u, %lf, %lf)", index, x, y)
    GLState.Lock();
    VertexAttribL2d_State(index, x, y);
    GLState.Unlock();
}

// --- glVertexAttribL3d ---

static void VertexAttribL3d_Backend(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GLES.glVertexAttrib3f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z);
    CHECK_GL_ERROR
}

static void VertexAttribL3d_State(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL3d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribL3d_Backend(index, x, y, z);
}

GLAPI GLAPIENTRY void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    LOG()
    LOG_D("glVertexAttribL3d(%u, %lf, %lf, %lf)", index, x, y, z)
    GLState.Lock();
    VertexAttribL3d_State(index, x, y, z);
    GLState.Unlock();
}

// --- glVertexAttribL4d ---

static void VertexAttribL4d_Backend(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GLES.glVertexAttrib4f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
    CHECK_GL_ERROR
}

static void VertexAttribL4d_State(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL4d: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    VertexAttribL4d_Backend(index, x, y, z, w);
}

GLAPI GLAPIENTRY void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    LOG()
    LOG_D("glVertexAttribL4d(%u, %lf, %lf, %lf, %lf)", index, x, y, z, w)
    GLState.Lock();
    VertexAttribL4d_State(index, x, y, z, w);
    GLState.Unlock();
}

// --- glVertexAttribL1dv ---

static void VertexAttribL1dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib1f(index, (GLfloat)v[0]);
    CHECK_GL_ERROR
}

static void VertexAttribL1dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL1dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL1dv: v is null"));
        return;
    }
    VertexAttribL1dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribL1dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttribL1dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribL1dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttribL2dv ---

static void VertexAttribL2dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib2f(index, (GLfloat)v[0], (GLfloat)v[1]);
    CHECK_GL_ERROR
}

static void VertexAttribL2dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL2dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL2dv: v is null"));
        return;
    }
    VertexAttribL2dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribL2dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttribL2dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribL2dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttribL3dv ---

static void VertexAttribL3dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib3f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
    CHECK_GL_ERROR
}

static void VertexAttribL3dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL3dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL3dv: v is null"));
        return;
    }
    VertexAttribL3dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribL3dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttribL3dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribL3dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttribL4dv ---

static void VertexAttribL4dv_Backend(GLuint index, const GLdouble* v) {
    GLES.glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
    CHECK_GL_ERROR
}

static void VertexAttribL4dv_State(GLuint index, const GLdouble* v) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL4dv: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!v) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribL4dv: v is null"));
        return;
    }
    VertexAttribL4dv_Backend(index, v);
}

GLAPI GLAPIENTRY void glVertexAttribL4dv(GLuint index, const GLdouble* v) {
    LOG()
    LOG_D("glVertexAttribL4dv(%u, %p)", index, v)
    GLState.Lock();
    VertexAttribL4dv_State(index, v);
    GLState.Unlock();
}

// --- glVertexAttribLPointer (CPU simulation: double → float) ---

static void VertexAttribLPointer_Backend(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    // LPointer specifies double-precision vertex attribute data.
    // GLES 3.2 doesn't support GL_DOUBLE vertex attributes natively.
    // We emit a warning and treat as glVertexAttribPointer with GL_FLOAT.
    (void)index;
    (void)size;
    (void)type;
    (void)stride;
    (void)pointer;
    // No direct GLES call available; this is a no-op for now.
    // A full implementation would require a CPU-side buffer copy converting double→float.
}

static void VertexAttribLPointer_State(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribLPointer: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribLPointer: invalid size"));
        return;
    }
    if (type != GL_DOUBLE) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexAttribLPointer: type must be GL_DOUBLE"));
        return;
    }
    if (stride < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexAttribLPointer: stride < 0"));
        return;
    }
    VertexAttribLPointer_Backend(index, size, type, stride, pointer);
}

GLAPI GLAPIENTRY void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    LOG()
    LOG_D("glVertexAttribLPointer(%u, %d, %s, %d, %p)", index, size, glEnumToString(type), stride, pointer)
    GLState.Lock();
    VertexAttribLPointer_State(index, size, type, stride, pointer);
    GLState.Unlock();
}

// ============================================================================
// SECTION 10: VAO Lifecycle Functions (glGenVertexArrays, glDeleteVertexArrays,
//             glIsVertexArray, glBindVertexArray, glCreateVertexArrays)
// ============================================================================

// VAO helper functions are declared in buffer.h and defined in buffer.cpp.
// We re-implement them here following the DirectGLES pattern for consistency.

// --- glGenVertexArrays ---

static void GenVertexArrays_Backend(GLsizei n, GLuint* /*arrays*/) {
    // No GLES call needed; real VAOs are lazily created on first bind
    (void)n;
}

static void GenVertexArrays_State(GLsizei n, GLuint* arrays) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenVertexArrays: n < 0"));
        return;
    }
    if (!arrays) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenVertexArrays: arrays is null"));
        return;
    }
    for (int i = 0; i < n; ++i) {
        arrays[i] = gen_array();
    }
    GenVertexArrays_Backend(n, arrays);
}

GLAPI GLAPIENTRY void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    LOG()
    LOG_D("glGenVertexArrays(%i, %p)", n, arrays)
    GLState.Lock();
    GenVertexArrays_State(n, arrays);
    GLState.Unlock();
}

// --- glDeleteVertexArrays ---

static void DeleteVertexArrays_Backend(GLsizei n, const GLuint* arrays) {
    for (int i = 0; i < n; ++i) {
        if (has_array(arrays[i]) && arrays[i] != 0) {
            GLuint real_array = find_real_array(arrays[i]);
            if (real_array) {
                GLES.glDeleteVertexArrays(1, &real_array);
                CHECK_GL_ERROR
            }
        }
    }
}

static void DeleteVertexArrays_State(GLsizei n, const GLuint* arrays) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteVertexArrays: n < 0"));
        return;
    }
    if (!arrays) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteVertexArrays: arrays is null"));
        return;
    }
    DeleteVertexArrays_Backend(n, arrays);
    for (int i = 0; i < n; ++i) {
        remove_array(arrays[i]);
    }
}

GLAPI GLAPIENTRY void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    LOG()
    LOG_D("glDeleteVertexArrays(%i, %p)", n, arrays)
    GLState.Lock();
    DeleteVertexArrays_State(n, arrays);
    GLState.Unlock();
}

// --- glIsVertexArray ---

GLAPI GLAPIENTRY GLboolean glIsVertexArray(GLuint array) {
    LOG()
    LOG_D("glIsVertexArray(%d)", array)
    GLState.Lock();
    GLboolean result = has_array(array);
    GLState.Unlock();
    return result;
}

// --- glBindVertexArray ---

static void BindVertexArray_Backend(GLuint array) {
    if (!has_array(array) || array == 0) [[unlikely]] {
        LOG_D("Does not have va=%d found!", array)
        GLES.glBindVertexArray(array);
        CHECK_GL_ERROR
        return;
    }

    GLuint real_array = find_real_array(array);
    if (!real_array) {
        LOG_D("va=%d not initialized, initializing...", array)
        GLES.glGenVertexArrays(1, &real_array);
        modify_array(array, real_array);
        CHECK_GL_ERROR
    }
    LOG_D("glBindVertexArray: %d -> %d", array, real_array)
    GLES.glBindVertexArray(real_array);
    CHECK_GL_ERROR
}

static void BindVertexArray_State(GLuint array) {
    // Update CPU-side VAO tracking
    GLState.vertexArray.currentVAO = array;
    BindVertexArray_Backend(array);
}

GLAPI GLAPIENTRY void glBindVertexArray(GLuint array) {
    LOG()
    LOG_D("glBindVertexArray(%d)", array)
    GLState.Lock();
    BindVertexArray_State(array);
    GLState.Unlock();
}

// --- glCreateVertexArrays (DSA-style: creates VAO without binding) ---

static void CreateVertexArrays_Backend(GLsizei n, GLuint* arrays) {
    for (GLsizei i = 0; i < n; ++i) {
        if (arrays[i] == 0) continue;
        if (!has_array(arrays[i])) continue;
        GLuint real_array = find_real_array(arrays[i]);
        if (!real_array) {
            // Create the real VAO by temporarily binding
            GLuint prev_vao = 0;
            GLES.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&prev_vao);
            GLES.glGenVertexArrays(1, &real_array);
            modify_array(arrays[i], real_array);
            GLES.glBindVertexArray(real_array);
            CHECK_GL_ERROR
            GLES.glBindVertexArray(prev_vao);
        }
    }
}

static void CreateVertexArrays_State(GLsizei n, GLuint* arrays) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCreateVertexArrays: n < 0"));
        return;
    }
    if (!arrays) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCreateVertexArrays: arrays is null"));
        return;
    }
    // Generate virtual IDs first
    for (GLsizei i = 0; i < n; ++i) {
        arrays[i] = gen_array();
    }
    CreateVertexArrays_Backend(n, arrays);
}

GLAPI GLAPIENTRY void glCreateVertexArrays(GLsizei n, GLuint* arrays) {
    LOG()
    LOG_D("glCreateVertexArrays(%d, %p)", n, arrays)
    GLState.Lock();
    CreateVertexArrays_State(n, arrays);
    GLState.Unlock();
}

// ============================================================================
// SECTION 11: DSA Vertex Array Functions
// ============================================================================

// --- glDisableVertexArrayAttrib ---

static void DisableVertexArrayAttrib_Backend(GLuint vaobj, GLuint index) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLES.glDisableVertexAttribArray(index);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void DisableVertexArrayAttrib_State(GLuint vaobj, GLuint index) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDisableVertexArrayAttrib: vaobj is 0"));
        return;
    }
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDisableVertexArrayAttrib: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    DisableVertexArrayAttrib_Backend(vaobj, index);
}

GLAPI GLAPIENTRY void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    LOG()
    LOG_D("glDisableVertexArrayAttrib(%u, %u)", vaobj, index)
    GLState.Lock();
    DisableVertexArrayAttrib_State(vaobj, index);
    GLState.Unlock();
}

// --- glEnableVertexArrayAttrib ---

static void EnableVertexArrayAttrib_Backend(GLuint vaobj, GLuint index) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLES.glEnableVertexAttribArray(index);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void EnableVertexArrayAttrib_State(GLuint vaobj, GLuint index) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glEnableVertexArrayAttrib: vaobj is 0"));
        return;
    }
    if (!isValidAttribIndex(index)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glEnableVertexArrayAttrib: index >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    EnableVertexArrayAttrib_Backend(vaobj, index);
}

GLAPI GLAPIENTRY void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    LOG()
    LOG_D("glEnableVertexArrayAttrib(%u, %u)", vaobj, index)
    GLState.Lock();
    EnableVertexArrayAttrib_State(vaobj, index);
    GLState.Unlock();
}

// --- glVertexArrayElementBuffer ---

static void VertexArrayElementBuffer_Backend(GLuint vaobj, GLuint buffer) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLuint real_buffer = getRealBuffer(buffer);
    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, real_buffer ? real_buffer : buffer);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void VertexArrayElementBuffer_State(GLuint vaobj, GLuint buffer) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayElementBuffer: vaobj is 0"));
        return;
    }
    VertexArrayElementBuffer_Backend(vaobj, buffer);
}

GLAPI GLAPIENTRY void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
    LOG()
    LOG_D("glVertexArrayElementBuffer(%u, %u)", vaobj, buffer)
    GLState.Lock();
    VertexArrayElementBuffer_State(vaobj, buffer);
    GLState.Unlock();
}

// --- glVertexArrayVertexBuffer ---

static void VertexArrayVertexBuffer_Backend(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLuint real_buffer = getRealBuffer(buffer);
    GLES.glBindVertexBuffer(bindingindex, real_buffer ? real_buffer : buffer, offset, stride);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void VertexArrayVertexBuffer_State(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayVertexBuffer: vaobj is 0"));
        return;
    }
    if (bindingindex >= MAX_VERTEX_ATTRIBS) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayVertexBuffer: bindingindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (stride < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayVertexBuffer: stride < 0"));
        return;
    }
    if (offset < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayVertexBuffer: offset < 0"));
        return;
    }
    VertexArrayVertexBuffer_Backend(vaobj, bindingindex, buffer, offset, stride);
}

GLAPI GLAPIENTRY void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    LOG()
    LOG_D("glVertexArrayVertexBuffer(%u, %u, %u, %lld, %d)", vaobj, bindingindex, buffer, (long long)offset, stride)
    GLState.Lock();
    VertexArrayVertexBuffer_State(vaobj, bindingindex, buffer, offset, stride);
    GLState.Unlock();
}

// --- glVertexArrayAttribFormat ---

static void VertexArrayAttribFormat_Backend(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLES.glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void VertexArrayAttribFormat_State(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribFormat: vaobj is 0"));
        return;
    }
    if (!isValidAttribIndex(attribindex)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribFormat: attribindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribFormat: invalid size"));
        return;
    }
    if (!isValidAttribFormatType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribFormat: invalid type"));
        return;
    }
    VertexArrayAttribFormat_Backend(vaobj, attribindex, size, type, normalized, relativeoffset);
}

GLAPI GLAPIENTRY void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    LOG()
    LOG_D("glVertexArrayAttribFormat(%u, %u, %d, %s, %d, %u)", vaobj, attribindex, size, glEnumToString(type), normalized, relativeoffset)
    GLState.Lock();
    VertexArrayAttribFormat_State(vaobj, attribindex, size, type, normalized, relativeoffset);
    GLState.Unlock();
}

// --- glVertexArrayAttribIFormat ---

static void VertexArrayAttribIFormat_Backend(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GLuint prev_vao = GLState.vertexArray.currentVAO;
    GLuint real_vaobj = getRealVAO(vaobj);
    if (real_vaobj) {
        GLES.glBindVertexArray(real_vaobj);
    } else {
        GLES.glBindVertexArray(vaobj);
    }
    CHECK_GL_ERROR
    GLES.glVertexAttribIFormat(attribindex, size, type, relativeoffset);
    CHECK_GL_ERROR
    // Restore previous VAO
    if (prev_vao != vaobj) {
        GLuint real_prev = getRealVAO(prev_vao);
        GLES.glBindVertexArray(real_prev ? real_prev : prev_vao);
        CHECK_GL_ERROR
    }
}

static void VertexArrayAttribIFormat_State(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    if (vaobj == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribIFormat: vaobj is 0"));
        return;
    }
    if (!isValidAttribIndex(attribindex)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribIFormat: attribindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (!isValidAttribSize(size)) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribIFormat: invalid size"));
        return;
    }
    if (!isValidAttribIFormatType(type)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glVertexArrayAttribIFormat: invalid type"));
        return;
    }
    VertexArrayAttribIFormat_Backend(vaobj, attribindex, size, type, relativeoffset);
}

GLAPI GLAPIENTRY void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    LOG()
    LOG_D("glVertexArrayAttribIFormat(%u, %u, %d, %s, %u)", vaobj, attribindex, size, glEnumToString(type), relativeoffset)
    GLState.Lock();
    VertexArrayAttribIFormat_State(vaobj, attribindex, size, type, relativeoffset);
    GLState.Unlock();
}

// ============================================================================
// SECTION 12: Bind Vertex Buffer (DSA-style and direct)
// ============================================================================

// --- glBindVertexBuffer (direct binding, operates on current VAO) ---

static void BindVertexBuffer_Backend(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    if (buffer == 0) [[unlikely]] {
        GLES.glBindVertexBuffer(bindingindex, 0, offset, stride);
        CHECK_GL_ERROR
        return;
    }
    GLuint real_buffer = getRealBuffer(buffer);
    if (!real_buffer) {
        GLES.glBindVertexBuffer(bindingindex, buffer, offset, stride);
        CHECK_GL_ERROR
        return;
    }
    GLES.glBindVertexBuffer(bindingindex, real_buffer, offset, stride);
    CHECK_GL_ERROR
}

static void BindVertexBuffer_State(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    if (bindingindex >= MAX_VERTEX_ATTRIBS) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindVertexBuffer: bindingindex >= GL_MAX_VERTEX_ATTRIBS"));
        return;
    }
    if (stride < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindVertexBuffer: stride < 0"));
        return;
    }
    if (offset < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBindVertexBuffer: offset < 0"));
        return;
    }
    BindVertexBuffer_Backend(bindingindex, buffer, offset, stride);
}

GLAPI GLAPIENTRY void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    LOG()
    LOG_D("glBindVertexBuffer(%u, %u, %lld, %d)", bindingindex, buffer, (long long)offset, stride)
    GLState.Lock();
    BindVertexBuffer_State(bindingindex, buffer, offset, stride);
    GLState.Unlock();
}