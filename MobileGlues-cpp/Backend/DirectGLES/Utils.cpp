#include "Utils.h"

namespace MobileGL { namespace MG_Backend {

namespace DebugImpl {
    void ErrorLopper::Clear() {}
    void ErrorLopper::Loop(const char* file, int line, const char* expr) { (void)file; (void)line; (void)expr; }
    OpenGLScopeMarker::OpenGLScopeMarker(const char* label) : m_active(false) { (void)label; }
    OpenGLScopeMarker::~OpenGLScopeMarker() {}
}

namespace TextureImpl {
    TextureFormatInfo GenerateTextureFormatInfo(GLenum internalFormat) {
        TextureFormatInfo info = {};
        info.internalFormat = internalFormat;
        info.isValid = true;
        return info;
    }
    RenderbufferFormatInfo GenerateRenderbufferFormatInfo(GLenum internalFormat) {
        RenderbufferFormatInfo info = {};
        info.internalFormat = internalFormat;
        info.isValid = true;
        return info;
    }
    bool ShouldUseCaveatTextureFormat(GLenum) { return false; }
    bool ShouldUseCaveatRenderbufferFormat(GLenum) { return false; }
}

namespace PrgramImpl {
    int ProcessOutColorLocations(std::string&) { return 0; }
    void ForceSupporterOutput(std::string&) {}
    void ForceFlatIntegerVaryings(std::string&) {}
    int RemoveLayoutBinding(std::string&) { return 0; }
    void ClampNormFallbackOutputs(std::string&) {}
}

namespace Utils {
    void CheckGLESError(const char*, int, const char*) {}
    GLenum GetBindingQuery(GLenum) { return GL_NONE; }
    GLenum TranslateToGLESEnum(GLenum desktopEnum) { return desktopEnum; }
    GLenum TranslateInternalFormat(GLenum desktopFormat) { return desktopFormat; }
    bool TranslateFormatType(GLenum desktopFormat, GLenum desktopType, GLenum& outFormat, GLenum& outType) {
        outFormat = desktopFormat; outType = desktopType; return true;
    }
    bool HasExtension(const char*) { return false; }
    size_t GetGLTypeSize(GLenum) { return 4; }
    int GetGLFormatComponents(GLenum) { return 4; }
}

} } // namespace MobileGL::MG_Backend