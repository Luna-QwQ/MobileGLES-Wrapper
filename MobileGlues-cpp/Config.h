#pragma once

#include "Includes.h"

namespace MobileGL {

struct Config {
    bool enableDebugLogging = false;
    bool enableStateCache = true;
    bool enableTextureFormatFallback = true;
    int maxTextureSize = 4096;
    int maxRenderbufferSize = 4096;
    int maxSamples = 4;

    static BackendType ActiveBackendType;
};

extern Config gConfig;

} // namespace MobileGL