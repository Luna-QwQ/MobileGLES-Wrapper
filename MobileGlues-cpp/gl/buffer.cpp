// MobileGlues - gl/buffer.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

// ============================================================================
// ES 3.2 native → native, ES 3.2 not native → CPU simulation
// Architecture: DirectGLES — public wrappers → _State (validation + state) → _Backend (GLES call)
// ============================================================================

#include "buffer.h"
#include "ankerl/unordered_dense.h"
#include "texture.h"

#define DEBUG 0

// ============================================================================
// Internal State: Buffer Map Management
// ============================================================================

static GLint maxBufferId = 0;
static GLint maxArrayId = 0;

static std::vector<GLuint> g_free_buffer_ids;
static std::vector<GLuint> g_free_array_ids;

// Per-VAO element array buffer tracking (virtual VAO → virtual IBO)
static std::vector<GLuint> g_element_array_buffer_per_vao;

// ============================================================================
// Binding Index Helpers (for backward-compatible set_bound_buffer_by_target)
// ============================================================================

enum BindingIndex : int {
    BI_ARRAY_BUFFER = 0,
    BI_ATOMIC_COUNTER,
    BI_COPY_READ,
    BI_COPY_WRITE,
    BI_DRAW_INDIRECT,
    BI_DISPATCH_INDIRECT,
    BI_ELEMENT_ARRAY,
    BI_PIXEL_PACK,
    BI_PIXEL_UNPACK,
    BI_SHADER_STORAGE,
    BI_TRANSFORM_FEEDBACK,
    BI_UNIFORM_BUFFER,
    BINDING_COUNT
};

static inline int binding_target_to_index(GLenum target) {
    switch (target) {
    case GL_ARRAY_BUFFER:             return BI_ARRAY_BUFFER;
    case GL_ATOMIC_COUNTER_BUFFER:    return BI_ATOMIC_COUNTER;
    case GL_COPY_READ_BUFFER:         return BI_COPY_READ;
    case GL_COPY_WRITE_BUFFER:        return BI_COPY_WRITE;
    case GL_DRAW_INDIRECT_BUFFER:     return BI_DRAW_INDIRECT;
    case GL_DISPATCH_INDIRECT_BUFFER: return BI_DISPATCH_INDIRECT;
    case GL_ELEMENT_ARRAY_BUFFER:     return BI_ELEMENT_ARRAY;
    case GL_PIXEL_PACK_BUFFER:        return BI_PIXEL_PACK;
    case GL_PIXEL_UNPACK_BUFFER:      return BI_PIXEL_UNPACK;
    case GL_SHADER_STORAGE_BUFFER:    return BI_SHADER_STORAGE;
    case GL_TRANSFORM_FEEDBACK_BUFFER:return BI_TRANSFORM_FEEDBACK;
    case GL_UNIFORM_BUFFER:           return BI_UNIFORM_BUFFER;
    default:                          return -1;
    }
}

// ============================================================================
// Buffer Map Helpers: Capacity & Lifecycle (backward-compatible API)
// ============================================================================

static inline void ensure_array_capacity(GLuint id) {
    if (id < (GLuint)g_element_array_buffer_per_vao.size()) [[likely]] return;
    g_element_array_buffer_per_vao.resize(id + 1, 0);
}

GLuint gen_buffer() {
    GLuint id;
    if (!g_free_buffer_ids.empty()) {
        id = g_free_buffer_ids.back();
        g_free_buffer_ids.pop_back();
    } else {
        id = (GLuint)(++maxBufferId);
    }
    GLState.buffer.bufferMap[id] = 0;
    GLState.buffer.bufferInfo[id] = MGBufferInfo{};
    return id;
}

GLboolean has_buffer(GLuint key) {
    auto it = GLState.buffer.bufferMap.find(key);
    return (it != GLState.buffer.bufferMap.end()) ? GL_TRUE : GL_FALSE;
}

void modify_buffer(GLuint key, GLuint value) {
    GLState.buffer.bufferMap[key] = value;
    if (value != 0) {
        GLState.buffer.bufferMapReverse[value] = key;
    }
}

void remove_buffer(GLuint key) {
    auto it = GLState.buffer.bufferMap.find(key);
    if (it != GLState.buffer.bufferMap.end()) {
        GLuint realId = it->second;
        if (realId != 0) {
            GLState.buffer.bufferMapReverse.erase(realId);
        }
        GLState.buffer.bufferMap.erase(it);
        GLState.buffer.bufferInfo.erase(key);
        g_free_buffer_ids.push_back(key);
        // Unbind from all targets
        for (auto& kv : GLState.buffer.boundBuffer) {
            if (kv.second == key) kv.second = 0;
        }
    }
}

GLuint find_real_buffer(GLuint key) {
    auto it = GLState.buffer.bufferMap.find(key);
    return (it != GLState.buffer.bufferMap.end()) ? it->second : 0;
}

// Combined lookup: returns {real_buffer, exists} in a single lookup.
static inline std::pair<GLuint, bool> find_real_buffer_with_exists(GLuint key) {
    auto it = GLState.buffer.bufferMap.find(key);
    if (it != GLState.buffer.bufferMap.end()) [[likely]]
        return {it->second, true};
    return {0, false};
}

// Fast-path: direct assignment without checks, for use when the caller
// has already verified the buffer exists.
static inline void modify_buffer_direct(GLuint key, GLuint value) {
    GLState.buffer.bufferMap[key] = value;
    if (value != 0) {
        GLState.buffer.bufferMapReverse[value] = key;
    }
}

GLuint get_ibo_by_vao(GLuint vao) {
    if (vao < (GLuint)g_element_array_buffer_per_vao.size())
        return g_element_array_buffer_per_vao[vao];
    return 0;
}

GLuint find_bound_array() {
    return GLState.vertexArray.currentVAO;
}

void update_vao_ibo_binding(GLuint vao, GLuint ibo) {
    ensure_array_capacity(vao);
    g_element_array_buffer_per_vao[vao] = ibo;
}

void set_buffer_data_size(GLuint buffer, size_t size) {
    auto it = GLState.buffer.bufferInfo.find(buffer);
    if (it != GLState.buffer.bufferInfo.end()) {
        it->second.size = size;
    } else {
        GLState.buffer.bufferInfo[buffer] = MGBufferInfo{};
        GLState.buffer.bufferInfo[buffer].size = size;
    }
}

size_t get_buffer_data_size(GLuint buffer) {
    auto it = GLState.buffer.bufferInfo.find(buffer);
    if (it != GLState.buffer.bufferInfo.end()) return it->second.size;
    return 0;
}

// ============================================================================
// Buffer Binding Helpers
// ============================================================================

void set_bound_buffer_by_target(GLenum target, GLuint buffer) {
    GLState.buffer.boundBuffer[target] = buffer;
}

GLuint find_bound_buffer(GLenum key) {
    // Special case: ELEMENT_ARRAY_BUFFER uses VAO tracking
    if (key == GL_ELEMENT_ARRAY_BUFFER_BINDING) {
        return get_ibo_by_vao(GLState.vertexArray.currentVAO);
    }
    switch (key) {
    case GL_ARRAY_BUFFER_BINDING:              return GLState.buffer.boundBuffer[GL_ARRAY_BUFFER];
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:     return GLState.buffer.boundBuffer[GL_ATOMIC_COUNTER_BUFFER];
    case GL_COPY_READ_BUFFER_BINDING:          return GLState.buffer.boundBuffer[GL_COPY_READ_BUFFER];
    case GL_COPY_WRITE_BUFFER_BINDING:         return GLState.buffer.boundBuffer[GL_COPY_WRITE_BUFFER];
    case GL_DRAW_INDIRECT_BUFFER_BINDING:      return GLState.buffer.boundBuffer[GL_DRAW_INDIRECT_BUFFER];
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:  return GLState.buffer.boundBuffer[GL_DISPATCH_INDIRECT_BUFFER];
    case GL_PIXEL_PACK_BUFFER_BINDING:         return GLState.buffer.boundBuffer[GL_PIXEL_PACK_BUFFER];
    case GL_PIXEL_UNPACK_BUFFER_BINDING:       return GLState.buffer.boundBuffer[GL_PIXEL_UNPACK_BUFFER];
    case GL_SHADER_STORAGE_BUFFER_BINDING:     return GLState.buffer.boundBuffer[GL_SHADER_STORAGE_BUFFER];
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING: return GLState.buffer.boundBuffer[GL_TRANSFORM_FEEDBACK_BUFFER];
    case GL_UNIFORM_BUFFER_BINDING:            return GLState.buffer.boundBuffer[GL_UNIFORM_BUFFER];
    default:                                   return 0;
    }
}

static GLenum get_binding_query(GLenum target) {
    switch (target) {
    case GL_ARRAY_BUFFER:              return GL_ARRAY_BUFFER_BINDING;
    case GL_ELEMENT_ARRAY_BUFFER:      return GL_ELEMENT_ARRAY_BUFFER_BINDING;
    case GL_PIXEL_PACK_BUFFER:         return GL_PIXEL_PACK_BUFFER_BINDING;
    case GL_PIXEL_UNPACK_BUFFER:       return GL_PIXEL_UNPACK_BUFFER_BINDING;
    case GL_UNIFORM_BUFFER:            return GL_UNIFORM_BUFFER_BINDING;
    case GL_TRANSFORM_FEEDBACK_BUFFER: return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
    case GL_COPY_READ_BUFFER:          return GL_COPY_READ_BUFFER_BINDING;
    case GL_COPY_WRITE_BUFFER:         return GL_COPY_WRITE_BUFFER_BINDING;
    case GL_DRAW_INDIRECT_BUFFER:      return GL_DRAW_INDIRECT_BUFFER_BINDING;
    case GL_SHADER_STORAGE_BUFFER:     return GL_SHADER_STORAGE_BUFFER_BINDING;
    case GL_DISPATCH_INDIRECT_BUFFER:  return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
    case GL_ATOMIC_COUNTER_BUFFER:     return GL_ATOMIC_COUNTER_BUFFER_BINDING;
    default:                           return 0;
    }
}

// ============================================================================
// Vertex Array Map Helpers (backward-compatible API)
// ============================================================================

GLuint gen_array() {
    GLuint id;
    if (!g_free_array_ids.empty()) {
        id = g_free_array_ids.back();
        g_free_array_ids.pop_back();
    } else {
        id = (GLuint)(++maxArrayId);
    }
    ensure_array_capacity(id);
    GLState.buffer.vaoMap[id] = 0;
    g_element_array_buffer_per_vao[id] = 0;
    return id;
}

GLboolean has_array(GLuint key) {
    auto it = GLState.buffer.vaoMap.find(key);
    return (it != GLState.buffer.vaoMap.end()) ? GL_TRUE : GL_FALSE;
}

void modify_array(GLuint key, GLuint value) {
    GLState.buffer.vaoMap[key] = value;
    if (value != 0) {
        GLState.buffer.vaoMapReverse[value] = key;
    }
}

void remove_array(GLuint key) {
    auto it = GLState.buffer.vaoMap.find(key);
    if (it != GLState.buffer.vaoMap.end()) {
        GLuint realId = it->second;
        if (realId != 0) {
            GLState.buffer.vaoMapReverse.erase(realId);
        }
        GLState.buffer.vaoMap.erase(it);
        if (key < (GLuint)g_element_array_buffer_per_vao.size())
            g_element_array_buffer_per_vao[key] = 0;
        g_free_array_ids.push_back(key);
        if (GLState.vertexArray.currentVAO == key)
            GLState.vertexArray.currentVAO = 0;
    }
}

GLuint find_real_array(GLuint key) {
    auto it = GLState.buffer.vaoMap.find(key);
    return (it != GLState.buffer.vaoMap.end()) ? it->second : 0;
}

// ============================================================================
// Buffer Target Validation
// ============================================================================

static bool is_valid_buffer_target(GLenum target) {
    return binding_target_to_index(target) >= 0;
}

// ============================================================================
// Map Initialization
// ============================================================================

void InitBufferMap(size_t expectedSize) {
    // Reserve space in GLState maps (UnorderedMap doesn't have reserve, but we pre-allocate the free list)
    g_free_buffer_ids.reserve(expectedSize + 2);
}

void InitVertexArrayMap(size_t expectedSize) {
    g_free_array_ids.reserve(expectedSize + 2);
    g_element_array_buffer_per_vao.reserve(expectedSize + 2);
    g_element_array_buffer_per_vao.resize(1, 0);
}

// ============================================================================
// Atomic Counter Buffer Emulation State
// ============================================================================

struct atomic_buffer {
    GLuint id;
    GLsizeiptr size;
    GLintptr offset;
};

static std::vector<atomic_buffer> g_buffer_map_atomic_buffer_info;
static std::vector<GLuint> g_buffer_map_ssbo_id;

void bindAllAtomicCounterAsSSBO() {
    const size_t count = g_buffer_map_atomic_buffer_info.size();
    for (size_t i = 0; i < count; ++i) {
        atomic_buffer buf = g_buffer_map_atomic_buffer_info[i];
        if (buf.id != 0) {
            GLuint realID = find_real_buffer(buf.id);
            GLES.glBindBufferRange(GL_SHADER_STORAGE_BUFFER, (GLuint)i, realID, buf.offset, buf.size);
            LOG_D("Bound atomic counter buffer %u(real: %u) as SSBO at index %zu", buf.id, realID, i);
        }
    }
}

// ============================================================================
// Buffer Object Lifecycle — Backend (ES 3.2 native)
// ============================================================================

static void GenBuffers_Backend(GLsizei n, GLuint* /*buffers*/) {
    // No GLES call needed here; real IDs are lazily created on first bind
    (void)n;
}

static void DeleteBuffers_Backend(GLsizei n, const GLuint* buffers) {
    for (int i = 0; i < n; ++i) {
        auto [real_buff, exists] = find_real_buffer_with_exists(buffers[i]);
        if (exists && real_buff != 0) {
            GLES.glDeleteBuffers(1, &real_buff);
            CHECK_GL_ERROR
        }
    }
}

// ============================================================================
// Buffer Object Lifecycle — State (validation + state management)
// ============================================================================

static void GenBuffers_State(GLsizei n, GLuint* buffers) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenBuffers: n < 0"));
        return;
    }
    if (!buffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGenBuffers: buffers is null"));
        return;
    }
    for (int i = 0; i < n; ++i) {
        buffers[i] = gen_buffer();
    }
    GenBuffers_Backend(n, buffers);
}

static void DeleteBuffers_State(GLsizei n, const GLuint* buffers) {
    if (n < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteBuffers: n < 0"));
        return;
    }
    if (!buffers) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glDeleteBuffers: buffers is null"));
        return;
    }
    DeleteBuffers_Backend(n, buffers);
    for (int i = 0; i < n; ++i) {
        remove_buffer(buffers[i]);
    }
}

// ============================================================================
// Buffer Object Lifecycle — Public Wrappers
// ============================================================================

void glGenBuffers(GLsizei n, GLuint* buffers) {
    LOG()
    LOG_D("glGenBuffers(%i, %p)", n, buffers)
    GLState.Lock();
    GenBuffers_State(n, buffers);
    GLState.Unlock();
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    LOG()
    LOG_D("glDeleteBuffers(%i, %p)", n, buffers)
    GLState.Lock();
    DeleteBuffers_State(n, buffers);
    GLState.Unlock();
}

GLboolean glIsBuffer(GLuint buffer) {
    LOG()
    LOG_D("glIsBuffer, buffer = %d", buffer)
    GLState.Lock();
    GLboolean result = has_buffer(buffer);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Buffer Binding — Backend
// ============================================================================

static void BindBuffer_Backend(GLenum target, GLuint buffer) {
    if (buffer == 0) [[unlikely]] {
        GLES.glBindBuffer(target, 0);
        CHECK_GL_ERROR
        return;
    }
    auto [real_buffer, exists] = find_real_buffer_with_exists(buffer);
    if (!exists) [[unlikely]] {
        GLES.glBindBuffer(target, buffer);
        CHECK_GL_ERROR
        return;
    }
    if (!real_buffer) {
        GLES.glGenBuffers(1, &real_buffer);
        modify_buffer_direct(buffer, real_buffer);
        CHECK_GL_ERROR
    }
    LOG_D("glBindBuffer: %d -> %d", buffer, real_buffer)
    GLES.glBindBuffer(target, real_buffer);
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Binding — State
// ============================================================================

static void BindBuffer_State(GLenum target, GLuint buffer) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBindBuffer: invalid target"));
        return;
    }
    // Update CPU-side binding state
    GLState.buffer.boundBuffer[target] = buffer;
    // Save IBO binding to VAO
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        update_vao_ibo_binding(GLState.vertexArray.currentVAO, buffer);
    }
    BindBuffer_Backend(target, buffer);
}

// ============================================================================
// Buffer Binding — Public
// ============================================================================

void glBindBuffer(GLenum target, GLuint buffer) {
    LOG()
    LOG_D("glBindBuffer, target = %s, buffer = %d", glEnumToString(target), buffer)
    GLState.Lock();
    BindBuffer_State(target, buffer);
    GLState.Unlock();
}

// ============================================================================
// Buffer Data Upload — Backend (ES 3.2 native)
// ============================================================================

static void BufferData_Backend(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    GLES.glBufferData(target, size, data, usage);
    CHECK_GL_ERROR
}

static void BufferSubData_Backend(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    GLES.glBufferSubData(target, offset, size, data);
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Data Upload — State
// ============================================================================

static void BufferData_State(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBufferData: invalid target"));
        return;
    }
    if (size < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBufferData: size < 0"));
        return;
    }
    // Update buffer info in state
    GLuint boundBuffer = GLState.buffer.boundBuffer[target];
    if (boundBuffer != 0) {
        set_buffer_data_size(boundBuffer, (size_t)size);
    }
    BufferData_Backend(target, size, data, usage);
}

static void BufferSubData_State(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBufferSubData: invalid target"));
        return;
    }
    if (offset < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBufferSubData: offset < 0"));
        return;
    }
    if (size < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBufferSubData: size < 0"));
        return;
    }
    BufferSubData_Backend(target, offset, size, data);
}

// ============================================================================
// Buffer Data Upload — Public
// ============================================================================

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    LOG()
    LOG_D("glBufferData, target = %s, size = %d, data = 0x%x, usage = %s", glEnumToString(target), size, data,
          glEnumToString(usage))
    GLState.Lock();
    BufferData_State(target, size, data, usage);
    GLState.Unlock();
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    LOG()
    LOG_D("glBufferSubData, target = %s, offset = %d, size = %d, data = %p", glEnumToString(target), offset, size, data)
    GLState.Lock();
    BufferSubData_State(target, offset, size, data);
    GLState.Unlock();
}

// ============================================================================
// Buffer Mapping — Backend (ES 3.2 native)
// ============================================================================

static void* MapBuffer_Backend(GLenum target, GLenum access) {
    if (g_gles_caps.GL_OES_mapbuffer) {
        return GLES.glMapBufferOES(target, access);
    }
    GLint buffer_size;
    GLES.glGetBufferParameteriv(target, GL_BUFFER_SIZE, &buffer_size);
    if (buffer_size <= 0) {
        return nullptr;
    }
    GLbitfield flags = 0;
    switch (access) {
    case GL_READ_ONLY:  flags = GL_MAP_READ_BIT;                              break;
    case GL_WRITE_ONLY: flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT; break;
    case GL_READ_WRITE: flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;            break;
    default:            return nullptr;
    }
    return GLES.glMapBufferRange(target, 0, (GLsizeiptr)buffer_size, flags);
}

static void* MapBufferRange_Backend(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    if (global_settings.buffer_coherent_as_flush) access &= ~GL_MAP_FLUSH_EXPLICIT_BIT;
    return GLES.glMapBufferRange(target, offset, length, access);
}

static GLboolean UnmapBuffer_Backend(GLenum target) {
    if (g_gles_caps.GL_OES_mapbuffer) return GLES.glUnmapBuffer(target);
    GLboolean result = GLES.glUnmapBuffer(target);
    CHECK_GL_ERROR
    return result;
}

static void FlushMappedBufferRange_Backend(GLenum target, GLintptr offset, GLsizeiptr length) {
    if (!global_settings.buffer_coherent_as_flush) GLES.glFlushMappedBufferRange(target, offset, length);
}

// ============================================================================
// Buffer Mapping — State
// ============================================================================

static void* MapBuffer_State(GLenum target, GLenum access) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glMapBuffer: invalid target"));
        return nullptr;
    }
    if (access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glMapBuffer: invalid access"));
        return nullptr;
    }
    return MapBuffer_Backend(target, access);
}

static void* MapBufferRange_State(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glMapBufferRange: invalid target"));
        return nullptr;
    }
    if (offset < 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMapBufferRange: offset < 0"));
        return nullptr;
    }
    if (length <= 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glMapBufferRange: length <= 0"));
        return nullptr;
    }
    return MapBufferRange_Backend(target, offset, length, access);
}

static GLboolean UnmapBuffer_State(GLenum target) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glUnmapBuffer: invalid target"));
        return GL_FALSE;
    }
    return UnmapBuffer_Backend(target);
}

static void FlushMappedBufferRange_State(GLenum target, GLintptr offset, GLsizeiptr length) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glFlushMappedBufferRange: invalid target"));
        return;
    }
    FlushMappedBufferRange_Backend(target, offset, length);
}

// ============================================================================
// Buffer Mapping — Public
// ============================================================================

void* glMapBuffer(GLenum target, GLenum access) {
    LOG()
    LOG_D("glMapBuffer, target = %s, access = %s", glEnumToString(target), glEnumToString(access))
    GLState.Lock();
    void* result = MapBuffer_State(target, access);
    GLState.Unlock();
    return result;
}

void* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    LOG()
    GLState.Lock();
    void* result = MapBufferRange_State(target, offset, length, access);
    GLState.Unlock();
    return result;
}

GLboolean glUnmapBuffer(GLenum target) {
    LOG()
    LOG_D("%s(%s)", __func__, glEnumToString(target));
    GLState.Lock();
    GLboolean result = UnmapBuffer_State(target);
    GLState.Unlock();
    return result;
}

void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    LOG()
    GLState.Lock();
    FlushMappedBufferRange_State(target, offset, length);
    GLState.Unlock();
}

// ============================================================================
// Buffer Range / Base Binding — Backend
// ============================================================================

static void BindBufferRange_Backend(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (buffer == 0) {
        GLES.glBindBufferRange(target, index, 0, offset, size);
        CHECK_GL_ERROR
        return;
    }
    auto [real_buffer, exists] = find_real_buffer_with_exists(buffer);
    if (!exists) {
        GLES.glBindBufferRange(target, index, buffer, offset, size);
        CHECK_GL_ERROR
        return;
    }
    if (!real_buffer) {
        GLES.glGenBuffers(1, &real_buffer);
        modify_buffer_direct(buffer, real_buffer);
        CHECK_GL_ERROR
    }
    GLES.glBindBufferRange(target, index, real_buffer, offset, size);
    CHECK_GL_ERROR
}

static void BindBufferBase_Backend(GLenum target, GLuint index, GLuint buffer) {
    if (buffer == 0) {
        GLES.glBindBufferBase(target, index, 0);
        CHECK_GL_ERROR
        return;
    }
    auto [real_buffer, exists] = find_real_buffer_with_exists(buffer);
    if (!exists) {
        GLES.glBindBufferBase(target, index, buffer);
        CHECK_GL_ERROR
        return;
    }
    if (!real_buffer) {
        GLES.glGenBuffers(1, &real_buffer);
        modify_buffer_direct(buffer, real_buffer);
        CHECK_GL_ERROR
    }
    GLES.glBindBufferBase(target, index, real_buffer);
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Range / Base Binding — State
// ============================================================================

static void BindBufferRange_State(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (!is_valid_buffer_target(target) && target != GL_ATOMIC_COUNTER_BUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBindBufferRange: invalid target"));
        return;
    }
    // Atomic counter buffer emulation
    if (target == GL_ATOMIC_COUNTER_BUFFER) {
        if (g_buffer_map_atomic_buffer_info.empty()) {
            g_buffer_map_atomic_buffer_info.resize(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, {});
        }
        g_buffer_map_atomic_buffer_info[index] = {buffer, size, offset};
    }
    BindBufferRange_Backend(target, index, buffer, offset, size);
}

static void BindBufferBase_State(GLenum target, GLuint index, GLuint buffer) {
    if (!is_valid_buffer_target(target) && target != GL_ATOMIC_COUNTER_BUFFER) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBindBufferBase: invalid target"));
        return;
    }
    // Track SSBO bindings
    if (target == GL_SHADER_STORAGE_BUFFER) {
        if (g_buffer_map_ssbo_id.empty()) {
            g_buffer_map_ssbo_id.resize(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, 0);
        }
        if (index < (GLuint)g_buffer_map_ssbo_id.size()) {
            g_buffer_map_ssbo_id[index] = buffer;
        }
    }
    BindBufferBase_Backend(target, index, buffer);
}

// ============================================================================
// Buffer Range / Base Binding — Public
// ============================================================================

void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    LOG()
    LOG_D("glBindBufferRange, target = %s, index = %d, buffer = %d, offset = %p, size = %zi", glEnumToString(target),
          index, buffer, (void*)offset, size)
    GLState.Lock();
    BindBufferRange_State(target, index, buffer, offset, size);
    GLState.Unlock();
}

void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    LOG()
    LOG_D("glBindBufferBase, target = %s, index = %d, buffer = %d", glEnumToString(target), index, buffer)
    GLState.Lock();
    BindBufferBase_State(target, index, buffer);
    GLState.Unlock();
}

// ============================================================================
// Vertex Buffer Binding — Backend
// ============================================================================

static void BindVertexBuffer_Backend(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    if (buffer == 0) [[unlikely]] {
        GLES.glBindVertexBuffer(bindingindex, 0, offset, stride);
        CHECK_GL_ERROR
        return;
    }
    auto [real_buffer, exists] = find_real_buffer_with_exists(buffer);
    if (!exists) [[unlikely]] {
        GLES.glBindVertexBuffer(bindingindex, buffer, offset, stride);
        CHECK_GL_ERROR
        return;
    }
    if (!real_buffer) {
        GLES.glGenBuffers(1, &real_buffer);
        modify_buffer_direct(buffer, real_buffer);
        CHECK_GL_ERROR
    }
    GLES.glBindVertexBuffer(bindingindex, real_buffer, offset, stride);
    CHECK_GL_ERROR
}

// ============================================================================
// Vertex Buffer Binding — State
// ============================================================================

static void BindVertexBuffer_State(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    BindVertexBuffer_Backend(bindingindex, buffer, offset, stride);
}

// ============================================================================
// Vertex Buffer Binding — Public
// ============================================================================

void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    LOG()
    LOG_D("glBindVertexBuffer, bindingindex = %d, buffer = %d, offset = %p, stride = %i", bindingindex, buffer, offset,
          stride)
    GLState.Lock();
    BindVertexBuffer_State(bindingindex, buffer, offset, stride);
    GLState.Unlock();
}

// ============================================================================
// Buffer Copy — Backend (ES 3.2 native)
// ============================================================================

static void CopyBufferSubData_Backend(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset,
                                      GLsizeiptr size) {
    GLES.glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Copy — State
// ============================================================================

static void CopyBufferSubData_State(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset,
                                    GLsizeiptr size) {
    if (!is_valid_buffer_target(readTarget)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glCopyBufferSubData: invalid readTarget"));
        return;
    }
    if (!is_valid_buffer_target(writeTarget)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glCopyBufferSubData: invalid writeTarget"));
        return;
    }
    CopyBufferSubData_Backend(readTarget, writeTarget, readOffset, writeOffset, size);
}

// ============================================================================
// Buffer Copy — Public
// ============================================================================

void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    LOG()
    LOG_D("glCopyBufferSubData, readTarget = %s, writeTarget = %s, readOffset = %d, writeOffset = %d, size = %d",
          glEnumToString(readTarget), glEnumToString(writeTarget), readOffset, writeOffset, size)
    GLState.Lock();
    CopyBufferSubData_State(readTarget, writeTarget, readOffset, writeOffset, size);
    GLState.Unlock();
}

// ============================================================================
// Buffer Parameter Queries — Backend (ES 3.2 native)
// ============================================================================

static void GetBufferParameteriv_Backend(GLenum target, GLenum pname, GLint* params) {
    GLES.glGetBufferParameteriv(target, pname, params);
    CHECK_GL_ERROR
}

static void GetBufferParameteri64v_Backend(GLenum target, GLenum pname, GLint64* params) {
    GLES.glGetBufferParameteri64v(target, pname, params);
    CHECK_GL_ERROR
}

static void GetBufferPointerv_Backend(GLenum target, GLenum pname, void** params) {
    GLES.glGetBufferPointerv(target, pname, params);
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Parameter Queries — State
// ============================================================================

static void GetBufferParameteriv_State(GLenum target, GLenum pname, GLint* params) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetBufferParameteriv: invalid target"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetBufferParameteriv: params is null"));
        return;
    }
    GetBufferParameteriv_Backend(target, pname, params);
}

static void GetBufferParameteri64v_State(GLenum target, GLenum pname, GLint64* params) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetBufferParameteri64v: invalid target"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetBufferParameteri64v: params is null"));
        return;
    }
    GetBufferParameteri64v_Backend(target, pname, params);
}

static void GetBufferPointerv_State(GLenum target, GLenum pname, void** params) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glGetBufferPointerv: invalid target"));
        return;
    }
    if (!params) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glGetBufferPointerv: params is null"));
        return;
    }
    GetBufferPointerv_Backend(target, pname, params);
}

// ============================================================================
// Buffer Parameter Queries — Public
// ============================================================================

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetBufferParameteriv, target = %s, pname = %s", glEnumToString(target), glEnumToString(pname))
    GLState.Lock();
    GetBufferParameteriv_State(target, pname, params);
    GLState.Unlock();
}

void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params) {
    LOG()
    LOG_D("glGetBufferParameteri64v, target = %s, pname = %s", glEnumToString(target), glEnumToString(pname))
    GLState.Lock();
    GetBufferParameteri64v_State(target, pname, params);
    GLState.Unlock();
}

void glGetBufferPointerv(GLenum target, GLenum pname, void** params) {
    LOG()
    LOG_D("glGetBufferPointerv, target = %s, pname = %s", glEnumToString(target), glEnumToString(pname))
    GLState.Lock();
    GetBufferPointerv_State(target, pname, params);
    GLState.Unlock();
}

// ============================================================================
// Buffer Storage — Backend (ES 3.2 native via EXT)
// ============================================================================

static void BufferStorage_Backend(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    if (GLES.glBufferStorageEXT) {
        if (global_settings.buffer_coherent_as_flush &&
            ((flags & GL_MAP_PERSISTENT_BIT) != 0 || (flags & GL_DYNAMIC_STORAGE_BIT) != 0))
            flags |= (GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
        GLES.glBufferStorageEXT(target, size, data, flags);
    }
    CHECK_GL_ERROR
}

// ============================================================================
// Buffer Storage — State
// ============================================================================

static void BufferStorage_State(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    if (!is_valid_buffer_target(target)) {
        GLState.errorState.RecordError(ErrorCode::InvalidEnum,
            std::make_unique<GenericErrorInfo>("glBufferStorage: invalid target"));
        return;
    }
    if (size <= 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glBufferStorage: size <= 0"));
        return;
    }
    // Update buffer info in state
    GLuint boundBuffer = GLState.buffer.boundBuffer[target];
    if (boundBuffer != 0) {
        set_buffer_data_size(boundBuffer, (size_t)size);
    }
    BufferStorage_Backend(target, size, data, flags);
}

// ============================================================================
// Buffer Storage — Public
// ============================================================================

void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    LOG()
    GLState.Lock();
    BufferStorage_State(target, size, data, flags);
    GLState.Unlock();
}

// ============================================================================
// Texture Buffer Objects — TBO availability check
// ============================================================================

// Checked once at first call; static const avoids per-call branch overhead.
static bool tbo_available() {
    static const bool avail = [] {
        if (GLES.glTexBuffer == nullptr) {
            LOG_E("glTexBuffer is not available on this driver — TBO unsupported");
            return false;
        }
        return true;
    }();
    return avail;
}

// Resolve virtual buffer → real GPU buffer in a single lookup.
static inline GLuint resolve_tbo_buffer(GLuint buffer) {
    if (buffer == 0) return 0;
    auto [rb, exists] = find_real_buffer_with_exists(buffer);
    if (!exists) return buffer; // not tracked, pass through
    if (rb != 0) return rb;     // already resolved
    // Tracked but not yet allocated → create GPU backing
    GLuint new_rb = 0;
    GLES.glGenBuffers(1, &new_rb);
    modify_buffer_direct(buffer, new_rb);
    return new_rb;
}

// ============================================================================
// Texture Buffer Objects — Backend
// ============================================================================

static void TexBuffer_Backend(GLenum target, GLenum internalformat, GLuint real_buffer) {
    GLES.glTexBuffer(target, internalformat, real_buffer);
    CHECK_GL_ERROR
}

static void TexBufferRange_Backend(GLenum target, GLenum internalformat, GLuint real_buffer, GLintptr offset,
                                   GLsizeiptr size) {
    GLES.glTexBufferRange(target, internalformat, real_buffer, offset, size);
    CHECK_GL_ERROR
}

// ============================================================================
// Texture Buffer Objects — State
// ============================================================================

static void TexBuffer_State(GLenum target, GLenum internalformat, GLuint buffer) {
    if (target != GL_TEXTURE_BUFFER) return;
    if (!tbo_available()) [[unlikely]] return;
    GLuint real_buffer = resolve_tbo_buffer(buffer);
    TexBuffer_Backend(target, internalformat, real_buffer);
}

static void TexBufferRange_State(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (target != GL_TEXTURE_BUFFER) return;
    if (!tbo_available()) [[unlikely]] return;
    GLuint real_buffer = resolve_tbo_buffer(buffer);
    TexBufferRange_Backend(target, internalformat, real_buffer, offset, size);
}

// ============================================================================
// Texture Buffer Objects — Public
// ============================================================================

void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
    LOG()
    LOG_D("glTexBuffer, target = %s, internalformat = %s, buffer = %d", glEnumToString(target),
          glEnumToString(internalformat), buffer)
    GLState.Lock();
    TexBuffer_State(target, internalformat, buffer);
    GLState.Unlock();
}

void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    LOG()
    LOG_D("glTexBufferRange, target = %s, internalformat = %s, buffer = %d, offset = %p, size = %zi",
          glEnumToString(target), glEnumToString(internalformat), buffer, (void*)offset, size)
    GLState.Lock();
    TexBufferRange_State(target, internalformat, buffer, offset, size);
    GLState.Unlock();
}

// ============================================================================
// Vertex Array Object Lifecycle — Backend
// ============================================================================

static void GenVertexArrays_Backend(GLsizei n, GLuint* /*arrays*/) {
    // No GLES call needed; real VAOs are lazily created on first bind
    (void)n;
}

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

// ============================================================================
// Vertex Array Object Lifecycle — State
// ============================================================================

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

static void BindVertexArray_State(GLuint array) {
    // Update CPU-side VAO tracking
    GLState.vertexArray.currentVAO = array;
    // Update bound IBO from VAO's stored IBO
    GLState.buffer.boundBuffer[GL_ELEMENT_ARRAY_BUFFER] = get_ibo_by_vao(array);
    BindVertexArray_Backend(array);
}

// ============================================================================
// Vertex Array Object Lifecycle — Public
// ============================================================================

void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    LOG()
    LOG_D("glGenVertexArrays(%i, %p)", n, arrays)
    GLState.Lock();
    GenVertexArrays_State(n, arrays);
    GLState.Unlock();
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    LOG()
    LOG_D("glDeleteVertexArrays(%i, %p)", n, arrays)
    GLState.Lock();
    DeleteVertexArrays_State(n, arrays);
    GLState.Unlock();
}

GLboolean glIsVertexArray(GLuint array) {
    LOG()
    LOG_D("glIsVertexArray(%d)", array)
    GLState.Lock();
    GLboolean result = has_array(array);
    GLState.Unlock();
    return result;
}

void glBindVertexArray(GLuint array) {
    LOG()
    LOG_D("glBindVertexArray(%d)", array)
    GLState.Lock();
    BindVertexArray_State(array);
    GLState.Unlock();
}

// ============================================================================
// ARB Aliases
// ============================================================================

#if GLOBAL_DEBUG || DEBUG
#include <fstream>
#define BIN_FILE_PREFIX "/sdcard/MG/buf/"
#endif

extern "C"
{
    GLAPI GLAPIENTRY void* glMapBufferARB(GLenum target, GLenum access) __attribute__((alias("glMapBuffer")));
    GLAPI GLAPIENTRY void glBufferDataARB(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
        __attribute__((alias("glBufferData")));
    GLAPI GLAPIENTRY GLboolean glUnmapBufferARB(GLenum target) __attribute__((alias("glUnmapBuffer")));
    GLAPI GLAPIENTRY void glBufferStorageARB(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags)
        __attribute__((alias("glBufferStorage")));
    GLAPI GLAPIENTRY void glBindBufferARB(GLenum target, GLuint buffer) __attribute__((alias("glBindBuffer")));
}