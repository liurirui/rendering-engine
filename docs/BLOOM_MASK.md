# 显式 Bloom Mask

## 目的

Bloom 不再从最终场景颜色使用 Rec.709 亮度阈值推断。红、绿、蓝三种颜色的感知亮度不同，单一阈值会让红光和蓝光被错误过滤；同时普通材质被强光照亮也可能误进入 Bloom。

## 渲染流程

场景 RenderTarget 现在包含两个颜色附件：

| 附件 | 内容 |
| --- | --- |
| `colorAttachments[0]` | HDR 场景颜色，供 Tone Mapping 和最终合成使用 |
| `colorAttachments[1]` | HDR Bloom Mask，只包含明确声明的泛光来源 |

PBR 材质 Shader 通过 MRT 同时写入两个附件。场景颜色仍包含完整的直接光、IBL 和自发光；Bloom Mask 只写入自发光颜色。灯光可视化 Cube 也同时写入场景颜色和独立的 Bloom Mask，因此 `lightVisualIntensity` 不会影响真实点光源的照明强度。

后处理高亮提取 Pass 只采样 `scene.bloom`，之后继续执行 downsample、upsample 和 Bloom 合成。普通光照、镜面高光和环境光不会因为 RGB 亮度阈值自动产生泛光。

## 扩展方式

后续可以在材质数据中增加 `emissiveBloomIntensity`，或为粒子、特效添加独立的 Bloom 写入策略。需要保持 Scene Color 与 Bloom Mask 的职责分离，不能通过提高 PointLight 的真实强度来模拟泛光。
