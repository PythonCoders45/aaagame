#pragma once

#include <string>

class BountyCalculator {
public:
    BountyCalculator();
    ~BountyCalculator() = default;

    // Triggered when crimes are registered in the world loop
    void RecordInfraction(const std::string& crimeType);
    
    // Smoothly steps the bounty value display over time (ticking up to the goal value)
    void UpdateInterpolation(float deltaTime);

    // Getters
    int GetTargetBounty() const { return mTargetBounty; }
    int GetDisplayBounty() const { return static_cast<int>(mDisplayBounty); }
    void Reset();

private:
    int mTargetBounty;     // The actual mathematical total bounty
    float mDisplayBounty;  // The current ticking number shown on screen
    float mIncrementSpeed; // Speed multiplier for the counter animation
};
