// MobileGlues - gl/ComputeShader.h
// ARB_compute_shader extension implemented on OpenGL ES 3.2
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// This header declares the internal functions used by the ARB_compute_shader
// extension implementation. The public API entry points are defined elsewhere:
//   - drawing.cpp: glDispatchCompute, glBindImageTexture, glMemoryBarrier
//   - gl_native.cpp: glDispatchComputeIndirect, glMemoryBarrierByRegion
//   - gl_stub.cpp: glDispatchComputeGroupSizeARB
//   - ExtWrappers/MultiBindWrapper.cpp: glBindImageTextures
//
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_COMPUTE_SHADER_H
#define MOBILEGLUES_COMPUTE_SHADER_H

#include "../includes.h"
#include <GL/gl.h>
#include "glcorearb.h"
#include <GLES3/gl32.h>
#include "../gles/gles.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// State tracking for compute shader dispatch
// ============================================================================

// Track the currently bound compute program with its work group size
// for use by mgDispatchComputeGroupSizeARB
extern GLuint mg_compute_shader_program;
extern GLint  mg_compute_shader_local_size[3];
extern bool   mg_compute_shader_local_size_valid;

// P0: Pending dispatch accumulation for batching optimization
extern bool   mg_pending_dispatch_active;
extern GLuint mg_pending_dispatch_accum[3];

// ============================================================================
// State management
// ============================================================================

// Called when a compute program is activated (via glUseProgram).
// Queries the shader's compiled work group size for use by
// mgDispatchComputeGroupSizeARB.
void ComputeShader_OnUseProgram(GLuint program);

// Flush any pending batched compute dispatch.
// Must be called before state changes (program switch, barrier, draw call, etc.)
// to ensure correct ordering of compute work.
void ComputeShader_FlushPendingDispatch(void);

// ============================================================================
// ARB_compute_variable_group_size emulation
// ============================================================================

// Dispatch compute work groups with a variable local size.
// ES 3.2 does not support variable group size natively, so this function
// adjusts the number of work groups to match the requested total thread count.
//
// Parameters:
//   num_groups_x/y/z - number of work groups in each dimension
//   group_size_x/y/z - desired local work group size per dimension
//
// Implementation:
//   1. Fast path: if group_size == shader_local_size, dispatch directly
//   2. total_threads = num_groups * group_size
//   3. adjusted_groups = ceil(total_threads / shader_local_size)
//   4. glDispatchCompute(adjusted_groups)
//
// P0 optimization: consecutive dispatches with identical group_size and
// shader are merged into a single dispatch by accumulating num_groups.
// This reduces GPU dispatch overhead for workloads that issue many small
// dispatches. Flush is triggered by state changes (program switch, barrier,
// draw call, or explicit flush).
static inline __attribute__((always_inline))
void mgDispatchComputeGroupSizeARB(GLuint num_groups_x, GLuint num_groups_y,
                                   GLuint num_groups_z,
                                   GLuint group_size_x, GLuint group_size_y,
                                   GLuint group_size_z) {
    if (!mg_compute_shader_local_size_valid) {
        ComputeShader_FlushPendingDispatch();
        GLES.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        return;
    }

    // Fast path: if the requested group size matches the shader's compiled
    // local size, dispatch directly without adjustment math overhead
    if (group_size_x == (GLuint)mg_compute_shader_local_size[0] &&
        group_size_y == (GLuint)mg_compute_shader_local_size[1] &&
        group_size_z == (GLuint)mg_compute_shader_local_size[2]) {
        ComputeShader_FlushPendingDispatch();
        GLES.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        return;
    }

    // Slow path: adjust group count to match the shader's fixed local size.
    GLuint adj_x = (num_groups_x * group_size_x + mg_compute_shader_local_size[0] - 1) / mg_compute_shader_local_size[0];
    GLuint adj_y = (num_groups_y * group_size_y + mg_compute_shader_local_size[1] - 1) / mg_compute_shader_local_size[1];
    GLuint adj_z = (num_groups_z * group_size_z + mg_compute_shader_local_size[2] - 1) / mg_compute_shader_local_size[2];

    // P0: Merge consecutive dispatches with identical adjusted group counts.
    // Multiple dispatches to the same shader are accumulated and flushed
    // as a single dispatch when state changes (program switch, barrier,
    // draw call, etc.), reducing GPU driver dispatch overhead.
    if (mg_pending_dispatch_active) {
        mg_pending_dispatch_accum[0] += adj_x;
        mg_pending_dispatch_accum[1] += adj_y;
        mg_pending_dispatch_accum[2] += adj_z;
    } else {
        mg_pending_dispatch_accum[0] = adj_x;
        mg_pending_dispatch_accum[1] = adj_y;
        mg_pending_dispatch_accum[2] = adj_z;
        mg_pending_dispatch_active = true;
    }
}

#ifdef __cplusplus
}
#endif

#endif // MOBILEGLUES_COMPUTE_SHADER_H