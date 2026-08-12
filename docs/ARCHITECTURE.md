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

当前 `RenderGraph` 是轻量顺序 pass 列表，能够把 pass 组织从应用层移走，但尚未分析资源依赖、自动排序、做 transient aliasing 或 barrier 管理。

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
      -> 上传 GPU buffer
      -> Transform 保存导入节点矩阵
  -> Scene 保存 ModelAsset shared_ptr 并登记可渲染对象
```

同一路径模型不会重复 Assimp 导入，同一路径贴图不会重复创建 `Texture2D`。当前 `MeshAsset` 保存 CPU 几何，但每次实例化仍会重新创建 GPU buffer；后续应增加共享 `MeshResource`。

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
3. GPU Mesh buffer 不能在同一模型的多个实例间共享。
4. Shader 源码硬编码在 `ShaderCode.cpp`，缺少外置文件、variant 和热重载。
5. RenderGraph 没有资源句柄、读写依赖、自动排序与瞬时资源复用。
6. 方向光阴影没有相机视锥拟合、CSM 和完善调试工具。
7. glTF alpha mode、double-sided、transmission、skin animation 尚未完整实现。
8. 后处理参数多为硬编码，缺少统一设置对象与可组合效果栈。
9. 尚无 draw call、triangle、显存、GPU timestamp 等渲染统计。
10. CMake 仍使用全局 include 和 GLOB，自动测试/CI 尚未建立。

## 11. 推荐后续路线

- P0：增加 `MeshResource` 和 GPU 资源缓存，多实例共享 VAO/VBO/IBO。
- P1：构建 `RenderItem`、视锥裁剪和按 pipeline/material 排序的渲染队列。
- P2：Shader 外置，增加 Technique/ShaderPass、宏变体与热重载。
- P3：让 RenderGraph 声明 texture/buffer 读写依赖，并接入 RenderTargetPool。
- P4：CSM、glTF skin/alpha/transmission、SSAO、TAA、自动曝光和 GPU profiler。

## 12. 本轮验证

- CMake 重新配置成功，新 RenderTarget 源文件进入 engine target。
- Debug 与 Release 构建通过。
- 默认场景持续运行 8 秒，无 Shader、Framebuffer 或高优先级 OpenGL 错误。
- Bloom 完整链路持续运行 8 秒，14 个运行时 framebuffer 正常创建。
- 非标准尺寸 `1919 × 1009` 和 `800 × 600` 连续 resize 通过。
- 临时测试代码已恢复，Example 默认仍使用 Origin 效果。
