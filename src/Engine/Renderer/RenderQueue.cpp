#include "RenderQueue.h"

#include <Base/Material.h>
#include <algorithm>
#include <cstdint>

NAMESPACE_START

namespace {

const std::string& shaderName(const RenderItem& item) {
    static const std::string empty;
    return item.materialAsset ? item.materialAsset->shader.name : empty;
}

bool opaqueLess(const RenderItem& lhs, const RenderItem& rhs) {
    const std::string& lhsShader = shaderName(lhs);
    const std::string& rhsShader = shaderName(rhs);
    if (lhsShader != rhsShader) {
        return lhsShader < rhsShader;
    }

    const auto lhsMaterial = reinterpret_cast<std::uintptr_t>(lhs.materialAsset.get());
    const auto rhsMaterial = reinterpret_cast<std::uintptr_t>(rhs.materialAsset.get());
    if (lhsMaterial != rhsMaterial) {
        return lhsMaterial < rhsMaterial;
    }

    const auto lhsMesh = reinterpret_cast<std::uintptr_t>(lhs.meshResource.get());
    const auto rhsMesh = reinterpret_cast<std::uintptr_t>(rhs.meshResource.get());
    if (lhsMesh != rhsMesh) {
        return lhsMesh < rhsMesh;
    }
    return lhs.cameraDistance < rhs.cameraDistance;
}

} // namespace

void RenderQueue::clear() {
    shadowCasters.clear();
    opaqueItems.clear();
    transparentItems.clear();
    stats = RenderStats();
}

void RenderQueue::sort() {
    // 不透明物体优先减少 Shader/Material/Mesh 状态切换；透明物体必须从远到近混合。
    std::stable_sort(opaqueItems.begin(), opaqueItems.end(), opaqueLess);
    std::stable_sort(shadowCasters.begin(), shadowCasters.end(), opaqueLess);
    std::stable_sort(transparentItems.begin(), transparentItems.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.cameraDistance > rhs.cameraDistance;
    });
}

NAMESPACE_END
