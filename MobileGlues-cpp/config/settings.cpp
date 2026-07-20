// MobileGLES - config/settings.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "settings.h"
#include "config.h"
#include "../gl/log.h"
#include "../gl/envvars.h"
#include "../gl/getter.h"

#define DEBUG 0

global_settings_t global_settings;

// iOS-only: ANGLE is the framework we link against (libEGL.framework /
// libGLESv2.framework), so there is no separate "enable ANGLE" decision
// to make at runtime. All GPU/launcher/Vulkan detection logic from the
// Android/Linux branch has been removed. Settings use safe hardcoded
// defaults; users can still override via config.json in the MG directory.
void init_settings() {
    int success = initialized;
    if (!success) {
        success = config_refresh();
        if (!success) {
            LOG_V("Failed to load config. Use default config.")
        }
    }

    // On iOS, ANGLE is always the backing implementation (linked framework).
    global_settings.angle = AngleMode::Disabled; // The framework already IS ANGLE; no separate runtime toggle.
    global_settings.buffer_coherent_as_flush = (global_settings.angle == AngleMode::Disabled);

    // Allow overriding ignore_error via config.json (default: Partial).
    NoErrorConfig noErrorConfig =
        success ? static_cast<NoErrorConfig>(config_get_int("enableNoError")) : NoErrorConfig::Auto;
    if (static_cast<int>(noErrorConfig) < 0 || static_cast<int>(noErrorConfig) > 3) {
        noErrorConfig = NoErrorConfig::Auto;
    }
    switch (noErrorConfig) {
    case NoErrorConfig::Level1:
        global_settings.ignore_error = IgnoreErrorLevel::Partial;
        LOG_D("Error ignoring: Level 1 (Partial)");
        break;
    case NoErrorConfig::Level2:
        global_settings.ignore_error = IgnoreErrorLevel::Full;
        LOG_D("Error ignoring: Level 2 (Full)");
        break;
    case NoErrorConfig::Auto:
    case NoErrorConfig::Disable:
    default:
        global_settings.ignore_error = IgnoreErrorLevel::Partial;
        LOG_D("Error ignoring: Partial (iOS default)");
        break;
    }

    global_settings.ext_compute_shader = success ? (config_get_int("enableExtComputeShader") > 0) : false;
    global_settings.ext_timer_query = success ? (config_get_int("enableExtTimerQuery") > 0) : true;
    global_settings.ext_direct_state_access = success ? (config_get_int("enableExtDirectStateAccess") > 0) : true;

    size_t maxGlslCacheSize = 0;
    if (success && config_get_int("maxGlslCacheSize") > 0) {
        maxGlslCacheSize = config_get_int("maxGlslCacheSize") * 1024 * 1024;
    }
    global_settings.max_glsl_cache_size = maxGlslCacheSize;

    AngleDepthClearFixMode angleDepthClearFixMode =
        success ? static_cast<AngleDepthClearFixMode>(config_get_int("angleDepthClearFixMode"))
                : AngleDepthClearFixMode::Disabled;
    if (static_cast<int>(angleDepthClearFixMode) < 0 ||
        static_cast<int>(angleDepthClearFixMode) >= static_cast<int>(AngleDepthClearFixMode::MaxValue)) {
        angleDepthClearFixMode = AngleDepthClearFixMode::Disabled;
    }
    global_settings.angle_depth_clear_fix_mode = angleDepthClearFixMode;

    int customGLVersionInt = success ? config_get_int("customGLVersion") : 0;
    if (customGLVersionInt < 0) customGLVersionInt = 0;
    if (customGLVersionInt > 46) {
        customGLVersionInt = 46;
    } else if (customGLVersionInt < 32 && customGLVersionInt != 0) {
        customGLVersionInt = 32;
    } else if (customGLVersionInt > 33 && customGLVersionInt < 40) {
        customGLVersionInt = 33;
    } else if (customGLVersionInt == 0) {
        customGLVersionInt = DEFAULT_GL_VERSION;
    }
    global_settings.custom_gl_version = Version(customGLVersionInt);

    FSR1_Quality_Preset fsr1Setting =
        success ? static_cast<FSR1_Quality_Preset>(config_get_int("fsr1Setting")) : FSR1_Quality_Preset::Disabled;
    if (static_cast<int>(fsr1Setting) < 0 ||
        static_cast<int>(fsr1Setting) >= static_cast<int>(FSR1_Quality_Preset::MaxValue)) {
        fsr1Setting = FSR1_Quality_Preset::Disabled;
    }
    global_settings.fsr1_setting = fsr1Setting;

    HideMGEnvLevel hideMGEnvLevel =
        success ? static_cast<HideMGEnvLevel>(config_get_int("hideMGEnvLevel")) : HideMGEnvLevel::Disabled;
    if (static_cast<int>(hideMGEnvLevel) < 0 ||
        static_cast<int>(hideMGEnvLevel) >= static_cast<int>(HideMGEnvLevel::MaxValue)) {
        hideMGEnvLevel = HideMGEnvLevel::Disabled;
    }
    global_settings.hide_mg_env_level = hideMGEnvLevel;

    LOG_V("MG_DIR_PATH = %s", mg_directory_path ? mg_directory_path : "(default)")

    LOG_V("[MobileGLES] Setting: enableAngle                 = %s",
          global_settings.angle == AngleMode::Enabled ? "true" : "false")
    LOG_V("[MobileGLES] Setting: ignoreError                 = %i", static_cast<int>(global_settings.ignore_error))
    LOG_V("[MobileGLES] Setting: enableExtComputeShader      = %s",
          global_settings.ext_compute_shader ? "true" : "false")
    LOG_V("[MobileGLES] Setting: enableExtTimerQuery         = %s", global_settings.ext_timer_query ? "true" : "false")
    LOG_V("[MobileGLES] Setting: enableExtDirectStateAccess  = %s",
          global_settings.ext_direct_state_access ? "true" : "false")
    LOG_V("[MobileGLES] Setting: maxGlslCacheSize            = %i",
          static_cast<int>(global_settings.max_glsl_cache_size / 1024 / 1024))
    LOG_V("[MobileGLES] Setting: angleDepthClearFixMode      = %i",
          static_cast<int>(global_settings.angle_depth_clear_fix_mode))
    LOG_V("[MobileGLES] Setting: bufferCoherentAsFlush       = %i",
          static_cast<int>(global_settings.buffer_coherent_as_flush))
    if (global_settings.custom_gl_version.isEmpty()) {
        LOG_V("[MobileGLES] Setting: customGLVersion             = (default)");
    } else {
        LOG_V("[MobileGLES] Setting: customGLVersion             = %s",
              global_settings.custom_gl_version.toString().c_str());
    }
    LOG_V("[MobileGLES] Setting: fsr1Setting                 = %i", static_cast<int>(global_settings.fsr1_setting))
    LOG_V("[MobileGLES] Setting: hideMGEnvLevel              = %i",
          static_cast<int>(global_settings.hide_mg_env_level))

    GLVersion =
        global_settings.custom_gl_version.isEmpty() ? Version(DEFAULT_GL_VERSION) : global_settings.custom_gl_version;
}

void set_multidraw_setting() { // should be called after init_gles_target()
    // iOS-only: respect config.json if present, otherwise keep the value
    // already initialised in init_settings() and let init_settings_post()
    // resolve it against actual GLES capabilities.
    multidraw_mode_t multidrawMode = global_settings.multidraw_mode;
    int configured = initialized ? config_get_int("multidrawMode") : -1;
    if (configured >= 0) {
        multidrawMode = static_cast<multidraw_mode_t>(configured);
    }
    if (static_cast<int>(multidrawMode) < 0 ||
        static_cast<int>(multidrawMode) >= static_cast<int>(multidraw_mode_t::MaxValue)) {
        multidrawMode = multidraw_mode_t::Auto;
    }
    global_settings.multidraw_mode = multidrawMode;
    std::string draw_mode_str;
    switch (global_settings.multidraw_mode) {
    case multidraw_mode_t::PreferIndirect:
        draw_mode_str = "Indirect";
        break;
    case multidraw_mode_t::PreferBaseVertex:
        draw_mode_str = "Unroll";
        break;
    case multidraw_mode_t::PreferMultidrawIndirect:
        draw_mode_str = "Multidraw indirect";
        break;
    case multidraw_mode_t::DrawElements:
        draw_mode_str = "DrawElements";
        break;
    case multidraw_mode_t::Compute:
        draw_mode_str = "Compute";
        break;
    case multidraw_mode_t::Auto:
        draw_mode_str = "Auto";
        break;
    default:
        draw_mode_str = "(Unknown)";
        global_settings.multidraw_mode = multidraw_mode_t::Auto;
        break;
    }
    LOG_V("[MobileGLES] Setting: multidrawMode               = %s", draw_mode_str.c_str())
}

void init_settings_post() {
    bool multidraw = g_gles_caps.GL_EXT_multi_draw_indirect;
    bool basevertex = g_gles_caps.GL_EXT_draw_elements_base_vertex || g_gles_caps.GL_OES_draw_elements_base_vertex ||
                      (g_gles_caps.major == 3 && g_gles_caps.minor >= 2) || (g_gles_caps.major > 3);
    bool indirect = (g_gles_caps.major == 3 && g_gles_caps.minor >= 1) || (g_gles_caps.major > 3);

    switch (global_settings.multidraw_mode) {
    case multidraw_mode_t::PreferIndirect:
        LOG_V("multidrawMode = PreferIndirect")
        if (indirect) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferIndirect;
            LOG_V("    -> Indirect (OK)")
        } else if (basevertex) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferBaseVertex;
            LOG_V("    -> BaseVertex (Preferred not supported, falling back)")
        } else {
            global_settings.multidraw_mode = multidraw_mode_t::DrawElements;
            LOG_V("    -> DrawElements (Preferred not supported, falling back)")
        }
        break;
    case multidraw_mode_t::PreferBaseVertex:
        LOG_V("multidrawMode = PreferBaseVertex")
        if (basevertex) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferBaseVertex;
            LOG_V("    -> BaseVertex (OK)")
        } else if (multidraw) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferMultidrawIndirect;
            LOG_V("    -> MultidrawIndirect (Preferred not supported, falling back)")
        } else if (indirect) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferIndirect;
            LOG_V("    -> Indirect (Preferred not supported, falling back)")
        } else {
            global_settings.multidraw_mode = multidraw_mode_t::DrawElements;
            LOG_V("    -> DrawElements (Preferred not supported, falling back)")
        }
        break;
    case multidraw_mode_t::DrawElements:
        LOG_V("multidrawMode = DrawElements")
        global_settings.multidraw_mode = multidraw_mode_t::DrawElements;
        LOG_V("    -> DrawElements (OK)")
        break;
    case multidraw_mode_t::Compute:
        LOG_V("multidrawMode = Compute")
        global_settings.multidraw_mode = multidraw_mode_t::Compute;
        LOG_V("    -> Compute (OK)")
        break;
    case multidraw_mode_t::Auto:
    default:
        LOG_V("multidrawMode = Auto")
        if (multidraw) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferMultidrawIndirect;
            LOG_V("    -> MultidrawIndirect (Auto detected)")
        } else if (indirect) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferIndirect;
            LOG_V("    -> Indirect (Auto detected)")
        } else if (basevertex) {
            global_settings.multidraw_mode = multidraw_mode_t::PreferBaseVertex;
            LOG_V("    -> BaseVertex (Auto detected)")
        } else {
            global_settings.multidraw_mode = multidraw_mode_t::DrawElements;
            LOG_V("    -> DrawElements (Auto detected)")
        }
        break;
    }
}

std::string dump_settings_string(std::string prefix) {
    std::stringstream ss;

    ss << prefix << "Angle: " << (global_settings.angle == AngleMode::Enabled ? "Enabled" : "Disabled") << "\n";
    ss << prefix << "IgnoreError: ";
    switch (global_settings.ignore_error) {
    case IgnoreErrorLevel::None:
        ss << "None";
        break;
    case IgnoreErrorLevel::Partial:
        ss << "Partial";
        break;
    case IgnoreErrorLevel::Full:
        ss << "Full";
        break;
    }
    ss << "\n";

    ss << prefix << "ExtComputeShader: " << (global_settings.ext_compute_shader ? "True" : "False") << "\n";
    ss << prefix << "ExtTimerQuery: " << (global_settings.ext_timer_query ? "True" : "False") << "\n";
    ss << prefix << "ExtDirectStateAccess: " << (global_settings.ext_direct_state_access ? "True" : "False") << "\n";
    ss << prefix << "MaxGlslCacheSize: " << (global_settings.max_glsl_cache_size / 1024 / 1024) << "MB\n";

    ss << prefix << "MultidrawMode: ";
    switch (global_settings.multidraw_mode) {
    case multidraw_mode_t::Auto:
        ss << "Auto";
        break;
    case multidraw_mode_t::PreferIndirect:
        ss << "Indirect (glDrawElementsIndirect)";
        break;
    case multidraw_mode_t::PreferBaseVertex:
        ss << "BaseVertex (glDrawElementsBaseVertex)";
        break;
    case multidraw_mode_t::PreferMultidrawIndirect:
        ss << "MultidrawIndirect (glMultiDrawElementsIndirect)";
        break;
    case multidraw_mode_t::DrawElements:
        ss << "DrawElements (glDrawElements with per-draw CPU rebase)";
        break;
    case multidraw_mode_t::Compute:
        ss << "Compute (glDrawElements with compute-shader rebase)";
        break;
    default:
        ss << "Unknown";
        break;
    }
    ss << "\n";

    ss << prefix << "AngleDepthClearFixMode: "
       << (global_settings.angle_depth_clear_fix_mode == AngleDepthClearFixMode::Disabled ? "Disabled" : "Enabled")
       << "\n";

    ss << prefix << "BufferCoherentAsFlush: " << (global_settings.buffer_coherent_as_flush ? "True" : "False") << "\n";

    ss << prefix << "CustomGLVersion: "
       << ((GLVersion.toInt(2) == DEFAULT_GL_VERSION) ? "(Default)" : std::to_string(GLVersion.toInt(2))) << "\n";

    ss << prefix << "Fsr1Setting: ";

    switch (global_settings.fsr1_setting) {
    case FSR1_Quality_Preset::Disabled:
        ss << "Disabled";
        break;
    case FSR1_Quality_Preset::UltraQuality:
        ss << "UltraQuality";
        break;
    case FSR1_Quality_Preset::Quality:
        ss << "Quality";
        break;
    case FSR1_Quality_Preset::Balanced:
        ss << "Balanced";
        break;
    case FSR1_Quality_Preset::Performance:
        ss << "Performance";
        break;
    default:
        ss << "Unknown";
        break;
    }
    ss << "\n";

    ss << prefix << "HideMGEnvLevel: "
       << ((global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled)
               ? "Disabled"
               : std::to_string(static_cast<int>(global_settings.hide_mg_env_level)));

    ss << "\n";

    return ss.str();
}
