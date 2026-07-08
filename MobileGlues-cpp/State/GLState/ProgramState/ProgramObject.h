#pragma once
#include "../Core.h"

namespace MobileGL { namespace MG_State { namespace GLState {

// ============================================================================
// ShaderObject
// ============================================================================

class ShaderObject {
public:
    ShaderObject();
    explicit ShaderObject(Uint32 externalIndex);
    ~ShaderObject();

    ShaderObject(const ShaderObject&) = delete;
    ShaderObject& operator=(const ShaderObject&) = delete;
    ShaderObject(ShaderObject&&) noexcept = default;
    ShaderObject& operator=(ShaderObject&&) noexcept = default;

    const String& GetShaderSource() const;
    void SetShaderSource(const String& source);

    ShaderStage GetShaderStage() const;
    void SetShaderStage(ShaderStage stage);

    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

    Bool IsCompiled() const;
    void SetCompiled(Bool compiled);

    const String& GetInfoLog() const;
    void SetInfoLog(const String& log);

    Uint32 GetVersion() const;

private:
    Uint32 mExternalIndex;
    String mShaderSource;
    ShaderStage mStage;
    Bool mCompiled;
    String mInfoLog;
    Uint32 mVersion;
};

// ============================================================================
// ProgramObject
// ============================================================================

class ProgramObject {
public:
    ProgramObject();
    explicit ProgramObject(Uint32 externalIndex);
    ~ProgramObject();

    ProgramObject(const ProgramObject&) = delete;
    ProgramObject& operator=(const ProgramObject&) = delete;
    ProgramObject(ProgramObject&&) noexcept = default;
    ProgramObject& operator=(ProgramObject&&) noexcept = default;

    Uint32 GetExternalIndex() const;
    void SetExternalIndex(Uint32 index);

    // Link status
    Bool GetLinkStatus() const;
    void SetLinkStatus(Bool linked);

    // Attached shaders
    const Vector<SharedPtr<ShaderObject>>& GetAttachedShaders() const;
    void AttachShader(const SharedPtr<ShaderObject>& shader);
    void DetachShader(const SharedPtr<ShaderObject>& shader);
    Bool HasShader(const SharedPtr<ShaderObject>& shader) const;

    // SPIR-V
    const Vector<Uint32>& GetGeneratedSpirv() const;
    void SetGeneratedSpirv(const Vector<Uint32>& spirv);

    // Uniform buffer object size
    Int GetUBOSize(Uint32 bindingIndex) const;
    void SetUBOSize(Uint32 bindingIndex, Int size);

    // Info log
    const String& GetInfoLog() const;
    void SetInfoLog(const String& log);

    // Version
    Uint32 GetVersion() const;
    void IncrementVersion();

private:
    Uint32 mExternalIndex;
    Bool mLinked;
    Vector<SharedPtr<ShaderObject>> mAttachedShaders;
    Vector<Uint32> mSpirv;
    HashMap<Uint32, Int> mUBOSizes;
    String mInfoLog;
    Uint32 mVersion;
};

} } } // namespace MobileGL::MG_State::GLState