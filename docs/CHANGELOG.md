# 项目变更记录

本文档用于记录每一次提交具体改动了什么功能、修复了什么问题、调整了什么架构，以及这些改动的原因和影响范围。

建议后续每次提交后按以下结构追加：

```md
## <提交短哈希> - <提交标题>

- 提交时间：
- 改动类型：功能 / 修复 / 架构 / 构建 / 文档 / 资源
- 改动摘要：
- 具体改动：
- 架构影响：
- 验证情况：
- 后续注意：
```

## bfbdc98 - 调整架构

- 提交时间：2026-07-14 17:51:52 +0800
- 完整提交：`bfbdc983f8607494633c20756b3d666e86762db5`
- 改动类型：架构调整 / 资源管理 / 渲染流程整理 / 生命周期修复

### 改动摘要

本次提交把项目从“示例程序直接拼装渲染流程”的结构，调整为更接近小型渲染引擎的分层结构：

- `Scene` 负责场景对象、模型、灯光等场景数据的生命周期。
- `Renderer` 负责每帧组织渲染流程。
- `MeshRenderer` 从“持有场景的渲染器”调整为由 `Renderer` 调用的 mesh pass。
- `AssetManager` 负责统一资源根目录、路径解析和贴图生命周期。
- `RenderGraph` 修复 pass 对象每帧泄漏的问题。
- `PostProcessRenderer` 按当前选择的效果追加 pass，避免选择一个后处理时执行全部后处理。

### 具体改动

1. 新增 `AssetManager`

新增文件：

- `src/Engine/Base/AssetManager.h`
- `src/Engine/Base/AssetManager.cpp`

职责：保存项目资源根目录，统一处理相对路径和绝对路径，并用 `std::unique_ptr<Texture2D>` 管理加载出的贴图对象。

原因：之前资源路径分散在 `main.cpp`、`MeshRenderer`、`BasePassRenderer` 中，并且存在 `D:/...`、`E:/...` 这类硬编码路径。集中到 `AssetManager` 后，资源入口更清晰，也为后续模型、材质、Shader 缓存打基础。

2. 新增 `Renderer`

新增文件：

- `src/Engine/Renderer/Renderer.h`
- `src/Engine/Renderer/Renderer.cpp`

职责：持有 `MeshRenderer` 和 `PostProcessRenderer`，每帧创建并执行 `RenderGraph`，调用 `Scene::Update()`，组织 mesh pass 和后处理 pass。

原因：之前主循环直接拼接 `Scene`、`MeshRenderer`、`PostProcessRenderer` 和 `RenderGraph`，导致 `main.cpp` 同时负责窗口、输入、UI、场景、渲染流程和后处理选择。新增 `Renderer` 后，应用层只需要调用 `renderer->render(*scene, camera, useEffect)`。

3. 调整 `MeshRenderer` 与 `Scene` 的关系

修改文件：

- `src/Engine/Renderer/MeshRenderer.h`
- `src/Engine/Renderer/MeshRenderer.cpp`

主要变化：移除 `MeshRenderer::scene_`；`render` 和 `addShadowPass` 改为显式接收 `Scene&`；新增 `setFloorTexture(Texture2D*)`；方向光改为每帧从 `Scene` 查询，并在没有主方向光时提前返回；移除构造函数中的地板贴图绝对路径加载。

原因：`MeshRenderer` 不应该拥有或长期保存 `Scene*`。更合理的方向是：`Scene` 管“世界里有什么”，`Renderer/MeshRenderer` 只在当前帧读取场景数据并提交绘制。

4. 调整 `PostProcessRenderer` 执行策略

修改文件：

- `src/Engine/Renderer/PostProcessRenderer.h`
- `src/Engine/Renderer/PostProcessRenderer.cpp`

主要变化：`render` 增加 `effectNo` 参数；Bloom 仍作为公共前置 pass；只有当前选择的效果会追加对应 pass；各 pass 在自己的 lambda 内设置对应 shader；Motion Blur 输出改为返回最近一次写入的 framebuffer；析构函数释放后处理创建的贴图资源。

原因：之前只要选择任意后处理，就会把 Bloom、Radial、Motion、Cartoon、Ripple 全部执行一遍，最终只是显示其中一个结果。这会造成 GPU 浪费，也让效果之间产生隐式依赖。

5. 修复 `RenderGraph` pass 生命周期

修改文件：

- `src/Engine/Renderer/RenderGraph/RenderGraph.h`
- `src/Engine/Renderer/RenderGraph/RenderGraph.inl`
- `src/Engine/Renderer/RenderGraph/RenderGraphPass.h`

主要变化：`passes` 从 `std::vector<RenderGraphPass*>` 改为 `std::vector<std::unique_ptr<RenderGraphPass>>`；`addPass` 创建的 pass 交给 `unique_ptr` 管理；补上 `#define RenderGraph_h`。

原因：之前每帧创建局部 `RenderGraph`，每个 pass 通过 `new RenderGraphExecutePass` 创建，但析构时没有释放。改成 `unique_ptr` 后，`RenderGraph` 生命周期结束时会自动释放所有 pass。

6. 调整 `Scene` 生命周期管理

修改文件：

- `src/Engine/Base/Scene.h`
- `src/Engine/Base/Scene.cpp`

主要变化：`Scene` 增加析构函数，释放 `lights`、`models`、`root`；`createModel` 中保存 `Model*` 到 `models`，并处理模型加载失败。

原因：之前 `Scene` 创建了 `root`，`createModel` 创建了 `Model`，但 `Scene` 析构是默认实现，导致场景对象和模型对象没有明确释放。

7. 修复部分资源释放问题

修改文件：

- `src/Engine/Base/Renderable.h`
- `src/Engine/Base/Mesh.cpp`

主要变化：`GameObject` 析构时释放组件、mesh、子对象；`Mesh` 析构时用 `deleteVertexArray` 删除 VAO；`Mesh` 释放 `material`；bitangent 顶点属性 location 从 `3` 改为 `4`，避免与 tangent 共用同一个 attribute location。

8. 调整 `Example/main.cpp`

修改文件：

- `src/Example/main.cpp`

主要变化：新增 `findProjectRoot()`；创建 `AssetManager` 和 `Renderer`；移除主流程中手动创建和连接 `MeshRenderer`、`PostProcessRenderer`；主循环改为调用 `renderer->render(*scene, camera, useEffect)`；ImGui 文件选择默认目录改为通过 `AssetManager::resolvePath` 获取；切换地板贴图时通过 `AssetManager` 加载，再传给 `MeshRenderer`。

原因：示例层仍负责窗口、输入、ImGui 和最终 blit，但内部渲染流程不应继续散落在 `main.cpp`。这次改动先把“每帧怎么画”的职责移入 `Renderer`。

9. 调整 `BasePassRenderer` 资源路径

修改文件：

- `src/Engine/Renderer/BasePassRenderer.cpp`

主要变化：将 `E:/learnRenderC++/...` 这类绝对路径改为相对资源路径。

原因：减少本机路径依赖。该类目前更像早期实验代码，后续可以考虑删除、合并或改造成标准 pass。

### 架构影响

本次提交建立了新的基础依赖方向：

```text
Example / Application
  -> Renderer
  -> Scene
  -> AssetManager

Renderer
  -> MeshRenderer
  -> PostProcessRenderer
  -> RenderGraph
  -> RenderContext

MeshRenderer
  -> 读取 Scene
  -> 通过 RenderContext 提交绘制

Scene
  -> 管理 GameObject / Model / Light

AssetManager
  -> 管理资源路径和 Texture2D 生命周期
```

比之前更清晰的点：

- `Scene` 不再被 `MeshRenderer` 长期持有。
- `main.cpp` 不再直接拼完整渲染图。
- 资源路径不再散落在多个渲染类里。
- 后处理 pass 不再全部无条件执行。
- `RenderGraph` 不再每帧泄漏 pass。

### 验证情况

已做：

- 检查提交 diff 和文件变更。
- `git diff --check` 通过。
- 清理了临时构建目录。

未完成：

- 当前环境没有可用的 Visual Studio 17 2022 实例。
- 当前环境也没有 Ninja / C++ 编译器，因此未能完成实际编译验证。

建议在安装 VS2022 的环境中执行：

```bat
build_project.bat
cmake --build build --config Debug
```

### 后续注意

- `RenderContext::getInstance()` 仍然存在，后续应逐步改成显式依赖注入。
- `Texture2D`、`Material` 等 Base 层仍直接调用 OpenGL API，RHI 抽象还没有完全隔离。
- `Scene` 现在开始承担对象生命周期，但仍使用裸指针；后续建议继续迁移到 `std::unique_ptr`。
- `main.cpp` 仍负责最终屏幕 blit，后续可以抽成 `PresentPass`。
- `BasePassRenderer` 仍保留较多实验代码，后续应决定保留、删除或重构。
## 未提交 - 模型与材质资源加载流程调整

- 改动时间：2026-07-17
- 改动类型：资源管理 / 模型导入 / 材质贴图流程 / 架构演进

### 改动摘要

本次改动把模型导入后的贴图加载流程从 `Model` 内部直接创建 `Texture2D`，调整为优先通过 `AssetManager` 统一解析、缓存和管理。这样 OBJ、FBX、GLB 等格式导入后，材质贴图能进入统一资源系统，后续也更容易扩展模型缓存、异步加载和资源热重载。

### 具体改动

1. `Texture2D` 支持从内存中的压缩图片创建纹理。

- 新增 `Texture2D(const unsigned char* encodedData, int dataSize)`。
- 使用 `stbi_load_from_memory` 解码 GLB 等格式中的 embedded image。
- 文件和内存图片统一解码为 RGBA，避免 1/2/3/4 通道分支遗漏。
- 修正 `stbi_load` 的释放方式，从 `delete[]` 改为 `stbi_image_free`。

2. `AssetManager` 从简单的贴图持有者升级为贴图缓存入口。

- `loadTexture2D` 返回 `std::shared_ptr<Texture2D>`。
- 使用 `textureCache_` 按解析后的路径缓存贴图。
- 新增 `loadEmbeddedTexture2D`，用于加载并缓存 GLB 内嵌贴图。

3. `Model` 改为通过 `AssetManager` 加载材质贴图。

- 保留旧构造 `Model(path)`，新增 `Model(path, AssetManager*)`。
- `Scene::createModel` 会把 `AssetManager` 传给 `Model`。
- 材质贴图加载支持：BaseColor/Diffuse、Normal/Height、Specular、Metallic、Roughness、AO、Emissive。
- 支持 Assimp embedded texture：例如 GLB 中常见的 `*0`、`*1` 内嵌 PNG/JPEG。
- 修复 mesh 没有 normal 时仍直接访问 `mesh->mNormals[i]` 的风险。

4. `Scene` 接入资源系统。

- 新增 `Scene::setAssetManager(AssetManager*)`。
- `createModel` 创建 `Model` 时传入资源管理器。
- `Example/main.cpp` 在创建 `Scene` 后设置 `assetManager`。

### 架构影响

新的模型资源流向变为：

```text
Scene::createModel(path)
  -> Model(path, AssetManager*)
  -> Assimp 导入 OBJ / FBX / GLB
  -> Mesh + Material
  -> Material 通过 AssetManager 获取 Texture2D shared_ptr
  -> AssetManager 负责路径解析、缓存、embedded texture 解码入口
```

这比之前更适合中小型渲染引擎继续扩展，因为资源创建入口从模型解析逻辑里抽出来了，材质不再依赖每个 `Model` 私有的弱引用缓存。

### 后续注意

- 当前仍然是“导入时同步上传 GPU 纹理”，还没有拆分 CPU asset 和 GPU resource。
- GLB 的 uncompressed embedded texture 目前只提示不支持，常见 GLB 内嵌 PNG/JPEG 已覆盖。
- PBR factor 的读取需要根据当前 Assimp 版本封装兼容宏；本次先加载 PBR 贴图通道。
- 后续建议继续新增 `ModelAsset / MeshAsset / MaterialAsset`，让 `Scene` 只引用资源句柄，而不是直接拥有导入产物。