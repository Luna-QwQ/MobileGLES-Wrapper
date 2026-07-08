#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// VertexArrayObject
// ============================================================================

class VertexArrayObject {
public:
    static constexpr Int MAX_VERTEX_ATTRIBS = 16;

    VertexArrayObject();
    explicit VertexArrayObject(Uint32 externalIndex);
    ~VertexArrayObject();

    VertexArrayObject(const VertexArrayObject&) = delete;
    VertexArrayObject& operator=(const VertexArrayObject&) = delete;
    VertexArrayObject(VertexArrayObject&&) noexcept = default;
    VertexArrayObject& operator=(VertexArrayObject&&) noexcept = default;

    // External index
    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

    // Attribute access
    const Array<VertexAttribute, MAX_VERTEX_ATTRIBS>& GetAllAttributes() const;
    Array<VertexAttribute, MAX_VERTEX_ATTRIBS>& GetAllAttributesMutable();

    VertexAttribute& GetAttribute(Int index);
    const VertexAttribute& GetAttribute(Int index) const;

    void SetAttributeEnabled(Int index, Bool enabled);
    void SetAttributeFormat(Int index, Int size, DataType type, Bool normalized, Int stride, SizeT offset);
    void SetAttributeBuffer(Int index, const SharedPtr<BufferObject>& buffer);
    void SetAttributeDivisor(Int index, Int divisor);
    void SetAttributeInteger(Int index, Bool isInteger);

    // Version tracking
    const Array<VertexAttributeVersion, MAX_VERTEX_ATTRIBS>& GetAllAttributeVersions() const;

    Uint16 GetSwitchVersion(Int index) const;
    Uint16 GetFormatVersion(Int index) const;
    Uint16 GetBufferVersion(Int index) const;

    // Index buffer
    struct IndexBufferBinding {
        SharedPtr<BufferObject> buffer;
        DataType type = DataType::Uint32;
        SizeT offset = 0;
        Uint32 version = 0;
    };

    const IndexBufferBinding& GetIndexBufferBindingSlot() const;
    IndexBufferBinding& GetIndexBufferBindingSlotMutable();

    void SetIndexBuffer(const SharedPtr<BufferObject>& buffer, DataType type, SizeT offset);
    void UnsetIndexBuffer();

    // Overall VAO version
    Uint32 GetVersion() const;
    void IncrementVersion();

private:
    Uint32 mExternalIndex;
    Array<VertexAttribute, MAX_VERTEX_ATTRIBS> mAttributes;
    Array<VertexAttributeVersion, MAX_VERTEX_ATTRIBS> mVersions;
    IndexBufferBinding mIndexBufferBinding;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState