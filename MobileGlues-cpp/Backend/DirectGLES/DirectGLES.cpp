#include "DirectGLES.h"
#include "Managers.h"
#include "Backend/BackendObjects.h"

namespace MobileGL { namespace MG_Backend {

// DirectGLES backend implementation

void InitBackend() {
    MGLOG_I("DirectGLES backend initialized.");
}

void ShutdownBackend() {
    MGLOG_I("DirectGLES backend shutdown.");
}

} } // namespace MobileGL::MG_Backend