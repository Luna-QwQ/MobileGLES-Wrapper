#pragma once
#include "../Core.h"
#include "BufferObject.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// BufferBindingSlot - holds a single buffer bound to a target
// ============================================================================

struct BufferBindingSlot {
    SharedPtr<BufferObject> buffer;
    SizeT offset = 0;
    SizeT size = 0;
    Uint32 version = 0;

    void Bind(const SharedPtr<BufferObject>& buf, SizeT off, SizeT sz) {
        buffer = buf;
        offset = off;
        size = sz;
        ++version;
    }

    void Unbind() {
        buffer.reset();
        offset = 0;
        size = 0;
        ++version;
    }

    Bool IsBound() const {
        return buffer != nullptr;
    }

    Uint32 GetVersion() const {
        return version;
    }
};

// ============================================================================
// BufferBindingPoint - manages all binding slots for buffer targets
// ============================================================================

class BufferBindingPoint {
public:
    BufferBindingPoint();

    BufferBindingSlot& GetSlot(BufferTarget target);
    const BufferBindingSlot& GetSlot(BufferTarget target) const;

    void Bind(BufferTarget target, const SharedPtr<BufferObject>& buffer, SizeT offset, SizeT size);
    void Unbind(BufferTarget target);

    const SharedPtr<BufferObject>& GetBoundBuffer(BufferTarget target) const;
    Uint32 GetVersion(BufferTarget target) const;

private:
    static constexpr SizeT kTargetCount = static_cast<SizeT>(BufferTarget::BufferTargetCount);
    Array<BufferBindingSlot, kTargetCount> mSlots;
};

// ============================================================================
// BufferState - the complete buffer state manager
// ============================================================================

class BufferState {
public:
    BufferState();

    // Binding
    void BindBuffer(BufferTarget target, Uint32 bufferIndex);
    void BindBuffer(BufferTarget target, const SharedPtr<BufferObject>& buffer);
    void BindBufferRange(BufferTarget target, Uint32 bufferIndex, SizeT offset, SizeT size);
    void UnbindBuffer(BufferTarget target);

    // Buffer object management
    SharedPtr<BufferObject> GetBufferObject(Uint32 index);
    SharedPtr<BufferObject> CreateBufferObject(Uint32 index);
    void DeleteBufferObject(Uint32 index);
    Bool HasBufferObject(Uint32 index) const;

    // Query
    const SharedPtr<BufferObject>& GetBoundBuffer(BufferTarget target) const;
    BufferBindingPoint& GetBindingPoint();
    const BufferBindingPoint& GetBindingPoint() const;

private:
    BufferBindingPoint mBindingPoint;
    HashMap<Uint32, SharedPtr<BufferObject>> mBufferObjects;
};

} } } // namespace MobileGL::MG_State::GLState