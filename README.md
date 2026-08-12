# Render Engine

一个基于 C++11、OpenGL、GLFW、Assimp 和 ImGui 实现的学习型中小型实时渲染引擎。项目目前具备多格式模型导入、资源缓存、metallic-roughness PBR、IBL、方向光阴影、有限范围点光源、HDR/Bloom 和多种屏幕后处理，并正在持续将早期实验代码重构为职责清晰的引擎模块。

## 构建

双击 `build_project.bat` 生成 Visual Studio 工程，或执行：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

运行程序：

```powershell
.\build\src\Example\Release\example.exe
```

运行日志位于 `logs/engine.log`。Shader 编译、模型/贴图加载、Framebuffer 完整性和 OpenGL debug callback 信息会同时输出到日志控制台。

## 当前能力

- OBJ、FBX、GLB、GLTF、DAE 模型导入。
- 外部贴图与 GLB embedded texture 加载、纹理缓存、模型缓存。
- `ModelAsset / MeshAsset / MaterialAsset` 资源拆分及节点层级变换保留。
- metallic-roughness PBR、旧式 specular-gloss 材质近似转换。
- HDR 环境贴图、irradiance map、prefiltered environment map、BRDF LUT。
- 方向光阴影、最多 4 个有限范围点光源。
- HDR、Bloom、Radial Blur、Motion Blur、Cartoon、Ripple。
- 动态窗口 resize、实时投影宽高比和 RenderTarget 统一资源管理。
- ImGui 模型/贴图选择、光源参数和后处理选择。

详细架构、渲染流程和后续路线见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。
