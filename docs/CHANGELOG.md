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
## e6cec59 - 完善模型与材质资源加载流程

- 提交时间：2026-07-17 17:15:14 +0800
- 完整提交：`e6cec596bd425fd90b75d2459e492015edcb4265`
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

## 待提交 - 重构模型资产与材质 Shader 绑定架构

- 改动时间：2026-07-17
- 建议提交标题：`重构模型资产与材质 Shader 绑定架构`
- 改动类型：架构调整 / 资源缓存 / 材质系统 / 渲染流程整理

### 改动摘要

本次改动继续把项目往中小型渲染引擎方向推进：`Material` 不再直接持有 `Shader` 或执行 GL 绑定，模型导入结果拆成 `ModelAsset / MeshAsset / MaterialAsset`，`Scene` 只保存模型资产引用并实例化运行时 `GameObject`，同一路径模型通过 `AssetManager` 缓存，避免重复导入。

### 具体改动

1. `Material` 改为纯材质数据。

- 移除 `Material` 内部的 `Shader` 成员、`generateShader()` 和 `setUniform()`。
- 保留贴图、颜色、金属度、粗糙度、高光、透明度等材质属性。
- 新增 `MaterialAsset`，用于承载可被 mesh 共享的材质资源数据。

2. 新增模型资产拆分结构。

- 新增 `AssetTypes.h`。
- `MeshAsset` 保存 CPU 侧顶点、索引、顶点属性标记和材质引用。
- `ModelNodeAsset` 保存模型节点层级和 mesh 索引。
- `ModelAsset` 保存源路径、mesh 资产、material 资产和根节点。

3. `Model` 改为资产导入器和实例化器。

- `Model::loadAsset()` 负责用 Assimp 导入模型并生成 `ModelAsset`。
- `Model::instantiate()` 负责从 `ModelAsset` 创建运行时 `GameObject / Mesh` 树。
- 运行时 `Mesh` 复制 `MeshAsset` 的几何数据并引用 `MaterialAsset`。
- 模型材质贴图继续通过 `AssetManager` 加载，支持外部贴图和 GLB embedded texture。

4. `AssetManager` 增加模型缓存。

- 新增 `loadModelAsset()`。
- 新增 `modelCache_`，按解析后的路径缓存 `ModelAsset`。
- 同一个 OBJ / FBX / GLB 路径重复加载时不会重复 Assimp 导入。

5. `Scene` 不再直接拥有 `Model*` 导入对象。

- `Scene::createModel()` 改为通过 `AssetManager::loadModelAsset()` 获取模型资产。
- `Scene` 保存 `std::shared_ptr<ModelAsset>`，用于保持资产生命周期。
- 运行时对象仍挂到 `root` 下，`renderableObjects` 仍作为当前阶段的渲染列表。

6. 新增 `MaterialSystem`。

- 新增 `Renderer/MaterialSystem.h/.cpp`。
- `MaterialSystem` 负责在渲染时把 `Material / MaterialAsset` 绑定到指定 `Shader`。
- 支持从指定 texture slot 开始绑定，避免覆盖 shadow map 等 pass 级纹理。
- 为当前旧模型 shader 保留 `baseTexture` 兼容绑定，同时设置新材质 shader 使用的 `diffuseMap / normalMap / metallicMap / roughnessMap` 等 uniform。

7. `MeshRenderer` 改为显式控制材质绑定时机。

- shadow pass 调用 `Scene::DrawObjects(depthShader)`，只设置 model matrix，不绑定材质。
- scene pass 由 `MeshRenderer` 遍历可渲染对象，根据 `MaterialAsset::shader` 选择 shader，再由 `MaterialSystem` 绑定材质，并从 slot 2 开始避开 `baseTexture`/`shadowMap` 的历史占用。

### 架构影响

新的模型渲染链路变为：

```text
Scene::createModel(path)
  -> AssetManager::loadModelAsset(path)
  -> Model::loadAsset(path)
  -> ModelAsset / MeshAsset / MaterialAsset
  -> Scene 保存 ModelAsset shared_ptr
  -> Model::instantiate(ModelAsset)
  -> GameObject / Mesh 引用 MaterialAsset
  -> MeshRenderer 选择 pass shader
  -> MeshRenderer 按 MaterialAsset::shader 选择 shader
  -> MaterialSystem 绑定材质数据
  -> Mesh::draw()
```

职责边界更清晰：

- `Material`：只描述材质数据，不知道 Shader，不直接调 GL。
- `MaterialSystem`：负责“材质数据如何绑定到当前 shader”。
- `ModelAsset / MeshAsset / MaterialAsset`：保存导入后的资源数据。
- `Model`：负责导入和从资产实例化运行时对象。
- `AssetManager`：负责资源路径解析、贴图缓存、模型缓存。
- `Scene`：负责场景对象树和资产引用，不再直接拥有导入器对象，也不参与材质绑定。
- `MeshRenderer`：负责 pass 级渲染决策，决定阴影 pass 是否跳过材质绑定。

### 验证情况

已做：

- 扫描旧 `mesh->material`、`Material::setUniform()`、`generateShader()` 调用，已清理核心旧材质绑定路径。
- `git diff --check` 通过，仅有当前仓库行尾转换警告。
- 检查 CMake：`Renderer/*.cpp` 和 `Base/*.cpp` 使用 glob，新文件会被纳入 engine target。

未完成：

- 当前环境暂未完成真实编译和运行验证。

### 后续注意

- 当前主渲染仍使用旧 `Fragmodel_lighting`，它主要识别 `baseTexture`/`shadowMap`；`MaterialSystem` 已做兼容，但完整 PBR/多贴图效果需要下一步切到统一材质 shader 或引入 shader variant。
- `Scene` 仍使用裸指针管理 `GameObject / Mesh / Light`，后续应继续迁移到 `std::unique_ptr` 或 ECS/Handle。
- `MeshAsset` 当前保存 CPU 几何数据，运行时 `Mesh` 每次实例化会重新创建 GPU buffer；后续可继续拆 `MeshAsset` 和 GPU `MeshResource`。
- 模型缓存目前按解析后路径缓存，不包含导入参数差异；以后如果导入 flags 可配置，需要把 flags 纳入 cache key。
### 追加改动：Shader 资源引用与渲染层按材质选 shader

本轮继续清理材质和 shader 的职责边界，目标是接近 Unity/中小型渲染引擎常见模型：`MaterialAsset` 可以引用 shader 资源，但不拥有具体 `Shader` 对象，也不直接执行渲染。

具体调整：

1. 新增 `ShaderHandle / ShaderLibrary`。

- 新增 `ShaderLibrary.h/.cpp`。
- `ShaderHandle` 是轻量资源引用，只保存 shader 名称。
- `ShaderLibrary` 统一创建和缓存内置 shader：`engine/default-lit`、`engine/depth-only`、`engine/light-debug`。
- `AssetManager` 持有 `ShaderLibrary`，shader 生命周期进入资源系统。

2. `MaterialAsset` 增加 shader 引用。

- `MaterialAsset` 新增 `ShaderHandle shader`。
- 默认材质使用 `engine/default-lit`。
- 这使材质可以描述“使用哪个 shader”，但不会退回到 `Material` 直接持有 `Shader` 并调用 GL 的旧结构。

3. `MeshRenderer` 改为按材质解析 shader。

- `MeshRenderer` 构造函数改为接收 `AssetManager&`。
- 默认 lit、depth、light debug shader 从 `AssetManager::getShaderLibrary()` 获取。
- 主渲染 pass 遍历 `Scene::GetRenderableObjects()`，对每个 mesh 通过 `mesh->materialAsset->shader` 找到 shader。
- 找到 shader 后由 `MeshRenderer` 设置 pass 级 uniform，由 `MaterialSystem` 绑定材质参数，最后提交 draw。

4. `Scene` 不再参与材质绑定。

- `Scene::RenderObject(...)` 改为 `Scene::DrawObjects(Shader&)`。
- `Scene` 只负责对象列表和 transform，不再 include `MaterialSystem`。
- 阴影 pass 使用 `DrawObjects(depthShader)`，主渲染 pass 由 `MeshRenderer` 自己按材质处理。

5. `Shader` 补充 GL program 生命周期释放。

- `Shader::ID` 默认初始化为 0。
- 新增 `Shader::~Shader()`，析构时调用 `glDeleteProgram`。
- 这是 shader 进入资源库托管后的必要清理。

新的主渲染路径：

```text
MeshRenderer::render(scene)
  -> scene.GetRenderableObjects()
  -> mesh->materialAsset
  -> materialAsset->shader
  -> AssetManager::ShaderLibrary::get(shaderHandle)
  -> MeshRenderer 设置相机/灯光/阴影等 pass uniform
  -> MaterialSystem 绑定材质贴图和参数
  -> Mesh::draw()
```

这次调整后的职责边界：

- `MaterialAsset`：保存 shader 引用和材质参数。
- `ShaderLibrary`：创建、缓存、管理 Shader 对象。
- `AssetManager`：统一资源入口，持有 shader library / texture cache / model cache。
- `Scene`：只关心场景对象和对象列表。
- `MeshRenderer`：负责根据 pass 和材质选择 shader，并提交 draw。
- `MaterialSystem`：负责把材质数据绑定到当前 shader。

后续建议：

- 把 `engine/default-lit` 替换成真正统一的材质 shader，消除当前 `baseTexture` 兼容逻辑。
- 引入 `ShaderPass` 或 `Technique`，让一个 shader 资产能描述 Forward / Shadow / DepthOnly / GBuffer 等多个 pass。
- 把 `GraphicsPipeline` 从临时结构升级成可缓存的 `PipelineState`，避免每个 mesh 反复拼 pipeline。
### 追加改动：删除旧 BasePassRenderer 实验 pass

本轮还清理了旧的 `BasePassRenderer`：

- 从 `Example/main.cpp` 删除旧备用入口函数和 `BasePassRenderer` 注释残留。
- 删除 `Renderer/BasePassRenderer.h/.cpp`。
- 原因：该类已经不在新 `Renderer` 主流程中使用，并且保留旧式 `TRefCountPtr<Shader>`、直接资源路径、独立 pass 组织方式，继续保留会干扰当前架构方向。
- 后续如需要 base pass，应基于 `Renderer + RenderGraph + AssetManager + MaterialSystem` 重新实现，而不是恢复旧类。
### 追加改动：运行时可见日志与关键错误输出

本轮增加运行时日志系统，解决 WIN32 子系统启动后没有控制台、shader 编译失败和资源加载失败不容易发现的问题。

具体调整：

1. 新增 `Logger`。

- 新增 `Base/Logger.h/.cpp`。
- Windows 下启动时主动 `AllocConsole()`，显示 `RenderEngine Log Console`。
- 同时写入 `logs/engine.log`。
- 提供 `Logger::Info / Warn / Error`，后续模块统一通过 Logger 输出运行时信息。

2. 应用启动阶段初始化日志。

- `WinMain` 中在 `glfwInit()` 前初始化 Logger。
- GLFW 初始化失败、窗口创建失败、GLAD 初始化失败都会输出明确错误。
- 关闭程序时调用 `Logger::Shutdown()`。

3. Shader 编译/链接错误增强。

- `Shader` 构造函数增加 debug name 参数。
- shader 编译失败时输出 shader 名称、阶段、OpenGL info log。
- 同时输出 shader 源码前 80 行预览并带行号，方便直接定位语法错误。
- `ShaderLibrary` 注册 shader 时输出 shader 名称；未知 shader 会输出 fallback warning。

4. 资源加载日志。

- `Texture2D` 贴图文件解码失败时输出路径和 stb failure reason。
- embedded texture 解码失败时输出数据大小和原因。
- `AssetManager` 输出 texture/model cache hit、首次加载、模型加载失败等信息。
- `Model` 输出 Assimp 导入失败路径和错误信息，导入成功时输出 mesh/material 数量。
- `Scene::createModel` 输出模型资产为空、实例化失败和创建成功信息。

5. 渲染关键错误日志。

- `OpenGLRenderContext::bindPipeline` 遇到 null shader 会输出错误。
- FBO 创建后检查 `glCheckFramebufferStatus`，不完整时输出 framebuffer id 和状态码。
- 启动后尽量启用 OpenGL debug callback；如果当前驱动/context 不支持，会输出 warning。
- 后处理关键 pass 中的 `glGetError()` 检查会通过 Logger 输出上下文信息。

运行后排查方式：

```text
1. 启动程序后看 RenderEngine Log Console。
2. 如果窗口闪退或控制台没保留，打开 <项目根目录>/logs/engine.log。
3. shader 编译错误优先看 "Shader compile failed"，里面会有 shader name、stage、info log、源码预览。
4. 黑屏优先看 "Framebuffer incomplete"、"bindPipeline called with null shader"、"OpenGL debug message"。
```
