#include "BountyCalculator.h"
#include <algorithm>

BountyCalculator::BountyCalculator() 
    : mTargetBounty(0), mDisplayBounty(0.0f), mIncrementSpeed(500.0f) {
}

void BountyCalculator::RecordInfraction(const std::string& crimeType) {
    int penalty = 0;

    // Map-based dynamic structural crime logic thresholds
    if (crimeType == "vandalism")       penalty = 150;
    else if (crimeType == "grand_theft") penalty = 1250;
    else if (crimeType == "reckless_drv") penalty = 300;
    else                                 penalty = 50; // Ambient default fallback

    mTargetBounty += penalty;
}

void BountyCalculator::UpdateInterpolation(float deltaTime) {
    if (mDisplayBounty < mTargetBounty) {
        // Linearly advance the counter toward the true total
        mDisplayBounty += mIncrementSpeed * deltaTime;
        if (mDisplayBounty > mTargetBounty) {
            mDisplayBounty = static_cast<float>(mTargetBounty);
        }
    }
}

void BountyCalculator::Reset() {
    mTargetBounty = 0;
    mDisplayBounty = 0.0f;
}
