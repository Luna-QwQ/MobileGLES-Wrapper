#pragma once
#include "Includes.h"
#include "BackendObject.h"

namespace MobileGL { namespace MG_Backend {

// Global pointer to the active backend object (used by Init.cpp and EGL exporting)
extern BackendObject* pActiveBackendObject;

// Global backend functions table (populated by Init.cpp)
// Contains only draw/clear/compute/blit functions
extern GlobalBackendFunctionsTable gBackendFunctionsTable;

// Global pointer to the backend's full GL functions table
// Points to the active BackendObject's internal functions table
// Used by GL/Exporting/Definitions.cpp for all GL function delegation
extern const GLFunctionsTable* g_pGLFunctionsTable;

void Init();
void Shutdown();

} } // namespace MobileGL::MG_Backend