// MobileGLES - version.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLES_VERSION_H

#define VERSION_DEVELOPMENT 0
#define VERSION_ALPHA 1
#define VERSION_BETA 2
#define VERSION_RC 3
#define VERSION_RELEASE 10

// MobileGLES is a fresh fork of MobileGlues targeting iOS exclusively.
// The version numbering restarts from 1.0.0.0 to mark the new identity.
#define MAJOR 1
#define MINOR 0
#define REVISION 0
#define PATCH 0

#define VERSION_TYPE VERSION_RELEASE

#if VERSION_TYPE == VERSION_RC
#define VERSION_RC_NUMBER 1
#endif

#define VERSION_SUFFIX ""

#define MOBILEGLES_VERSION_H

#endif // MOBILEGLES_VERSION_H
