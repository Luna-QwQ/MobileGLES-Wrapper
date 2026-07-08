// MobileGlues - gl/shader.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

// ============================================================================
// Shader Compilation Architecture
//
// Principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// ES 3.2 natively supports:
//   - GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER
//   - glCreateShader, glCompileShader, glShaderSource
//
// ES 3.2 does NOT natively support:
//   - Desktop GLSL (non-ESSL) shader sources
//     → CPU simulation: GLSL → GLSL ES conversion via GLSLtoGLSLES()
//   - GL_GEOMETRY_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER
//     → Detected and rejected (no ES 3.2 equivalent)
//
// Conversion Pipeline:
//   1. glCreateShader(type)           → native GLES 3.2 pass-through
//   2. glShaderSource(shader, source) → if ESSL 100/300/310/320: native pass-through
//                                       otherwise: GLSL→ESSL (version 320) conversion
//   3. glCompileShader(shader)        → native GLES 3.2 pass-through
//
// Architecture: DirectGLES pattern
//   Public function → _State (validate + state) → _Backend (actual GLES call)
// ============================================================================

#include "shader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"
#include "log.h"
#include "../gles/loader.h"
#include "mg.h"
#include "GL/gl.h"
#include <string>
#include <vector>

#define DEBUG 0

// ============================================================================
// Global shader info (preserved for backward compatibility)
// ============================================================================
struct shader_t shaderInfo = {};

// ============================================================================
// Shader type cache: avoids glGetShaderiv GPU round-trip on every glShaderSource
// Indexed by shader object ID. Stores the shader type and whether the shader
// source was previously verified as ESSL (to skip scanShaderSource on re-upload).
// ============================================================================
struct ShaderCacheEntry {
    GLenum type = 0;         // cached shader type (GL_VERTEX_SHADER, etc.)
    bool is_essl_verified = false; // true if source was confirmed ESSL
};
static std::vector<ShaderCacheEntry> g_shader_cache;

static inline ShaderCacheEntry& get_shader_cache(GLuint shader) {
    if (shader >= g_shader_cache.size()) [[unlikely]]
        g_shader_cache.resize(shader + 1);
    return g_shader_cache[shader];
}

void invalidate_shader_cache(GLuint shader) {
    if (shader < g_shader_cache.size()) {
        g_shader_cache[shader] = ShaderCacheEntry{};
    }
}

// ============================================================================
// Section: Shader Type Detection Helpers
//
// Single-pass GLSL source analysis. Instead of multiple strstr() calls
// (up to 20+ per shader source), we scan once and check all patterns
// simultaneously. This avoids O(n * m) overhead where n = source length
// and m = number of patterns.
// ============================================================================

// ---------------------------------------------------------------------------
// Result of single-pass shader source analysis
// ---------------------------------------------------------------------------
struct ShaderSourceInfo {
    bool is_essl;          // true if #version 100/300/310/320
    bool is_geometry;      // geometry shader keywords found
    bool is_tessellation;  // tessellation shader keywords found
    bool is_compute;       // compute shader keywords found
};

// ---------------------------------------------------------------------------
// Single-pass scan of GLSL source for version + shader type detection.
// Scans the entire source string once, checking for all patterns.
// ---------------------------------------------------------------------------
static ShaderSourceInfo scanShaderSource(const char* code) {
    ShaderSourceInfo info = {};
    const char* p = code;

    while (*p) {
        // Skip whitespace and comments efficiently
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        // Check for #version directive (only at/near the beginning matters)
        if (!info.is_essl && *p == '#') {
            const char* v = p + 1;
            while (*v == ' ' || *v == '\t') v++;
            if (v[0] == 'v' && v[1] == 'e' && v[2] == 'r' && v[3] == 's' && v[4] == 'i' && v[5] == 'o' && v[6] == 'n') {
                v += 7;
                while (*v == ' ' || *v == '\t') v++;
                int ver = 0;
                while (*v >= '0' && *v <= '9') { ver = ver * 10 + (*v - '0'); v++; }
                info.is_essl = (ver == 100 || ver == 300 || ver == 310 || ver == 320);
            }
        }

        // Check for shader-type-defining keywords
        // We use multi-character prefix matching to reduce false positives
        switch (*p) {
        case 'E':
            if (!info.is_geometry) {
                if (p[1] == 'm' && p[2] == 'i' && p[3] == 't' && p[4] == 'V' && p[5] == 'e' && p[6] == 'r' && p[7] == 't' && p[8] == 'e' && p[9] == 'x')
                    info.is_geometry = true;
                else if (p[1] == 'n' && p[2] == 'd' && p[3] == 'P' && p[4] == 'r' && p[5] == 'i' && p[6] == 'm' && p[7] == 'i' && p[8] == 't' && p[9] == 'i' && p[10] == 'v' && p[11] == 'e')
                    info.is_geometry = true;
            }
            break;
        case 'g':
            if (!info.is_tessellation && p[1] == 'l' && p[2] == '_') {
                if (p[3] == 'T' && p[4] == 'e' && p[5] == 's' && p[6] == 's' && (p[7] == 'L' || p[7] == 'C'))
                    info.is_tessellation = true;
            }
            if (!info.is_compute && p[1] == 'l' && p[2] == '_') {
                if ((p[3] == 'W' && p[4] == 'o' && p[5] == 'r' && p[6] == 'k' && p[7] == 'G') ||
                    (p[3] == 'L' && p[4] == 'o' && p[5] == 'c' && p[6] == 'a' && p[7] == 'l') ||
                    (p[3] == 'G' && p[4] == 'l' && p[5] == 'o' && p[6] == 'b' && p[7] == 'a' && p[8] == 'l') ||
                    (p[3] == 'N' && p[4] == 'u' && p[5] == 'm' && p[6] == 'W' && p[7] == 'o' && p[8] == 'r' && p[9] == 'k'))
                    info.is_compute = true;
            }
            break;
        case 'l':
            if (!info.is_geometry && p[1] == 'a' && p[2] == 'y' && p[3] == 'o' && p[4] == 'u' && p[5] == 't' && p[6] == '(') {
                p += 7;
                while (*p == ' ' || *p == '\t') p++;
                if (p[0] == 't' && p[1] == 'r' && p[2] == 'i') info.is_geometry = true;
                else if (p[0] == 'p' && p[1] == 'o' && p[2] == 'i' && p[3] == 'n' && p[4] == 't' && p[5] == 's') info.is_geometry = true;
                else if (p[0] == 'l' && p[1] == 'i' && p[2] == 'n' && p[3] == 'e') info.is_geometry = true;
                else if (p[0] == 'q' && p[1] == 'u' && p[2] == 'a' && p[3] == 'd' && p[4] == 's') info.is_geometry = true;
                else if (p[0] == 'i' && p[1] == 's' && p[2] == 'o' && p[3] == 'l' && p[4] == 'i' && p[5] == 'n' && p[6] == 'e' && p[7] == 's') info.is_geometry = true;
                continue;
            }
            if (!info.is_compute && p[1] == 'a' && p[2] == 'y' && p[3] == 'o' && p[4] == 'u' && p[5] == 't' && p[6] == '(') {
                p += 7;
                while (*p == ' ' || *p == '\t') p++;
                if (p[0] == 'l' && p[1] == 'o' && p[2] == 'c' && p[3] == 'a' && p[4] == 'l' && p[5] == '_' && p[6] == 's' && p[7] == 'i' && p[8] == 'z' && p[9] == 'e' && p[10] == '_')
                    info.is_compute = true;
                continue;
            }
            if (!info.is_tessellation && p[1] == 'a' && p[2] == 'y' && p[3] == 'o' && p[4] == 'u' && p[5] == 't' && p[6] == '(') {
                p += 7;
                while (*p == ' ' || *p == '\t') p++;
                if (p[0] == 'v' && p[1] == 'e' && p[2] == 'r' && p[3] == 't' && p[4] == 'i' && p[5] == 'c' && p[6] == 'e' && p[7] == 's')
                    info.is_tessellation = true;
                continue;
            }
            break;
        case 'm':
            if (!info.is_geometry && p[1] == 'a' && p[2] == 'x' && p[3] == '_' && p[4] == 'v' && p[5] == 'e' && p[6] == 'r' && p[7] == 't' && p[8] == 'i' && p[9] == 'c' && p[10] == 'e' && p[11] == 's')
                info.is_geometry = true;
            break;
        case 'p':
            if (!info.is_tessellation && p[1] == 'a' && p[2] == 't' && p[3] == 'c' && p[4] == 'h') {
                // "patch" is 5 chars, check it's not part of a longer word
                char next = p[5];
                if (next == ' ' || next == '\t' || next == '\n' || next == '\r' || next == ';' || next == '(' || next == '\0')
                    info.is_tessellation = true;
            }
            break;
        }
        p++;
    }
    return info;
}

// ---------------------------------------------------------------------------
// Determine if shader is already ES-compatible (direct passthrough)
// ESSL versions recognized: 100 (ES 1.0), 300 (ES 3.0), 310 (ES 3.1), 320 (ES 3.2)
// ---------------------------------------------------------------------------
static inline bool is_direct_shader(const ShaderSourceInfo& info) {
    return info.is_essl;
}

// ---------------------------------------------------------------------------
// Detect the actual shader type from GLSL source content
// Used when the shader type was not explicitly set (e.g. Iris sets it later)
// ---------------------------------------------------------------------------
static GLenum detect_shader_type_from_source(const ShaderSourceInfo& info) {
    if (info.is_geometry) return GL_GEOMETRY_SHADER;
    if (info.is_tessellation) return GL_TESS_EVALUATION_SHADER;
    if (info.is_compute) return GL_COMPUTE_SHADER;
    return GL_FRAGMENT_SHADER; // default
}

// ============================================================================
// Section: glCreateShader - Shader Object Creation (DirectGLES pattern)
// ============================================================================

// ---------------------------------------------------------------------------
// CreateShader_Backend - Actual GLES call to create the shader object
// ---------------------------------------------------------------------------
static GLuint CreateShader_Backend(GLenum type) {
    return GLES.glCreateShader(type);
}

// ---------------------------------------------------------------------------
// CreateShader_State - Validates shader type, creates real GLES shader,
//                      stores mapping in GLState, returns virtual ID.
// ---------------------------------------------------------------------------
static GLuint CreateShader_State(GLenum type) {
    // Step 1: Validate shader type
    // ES 3.2 natively supports: VERTEX_SHADER, FRAGMENT_SHADER, COMPUTE_SHADER
    // Desktop-only types (GEOMETRY, TESS_*) are rejected with InvalidEnum
    switch (type) {
        case GL_VERTEX_SHADER:
        case GL_FRAGMENT_SHADER:
        case GL_COMPUTE_SHADER:
            break; // valid ES 3.2 shader types
        case GL_GEOMETRY_SHADER:
        case GL_TESS_CONTROL_SHADER:
        case GL_TESS_EVALUATION_SHADER:
            GLState.errorState.RecordError(ErrorCode::InvalidEnum,
                std::make_unique<GenericErrorInfo>("glCreateShader: geometry/tessellation shaders are not supported in OpenGL ES 3.2"));
            return 0;
        default:
            GLState.errorState.RecordError(ErrorCode::InvalidEnum,
                std::make_unique<GenericErrorInfo>("glCreateShader: invalid shader type"));
            return 0;
    }

    // Step 2: Create the real GLES shader
    GLuint shader = CreateShader_Backend(type);
    if (shader == 0) {
        GLState.errorState.RecordError(ErrorCode::OutOfMemory,
            std::make_unique<GenericErrorInfo>("glCreateShader: failed to create shader object"));
        return 0;
    }

    // Step 3: Cache the shader type to avoid glGetShaderiv GPU round-trip later
    auto& cacheEntry = get_shader_cache(shader);
    cacheEntry.type = type;

    // Step 4: Track in unified state manager (1:1 virtual→real mapping)
    auto &ss = GLState.shader;
    ss.shaderMap[shader] = shader;
    ss.shaderMapReverse[shader] = shader;
    auto &info = ss.shaderInfo[shader];
    info.type = type;
    info.source.clear();
    info.compiled = false;

    return shader;
}

// ---------------------------------------------------------------------------
// glCreateShader - Public entry point (thin wrapper)
// ---------------------------------------------------------------------------
GLAPI GLAPIENTRY GLuint glCreateShader(GLenum type) {
    LOG()
    GLState.Lock();
    GLuint result = CreateShader_State(type);
    GLState.Unlock();
    return result;
}

// ============================================================================
// Section: glShaderSource - Shader Source Upload (DirectGLES pattern)
//
// glShaderSource is the entry point for shader source code. It implements
// the CPU simulation layer for non-ES GLSL code by converting desktop GLSL
// to GLSL ES 3.2 (version 320) using the GLSLtoGLSLES() conversion engine.
//
// Pipeline:
//   1. If shader is already ESSL (100/300/310/320) → native pass-through
//   2. If shader type is unknown → detect from source content
//   3. Convert GLSL → GLSL ES 3.2 via GLSLtoGLSLES()
//   4. On conversion failure → fall back to original source
// ============================================================================

// ---------------------------------------------------------------------------
// ShaderSource_Backend - Actual GLES call to set shader source
// ---------------------------------------------------------------------------
static void ShaderSource_Backend(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    GLES.glShaderSource(shader, count, string, length);
}

// ---------------------------------------------------------------------------
// ShaderSource_State - Validates parameters, applies GLSL→ESSL conversion,
//                      stores source in GLState, dispatches to backend.
// ---------------------------------------------------------------------------
static void ShaderSource_State(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    LOG_D("glShaderSource hook, shader=%d, count=%d", shader, count)

    // Step 1: Validate shader object exists
    if (shader == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glShaderSource: shader is 0"));
        return;
    }

    // Step 2: Handle empty/null source
    if (count <= 0 || !string || !string[0]) {
        // Invalidate cache when clearing source
        auto& entry = get_shader_cache(shader);
        entry.is_essl_verified = false;
        ShaderSource_Backend(shader, count, string, length);
        return;
    }

    const char* raw_code = string[0];
    auto& cacheEntry = get_shader_cache(shader);

    // Step 3: Get shader type — use cache to avoid glGetShaderiv GPU round-trip
    GLenum shaderType = cacheEntry.type;
    if (shaderType == 0) [[unlikely]] {
        // First call: query type from GLES and cache it
        GLES.glGetShaderiv(shader, GL_SHADER_TYPE, (GLint*)&shaderType);
        cacheEntry.type = shaderType;
    }

    // Step 4: Fast path — if previously verified as ESSL, skip scan and pass through
    if (cacheEntry.is_essl_verified) [[likely]] {
        LOG_D("Cached ESSL shader %d, passing through directly", shader)
        ShaderSource_Backend(shader, count, string, length);
        return;
    }

    // Step 5: Single-pass scan of source for version + shader type detection
    ShaderSourceInfo sourceInfo = scanShaderSource(raw_code);

    // Step 6: If shader type is GL_FRAGMENT_SHADER, check if source actually
    // contains geometry/tessellation/compute shader keywords and correct the type.
    if (shaderType == GL_FRAGMENT_SHADER) {
        GLenum detected = detect_shader_type_from_source(sourceInfo);
        if (detected != GL_FRAGMENT_SHADER) {
            shaderType = detected;
            cacheEntry.type = shaderType; // update cache with detected type
            LOG_I("[MobileGLES] Detected non-fragment shader type from source: %d", shaderType)
        }
    }

    LOG_D("glShaderSource: shaderType=%d", shaderType)

    // Step 7: Check if already an ES-compatible shader → native pass-through
    if (is_direct_shader(sourceInfo)) {
        LOG_D("Direct ES shader (ESSL 100/300/310/320), passing through")
        cacheEntry.is_essl_verified = true; // mark as verified ESSL for future calls
        ShaderSource_Backend(shader, count, string, length);
        return;
    }

    // Step 8: Convert desktop GLSL → GLSL ES 3.2 (version 320)
    std::string glsl_code = raw_code;
    uint essl_version = 320;
    int return_code = -1;
    std::string converted = GLSLtoGLSLES(glsl_code.c_str(), shaderType, essl_version, 0, return_code);

    if (return_code < 0) {
        // Conversion failed — fall back to original source
        LOG_I("[MobileGLES] GLSL conversion failed for shader %d (type=%d), passing through original desktop GLSL (will likely fail to compile on GLES)", shader, shaderType)
        ShaderSource_Backend(shader, count, string, length);
        return;
    }

    LOG_D("Converted GLSL ES:\n%s", converted.c_str())

    // Step 9: Set the converted GLSL ES source
    const char* converted_code = converted.c_str();
    ShaderSource_Backend(shader, 1, &converted_code, nullptr);

    // Track in unified state manager
    auto &ss = GLState.shader;
    auto infoIt = ss.shaderInfo.find(shader);
    if (infoIt != ss.shaderInfo.end()) {
        infoIt->second.source = converted;
    }
}

// ---------------------------------------------------------------------------
// glShaderSource - Public entry point (thin wrapper)
// ---------------------------------------------------------------------------
GLAPI GLAPIENTRY void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    LOG()
    GLState.Lock();
    ShaderSource_State(shader, count, string, length);
    GLState.Unlock();
}

// ============================================================================
// Section: glCompileShader - Shader Compilation (DirectGLES pattern)
//
// The shader source must already be valid ESSL — conversion is handled by
// glShaderSource before this function is called.
// ============================================================================

// ---------------------------------------------------------------------------
// CompileShader_Backend - Actual GLES call to compile the shader
// ---------------------------------------------------------------------------
static void CompileShader_Backend(GLuint shader) {
    GLES.glCompileShader(shader);
}

// ---------------------------------------------------------------------------
// CompileShader_State - Validates shader, compiles via GLES, tracks result
// ---------------------------------------------------------------------------
static void CompileShader_State(GLuint shader) {
    // Step 1: Validate shader object exists
    if (shader == 0) {
        GLState.errorState.RecordError(ErrorCode::InvalidValue,
            std::make_unique<GenericErrorInfo>("glCompileShader: shader is 0"));
        return;
    }

    // Step 2: Compile via GLES backend
    CompileShader_Backend(shader);

    // Step 3: Query compile status
    GLint compiled = 0;
    GLES.glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    // Step 4: Track in unified state manager
    auto &ss = GLState.shader;
    auto it = ss.shaderInfo.find(shader);
    if (it != ss.shaderInfo.end()) {
        it->second.compiled = (compiled == GL_TRUE);
    }

    // Step 5: Log compilation errors for diagnostics
    if (!compiled) {
        GLint infoLen = 0;
        GLES.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            // Include shader type from cache for better diagnostics
            GLenum shaderType = (shader < g_shader_cache.size()) ? g_shader_cache[shader].type : 0;
            std::vector<char> log(infoLen);
            GLES.glGetShaderInfoLog(shader, infoLen, nullptr, log.data());
            LOG_I("[MobileGLES] Shader %d (type=%d) compilation failed: %s", shader, shaderType, log.data())
        } else {
            LOG_I("[MobileGLES] Shader %d compilation failed (no info log)", shader)
        }
    }
}

// ---------------------------------------------------------------------------
// glCompileShader - Public entry point (thin wrapper)
// ---------------------------------------------------------------------------
GLAPI GLAPIENTRY void glCompileShader(GLuint shader) {
    LOG()
    GLState.Lock();
    CompileShader_State(shader);
    GLState.Unlock();
}