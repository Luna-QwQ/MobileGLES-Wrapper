#include "Includes.h"
#include "SamplerObject.h"

namespace MobileGL { namespace MG_State { namespace GLState {

SamplerObject::SamplerObject()
    : mExternalIndex(0)
    , mVersion(0) {
    MGLOG_T("SamplerObject created (index=%u)", mExternalIndex);
}

SamplerObject::SamplerObject(Uint32 externalIndex)
    : mExternalIndex(externalIndex)
    , mVersion(0) {
    MGLOG_T("SamplerObject created (index=%u)", mExternalIndex);
}

SamplerObject::~SamplerObject() {
    MGLOG_T("SamplerObject destroyed (index=%u)", mExternalIndex);
}

Uint32 SamplerObject::GetVersion() const {
    return mVersion;
}

void SamplerObject::IncrementVersion() {
    ++mVersion;
}

const SamplerParameters& SamplerObject::GetAllSamplerParameters() const {
    return mParams;
}

SamplerParameters& SamplerObject::GetAllSamplerParametersMutable() {
    return mParams;
}

void SamplerObject::SetMinFilter(SamplerFilterMode mode) {
    if (mParams.minFilter != mode) {
        mParams.minFilter = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetMagFilter(SamplerFilterMode mode) {
    if (mParams.magFilter != mode) {
        mParams.magFilter = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetMipmapMode(SamplerMipmapMode mode) {
    if (mParams.mipmapMode != mode) {
        mParams.mipmapMode = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetWrapS(SamplerWrapMode mode) {
    if (mParams.wrapS != mode) {
        mParams.wrapS = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetWrapT(SamplerWrapMode mode) {
    if (mParams.wrapT != mode) {
        mParams.wrapT = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetWrapR(SamplerWrapMode mode) {
    if (mParams.wrapR != mode) {
        mParams.wrapR = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetCompareMode(SamplerCompareMode mode) {
    if (mParams.compareMode != mode) {
        mParams.compareMode = mode;
        IncrementVersion();
    }
}

void SamplerObject::SetSamplerCompareFunc(SamplerCompareFunc func) {
    if (mParams.compareFunc != func) {
        mParams.compareFunc = func;
        IncrementVersion();
    }
}

void SamplerObject::SetMinLod(Float lod) {
    if (mParams.minLod != lod) {
        mParams.minLod = lod;
        IncrementVersion();
    }
}

void SamplerObject::SetMaxLod(Float lod) {
    if (mParams.maxLod != lod) {
        mParams.maxLod = lod;
        IncrementVersion();
    }
}

SamplerFilterMode SamplerObject::GetMinFilter() const {
    return mParams.minFilter;
}

SamplerFilterMode SamplerObject::GetMagFilter() const {
    return mParams.magFilter;
}

SamplerMipmapMode SamplerObject::GetMipmapMode() const {
    return mParams.mipmapMode;
}

SamplerWrapMode SamplerObject::GetWrapS() const {
    return mParams.wrapS;
}

SamplerWrapMode SamplerObject::GetWrapT() const {
    return mParams.wrapT;
}

SamplerWrapMode SamplerObject::GetWrapR() const {
    return mParams.wrapR;
}

SamplerCompareMode SamplerObject::GetCompareMode() const {
    return mParams.compareMode;
}

SamplerCompareFunc SamplerObject::GetCompareFunc() const {
    return mParams.compareFunc;
}

Float SamplerObject::GetMinLod() const {
    return mParams.minLod;
}

Float SamplerObject::GetMaxLod() const {
    return mParams.maxLod;
}

Uint32 SamplerObject::GetExternalIndex() const {
    return mExternalIndex;
}

void SamplerObject::SetExternalIndex(Uint32 index) {
    mExternalIndex = index;
}

} } } // namespace MobileGL::MG_State::GLState