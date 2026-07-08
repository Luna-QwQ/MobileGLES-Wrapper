#pragma once

// Log.h - intentionally does NOT include Includes.h to avoid circular dependency.
// This header only provides the logging macros and declarations.

#include "Defines.h"

// ============================================================================
// Basic logging macros
// ============================================================================

#ifndef MOBILEGL_LOG_ACTIVE_LEVEL
#define MOBILEGL_LOG_ACTIVE_LEVEL 0
#endif

#define MGLOG_LEVEL_ERROR   0
#define MGLOG_LEVEL_WARN    1
#define MGLOG_LEVEL_INFO    2
#define MGLOG_LEVEL_DEBUG   3
#define MGLOG_LEVEL_TRACE   4

#if MOBILEGL_LOG_ACTIVE_LEVEL >= MGLOG_LEVEL_ERROR
#define MGLOG_E(fmt, ...) std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define MGLOG_E(fmt, ...) ((void)0)
#endif

#if MOBILEGL_LOG_ACTIVE_LEVEL >= MGLOG_LEVEL_WARN
#define MGLOG_W(fmt, ...) std::fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#else
#define MGLOG_W(fmt, ...) ((void)0)
#endif

#if MOBILEGL_LOG_ACTIVE_LEVEL >= MGLOG_LEVEL_INFO
#define MGLOG_I(fmt, ...) std::fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)
#else
#define MGLOG_I(fmt, ...) ((void)0)
#endif

#if MOBILEGL_LOG_ACTIVE_LEVEL >= MGLOG_LEVEL_DEBUG
#define MGLOG_D(fmt, ...) std::fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define MGLOG_D(fmt, ...) ((void)0)
#endif

#if MOBILEGL_LOG_ACTIVE_LEVEL >= MGLOG_LEVEL_TRACE
#define MGLOG_T(fmt, ...) std::fprintf(stdout, "[TRACE] " fmt "\n", ##__VA_ARGS__)
#else
#define MGLOG_T(fmt, ...) ((void)0)
#endif