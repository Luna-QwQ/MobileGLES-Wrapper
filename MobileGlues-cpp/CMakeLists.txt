cmake_minimum_required(VERSION 3.16)
project(GLESWrapper VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find packages
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

# This project is a GLES wrapper — it needs EGL + GLESv2, not desktop OpenGL.
# Try pkg-config first (works on most Linux distros and CI), then fall back to CMake's FindOpenGL.
set(EGL_FOUND FALSE)
set(GLESv2_FOUND FALSE)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(EGL QUIET egl)
    pkg_check_modules(GLESv2 QUIET glesv2)
endif()

if(EGL_FOUND AND GLESv2_FOUND)
    message(STATUS "Found EGL via pkg-config: ${EGL_LIBRARIES}")
    message(STATUS "Found GLESv2 via pkg-config: ${GLESv2_LIBRARIES}")
    set(EGL_LIBRARY ${EGL_LIBRARIES})
    set(GLESv2_LIBRARY ${GLESv2_LIBRARIES})
    set(EGL_INCLUDE_DIR ${EGL_INCLUDE_DIRS})
    set(GLESv2_INCLUDE_DIR ${GLESv2_INCLUDE_DIRS})
else()
    # Fallback: try CMake's FindOpenGL for EGL component only
    find_package(OpenGL QUIET COMPONENTS EGL)
    if(OpenGL_EGL_FOUND)
        message(STATUS "Found EGL via FindOpenGL: ${OPENGL_egl_LIBRARY}")
        set(EGL_LIBRARY ${OPENGL_egl_LIBRARY})
        set(EGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
    else()
        # Last resort: try to find EGL in common system paths
        find_library(EGL_LIBRARY NAMES EGL egl PATHS /usr/lib /usr/lib64 /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu)
        if(EGL_LIBRARY)
            message(STATUS "Found EGL at: ${EGL_LIBRARY}")
        endif()
    endif()
    
    find_package(OpenGL QUIET COMPONENTS OpenGL)
    if(OPENGL_FOUND)
        message(STATUS "Found GLESv2 via FindOpenGL (GLESv2 bundled with OpenGL)")
        set(GLESv2_LIBRARY ${OPENGL_opengl_LIBRARY})
        set(GLESv2_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
    else()
        find_library(GLESv2_LIBRARY NAMES GLESv2 GLESv2 glesv2 PATHS /usr/lib /usr/lib64 /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu)
        if(GLESv2_LIBRARY)
            message(STATUS "Found GLESv2 at: ${GLESv2_LIBRARY}")
        endif()
    endif()
endif()

if(NOT EGL_LIBRARY)
    message(FATAL_ERROR "EGL library not found. Install libegl1-mesa-dev or equivalent.")
endif()
if(NOT GLESv2_LIBRARY)
    message(FATAL_ERROR "GLESv2 library not found. Install libgles2-mesa-dev or equivalent.")
endif()

# Source files
set(SOURCES
    GlobalObjects.cpp
    Config.cpp
    
    Backend/BackendObject.cpp
    Backend/DirectGLES/DirectGLES.cpp
    Backend/DirectGLES/Managers.cpp
    Backend/DirectGLES/Utils.cpp
    Backend/DirectGLES/BackendObject_DirectGLES.cpp
    
    GL/Exporting/Definitions.cpp
    EGL/Exporting/Definitions.cpp
    
    State/GLState/Core.cpp
    State/GLState/BufferState/BufferObject.cpp
    State/GLState/SamplerState/SamplerObject.cpp
    State/GLState/VertexArrayState/VertexArrayObject.cpp
    
    Util/Loaders/OpenGL/Loader.cpp
    Util/Converters/MGToGL/TextureEnumConverter.cpp
    Util/Converters/GLToMG/TextureEnumConverter.cpp
    Util/Converters/MGToGL/BufferEnumConverter.cpp
    Util/Converters/MGToGL/FramebufferEnumConverter.cpp
    Util/Converters/MGToGL/ProgramEnumConverter.cpp
    Util/Converters/MGToStr/TextureEnumConverter.cpp
    Util/Converters/GLToStr/GLEnumConverter.cpp
    Util/Converters/MGToStr/FramebufferEnumConverter.cpp
    Util/Debug/Log.cpp
    Util/Classifiers/TextureEnumClassifier.cpp
    Util/Texture/TextureFormatProcessor.cpp
    Util/Texture/PixelStoreProcessor.cpp
    Util/Metrics/TextureMetrics.cpp
)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(EGL_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${EGL_INCLUDE_DIR})
endif()
if(GLESv2_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLESv2_INCLUDE_DIR})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${EGL_LIBRARY}
    ${GLESv2_LIBRARY}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_compile_definitions(${PROJECT_NAME} PRIVATE MOBILEGL_LOG_ACTIVE_LEVEL=0)