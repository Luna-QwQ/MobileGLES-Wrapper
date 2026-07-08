cmake_minimum_required(VERSION 3.16)
project(GLESWrapper VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
find_package(Threads REQUIRED)

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

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::OpenGL
    OpenGL::EGL
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_compile_definitions(${PROJECT_NAME} PRIVATE MOBILEGL_LOG_ACTIVE_LEVEL=0)

# For Android, the library is loaded by the app
# For desktop testing, build as regular shared library