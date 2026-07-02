#pragma once

#include <string>
#include <vector>
#include <cmath>

struct UISpriteQuad {
    float x, y;          // Screen pixel coordinates
    float width, height; // Size metrics
    float u, v;          // Texture atlas coordinate starts
    float uWidth, vVert; // Texture atlas mapping widths
    float opacity;       // Alpha blending states
};

class HUDSystem {
public:
    HUDSystem();
    ~HUDSystem() = default;

    // Interface Hooks
    void ResetWantedStatus();
    void CommitCrime(int severityPoints);
    
    // Core Tick updates for flashing star animations
    void UpdateHUDTimers(float deltaTime);

    // Compiles drawing commands into an array of 2D quads to send to Vulkan
    std::vector<UISpriteQuad> CompileHUDFrameQuads(int screenWidth, int screenHeight);

    // Getters for gameplay evaluation loops
    int GetWantedLevelStars() const { return mCurrentStars; }
    bool IsPlayerWanted() const { return mCurrentStars > 0; }

private:
    int mCurrentStars;
    int mCrimeSeverityPoints;
    
    // Animation trackers
    float mFlashTimer;
    bool mIsStarTextureAlternativeActive;
    
    // Threshold boundaries determining crime escalation scaling
    const int mPointsPerStar = 100;
    const int mMaxStars = 6;
};
