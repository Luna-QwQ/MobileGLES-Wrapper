// MobileGlues - gl/ComputeShader.h
// ARB_compute_shader extension implemented on OpenGL ES 3.2
//
// Architecture principle: "ES 3.2 native → native, ES 3.2 not native → CPU simulation"
//
// This header declares the internal functions used by the ARB_compute_shader
// extension implementation. The public API entry points are defined elsewhere:
//   - drawing.cpp: glDispatchCompute, glBindImageTexture, glMemoryBarrier
//   - gl_native.cpp: glDispatchComputeIndirect, glMemoryBarrierByRegion
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



#ifdef __cplusplus
}
#endif

#endif // MOBILEGLUES_COMPUTE_SHADER_H