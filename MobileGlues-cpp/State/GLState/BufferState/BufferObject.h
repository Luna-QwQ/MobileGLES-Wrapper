#pragma once
#include "../Core.h"
#include <utility>

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// BufferObject
// ============================================================================

class BufferObject {
public:
    BufferObject();
    explicit BufferObject(Uint32 externalIndex);
    ~BufferObject();

    BufferObject(const BufferObject&) = delete;
    BufferObject& operator=(const BufferObject&) = delete;
    BufferObject(BufferObject&&) noexcept = default;
    BufferObject& operator=(BufferObject&&) noexcept = default;

    // Core accessors
    SizeT GetSize() const;
    const Uint8* GetDataReadOnly() const;
    Uint8* GetDataMutable();
    BufferUsage GetUsage() const;
    BufferChangeBits GetChangeBits() const;
    const Vector<std::pair<SizeT, SizeT>>& GetDirtyRanges() const;
    Uint32 GetExternalIndex() const;

    // Mutators
    void SetSize(SizeT size);
    void SetUsage(BufferUsage usage);
    void SetExternalIndex(Uint32 index);
    void ClearDirty();
    void MarkDirty(SizeT offset, SizeT length);
    void MarkPersistentMappedRangeDirty(SizeT offset, SizeT length);
    void SetChangeBits(BufferChangeBits bits);
    void AddChangeBits(BufferChangeBits bits);

    // Version tracking
    Uint32 GetVersion() const;
    void IncrementVersion();

    // Internal storage
    Vector<Uint8>& GetStorage();
    const Vector<Uint8>& GetStorage() const;

private:
    Uint32 mExternalIndex;
    Vector<Uint8> mData;
    BufferUsage mUsage;
    BufferChangeBits mChangeBits;
    Vector<std::pair<SizeT, SizeT>> mDirtyRanges;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState