#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// SamplerObject
// ============================================================================

class SamplerObject {
public:
    SamplerObject();
    explicit SamplerObject(Uint32 externalIndex);
    ~SamplerObject();

    SamplerObject(const SamplerObject&) = delete;
    SamplerObject& operator=(const SamplerObject&) = delete;
    SamplerObject(SamplerObject&&) noexcept = default;
    SamplerObject& operator=(SamplerObject&&) noexcept = default;

    // Version tracking
    Uint32 GetVersion() const;
    void IncrementVersion();

    // Parameter access
    const SamplerParameters& GetAllSamplerParameters() const;
    SamplerParameters& GetAllSamplerParametersMutable();

    // Individual parameter setters/getters
    void SetMinFilter(SamplerFilterMode mode);
    void SetMagFilter(SamplerFilterMode mode);
    void SetMipmapMode(SamplerMipmapMode mode);
    void SetWrapS(SamplerWrapMode mode);
    void SetWrapT(SamplerWrapMode mode);
    void SetWrapR(SamplerWrapMode mode);
    void SetCompareMode(SamplerCompareMode mode);
    void SetSamplerCompareFunc(SamplerCompareFunc func);
    void SetMinLod(Float lod);
    void SetMaxLod(Float lod);

    SamplerFilterMode GetMinFilter() const;
    SamplerFilterMode GetMagFilter() const;
    SamplerMipmapMode GetMipmapMode() const;
    SamplerWrapMode GetWrapS() const;
    SamplerWrapMode GetWrapT() const;
    SamplerWrapMode GetWrapR() const;
    SamplerCompareMode GetCompareMode() const;
    SamplerCompareFunc GetCompareFunc() const;
    Float GetMinLod() const;
    Float GetMaxLod() const;

    // External index
    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

private:
    Uint32 mExternalIndex;
    SamplerParameters mParams;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState