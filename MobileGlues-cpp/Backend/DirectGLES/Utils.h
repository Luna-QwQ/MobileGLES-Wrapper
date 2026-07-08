#pragma once
#include "DirectGLES.h"
#include <GLES3/gl3.h>

namespace MobileGL { namespace MG_Backend {

// Debug utilities
namespace DebugImpl {
    struct ErrorLopper {
        static void Clear();
        static void Loop(const char* file, int line, const char* expr);
    };
    struct OpenGLScopeMarker {
        bool m_active;
        OpenGLScopeMarker(const char* label);
        ~OpenGLScopeMarker();
    };
}

// Texture format info
namespace TextureImpl {
    struct TextureFormatInfo {
        GLenum format = 0;
        GLenum type = 0;
        GLenum internalFormat = 0;
        Int pixelSize = 0;
        Bool isCompressed = false;
        Bool isDepthStencil = false;
        Bool isValid = false;
    };
    struct RenderbufferFormatInfo {
        GLenum internalFormat = 0;
        Bool isDepthStencil = false;
        Bool isValid = false;
    };
    TextureFormatInfo GenerateTextureFormatInfo(GLenum internalFormat);
    RenderbufferFormatInfo GenerateRenderbufferFormatInfo(GLenum internalFormat);
    bool ShouldUseCaveatTextureFormat(GLenum internalFormat);
    bool ShouldUseCaveatRenderbufferFormat(GLenum internalFormat);
}

// Shader processing utilities
namespace PrgramImpl {
    int ProcessOutColorLocations(std::string& source);
    void ForceSupporterOutput(std::string& source);
    void ForceFlatIntegerVaryings(std::string& source);
    int RemoveLayoutBinding(std::string& source);
    void ClampNormFallbackOutputs(std::string& source);
}

// General utilities
namespace Utils {
    void CheckGLESError(const char* file, int line, const char* expr);
    GLenum GetBindingQuery(GLenum target);
    GLenum TranslateToGLESEnum(GLenum desktopEnum);
    GLenum TranslateInternalFormat(GLenum desktopFormat);
    bool TranslateFormatType(GLenum desktopFormat, GLenum desktopType, GLenum& outFormat, GLenum& outType);
    bool HasExtension(const char* extension);
    size_t GetGLTypeSize(GLenum type);
    int GetGLFormatComponents(GLenum format);
}

} } // namespace MobileGL::MG_Backend