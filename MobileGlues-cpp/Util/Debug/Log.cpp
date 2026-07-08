#include "Util/Debug/Log.h"

// Debug logging placeholder
namespace MobileGL { namespace Util { namespace Debug {

void LogMessage(const char* msg) {
    MGLOG_D("%s", msg);
}

} } } // namespace MobileGL::Util::Debug