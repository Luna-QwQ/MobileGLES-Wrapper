#pragma once
#include "Includes.h"
#include <GLES3/gl3.h>

namespace MobileGL { namespace MG_Backend {

// Backend function table
struct GLESFunctionTable {
    void* (*glGetError)() = nullptr;
    void* (*glGenBuffers)() = nullptr;
    void* (*glDeleteBuffers)() = nullptr;
    void* (*glBindBuffer)() = nullptr;
    void* (*glBufferData)() = nullptr;
    void* (*glBufferSubData)() = nullptr;
    void* (*glFlushMappedBufferRange)() = nullptr;
    void* (*glGenVertexArrays)() = nullptr;
    void* (*glDeleteVertexArrays)() = nullptr;
    void* (*glBindVertexArray)() = nullptr;
    void* (*glEnableVertexAttribArray)() = nullptr;
    void* (*glDisableVertexAttribArray)() = nullptr;
    void* (*glVertexAttribPointer)() = nullptr;
    void* (*glVertexAttribIPointer)() = nullptr;
    void* (*glVertexAttribDivisor)() = nullptr;
    void* (*glGenTextures)() = nullptr;
    void* (*glDeleteTextures)() = nullptr;
    void* (*glBindTexture)() = nullptr;
    void* (*glActiveTexture)() = nullptr;
    void* (*glTexImage2D)() = nullptr;
    void* (*glTexSubImage2D)() = nullptr;
    void* (*glCompressedTexImage2D)() = nullptr;
    void* (*glCompressedTexSubImage2D)() = nullptr;
    void* (*glTexImage3D)() = nullptr;
    void* (*glTexSubImage3D)() = nullptr;
    void* (*glCompressedTexImage3D)() = nullptr;
    void* (*glCompressedTexSubImage3D)() = nullptr;
    void* (*glTexStorage2D)() = nullptr;
    void* (*glTexStorage3D)() = nullptr;
    void* (*glTexStorage2DMultisample)() = nullptr;
    void* (*glTexBuffer)() = nullptr;
    void* (*glTexParameteri)() = nullptr;
    void* (*glTexParameterf)() = nullptr;
    void* (*glTexParameterfv)() = nullptr;
    void* (*glGenFramebuffers)() = nullptr;
    void* (*glDeleteFramebuffers)() = nullptr;
    void* (*glBindFramebuffer)() = nullptr;
    void* (*glFramebufferTexture2D)() = nullptr;
    void* (*glFramebufferTextureLayer)() = nullptr;
    void* (*glFramebufferRenderbuffer)() = nullptr;
    void* (*glGenRenderbuffers)() = nullptr;
    void* (*glDeleteRenderbuffers)() = nullptr;
    void* (*glBindRenderbuffer)() = nullptr;
    void* (*glRenderbufferStorage)() = nullptr;
    void* (*glRenderbufferStorageMultisample)() = nullptr;
    void* (*glDrawBuffers)() = nullptr;
    void* (*glReadBuffer)() = nullptr;
    void* (*glCreateProgram)() = nullptr;
    void* (*glDeleteProgram)() = nullptr;
    void* (*glCreateShader)() = nullptr;
    void* (*glDeleteShader)() = nullptr;
    void* (*glShaderSource)() = nullptr;
    void* (*glCompileShader)() = nullptr;
    void* (*glGetShaderiv)() = nullptr;
    void* (*glGetShaderInfoLog)() = nullptr;
    void* (*glAttachShader)() = nullptr;
    void* (*glDetachShader)() = nullptr;
    void* (*glGetAttachedShaders)() = nullptr;
    void* (*glLinkProgram)() = nullptr;
    void* (*glGetProgramiv)() = nullptr;
    void* (*glGetProgramInfoLog)() = nullptr;
    void* (*glUseProgram)() = nullptr;
    void* (*glBindAttribLocation)() = nullptr;
    void* (*glGetUniformBlockIndex)() = nullptr;
    void* (*glUniformBlockBinding)() = nullptr;
    void* (*glGetUniformLocation)() = nullptr;
    void* (*glUniform1i)() = nullptr;
    void* (*glGetString)() = nullptr;
    void* (*glGenSamplers)() = nullptr;
    void* (*glDeleteSamplers)() = nullptr;
    void* (*glBindSampler)() = nullptr;
    void* (*glSamplerParameteri)() = nullptr;
    void* (*glSamplerParameterf)() = nullptr;
    void* (*glSamplerParameterfv)() = nullptr;
    void* (*glTransformFeedbackVaryings)() = nullptr;
};

extern GLESFunctionTable g_GLESFuncs;

} } // namespace MobileGL::MG_Backend