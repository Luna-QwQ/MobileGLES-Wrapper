# MobileGLES

> **非官方分支。** MobileGLES 是
> [MobileGlues](https://github.com/MobileGL-Dev/MobileGlues)
> 项目（"(on) Mobile, GL uses ES"）的**非官方**社区分支。它与上游
> MobileGlues 团队不存在隶属、背书或维护关系，亦不由其维护。所有
> MobileGLES 自身的改动均由本分支的贡献者负责。

MobileGLES 是一个桌面 OpenGL 兼容层，运行在宿主 **OpenGL ES 3.0** 后端
之上，旨在让 Minecraft: Java Edition 能够在移动级 GPU 上运行。它通过
glslang + SPIRV-Cross 将桌面 GL / GLSL 调用翻译为 OpenGL ES 3.0 /
GLSL ES 3.00，从而让 Java 版的 OpenGL 渲染器在移动类 GPU 上跑起来。

## 仅支持 OpenGL ES 3.0

MobileGLES **仅以 OpenGL ES 3.0 为目标**。

- **已移除 GLES 3.1 与 GLES 3.2 支持。** 层所感知到的宿主后端版本会被
  强制钳制到 3.0，所有按版本分支的代码路径一律走 GLES 3.0 路线。
- 不属于 OpenGL ES 3.0 核心的功能（计算着色器、仅 SSBO 路径、纹理缓冲
  区、核心版 `glDrawElementsBaseVertex` 等），要么**在软件层模拟**，
  要么在宿主驱动暴露相应扩展时通过 **EXT/OES 扩展**使用——绝不会通过
  3.1/3.2 的核心依赖来使用。
- 暴露给应用程序的着色语言版本始终反映 GLES 3.0 基线
  （`#version 300 es` / GLSL `4.00`）。
- 仅声明支持 OpenGL ES 3.0 的宿主被完全支持；声明 3.1/3.2 的宿主会被
  本层视作 3.0。

如果宿主报告的版本低于 3.0，MobileGLES 会在日志中记录该版本不受支持，
并拒绝使用任何不可用的特性。

## 平台范围：仅限 iOS

MobileGLES **仅以 iOS 为目标平台**，拒绝在任何其他平台
（Android、Linux、macOS 桌面、Windows）上构建或运行：

- CMake 配置会在非 Apple 平台上发出致命错误。
- 编译期检查（`TARGET_OS_IPHONE`）会拒绝 macOS 桌面等非 iOS 的 Apple
  目标。
- 它依赖预构建的 **ANGLE** 框架（`libEGL.framework`、
  `libGLESv2.framework`，基于 **Metal** 构建），并静态链接进宿主 iOS
  应用。运行时不进行 `dlopen`——EGL/GLES 符号通过主可执行镜像解析。

### iOS 构建优化

iOS 构建应用了若干平台相关的优化：

- 在工具链支持时启用 **ThinLTO**（链接期优化），否则回退到完整 IPO。
- 以 `-O2` 为基线，并配合 `-ffunction-sections -fdata-sections` 与
  `-Wl,-dead_strip` 去除不可达代码/数据，缩小内嵌 dylib 体积。
- 使用 `-fvisibility-inlines-hidden` 与 `-fomit-frame-pointer` 生成更
  小、更快的 arm64 代码。
- 显式请求 **GLES 3.0 EGL 上下文**（`EGL_OPENGL_ES3_BIT`、
  `EGL_CONTEXT_CLIENT_VERSION = 3`），而非 ES2 上下文。

## 致着色器开发者

1. MobileGLES 会自动：
   - 将桌面 GLSL 转换为 GLSL ES 3.00
   - 移除 `layout(binding)` 语法
   - 处理版本指令
   - 始终显式声明精度：
     ```glsl
     precision highp float;
     precision highp int;
     ```

2. MobileGLES 会向你的着色器注入以下宏：
   ```glsl
   #define MG_MOBILEGLES                   // 标识 MobileGLES 环境
   #define MG_MOBILEGLES_VERSION 1000      // 版本号（例如 1000 = V1.0.0）
   ```

3. 遇到问题时：
   - 启用 `Ignore shader/program error`，并查看日志（位于 iOS 应用沙盒
     内的 `$HOME/Documents/MG/latest.log`）。

## 许可证

MobileGLES 采用 **GNU LGPL-2.1** 许可证。

详见 [LICENSE](LICENSE)。

## 第三方组件

**SPIRV-Cross** — **KhronosGroup** 出品 - [Apache License 2.0](https://github.com/KhronosGroup/SPIRV-Cross/blob/master/LICENSE)：[github](https://github.com/KhronosGroup/SPIRV-Cross)

**glslang** — **KhronosGroup** 出品 - [多种许可证](https://github.com/KhronosGroup/glslang/blob/main/LICENSE.txt)：[github](https://github.com/KhronosGroup/glslang)

**cJSON** — **DaveGamble** 出品 - [MIT License](https://github.com/DaveGamble/cJSON/blob/master/LICENSE)：[github](https://github.com/DaveGamble/cJSON)

**OpenGL Mathematics (*GLM*)** — **G-Truc Creation** 出品 - [The Happy Bunny License](https://github.com/g-truc/glm/blob/master/copying.txt)：[github](https://github.com/g-truc/glm)

**FidelityFX-FSR** — **AMD** 出品 - [MIT License](https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/license.txt)：[github](https://github.com/GPUOpen-Effects/FidelityFX-FSR)

**xxHash** — **Yann Collet** 出品 - [BSD 2-Clause License](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE)：[github](https://github.com/Cyan4973/xxHash)

**ANGLE** — **Google** 出品 - [BSD 3-Clause License](https://chromium.googlesource.com/angle/angle/+/refs/heads/main/LICENSE)：[chromium](https://chromium.googlesource.com/angle/angle/)

---

*MobileGLES 是非官方分支，与 Mojang、Microsoft 及上游 MobileGlues 项目
均无隶属关系。Minecraft: Java Edition 是 Mojang Synergies AB 的商标。*
