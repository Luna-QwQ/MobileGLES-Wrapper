# GLES Wrapper

> **OpenGL → OpenGL ES 3.2 转译层**

本项目是一个运行在 OpenGL ES 3.2 之上的桌面 OpenGL 实现，主要面向 Minecraft: Java Edition 的运行场景。

## 开发进度目标

### 第一阶段：核心架构搭建 ✅
- [x] 后端基础设施（BackendObject, 类型系统）
- [x] GL 状态管理模块（GLState）
- [x] DirectGLES 后端框架

### 第二阶段：基础渲染管线 🔄
- [ ] Buffer 对象管理（VBO/IBO/UBO 同步）
- [ ] Vertex Array 对象管理（VAO 同步）
- [ ] 纹理对象管理（2D/3D/CubeMap 同步）
- [ ] Framebuffer 对象管理（FBO 同步）
- [ ] 基础 DrawCall（DrawArrays/DrawElements）

### 第三阶段：着色器系统 🔄
- [ ] SPIRV-Cross 着色器转译（GLSL → GLSL ES）
- [ ] Program 对象管理（编译/链接）
- [ ] Sampler 对象管理
- [ ] Uniform 同步

### 第四阶段：高级特性
- [ ] 实例化渲染（Instanced Rendering）
- [ ] 计算着色器（Compute Shader）
- [ ] 多重绘制（MultiDraw）
- [ ] Indirect Draw
- [ ] Image Load/Store
- [ ] Transform Feedback

### 第五阶段：格式兼容性
- [ ] 纹理格式能力探测
- [ ] 格式回退策略（SNORM/UNORM fallback）
- [ ] Renderbuffer 格式支持

### 第六阶段：平台适配
- [ ] Android EGL 窗口管理
- [ ] Linux X11 EGL 窗口管理
- [ ] 配置系统
- [ ] 日志系统

### 第七阶段：优化与稳定
- [ ] 状态缓存优化（减少冗余 GLES 调用）
- [ ] 资源 GC 管理
- [ ] 性能分析支持
- [ ] 错误处理完善

## 架构说明

```text
Application (Minecraft: Java Edition)
         |
    [GL 导出层] - 拦截桌面 GL 调用
         |
    [GL 状态管理] - 维护虚拟 GL 状态
         |
    [DirectGLES 后端] - 将 GL 状态同步到 GLES
         |
    [OpenGL ES 3.2] - 底层 GPU 驱动
```

## 许可证

本项目基于 GNU LGPL-3.0 License 许可。

## 第三方组件

- **SPIRV-Cross** by KhronosGroup - Apache License 2.0
- **glslang** by KhronosGroup - Various Licenses
- **cJSON** by DaveGamble - MIT License