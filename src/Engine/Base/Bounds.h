#pragma once

#include "Constants.h"
#include <glm/glm.hpp>
#include <limits>

NAMESPACE_START

// 轴对齐包围盒（AABB）。模型导入时保存局部空间 Bounds；提交渲染前再用世界矩阵
// 生成 world Bounds，供视锥裁剪、透明排序和调试显示复用。
struct Bounds {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    void encapsulate(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    glm::vec3 center() const {
        return isValid() ? (min + max) * 0.5f : glm::vec3(0.0f);
    }

    glm::vec3 extents() const {
        return isValid() ? (max - min) * 0.5f : glm::vec3(0.0f);
    }

    // 使用 center/extents 变换 AABB，比逐个变换八个角点更紧凑；abs(mat3) 会把旋转、
    // 非均匀缩放对三个世界轴的贡献累加起来，最终仍得到轴对齐的世界包围盒。
    Bounds transformed(const glm::mat4& matrix) const {
        if (!isValid()) {
            return Bounds();
        }

        const glm::vec3 worldCenter = glm::vec3(matrix * glm::vec4(center(), 1.0f));
        const glm::mat3 linear(matrix);
        const glm::vec3 localExtents = extents();
        const glm::vec3 worldExtents(
            glm::abs(linear[0][0]) * localExtents.x + glm::abs(linear[1][0]) * localExtents.y + glm::abs(linear[2][0]) * localExtents.z,
            glm::abs(linear[0][1]) * localExtents.x + glm::abs(linear[1][1]) * localExtents.y + glm::abs(linear[2][1]) * localExtents.z,
            glm::abs(linear[0][2]) * localExtents.x + glm::abs(linear[1][2]) * localExtents.y + glm::abs(linear[2][2]) * localExtents.z);

        Bounds result;
        result.min = worldCenter - worldExtents;
        result.max = worldCenter + worldExtents;
        return result;
    }
};

NAMESPACE_END
