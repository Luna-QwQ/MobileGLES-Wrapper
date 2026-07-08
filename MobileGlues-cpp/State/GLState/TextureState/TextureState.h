#pragma once
#include "../Core.h"
#include "TextureObject.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// TextureBindingSlot - holds a single texture bound to a target
// ============================================================================

struct TextureBindingSlot {
    SharedPtr<ITextureObject> texture;
    Uint32 version = 0;

    void Bind(const SharedPtr<ITextureObject>& tex) {
        texture = tex;
        ++version;
    }

    void Unbind() {
        texture.reset();
        ++version;
    }

    Bool IsBound() const {
        return texture != nullptr;
    }

    Uint32 GetVersion() const {
        return version;
    }
};

// ============================================================================
// TextureUnit - represents one texture image unit
// ============================================================================

static constexpr Int MAX_TEXTURE_IMAGE_UNITS = 32;

struct TextureUnit {
    TextureBindingSlot slots[4]; // For 1D, 2D, 3D, CubeMap bindings
    Uint32 unitIndex = 0;
    Uint32 version = 0;

    TextureUnit() {
        for (auto& slot : slots) {
            slot.version = 0;
        }
    }

    void BindTexture(Uint32 slotIndex, const SharedPtr<ITextureObject>& tex) {
        if (slotIndex < 4) {
            slots[slotIndex].Bind(tex);
            ++version;
        }
    }

    void UnbindTexture(Uint32 slotIndex) {
        if (slotIndex < 4) {
            slots[slotIndex].Unbind();
            ++version;
        }
    }

    const TextureBindingSlot& GetSlot(Uint32 slotIndex) const {
        return slots[slotIndex < 4 ? slotIndex : 0];
    }

    TextureBindingSlot& GetSlot(Uint32 slotIndex) {
        return slots[slotIndex < 4 ? slotIndex : 0];
    }

    Uint32 GetVersion() const {
        return version;
    }
};

// ============================================================================
// TextureState - manages all texture units and bindings
// ============================================================================

class TextureState {
public:
    TextureState();

    TextureUnit& GetUnit(Int index);
    const TextureUnit& GetUnit(Int index) const;

    void BindTexture(Int unit, TextureTarget target, const SharedPtr<ITextureObject>& texture);
    void UnbindTexture(Int unit, TextureTarget target);

    // Texture object management
    SharedPtr<ITextureObject> GetTextureObject(Uint32 index);
    void RegisterTextureObject(Uint32 index, const SharedPtr<ITextureObject>& tex);
    void DeleteTextureObject(Uint32 index);
    Bool HasTextureObject(Uint32 index) const;

    Int GetActiveUnit() const;
    void SetActiveUnit(Int unit);

private:
    TextureUnit mUnits[MAX_TEXTURE_IMAGE_UNITS];
    Int mActiveUnit;
    HashMap<Uint32, SharedPtr<ITextureObject>> mTextureObjects;
};

} } } // namespace MobileGL::MG_State::GLState