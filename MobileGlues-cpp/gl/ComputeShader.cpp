// MobileGlues - gl/ComputeShader.cpp
// ARB_compute_shader extension implemented on OpenGL ES 3.2
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// This module provides the internal implementation for the ARB_compute_shader
// extension. The public API functions are defined in gl_stub.cpp (for non-native
// functions) and drawing.cpp (for native functions with extra state tracking).
//
// Functions provided:
//   mgDispatchComputeGroupSizeARB → ARB_compute_variable_group_size emulation
//   ComputeShader_OnUseProgram    → called when a compute program is activated
//
// glDispatchComputeGroupSizeARB rationale:
//   ES 3.2 does not support variable work group size at dispatch time.
//   The work group size is baked into the compiled shader (local_size_x/y/z).
//   For the ARB_compute_variable_group_size compatibility shim, we:
//   1. Query the shader's compiled local work group size
//   2. Adjust the number of groups to compensate for the difference
//   3. Dispatch with the adjusted group count
//   This preserves the same total number of invocations.
//
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "ComputeShader.h"
#include "../gles/gles.h"
#include "state.h"
#include "log.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// ---------------------------------------------------------------------------
// Internal state tracking for compute shader dispatch
// ---------------------------------------------------------------------------

// Track the currently bound compute program with its work group size
// for use by mgDispatchComputeGroupSizeARB (declared extern in ComputeShader.h)
GLuint mg_compute_shader_program = 0;
GLint  mg_compute_shader_local_size[3] = { 1, 1, 1 };
bool   mg_compute_shader_local_size_valid = false;

// ---------------------------------------------------------------------------
// P0 Dispatch batching optimization: pending dispatch accumulation
// Batches small consecutive dispatches for the same program into
// one larger dispatch to reduce GPU driver dispatch overhead.
// ---------------------------------------------------------------------------
bool   mg_pending_dispatch_active = false;
GLuint mg_pending_dispatch_accum[3] = { 0, 0, 0 };

// ---------------------------------------------------------------------------
// Helper: query the current program's compute shader work group size
// ---------------------------------------------------------------------------
static void query_compute_work_group_size(GLuint program) {
    mg_compute_shader_local_size_valid = false;
    if (program == 0) return;

    // Use glGetProgramiv with GL_COMPUTE_WORK_GROUP_SIZE (ES 3.1+)
    // GL_COMPUTE_WORK_GROUP_SIZE = 0x8267, returns 3 ints: x, y, z
    GLint results[3] = { 0, 0, 0 };
    GLES.glGetProgramiv(program, 0x8267, results);

    // Check if the query returned valid data
    if (results[0] > 0) {
        mg_compute_shader_local_size[0] = results[0];
        mg_compute_shader_local_size[1] = (results[1] > 0) ? results[1] : 1;
        mg_compute_shader_local_size[2] = (results[2] > 0) ? results[2] : 1;
        mg_compute_shader_local_size_valid = true;
    }
}

// ---------------------------------------------------------------------------
// P0: Flush any accumulated pending dispatch
// ---------------------------------------------------------------------------
extern "C" void ComputeShader_FlushPendingDispatch(void) {
    if (!mg_pending_dispatch_active ||
        (mg_pending_dispatch_accum[0] == 0 && mg_pending_dispatch_accum[1] == 0 && mg_pending_dispatch_accum[2] == 0)) {
        // Nothing pending
        mg_pending_dispatch_active = false;
        return;
    }

    // P4: GL_DEBUG_OUTPUT labeling — annotate merged dispatch in debug tools
    // (RenderDoc, GPU Inspector, etc.) so batched dispatches are identifiable.
#if GLOBAL_DEBUG
    char debug_label[64];
    snprintf(debug_label, sizeof(debug_label),
             "MergedDispatch(%u,%u,%u) local=(%d,%d,%d)",
             mg_pending_dispatch_accum[0], mg_pending_dispatch_accum[1], mg_pending_dispatch_accum[2],
             mg_compute_shader_local_size[0], mg_compute_shader_local_size[1], mg_compute_shader_local_size[2]);
    GLES.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, debug_label);
#endif

    // Dispatch the accumulated groups
    GLES.glDispatchCompute(mg_pending_dispatch_accum[0],
                           mg_pending_dispatch_accum[1],
                           mg_pending_dispatch_accum[2]);

#if GLOBAL_DEBUG
    GLES.glPopDebugGroup();
#endif

    // Reset for next batch
    mg_pending_dispatch_accum[0] = 0;
    mg_pending_dispatch_accum[1] = 0;
    mg_pending_dispatch_accum[2] = 0;
    mg_pending_dispatch_active = false;
}

// ---------------------------------------------------------------------------
// Exposed state update - called when a compute program is used
// ---------------------------------------------------------------------------
extern "C" void ComputeShader_OnUseProgram(GLuint program) {
    // Flush before changing program to maintain correct ordering
    ComputeShader_FlushPendingDispatch();

    mg_compute_shader_program = program;
    mg_compute_shader_local_size_valid = false;

    if (program != 0) {
        // Check if this program has a compute shader attached
        GLint attached = 0;
        GLES.glGetProgramiv(program, GL_ATTACHED_SHADERS, &attached);
        if (attached > 0) {
            query_compute_work_group_size(program);
        }
    }
}