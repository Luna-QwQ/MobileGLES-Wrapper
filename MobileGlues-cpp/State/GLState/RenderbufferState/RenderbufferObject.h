#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// RenderbufferObject
// ============================================================================

class RenderbufferObject {
public:
    RenderbufferObject();
    explicit RenderbufferObject(Uint32 externalIndex);
    ~RenderbufferObject();

    RenderbufferObject(const RenderbufferObject&) = delete;
    RenderbufferObject& operator=(const RenderbufferObject&) = delete;
    RenderbufferObject(RenderbufferObject&&) noexcept = default;
    RenderbufferObject& operator=(RenderbufferObject&&) noexcept = default;

    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

    TextureInternalFormat GetInternalFormat() const;
    void SetInternalFormat(TextureInternalFormat format);

    Int GetWidth() const;
    void SetWidth(Int width);

    Int GetHeight() const;
    void SetHeight(Int height);

    Int GetSamples() const;
    void SetSamples(Int samples);

    Uint32 GetVersion() const;
    void IncrementVersion();

private:
    Uint32 mExternalIndex;
    TextureInternalFormat mInternalFormat;
    Int mWidth;
    Int mHeight;
    Int mSamples;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState