# Render Engine 架构与实现说明

## 1. 项目定位

本项目定位为学习型中小型实时渲染引擎，而不是只展示单个 OpenGL 效果的 Demo。演进目标是让场景、资源、材质、渲染流程和底层图形接口具有稳定边界，使新模型格式、新 pass 和后处理能够通过扩展模块加入。

技术栈：C++11、OpenGL、GLFW/GLAD、Assimp、GLM、ImGui、CMake。

## 2. 当前分层

```text
Example / Application
  ├─ GLFW 窗口与输入
  ├─ ImGui 调试界面
  └─ 调用 Renderer::render()

Renderer
  ├─ MeshRenderer：阴影、PBR 场景、灯光调试
  ├─ PostProcessRenderer：Bloom 与屏幕后处理
  ├─ RenderGraph：记录并顺序执行当前帧 pass
  ├─ RenderQueueBuilder：把场景对象转换成轻量 RenderItem，执行视锥裁剪与分类
  ├─ RenderQueue：保存 shadow / opaque / transparent 队列并完成排序
  ├─ RenderTarget：拥有 framebuffer attachment 和尺寸
  └─ MaterialSystem：把材质数据绑定到 Shader

Scene
  ├─ GameObject 层级树与 Transform
  ├─ 可渲染对象列表与 Light
  └─ ModelAsset 生命周期引用

AssetManager
  ├─ 路径解析、Texture2D 缓存、ModelAsset 缓存
  └─ ShaderLibrary

RHI / OpenGL
  ├─ RenderContext 抽象资源与绘制接口
  └─ OpenGLRenderContext 实现 OpenGL 命令
```

职责原则：

- `Scene` 回答“世界里有什么”，不决定用哪个 pass 渲染。
- `Renderer` 回答“这一帧如何渲染”，不拥有场景业务对象。
- `AssetManager` 负责资源定位、加载与复用。
- `MaterialAsset` 描述 Shader 引用和参数，不直接调用 OpenGL。
- `MaterialSystem` 把材质映射为 uniform 与 texture slot。
- `RenderTarget` 负责 attachment 所有权、Framebuffer 描述和 resize。
- `RenderContext` 隔离上层流程和具体 OpenGL API。

## 3. 每帧渲染流程

```text
Application Loop
  -> 输入与 ImGui
  -> Renderer::render(scene, camera, effect)
      -> Scene::Update()：递归计算 world matrix
      -> Shadow Pass：方向光 depth-only shadow map
      -> Scene Pass：HDR RenderTarget + PBR + 灯光
      -> Post Process [可选]
          -> highlight extraction
          -> bloom downsample mip chain
          -> bloom upsample chain
          -> bloom composite
          -> radial / motion / cartoon / ripple
      -> RenderGraph::execute()
  -> screen quad tone mapping + gamma
  -> ImGui -> swap buffers
```

当前 `RenderGraph` 是轻量顺序 pass 列表，能够把 pass 组织从应用层移走，但尚未分析资源依赖、自动排序、做 transient aliasing 或 barrier 管理。每帧执行前，`RenderQueueBuilder` 从 Scene 收集渲染数据，生成不依赖场景树结构的 `RenderItem` 快照；各 pass 只消费对应队列。

```text
Scene / GameObject
  -> RenderQueueBuilder
      -> local Bounds × world matrix
      -> camera frustum culling
      -> shadowCasters / opaqueItems / transparentItems
      -> pipeline / material / mesh 排序
  -> Shadow Pass / Forward PBR Pass
```

不透明队列按 Shader、Material、MeshResource 和距离排序，减少状态切换；透明队列按相机距离从远到近绘制，并在透明阶段关闭深度写入。阴影 caster 暂时不按相机视锥裁剪，因为相机视野外物体仍可能向画面内投影，后续应改成按方向光阴影视锥裁剪。

## 4. 模型、场景与资源流程

```text
Scene::createModel(path)
  -> AssetManager::loadModelAsset(path)
      -> 路径规范化与 model cache
      -> Model::loadAsset()
          -> Assimp importer
          -> ModelNodeAsset：节点矩阵与层级
          -> MeshAsset：CPU 顶点/索引与材质引用
          -> MaterialAsset：ShaderHandle、参数与共享贴图
          -> Texture cache / GLB embedded image
  -> Model::instantiate(asset)
      -> GameObject / Mesh 运行时树
      -> AssetManager 创建或复用 MeshResource
      -> Mesh 实例引用共享 VAO/VBO/IBO
      -> Transform 保存导入节点矩阵
  -> Scene 保存 ModelAsset shared_ptr 并登记可渲染对象
```

同一路径模型不会重复 Assimp 导入，同一路径贴图不会重复创建 `Texture2D`。模型首次加载时，`AssetManager` 为每个 `MeshAsset` 创建并缓存 `MeshResource`；后续实例化只共享 VAO/VBO/IBO，Transform 和 Material 仍属于实例或资产数据。CPU 顶点/索引只保存在共享的 `MeshAsset` 中，场景 `Mesh` 实例不再复制整份几何，只保存局部 `Bounds`、材质引用和 GPU 资源引用。

模型导入阶段同时为每个 `MeshAsset` 计算局部轴对齐包围盒。运行时可通过 `localBounds.transformed(worldMatrix)` 得到世界空间 AABB，为 RenderItem、视锥裁剪、透明排序和包围盒调试提供统一数据来源。

## 5. Transform 原理

模型文件节点矩阵与运行时 TRS 是两类数据：

- `m_SourceLocalMatrix`：DCC/FBX/glTF 层级保存的局部矩阵。
- `localPosition/localRotation/localScale`：运行时对实例的附加编辑。

```text
LocalMatrix = RuntimeTRS × SourceLocalMatrix
WorldMatrix = ParentWorldMatrix × LocalMatrix
```

这样不会丢失 FBX/glTF 节点预旋转、缩放或坐标变换，同时仍能移动和缩放场景实例。

## 6. 材质与 PBR

`Material` 是纯数据对象，支持 base color、normal、metallic、roughness、combined metallic-roughness、AO、emissive、specular、reflection、height，以及颜色、透明度和折射率等因子。

`MaterialAsset` 通过 `ShaderHandle` 指向 Shader 资源，但不持有 OpenGL program。渲染链路为：

```text
MeshRenderer
  -> MaterialAsset::shader
  -> ShaderLibrary::get(handle)
  -> 设置相机、灯光、阴影、IBL 等 pass 参数
  -> MaterialSystem::bindMaterialAsset()
  -> Mesh::draw()
```

### Shader Technique、Pass 与 Variant

运行时 Shader 源码已经从 `ShaderCode.cpp` 迁移到 `resources/shaders/`。`ShaderLibrary` 使用三层资源模型：

```text
ShaderHandle：Technique 名称 + Variant Defines
    -> ShaderTechniqueDesc
        -> Forward / Shadow / Debug ShaderPassDesc
            -> vertex / fragment / geometry 外置文件
                -> Program Cache
```

- Technique 表示一类渲染能力，例如 `engine/default-lit`、`engine/depth-only`。
- Pass 表示同一 Technique 在不同渲染阶段使用的 program，例如 Forward、Shadow、Debug。
- Variant 使用排序后的宏集合参与 program key；当前 `MATERIAL_NORMAL_MAP` 已作为真实材质变体接入。
- 相同 Technique、Pass 和宏集合只编译一次，MaterialAsset 继续只保存轻量 ShaderHandle。

Renderer 每 500ms 检查 Shader 文件时间戳。热重载会先编译候选 program：成功后保持 Shader 对象地址不变并替换内部 OpenGL ID；失败时保留上一版 program，日志输出源码行号和错误信息。同一个失败文件版本只尝试一次，避免持续刷日志；再次保存文件或点击 ImGui 的 `Reload Shaders` 后重试。

PBR 使用 metallic-roughness 工作流：GGX NDF、Smith/Schlick-GGX Geometry、Schlick Fresnel。直接光来自方向光和最多 4 个有限范围点光源；间接光来自 irradiance diffuse、prefiltered specular 和 BRDF LUT。glTF combined texture 按 G=roughness、B=metallic 读取；旧式 shininess 会近似转换为 roughness。

## 7. IBL 原理

`IBLSystem` 从 equirectangular HDR 生成：

1. Environment cubemap。
2. Irradiance map：环境漫反射低频卷积。
3. Prefiltered environment map：按 roughness 生成镜面反射 mip。
4. BRDF LUT：预积分视角、粗糙度与微表面 BRDF。

运行时 Shader 通过预计算纹理近似环境积分，避免逐像素执行高成本采样。

## 8. RenderTarget 升级

### 原问题

升级前，`MeshRenderer` 和 `PostProcessRenderer` 分别维护裸 `Texture2D*`、`FrameBufferInfo`、attachment 构造、resize 列表和 destructor delete 列表。增加一个 pass 必须在多处同步修改；Bloom viewport 又从全局窗口尺寸推导，可能与实际 mip 纹理不一致。

### 新结构

`RenderTargetDesc` 描述 debug name、宽高、颜色 attachment 和可选 depth/stencil attachment。每个 attachment 可以指定格式、Sampler 和 clear 参数。

`RenderTarget` 负责：

- 通过 `RenderContext` 创建 attachment。
- 用 `std::unique_ptr<Texture2D>` 表达独占所有权。
- 构建供 RHI 使用的 `FrameBufferInfo` 非拥有视图。
- 统一 resize 全部 attachment。
- 提供实际 width/height、color/depth texture 查询。
- 禁止复制，避免 GPU 资源所有权被隐式复制。

### 改造结果

- 主 HDR color + depth/stencil 合并为 `sceneTarget_`。
- Bloom highlight、5 级 downsample、5 级 upsample、composite 统一管理。
- Radial、Motion ping-pong、Cartoon、Ripple 统一管理。
- 删除十余个裸纹理字段和手写 delete。
- Bloom viewport 与 texel size 直接读取对应 RenderTarget 实际尺寸。
- `Renderer` 显式注入 `RenderContext&` 给子 Renderer，减少新代码对 singleton 的依赖。

## 9. C++ 知识点

### RAII 与智能指针

- RenderTarget 用 `unique_ptr` 管理 attachment 独占生命周期。
- 模型、材质、贴图缓存用 `shared_ptr` 表达跨系统共享。
- 裸指针只作为非拥有观察指针，例如 attachment 中的 texture view。

### 依赖注入

`Renderer(RenderContext&, AssetManager&)` 等构造函数使用引用表达“依赖必须存在且不由本对象拥有”，比到处访问 singleton 更易测试，也为多图形后端保留入口。

### Asset 与 Instance 分离

Asset 保存可缓存、可共享的导入数据；GameObject/Mesh/Transform 保存场景实例状态，为异步加载、序列化和多实例复用打基础。

### 模板与多态

`RenderGraph::addPass` 使用模板保存不同参数类型和 lambda，再通过 `RenderGraphPass` 基类统一执行，是异构可调用对象封装的简单实现。

## 10. 当前仍存在的问题

1. Scene/GameObject 仍大量使用裸指针，缺少 Entity Handle/Generation 校验。
2. `RenderContext` singleton 和直接 OpenGL 调用仍存在，RHI 隔离不完整。
3. Technique 当前由 C++ 注册文件路径，尚缺少 JSON/YAML 资产描述、include 预处理和依赖图。
4. RenderGraph 没有资源句柄、读写依赖、自动排序与瞬时资源复用。
5. 方向光阴影没有相机视锥拟合、CSM 和完善调试工具。
6. glTF alpha mode、double-sided、transmission、skin animation 尚未完整实现。
7. 后处理参数多为硬编码，缺少统一设置对象与可组合效果栈。
8. 尚无 draw call、显存、GPU timestamp 等完整渲染统计。
9. CMake 仍使用全局 include 和 GLOB，自动测试/CI 尚未建立。

## 11. 推荐后续路线

- P0（已完成）：`MeshResource` 和 GPU 资源缓存，多实例共享 VAO/VBO/IBO。
- P1（已完成）：引入 Bounds，移除实例 CPU 几何副本，并构建 `RenderItem`、视锥裁剪和按 pipeline/material 排序的渲染队列。
- P2（已完成）：Shader 外置，增加 Technique/ShaderPass、宏变体与安全热重载。
- P3：让 RenderGraph 声明 texture/buffer 读写依赖，并接入 RenderTargetPool。
- P4：CSM、glTF skin/alpha/transmission、SSAO、TAA、自动曝光和 GPU profiler。

## 12. 本轮验证

- CMake 重新配置成功，新 RenderTarget 源文件进入 engine target。
- Debug 与 Release 构建通过。
- 默认场景持续运行 8 秒，无 Shader、Framebuffer 或高优先级 OpenGL 错误。
- Bloom 完整链路持续运行 8 秒，14 个运行时 framebuffer 正常创建。
- 非标准尺寸 `1919 × 1009` 和 `800 × 600` 连续 resize 通过。
- 临时测试代码已恢复，Example 默认仍使用 Origin 效果。

## 13. MeshResource 验收补充

- 同一 `cat_mask.fbx` 创建两个场景实例：首次实例化约 `16.6 ms`，缓存实例化约 `0.35 ms`。
- 日志确认：`Model cache hit`、`resources=1`、`uploads=1`。
- GPU buffer 统计：`vertices=5469`、`indices=30864`、`gpuBufferBytes=429720`。
- Example 退出顺序已调整为先释放 Scene/Renderer/AssetManager 与所有 OpenGL 资源，再销毁 GLFW Window/Context。
- 优雅关闭窗口测试返回码为 `0`，日志输出 `RenderEngine shutdown.`。

## 14. Bounds 与 RenderQueue 阶段验收

- `MeshAsset` 在 Assimp 导入阶段生成局部 AABB；正常场景 Mesh 实例不再复制 CPU 顶点和索引。
- `RenderQueueBuilder` 使用 `localBounds.transformed(worldMatrix)` 生成世界 AABB，并通过相机 ViewProjection 六平面执行裁剪。
- 两个 `cat_mask.fbx` 实例中一个位于视锥外时，统计为 `submitted=2`、`visible=1`、`culled=1`。
- 可见实例统计为 10288 个三角形；两个实例仍只上传一份 MeshResource，第二次加载命中 Model cache。
- 阴影队列保留两个 caster，避免相机视锥外对象向画面内投影时错误丢失阴影。
- 不透明队列按 Shader、Material 和 MeshResource 排序；透明队列按相机距离从远到近绘制并关闭深度写入。
- 没有主方向光时，场景 pass 仍继续执行 IBL、点光源和 Emissive 渲染。
- Debug/Release 构建及默认场景 8 秒运行通过，未出现 Shader、Framebuffer 或高优先级 OpenGL 错误。

## 15. Shader System 阶段验收

- PBR、DepthOnly、LightDebug、Screen、IBL 与全部后处理 Shader 已统一迁移到 `resources/shaders/`。
- 启动日志确认所有 Technique/Pass 从外置文件成功创建 program，默认场景运行 8 秒无 Shader/FBO/高优先级 OpenGL 错误。
- 修改 `light_debug.frag` 后自动热重载成功；写入非法 GLSL 时编译错误直接输出，进程继续运行并保持上一版 program。
- 修复文件后再次自动热重载成功，验证失败回退与恢复链路完整。
- 同一份非法 GLSL 等待 3 秒只记录 1 次失败，不会每 500ms 重复编译刷日志。
- `ShaderHandle::defines` 参与 Variant 缓存；自动加载 `backpack.obj` 时成功创建 `engine/default-lit#forward+MATERIAL_NORMAL_MAP` 独立 program。
- 项目介绍 PPT 已覆盖更新为 17 页，Shader System、诊断、路线图和结论页通过中文编码与布局检查。

## 16. Editor/UI 与第三方依赖阶段

当前 Example 只负责创建窗口、初始化 Engine 和驱动主循环；ImGui 上下文、GLFW/OpenGL3 backend、文件选择器和编辑器面板由独立的 `editor_ui` target 管理。

```text
example
 ├── engine
 └── editor_ui
      ├── imgui
      ├── ImGuiFileDialog
      ├── GLFW
      └── GLAD
```

- `src/Example` 不再使用 GLOB 编译所有文件，只显式编译 `main.cpp`。
- 删除 Example 中重复的 ImGui backend 源码，统一使用 `src/3rdparty/imgui/backends` 官方实现。
- 增加 `src/Editor/EditorUI.*`，集中管理编辑器初始化、Frame 生命周期、模型/贴图选择、渲染统计、Shader 热重载和后处理控制。
- `Engine` 通过 `target_link_libraries(... PUBLIC ...)` 声明 Assimp、stb_image、GLAD 等静态库依赖，最终应用不再重复手工列出底层库。
- 为当前平铺式 GLFW 头文件增加 `<GLFW/glfw3.h>` 兼容转发头，以匹配官方 backend 的标准 include 方式。

这样调整的原因是隔离运行时引擎和开发期工具：未来可以构建不带 ImGui 的纯 Runtime，同时编辑器面板不会继续侵入 `Example/main.cpp`。
