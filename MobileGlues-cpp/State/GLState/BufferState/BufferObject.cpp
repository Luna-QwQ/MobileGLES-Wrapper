#include "Includes.h"
#include "BufferObject.h"
#include <algorithm>
#include <cstring>

namespace MobileGL { namespace MG_State { namespace GLState {

BufferObject::BufferObject()
    : mExternalIndex(0)
    , mUsage(BufferUsage::StaticDraw)
    , mChangeBits(BufferChangeBits::None)
    , mVersion(0) {
    MGLOG_T("BufferObject created (index=%u)", mExternalIndex);
}

BufferObject::BufferObject(Uint32 externalIndex)
    : mExternalIndex(externalIndex)
    , mUsage(BufferUsage::StaticDraw)
    , mChangeBits(BufferChangeBits::None)
    , mVersion(0) {
    MGLOG_T("BufferObject created (index=%u)", mExternalIndex);
}

BufferObject::~BufferObject() {
    MGLOG_T("BufferObject destroyed (index=%u)", mExternalIndex);
}

SizeT BufferObject::GetSize() const {
    return mData.size();
}

const Uint8* BufferObject::GetDataReadOnly() const {
    return mData.empty() ? nullptr : mData.data();
}

Uint8* BufferObject::GetDataMutable() {
    return mData.empty() ? nullptr : mData.data();
}

BufferUsage BufferObject::GetUsage() const {
    return mUsage;
}

BufferChangeBits BufferObject::GetChangeBits() const {
    return mChangeBits;
}

const Vector<std::pair<SizeT, SizeT>>& BufferObject::GetDirtyRanges() const {
    return mDirtyRanges;
}

Uint32 BufferObject::GetExternalIndex() const {
    return mExternalIndex;
}

void BufferObject::SetSize(SizeT size) {
    if (size != mData.size()) {
        mData.resize(size);
        mChangeBits = mChangeBits | BufferChangeBits::DirtyBit;
        mDirtyRanges.clear();
        if (size > 0) {
            mDirtyRanges.emplace_back(0, size);
        }
        IncrementVersion();
    }
}

void BufferObject::SetUsage(BufferUsage usage) {
    mUsage = usage;
}

void BufferObject::SetExternalIndex(Uint32 index) {
    mExternalIndex = index;
}

void BufferObject::ClearDirty() {
    mChangeBits = BufferChangeBits::None;
    mDirtyRanges.clear();
}

void BufferObject::MarkDirty(SizeT offset, SizeT length) {
    if (offset >= mData.size()) {
        MGLOG_W("MarkDirty: offset %zu out of range (size=%zu)", offset, mData.size());
        return;
    }
    SizeT end = offset + length;
    if (end > mData.size()) {
        end = mData.size();
    }
    mChangeBits = mChangeBits | BufferChangeBits::DirtyBit;
    mDirtyRanges.emplace_back(offset, end);
    IncrementVersion();
}

void BufferObject::MarkPersistentMappedRangeDirty(SizeT offset, SizeT length) {
    MarkDirty(offset, length);
}

void BufferObject::SetChangeBits(BufferChangeBits bits) {
    mChangeBits = bits;
}

void BufferObject::AddChangeBits(BufferChangeBits bits) {
    mChangeBits = mChangeBits | bits;
}

Uint32 BufferObject::GetVersion() const {
    return mVersion;
}

void BufferObject::IncrementVersion() {
    ++mVersion;
}

Vector<Uint8>& BufferObject::GetStorage() {
    return mData;
}

const Vector<Uint8>& BufferObject::GetStorage() const {
    return mData;
}

} } } // namespace MobileGL::MG_State::GLState