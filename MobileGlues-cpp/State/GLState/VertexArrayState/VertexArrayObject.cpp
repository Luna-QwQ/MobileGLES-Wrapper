#include "Includes.h"
#include "VertexArrayObject.h"
#include <algorithm>

namespace MobileGL { namespace MG_State { namespace GLState {

VertexArrayObject::VertexArrayObject()
    : mExternalIndex(0)
    , mVersion(0) {
    for (Int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) {
        mAttributes[i] = VertexAttribute{};
        mVersions[i] = VertexAttributeVersion{};
    }
    MGLOG_T("VertexArrayObject created (index=%u)", mExternalIndex);
}

VertexArrayObject::VertexArrayObject(Uint32 externalIndex)
    : mExternalIndex(externalIndex)
    , mVersion(0) {
    for (Int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) {
        mAttributes[i] = VertexAttribute{};
        mVersions[i] = VertexAttributeVersion{};
    }
    MGLOG_T("VertexArrayObject created (index=%u)", mExternalIndex);
}

VertexArrayObject::~VertexArrayObject() {
    MGLOG_T("VertexArrayObject destroyed (index=%u)", mExternalIndex);
}

Uint32 VertexArrayObject::GetExternalIndex() const {
    return mExternalIndex;
}

void VertexArrayObject::SetExternalIndex(Uint32 index) {
    mExternalIndex = index;
}

const Array<VertexAttribute, VertexArrayObject::MAX_VERTEX_ATTRIBS>& VertexArrayObject::GetAllAttributes() const {
    return mAttributes;
}

Array<VertexAttribute, VertexArrayObject::MAX_VERTEX_ATTRIBS>& VertexArrayObject::GetAllAttributesMutable() {
    return mAttributes;
}

VertexAttribute& VertexArrayObject::GetAttribute(Int index) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) {
        MGLOG_W("VertexArrayObject::GetAttribute: index %d out of range [0, %d)", index, MAX_VERTEX_ATTRIBS);
        static VertexAttribute dummy{};
        return dummy;
    }
    return mAttributes[index];
}

const VertexAttribute& VertexArrayObject::GetAttribute(Int index) const {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) {
        MGLOG_W("VertexArrayObject::GetAttribute: index %d out of range [0, %d)", index, MAX_VERTEX_ATTRIBS);
        static const VertexAttribute dummy{};
        return dummy;
    }
    return mAttributes[index];
}

void VertexArrayObject::SetAttributeEnabled(Int index, Bool enabled) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return;
    if (mAttributes[index].Enabled != enabled) {
        mAttributes[index].Enabled = enabled;
        ++mVersions[index].SwitchVersion;
        IncrementVersion();
    }
}

void VertexArrayObject::SetAttributeFormat(Int index, Int size, DataType type, Bool normalized, Int stride, SizeT offset) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return;
    auto& attr = mAttributes[index];
    if (attr.Size != size || attr.Type != type || attr.Normalized != normalized ||
        attr.Stride != stride || attr.Offset != offset) {
        attr.Size = size;
        attr.Type = type;
        attr.Normalized = normalized;
        attr.Stride = stride;
        attr.Offset = offset;
        ++mVersions[index].FormatVersion;
        IncrementVersion();
    }
}

void VertexArrayObject::SetAttributeBuffer(Int index, const SharedPtr<BufferObject>& buffer) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return;
    if (mAttributes[index].Buffer != buffer) {
        mAttributes[index].Buffer = buffer;
        ++mVersions[index].BufferVersion;
        IncrementVersion();
    }
}

void VertexArrayObject::SetAttributeDivisor(Int index, Int divisor) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return;
    if (mAttributes[index].Divisor != divisor) {
        mAttributes[index].Divisor = divisor;
        ++mVersions[index].FormatVersion;
        IncrementVersion();
    }
}

void VertexArrayObject::SetAttributeInteger(Int index, Bool isInteger) {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return;
    if (mAttributes[index].IsInteger != isInteger) {
        mAttributes[index].IsInteger = isInteger;
        ++mVersions[index].FormatVersion;
        IncrementVersion();
    }
}

const Array<VertexAttributeVersion, VertexArrayObject::MAX_VERTEX_ATTRIBS>& VertexArrayObject::GetAllAttributeVersions() const {
    return mVersions;
}

Uint16 VertexArrayObject::GetSwitchVersion(Int index) const {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return 0;
    return mVersions[index].SwitchVersion;
}

Uint16 VertexArrayObject::GetFormatVersion(Int index) const {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return 0;
    return mVersions[index].FormatVersion;
}

Uint16 VertexArrayObject::GetBufferVersion(Int index) const {
    if (index < 0 || index >= MAX_VERTEX_ATTRIBS) return 0;
    return mVersions[index].BufferVersion;
}

const VertexArrayObject::IndexBufferBinding& VertexArrayObject::GetIndexBufferBindingSlot() const {
    return mIndexBufferBinding;
}

VertexArrayObject::IndexBufferBinding& VertexArrayObject::GetIndexBufferBindingSlotMutable() {
    return mIndexBufferBinding;
}

void VertexArrayObject::SetIndexBuffer(const SharedPtr<BufferObject>& buffer, DataType type, SizeT offset) {
    mIndexBufferBinding.buffer = buffer;
    mIndexBufferBinding.type = type;
    mIndexBufferBinding.offset = offset;
    ++mIndexBufferBinding.version;
    IncrementVersion();
}

void VertexArrayObject::UnsetIndexBuffer() {
    mIndexBufferBinding.buffer.reset();
    mIndexBufferBinding.type = DataType::Uint32;
    mIndexBufferBinding.offset = 0;
    ++mIndexBufferBinding.version;
    IncrementVersion();
}

Uint32 VertexArrayObject::GetVersion() const {
    return mVersion;
}

void VertexArrayObject::IncrementVersion() {
    ++mVersion;
}

} } } // namespace MobileGL::MG_State::GLState