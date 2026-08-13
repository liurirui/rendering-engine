#pragma once

#include "RenderItem.h"
#include <Base/Constants.h>
#include <cstdint>
#include <vector>

NAMESPACE_START

struct RenderStats {
    uint32_t submittedItems = 0;
    uint32_t visibleItems = 0;
    uint32_t culledItems = 0;
    uint32_t invalidBoundsItems = 0;
    uint32_t shadowItems = 0;
    uint32_t opaqueItems = 0;
    uint32_t transparentItems = 0;
    uint64_t visibleTriangles = 0;
};

class RenderQueue {
public:
    void clear();
    void sort();

    std::vector<RenderItem> shadowCasters;
    std::vector<RenderItem> opaqueItems;
    std::vector<RenderItem> transparentItems;
    RenderStats stats;
};

NAMESPACE_END
