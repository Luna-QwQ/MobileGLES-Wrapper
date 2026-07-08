// License header
#include "Includes.h"
#include "Defines.h"
#include "Config.h"
#include "Backend/BackendObjects.h"
#include "Backend/Init.cpp" // Include Init directly

static void InitializeProject() {
    MGLOG_I("Initializing %s v%d.%d.%d ...", PROJECT_NAME, PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
    MobileGL::MG_Backend::Init();
    MGLOG_I("%s initialized successfully.", PROJECT_NAME);
}

// Static initialization block
struct StaticInit {
    StaticInit() {
        InitializeProject();
    }
};
static StaticInit g_staticInit;