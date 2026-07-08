#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <optional>
#include <array>
#include <functional>

#include "Defines.h"
#include "Types.h"
#include "Util/Debug/Log.h"

// ============================================================================
// Fundamental type aliases (using the same naming as the project convention)
// ============================================================================

using Uint8   = uint8_t;
using Uint16  = uint16_t;
using Uint32  = uint32_t;
using Uint64  = uint64_t;

using Int8    = int8_t;
using Int16   = int16_t;
using Int32   = int32_t;
using Int64   = int64_t;

using Int     = int32_t;
using Uint    = uint32_t;
using Float   = float;
using Bool    = bool;
using SizeT   = size_t;

// ============================================================================
// Standard library type aliases
// ============================================================================

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T>
using Vector = std::vector<T>;

template <typename T, SizeT N>
using Array = std::array<T, N>;

template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template <typename T>
using Optional = std::optional<T>;

using String = std::string;
using Mutex = std::mutex;
using LockGuard = std::lock_guard<std::mutex>;

// ============================================================================
// Flags template for enum bitmasks
// ============================================================================

template <typename E>
class Flags {
public:
    using UnderlyingType = typename std::underlying_type<E>::type;

    Flags() : mValue(0) {}
    Flags(E value) : mValue(static_cast<UnderlyingType>(value)) {}
    explicit Flags(UnderlyingType value) : mValue(value) {}

    Flags operator|(Flags other) const { return Flags(mValue | other.mValue); }
    Flags operator&(Flags other) const { return Flags(mValue & other.mValue); }
    Flags operator^(Flags other) const { return Flags(mValue ^ other.mValue); }
    Flags& operator|=(Flags other) { mValue |= other.mValue; return *this; }
    Flags& operator&=(Flags other) { mValue &= other.mValue; return *this; }
    Flags& operator^=(Flags other) { mValue ^= other.mValue; return *this; }
    bool operator!() const { return mValue == 0; }
    explicit operator bool() const { return mValue != 0; }
    bool operator==(Flags other) const { return mValue == other.mValue; }
    bool operator!=(Flags other) const { return mValue != other.mValue; }
    bool operator==(UnderlyingType v) const { return mValue == v; }
    bool operator!=(UnderlyingType v) const { return mValue != v; }
    UnderlyingType Value() const { return mValue; }

private:
    UnderlyingType mValue;
};

// ============================================================================
// BackendType enum (defined here to avoid circular dependencies)
// ============================================================================

enum class BackendType : Uint32 {
    DirectGLES = 0,
    Unknown = 0xFFFF
};

// ============================================================================
// GLdouble (not defined in GLES3 headers)
// ============================================================================

#ifndef GLdouble
typedef double GLdouble;
#endif

// ============================================================================
// Math vector types
// ============================================================================

template <typename T>
struct IntVec2 {
    T x = 0, y = 0;
    IntVec2() = default;
    IntVec2(T x, T y) : x(x), y(y) {}
};

template <typename T>
struct IntVec4 {
    T x = 0, y = 0, z = 0, w = 0;
    IntVec4() = default;
    IntVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
};