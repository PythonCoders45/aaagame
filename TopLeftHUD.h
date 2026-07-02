#pragma once

#include "BountyCalculator.h"
#include <vector>
#include <string>

struct UISpriteQuad {
    float x, y;          
    float width, height; 
    float u, v;          
    float uWidth, vVert; 
    float opacity;       
};

class TopLeftHUD {
public:
    TopLeftHUD() = default;
    ~TopLeftHUD() = default;

    // Loops through font sheets and shapes coordinate grids for the top-left margin layout
    std::vector<UISpriteQuad> BuildTopLeftBountyFrame(const BountyCalculator& calculator);
};
