#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// FramebufferAttachmentObject - represents a texture or renderbuffer attachment
// ============================================================================

class FramebufferAttachmentObject {
public:
    FramebufferAttachmentObject();
    ~FramebufferAttachmentObject();

    FramebufferAttachmentObject(const FramebufferAttachmentObject&) = delete;
    FramebufferAttachmentObject& operator=(const FramebufferAttachmentObject&) = delete;
    FramebufferAttachmentObject(FramebufferAttachmentObject&&) noexcept = default;
    FramebufferAttachmentObject& operator=(FramebufferAttachmentObject&&) noexcept = default;

    // Texture attachment
    void SetTexture(const SharedPtr<ITextureObject>& texture, Int level, Int layer);
    const SharedPtr<ITextureObject>& GetTexture() const;
    Int GetTextureLevel() const;
    Int GetTextureLayer() const;

    // Renderbuffer attachment
    void SetRenderbuffer(const SharedPtr<RenderbufferObject>& renderbuffer);
    const SharedPtr<RenderbufferObject>& GetRenderbuffer() const;

    // State
    Bool IsAttached() const;
    Bool IsTextureAttachment() const;
    Bool IsRenderbufferAttachment() const;
    void Detach();

    Uint32 GetVersion() const;

private:
    SharedPtr<ITextureObject> mTexture;
    SharedPtr<RenderbufferObject> mRenderbuffer;
    Int mTextureLevel;
    Int mTextureLayer;
    Uint32 mVersion;
};

// ============================================================================
// FramebufferObject
// ============================================================================

class FramebufferObject {
public:
    static constexpr Int MAX_DRAW_BUFFERS = 8;

    FramebufferObject();
    explicit FramebufferObject(Uint32 externalIndex);
    ~FramebufferObject();

    FramebufferObject(const FramebufferObject&) = delete;
    FramebufferObject& operator=(const FramebufferObject&) = delete;
    FramebufferObject(FramebufferObject&&) noexcept = default;
    FramebufferObject& operator=(FramebufferObject&&) noexcept = default;

    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

    // Draw buffers
    const Array<FramebufferAttachmentType, MAX_DRAW_BUFFERS>& GetDrawBuffers() const;
    void SetDrawBuffer(Int slot, FramebufferAttachmentType type);
    void SetDrawBuffers(const Array<FramebufferAttachmentType, MAX_DRAW_BUFFERS>& buffers);

    // Read buffer
    FramebufferAttachmentType GetReadBuffer() const;
    void SetReadBuffer(FramebufferAttachmentType type);

    // Attachment objects
    const HashMap<FramebufferAttachmentType, FramebufferAttachmentObject>& GetAllAttachmentObjects() const;
    HashMap<FramebufferAttachmentType, FramebufferAttachmentObject>& GetAllAttachmentObjectsMutable();

    FramebufferAttachmentObject& GetAttachment(FramebufferAttachmentType type);
    const FramebufferAttachmentObject& GetAttachment(FramebufferAttachmentType type) const;

    void AttachTexture(FramebufferAttachmentType type, const SharedPtr<ITextureObject>& texture, Int level, Int layer);
    void AttachRenderbuffer(FramebufferAttachmentType type, const SharedPtr<RenderbufferObject>& renderbuffer);
    void Detach(FramebufferAttachmentType type);

    // Version tracking
    const HashMap<FramebufferAttachmentType, Uint32>& GetAllFramebufferAttachmentVersions() const;
    Uint32 GetAttachmentVersion(FramebufferAttachmentType type) const;
    Uint32 GetVersion() const;

private:
    Uint32 mExternalIndex;
    Array<FramebufferAttachmentType, MAX_DRAW_BUFFERS> mDrawBuffers;
    FramebufferAttachmentType mReadBuffer;
    HashMap<FramebufferAttachmentType, FramebufferAttachmentObject> mAttachments;
    HashMap<FramebufferAttachmentType, Uint32> mAttachmentVersions;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState