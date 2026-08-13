#pragma once

#include <Base/Bounds.h>
#include <Base/Constants.h>
#include <glm/glm.hpp>

NAMESPACE_START

class Camera;
class Scene;
class RenderQueue;

class Frustum {
public:
    explicit Frustum(const glm::mat4& viewProjection);
    bool intersects(const Bounds& bounds) const;

private:
    glm::vec4 planes_[6]{};
};

class RenderQueueBuilder {
public:
    void build(const Scene& scene, const Camera& camera, const glm::mat4& projection, RenderQueue& output) const;
};

NAMESPACE_END
