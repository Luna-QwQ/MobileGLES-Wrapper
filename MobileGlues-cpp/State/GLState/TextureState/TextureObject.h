#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// ITextureObject - base class for all texture types
// ============================================================================

class ITextureObject {
public:
    virtual ~ITextureObject() = default;

    virtual TextureTarget GetTarget() const = 0;
    virtual TextureInternalFormat GetFormat() const = 0;
    virtual IntVec2<Int> GetBaseSize() const = 0;
    virtual Int GetSamples() const = 0;
    virtual Bool HasFixedSampleLocations() const = 0;
    virtual Bool IsComplete() const = 0;
    virtual Bool IsImmutable() const = 0;
    virtual TextureStorageType GetStorageType() const = 0;
    virtual SharedPtr<SamplerObject> GetSamplerObject() const = 0;
    virtual IntVec2<Int> GetLevelRange() const = 0;
    virtual Array<TextureSwizzleParam, 4> GetAllSwizzleParams() const = 0;
    virtual Array<Float, 4> GetBorderColor() const = 0;
    virtual Uint32 GetExternalIndex() const = 0;
    virtual Uint32 GetTextureParamsVersion() const = 0;

    // Default implementations for common queries
    virtual Bool IsTexture1D() const { return false; }
    virtual Bool IsTexture2D() const { return false; }
    virtual Bool IsTexture3D() const { return false; }
    virtual Bool IsTextureCubeMap() const { return false; }
    virtual Bool IsTextureArray() const { return false; }
    virtual Bool IsTextureMultisample() const { return false; }
};

} } } // namespace MobileGL::MG_State::GLState