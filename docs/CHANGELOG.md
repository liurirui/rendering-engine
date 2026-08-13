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

## 未提交 - 构建基于 Bounds 的 RenderQueue 与视锥裁剪流程

- 改动类型：渲染架构 / 内存优化 / 性能优化 / 渲染流程
- 建议提交标题：`feat: 构建基于 Bounds 的 RenderQueue 与视锥裁剪流程`

### 改动摘要

本提交完成渲染队列 P1 阶段的完整基础链路。首先引入统一 `Bounds`，在模型导入阶段计算共享局部包围盒，并移除正常模型实例重复保存的 CPU 顶点/索引；随后在 Scene 与 MeshRenderer 之间新增 `RenderItem / RenderQueue / RenderQueueBuilder` 提交层，根据世界包围盒执行视锥裁剪、透明分类和渲染排序。MeshRenderer 不再直接遍历 Scene 的 GameObject/Mesh 容器。

### 具体改动

- 新增 `Base/Bounds.h`，提供 AABB 有效性检查、点扩展、中心、半尺寸以及局部 AABB 到世界 AABB 的矩阵变换。
- `MeshAsset` 在 Assimp 顶点导入阶段计算 `localBounds`，避免实例化后重复遍历顶点。
- 正常共享资源路径下，`Mesh` 实例不再复制 `MeshAsset::vertices/indices`，只保存 Bounds、材质和 MeshResource 引用。
- 没有 `AssetManager/MeshResource` 的旧式 fallback 仍复制 CPU 几何并上传，保持兼容。
- 旧 `Renderable::calculateCenter()` 改为读取 Bounds，不再依赖实例顶点副本。
- 新增 `RenderItem`，保存 Mesh/MeshResource、MaterialAsset、世界矩阵、世界 Bounds、距离和阴影标记。
- 新增 `Frustum`，从 ViewProjection 提取六个平面并执行 AABB 裁剪。
- 新增 `RenderQueue` 与 `RenderStats`，记录 submitted、visible、culled、shadow、opaque、transparent 和可见三角形数量。
- 新增 `RenderQueueBuilder`，负责场景收集、世界 Bounds 计算、裁剪、材质透明分类和排序。
- 不透明队列按 Shader、Material、MeshResource、距离排序；透明队列按距离从远到近排序。
- Shadow pass 改为消费 `shadowCasters`；Forward PBR pass 分别消费 opaque 和 transparent 队列。
- 透明阶段保持深度测试并关闭深度写入，绘制完成后恢复。
- `Mesh` 增加 `castShadows / receiveShadows` 实例标记。
- RenderGraph pass 通过 `shared_ptr<RenderQueue>` 持有当前帧提交数据，避免延迟执行 lambda 引用局部变量。
- 没有主方向光时不再跳过整个场景 pass，仍可使用 IBL、点光源和 Emissive。
- Example 的 ImGui 增加 Render Statistics 面板。
- 更新项目介绍 PPT 及生成脚本：总体架构加入 RenderQueue，帧流程加入 Build Queue，模型资源页改为当前 MeshResource/Bounds 状态，并新增 Bounds 与 RenderQueue 专题页。
- 路线图将 P0 MeshResource、P1 RenderQueue 标记为已完成，下一阶段调整为 Shader 外置、Technique/ShaderPass、Variant 与热重载。

### 架构影响

模型几何现在形成清晰的三层关系：

```text
MeshAsset：共享 CPU 几何 + local Bounds
    -> MeshResource：共享 GPU VAO/VBO/IBO
    -> Mesh 实例：Bounds + MaterialAsset + MeshResource 引用
        -> RenderItem：当前帧世界矩阵、world Bounds 与渲染标记快照
            -> RenderQueue：Shadow / Opaque / Transparent
```

Scene 继续管理“世界中有哪些对象”，RenderQueueBuilder 负责把场景实例转换为当前帧渲染数据，MeshRenderer 只消费已经分类和排序的队列。这为后续 Shader Technique、LOD、实例化渲染、方向光阴影视锥裁剪和稳定 Entity Handle 提供了明确接入点。

### 验证情况

- `git diff --check`：通过。
- Debug 与 Release 构建：通过。
- 默认场景运行 8 秒：通过，没有 Shader 编译/链接失败、Framebuffer incomplete、HIGH severity 或 `[ERROR]`。
- 临时单模型加载测试：成功导入 `cat_mask.fbx` 的 5469 个顶点和 30864 个索引并创建共享 MeshResource。
- 临时创建两个 `cat_mask.fbx` 实例并将一个移动到视锥外：`submitted=2`、`visible=1`、`culled=1`。
- 两个实例仍只上传一次 MeshResource，第二次模型加载命中 Model cache。
- 可见实例统计为 10288 个三角形。
- 临时测试代码已撤回，不改变 Example 默认场景。
- 本轮新增及修改文件严格 UTF-8 解码通过，无非法字节或替换字符。
- 重新生成 16 页 `RenderEngine_Project_Overview.pptx`，并通过 PowerPoint 导出 PNG 全页检查，无明显文本溢出或中文乱码。
- 新版 PDF 已覆盖原 `RenderEngine_Project_Overview.pdf`，并校验源文件与覆盖结果 SHA-256 一致；临时 `_updated` 文件已删除。

### 后续注意

- Shadow caster 当前未按相机视锥裁剪是有意设计；下一步应使用方向光阴影视锥或 cascade 范围裁剪。
- 透明分类目前根据 `Material::opacity`，后续导入 glTF alphaMode 后应区分 Opaque、Mask 和 Blend。
- `RenderItem` 当前保留短生命周期 `Mesh*` 观察引用，待 Scene 所有权迁移为 Handle/Generation 后可改为稳定句柄。

## 未提交 - 补充核心源码中文注释并统一编码

- 改动类型：文档 / 可维护性 / 构建稳定性
- 建议提交标题：`docs: 补充核心渲染链路中文注释并统一源码编码`

### 改动摘要

为最近十次提交涉及的资源、场景、渲染、后处理和示例入口补充中文架构注释，解释 CPU 资产与 GPU 资源的边界、场景与 Renderer 的职责、材质绑定、RenderTarget 生命周期以及 IBL 预计算流程。本轮不改变渲染行为，只提升代码可读性和后续维护效率。

### 具体改动

- `AssetManager`、`AssetTypes`、`Model`、`Mesh`、`MeshResource`：说明模型导入、资源缓存、GPU 上传和共享所有权。
- `Material`、`MaterialSystem`、`ShaderLibrary`、`MeshRenderer`：说明材质数据、ShaderHandle 解析和 pass 绑定关系。
- `Scene`、`Renderable`、`Transform`、`Camera`、`Light`：说明场景树、节点矩阵、相机输入和阴影空间职责。
- `RenderTarget`、`RenderGraph`、`PostProcessRenderer`、`IBLSystem`、`Renderer`、OpenGL RHI：说明渲染目标、顺序 pass、后处理和 IBL 生命周期。
- `Example/main.cpp`：说明窗口 resize 转发以及 OpenGL Context 销毁前的资源释放顺序。
- 新增 `.editorconfig` 与 `.gitattributes`，统一源码、CMake、批处理和 Markdown 为 UTF-8，约束换行和 Git 工作树编码。
- MSVC 的 Engine/Example target 显式启用 `/utf-8`；历史含中文注释的 Scene、Renderable 文件已转换为 UTF-8。

### 编码与验证

- UTF-8 严格解码检查：本轮修改的源码/文档文件均通过，无非法字节和 `U+FFFD` 替换字符。
- `git diff --check`：通过。
- Debug/Release 构建：均已通过，未出现 C4819 或中文编码导致的级联语法错误。
- Release 程序运行 8 秒：进程保持正常，日志中未发现 Shader 编译/链接失败、Framebuffer incomplete、HIGH severity 或 `[ERROR]`。
- Git clean 编码转换检查：通过，`.gitattributes` 的 UTF-8 工作树规则可正常处理本轮文件。

### 后续注意

- 编辑器应遵循 `.editorconfig`；如果终端仍显示 `ç›®`、`æ¨¡åž‹` 等文本，那是终端代码页显示问题，不代表文件编码损坏。
- 新增源文件必须保存为 UTF-8，并避免使用本地 ANSI/GBK 保存。

## 未提交 - 引入 RenderTarget 统一渲染目标资源

- 改动类型：渲染架构 / GPU 资源生命周期 / 后处理 / 文档
- 建议提交标题：`refactor: 引入 RenderTarget 统一管理渲染目标`

### 改动摘要

新增 `RenderTargetDesc / RenderTarget`，统一管理 framebuffer 的颜色、深度/模板 attachment、纹理所有权和动态尺寸。主场景 HDR 目标和全部后处理目标完成迁移，删除 Renderer 中分散的裸纹理字段和手工 delete；Bloom 各级 pass 改为使用实际 RenderTarget 尺寸设置 viewport 与 texel size。

### 具体改动

1. 新增 RenderTarget 抽象

- `RenderTargetDesc` 描述名称、宽高、color attachment 和可选 depth/stencil attachment。
- attachment 描述包含纹理格式、Sampler、clear action、clear color/depth/stencil。
- `RenderTarget` 通过 `RenderContext` 创建纹理，用 `std::unique_ptr<Texture2D>` 管理生命周期。
- `FrameBufferInfo` 只保留指向 attachment 的非拥有指针，供 RHI 创建和绑定 FBO。
- 提供统一 `resize()`、color/depth texture 查询以及实际宽高查询。
- 禁止复制 RenderTarget，防止 GPU 资源所有权被隐式复制。

2. 主场景与后处理迁移

- `MeshRenderer` 构造函数显式接收 `RenderContext&`。
- 原主场景 FBO 和 color/depth 裸纹理合并为 `sceneTarget_`。
- highlight、5 级 downsample、5 级 upsample、Bloom composite 全部改为 RenderTarget。
- Radial、Motion A/B、Cartoon、Ripple 全部改为 RenderTarget。
- 删除十余个裸 `Texture2D*` 所有权字段和手工析构列表。
- 输出纹理通过 `targetForEffect()` 统一查询。

3. Bloom 尺寸修正

- 每一级 downsample/upsample viewport 使用目标自身 width/height。
- Shader 的 `textureSize` 使用实际目标尺寸的倒数。
- 极小窗口下 mip 尺寸至少为 1，避免 viewport 为 0 或与纹理存储不一致。

4. 文档

- 重写 README，补充项目定位、构建方式、运行日志和功能列表。
- 新增 `docs/ARCHITECTURE.md`，整理完整分层、帧流程、资源流程、PBR/IBL 原理、本轮架构改动、C++ 知识点、现存问题和后续路线。

### 原理与架构影响

RenderTarget 把“Framebuffer 使用哪些纹理”和“这些纹理由谁创建、resize、释放”收敛为一个对象。上层 pass 只消费目标，不再同时维护 FBO 描述和纹理所有权。显式注入 `RenderContext&` 也减少了新代码对全局 singleton 的依赖，为后续 RenderTargetPool、RenderGraph transient resource 和多图形后端继续演进提供基础。

### 验证情况

- CMake 重新配置：通过。
- Debug 构建：通过。
- Release 构建：通过。
- 默认场景运行 8 秒：通过。
- Bloom 完整链路运行 8 秒：通过，运行时 framebuffer id 2-14 正常创建。
- `1919 × 1009` 与 `800 × 600` 连续 resize：通过。
- 未出现 Shader compile/link、Framebuffer incomplete 或高优先级 OpenGL 错误。
- `git diff --check`：通过。

### 后续注意

- Shadow 和 IBL 内部临时 framebuffer 尚未迁移，因为 cubemap/mip/face attachment 需求需要先扩展 RenderTarget 描述。
- 本阶段已增加可共享的 GPU `MeshResource`，避免同一 ModelAsset 多实例重复创建 VAO/VBO/IBO。

## 未提交 - 增加可共享 MeshResource 与 GPU 资源缓存

- 改动类型：渲染架构 / GPU 资源管理 / 性能优化 / 诊断
- 建议提交标题：`perf: 缓存共享 MeshResource 避免重复上传 GPU`

### 改动摘要

将模型导入得到的 CPU `MeshAsset` 与运行时 GPU 资源分开。`AssetManager` 在模型首次加载时创建并缓存 `MeshResource`，多个 `Mesh` 实例共享同一组 VAO/VBO/IBO；Transform、Material 和场景对象仍保持实例级/资产级职责。

### 具体改动

- 新增 `Renderer/MeshResource.h/.cpp`，封装 GPU buffer 创建、顶点布局、绘制和 RAII 释放。
- `MeshAsset` 与 `Mesh` 增加共享 `MeshResource` 引用。
- `AssetManager` 增加 `meshResourceCache_`、`loadMeshResource()` 和资源统计接口。
- 模型资源 key 使用规范化模型路径和 mesh 索引，避免不同实例重复上传。
- 记录上传次数、缓存命中次数、顶点数、索引数和 GPU buffer 字节数。
- 无 AssetManager 的旧式 Model 构造保留实例上传 fallback，兼容旧调用路径。
- 修正旧 Mesh fallback 中 index buffer 的释放接口，并清理 size_t 到 RHI 整数参数的编译警告。

### 原理与架构影响

```text
ModelAsset -> MeshAsset（CPU 顶点/索引）
          -> MeshResource（共享 GPU VAO/VBO/IBO）
          -> Mesh 实例（Transform + Material 引用）
```

资源生命周期由 `shared_ptr` 表达：AssetManager 和 MeshAsset 持有资源所有权，Mesh 实例只共享引用；当最后一个场景实例和 AssetManager 缓存释放后，MeshResource 自动删除 GPU 对象。

### 验证情况

- Debug / Release 构建：通过。
- 同一 `cat_mask.fbx` 创建两个实例：首实例约 `16.6 ms`，缓存实例约 `0.35 ms`。
- 日志确认 `Model cache hit`、`resources=1`、`uploads=1`。
- GPU buffer 统计：`vertices=5469`、`indices=30864`、`gpuBufferBytes=429720`。
- 默认运行日志无 Shader 编译错误、Framebuffer incomplete 或高优先级 OpenGL 错误。
- 临时双实例测试代码已恢复，不改变 Example 默认场景。

### 后续注意

- 当前 Mesh 实例仍保留 CPU 顶点/索引副本，下一步应改为共享只读 CPU Geometry 或仅保存包围盒数据。
- 修复 Example 退出顺序：所有 OpenGL-backed 资源在 GLFW Context 销毁前释放，避免 MeshResource RAII 析构访问失效 Context。

## ce6a322 - 修复模型节点变换并完善相机与窗口适配

- 提交时间：2026-08-05 20:32:20 +0800
- 完整提交：`ce6a322bcd16a1cbbb7354d3df8d2033d01f8c7f`
- 改动类型：模型导入 / 相机控制 / 渲染修复 / 窗口适配

### 改动摘要

修复 Assimp 节点局部矩阵在 `ModelAsset` 和场景实例化流程中丢失的问题；增加 Q/E 世界空间垂直相机移动；完善窗口 resize 链路，使投影矩阵、主场景 HDR/Depth 纹理和全部后处理纹理随 framebuffer 尺寸同步调整，避免最大化或改变窗口尺寸后画面被拉伸。

### 具体改动

1. 模型节点变换

- `ModelNodeAsset` 增加 `localTransform`，保存 Assimp 节点的 `aiNode::mTransformation`。
- 补充 Assimp 矩阵到 GLM 矩阵的正确转换，并在 `Model::instantiateNode()` 中应用到场景对象。
- `Transform` 增加源局部矩阵，最终局部矩阵按“运行时 TRS × 导入节点矩阵”组合，避免运行时编辑覆盖模型文件自带变换。
- `ModelAsset` 增加 `transformedNodeCount` 导入诊断；使用 `cat_mask.fbx` 验证得到 `transformedNodes=1`。

2. 相机输入

- `Camera_Movement` 增加 `UP` 和 `DOWN`。
- Q 键沿世界 Y 轴上升，E 键沿世界 Y 轴下降。
- 垂直移动继续使用 `MovementSpeed` 和 `deltaTime`，与原有相机移动速度保持一致。

3. 动态窗口尺寸

- `Texture2D` 增加 `resize()`，在保留 OpenGL texture ID 的情况下重新分配纹理存储。
- `Renderer::resize()` 更新 `RenderContext` 尺寸，并把新尺寸传递给 `MeshRenderer` 和 `PostProcessRenderer`。
- 主场景 HDR color/depth 纹理以及 Bloom、downsample、upsample、radial、motion、cartoon、ripple 后处理纹理全部同步 resize。
- 相机投影宽高比改为使用实时 framebuffer 宽高，不再固定为 `800 / 600`。
- GLFW framebuffer size callback 除更新 viewport 外，同时通知 `Renderer` 重建相关渲染目标。
- resize 完成后输出尺寸日志，便于定位窗口与渲染目标不一致的问题。

### 架构影响

- 窗口尺寸变化由应用层 callback 统一传入 `Renderer`，再由 `Renderer` 分发给各渲染模块，避免 Example 直接操作各个 framebuffer 资源。
- `Transform` 明确区分模型导入产生的源节点矩阵与运行时 TRS，为后续动画、层级变换和编辑器操作保留清晰边界。
- 当前 resize 仍通过各渲染器主动重分配纹理；后续引入 RenderTargetPool 或完善 RenderGraph 资源系统后，可集中管理瞬时渲染目标。

### 验证情况

- Debug 构建：通过。
- Release 构建：通过。
- `git diff --check`：通过。
- 最大化窗口实测 framebuffer 尺寸为 `1920 × 1009`，主场景显示比例正常。
- Bloom 后处理完成窗口最大化验证。
- 未发现 framebuffer incomplete、Shader compile/link 或 OpenGL 功能错误。
- 验证使用的临时模型自动加载和 Bloom 默认开启代码均已恢复，不包含在待提交改动中。

### 后续注意

- 下一阶段应将 framebuffer/texture 尺寸依赖进一步收敛到统一的 RenderTarget 或 RenderGraph 资源描述中，减少每个 renderer 独立维护 resize 列表的成本。

## d88465d - 完善 PBR IBL 与多格式模型材质导入

- 提交时间：2026-08-04 14:18:29 +0800
- 完整提交：`d88465d94713dd176ac1117bcde36a8c7c921083`
- 改动类型：渲染功能 / 材质导入 / 阴影修复 / 示例工具 / 构建部署 / 文档

### 改动摘要

本轮将默认 lit pass 切换至完整的 metallic-roughness PBR，并补齐 IBL 环境光、HDR tone mapping、方向光阴影稳定性和有限范围点光。模型导入同时支持 glTF/GLB 原生 PBR 材质与旧式 specular-gloss 材质转换；示例程序的模型选择窗口默认展示 OBJ、FBX、GLB、GLTF、DAE。

### 具体改动

1. PBR 与 IBL

- 新增 `IBLSystem`：从 HDR 环境贴图生成 environment cubemap、irradiance map、prefiltered environment map 和 BRDF LUT。
- `engine/default-lit` 使用 GGX/Smith/Fresnel metallic-roughness BRDF，同时叠加方向光、点光、阴影和 IBL。
- 屏幕 pass 改为 exponential tone mapping + sRGB gamma；Example 提供 Exposure 调节，默认值为 `1.35`。

2. 材质导入与资源复用

- `Material` 保持纯数据对象，`MaterialSystem` 统一绑定参数及纹理；材质不再持有 Shader 或直接发起 GL 调用。
- GLB/GLTF 或明确声明 PBR 数据的材质使用模型自带的 base color、metallic、roughness 及合并 metallic-roughness 贴图；正确按 glTF 语义读取 G=roughness、B=metallic。
- 非 PBR 模型使用引擎默认材质因子：`metallic=0`、`roughness=0.5`，同时仍读取基础色、法线、AO、发光等兼容贴图。
- 旧式 specular-gloss 材质可转换：`map_Kd`→base color，`map_Bump`→normal，`map_Ks + Ks`→dielectric F0，`map_Ka`→IBL 反射遮罩，`Ns`→感知 roughness。nanosuit 的 `Ns=96.078` 转换结果约为 `0.143`。
- 按 Assimp 源材质索引只创建一次 `MaterialAsset`，同一模型内的多个 mesh 共享材质资源。
- 重建法线贴图 TBN 的正交基与手性，减少导入模型因切线空间失真造成的异常高光。

3. 灯光、阴影与泛光

- 方向光 shadow bias 改为使用恒定的 `-light.direction`；正交阴影范围扩大至 `±38`，并在 shadow pass 设置 viewport、polygon offset 及状态恢复。
- `PointLight` 增加平滑衰减的有限 `range`，低强度点光不再远距离染色场景。
- 灯光调试 cube 的 HDR 可见亮度与真实照明强度分离，保留 Bloom 效果。

4. 工具与构建

- 模型选择器默认过滤 `.obj,.fbx,.glb,.gltf,.dae`。
- 构建后自动复制运行时 Assimp DLL；忽略运行日志及 ImGui 本地布局文件。

### 验证情况

- `cmake --build build --config Debug`：通过。
- `cmake --build build --config Release`：通过。
- `git diff --check`：通过。

### 后续注意

- 真实透明玻璃仍未实现 glTF alpha mode、transmission、IOR、thickness；Glass 当前是旧式不透明 specular-gloss 近似。

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

## 076e0e5 - 修复运行时诊断日志与 OpenGL 状态绑定问题

- 提交时间：2026-07-20 14:52:14 +0800
- 改动类型：修复 / 运行时诊断 / 渲染稳定性 / 构建验证
- 建议提交标题：`修复运行时诊断日志与 OpenGL 状态绑定问题`

### 改动摘要

本轮目标是让程序运行失败时能直接看到错误原因，并修复验证过程中暴露出的 OpenGL 状态问题。当前 Release 版本已经可以正常启动，运行日志会输出到控制台和 `logs/engine.log`。

### 具体改动

1. 修复 `RenderGraph` 前置声明命名空间不一致。

- `RenderGraph` 当前是全局类，`Scene.h` 之前把它声明在 `realtimerenderingengine` 命名空间内，导致 `Scene`、`MeshRenderer` 使用到的类型不一致。
- 调整后 `Scene` 与渲染 pass 使用同一个 `::RenderGraph` 类型，避免编译期签名不匹配。

2. 重写 `OpenGLRenderContext::beginRendering(FrameBufferInfo&)` 的 FBO 创建逻辑。

- 清理原先损坏的括号和乱码注释区域，避免语法级联错误。
- 创建 framebuffer 后立即检查 `glCheckFramebufferStatus`。
- color attachment 为空时输出明确错误。
- depth / depth-stencil / cubemap depth attachment 的绑定路径统一处理。

3. 修复 pipeline 绑定时没有激活 shader 的问题。

- `OpenGLRenderContext::bindPipeline()` 现在会调用 `pipeline.shader->use()`。
- 原问题会导致后续 uniform 写入落到上一个仍处于 active 状态的 program 上，运行时表现为 OpenGL 报 `Uniform must be a matrix type in call to UniformMatrix*` 或渲染结果异常。
- 这是典型 OpenGL 状态机问题：`GraphicsPipeline` 绑定不完整时，材质和 pass 的 uniform 设置无法保证作用到目标 shader。

4. 修复贴图采样参数导致的 `GL_INVALID_ENUM`。

- `GL_TEXTURE_MAG_FILTER` 只能使用 `GL_LINEAR` 或 `GL_NEAREST`，不能使用 mipmap filter。
- 2D texture 不再设置 `GL_TEXTURE_WRAP_R`。
- mipmap 生成条件从只判断 `Linear` 调整为只要不是 `MipmapMode::None` 就生成。

5. 补齐编译稳定性修复。

- `MaterialAsset`、`ModelAsset` 的前置声明从 `class` 改为 `struct`，与真实定义一致。
- `PostProcessRenderer.h` 引入完整 `Shader` 类型，避免 `TRefCountPtr<Shader>` 析构时类型不完整。
- `main.cpp` 引入 `Shader.h`，并显式使用 `std::to_string`。
- `getBlendFactor()` 增加兜底返回值，避免控制流缺少返回值。
- 后处理 shader 参数字面量改为 `float`，消除类型警告。

6. 修复 Assimp 运行时 DLL 需要手动复制的问题。

- `Example/CMakeLists.txt` 增加 `ASSIMP_RUNTIME_DLL` 变量，集中记录 `assimp-vc142-mt.dll` 路径。
- 保留 `example` 的 `POST_BUILD copy_if_different`，确保 `example.exe` 链接完成后把 Assimp DLL 复制到 `$<TARGET_FILE_DIR:example>`。
- 新增 `copy_example_runtime_dlls` 目标并加入 `ALL`，即使 `example` 本身被判断为 up-to-date，构建 solution 时也会检查并补齐运行时 DLL。
- 原因：Windows 运行 exe 时默认不会去第三方库目录找 DLL，动态库必须位于 exe 同目录、系统目录或 PATH 中；只链接 `.lib` 不能自动解决运行时 DLL 部署。

7. 忽略运行产物。

- `.gitignore` 增加 `logs/` 和 `imgui.ini`。
- 原因：这些文件由本地运行生成，不属于源码提交内容；如果一次性暂存未暂存文件，容易把本机日志和 UI 状态提交进去。

### 验证情况

- `cmake --build build --config Debug`：通过。
- `cmake --build build --config Release`：通过。
- `build/src/Example/Release/example.exe`：正常启动，8 秒启动检查期间进程保持运行。
- `logs/engine.log`：成功生成，当前启动日志未出现 shader 编译失败、framebuffer incomplete 或高优先级 OpenGL 错误。

### 后续注意

- Debug exe 直接双击如果退出码为 `0xC0000135`，原因通常是缺少 Visual Studio Debug CRT DLL，并不是引擎主逻辑已经进入后崩溃；用 Visual Studio F5、Developer Command Prompt、补齐 Debug CRT PATH，或直接运行 Release。
- 后续如果继续往中小型渲染引擎方向推进，建议把当前日志继续扩展成 `Logger + Assert + GL_CHECK` 的统一诊断层，而不是在各模块零散调用 `glGetError()`。
