#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// ErrorState - manages the GL error state
// ============================================================================

class ErrorState {
public:
    ErrorState();

    // Set a GL error (only sets if no error is already pending)
    void SetError(GLError error);

    // Get and clear the current GL error
    GLError GetError();

    // Peek at the current error without clearing
    GLError PeekError() const;

    // Check if there is a pending error
    Bool HasError() const;

    // Clear all errors
    void ClearErrors();

    // Get the error string
    static const char* GetErrorString(GLError error);

private:
    GLError mCurrentError;
    Mutex mMutex;
};

} } } // namespace MobileGL::MG_State::GLState