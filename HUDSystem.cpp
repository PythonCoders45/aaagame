#include "HUDSystem.h"

HUDSystem::HUDSystem() 
    : mCurrentStars(0), mCrimeSeverityPoints(0), mFlashTimer(0.0f), mIsStarTextureAlternativeActive(false) {
}

void HUDSystem::ResetWantedStatus() {
    mCrimeSeverityPoints = 0;
    mCurrentStars = 0;
}

void HUDSystem::CommitCrime(int severityPoints) {
    mCrimeSeverityPoints += severityPoints;
    
    // Re-evaluate how many stars the player has earned based on crime scores
    int calculatedStars = mCrimeSeverityPoints / mPointsPerStar;
    if (calculatedStars > mMaxStars) calculatedStars = mMaxStars;
    
    if (calculatedStars > mCurrentStars) {
        mCurrentStars = calculatedStars;
        // Trigger star flashing timer instantly on status upgrade
        mFlashTimer = 5.0f; 
    }
}

void HUDSystem::UpdateHUDTimers(float deltaTime) {
    if (mCurrentStars > 0) {
        if (mFlashTimer > 0.0f) {
            mFlashTimer -= deltaTime;
            
            // Toggle star sprite switching state every 0.25 seconds to create a flashing effect
            mIsStarTextureAlternativeActive = (static_cast<int>(mFlashTimer * 4.0f) % 2 == 0);
        } else {
            mIsStarTextureAlternativeActive = false; // Steady state star rendering
        }
    }
}

std::vector<UISpriteQuad> HUDSystem::CompileHUDFrameQuads(int screenWidth, int screenHeight) {
    std::vector<UISpriteQuad> uiBatchList;
    
    if (mCurrentStars <= 0) return uiBatchList; // No quads needed if clean status

    // Layout configuration metrics anchor points (Top-Right Screen Padding Layouts)
    float rightMarginOffset = static_cast<float>(screenWidth) - 40.0f;
    float topMarginOffset = 40.0f;
    float starSpacingHorizontal = 28.0f;
    float starDimensionsPixel = 24.0f;

    // Loop through and draw each star sprite
    for (int i = 0; i < mMaxStars; ++i) {
        UISpriteQuad starQuad;
        
        // Arrange stars from right to left across the HUD line grid layout
        starQuad.x = rightMarginOffset - (i * starSpacingHorizontal);
        starQuad.y = topMarginOffset;
        starQuad.width = starDimensionsPixel;
        starQuad.height = starDimensionsPixel;
        starQuad.opacity = 1.0f;

        // Step Evaluation: Determine which part of the texture sheet coordinates to read
        if (i < mCurrentStars) {
            // Player earned this star slot
            if (mIsStarTextureAlternativeActive) {
                // Flashing alternative look (e.g., bright yellow/neon state offset)
                starQuad.u = 0.5f;  starQuad.v = 0.0f;
            } else {
                // Standard filled active star state
                starQuad.u = 0.0f;  starQuad.v = 0.0f;
            }
            starQuad.uWidth = 0.25f; starQuad.vVert = 0.25f;
        } else {
            // Unearned empty backplate shadow star slot (like traditional GTA titles)
            starQuad.u = 0.75f; starQuad.v = 0.0f;
            starQuad.uWidth = 0.25f; starQuad.vVert = 0.25f;
            starQuad.opacity = 0.4f; // Dim the background asset slot down
        }

        uiBatchList.push_back(starQuad);
    }

    return uiBatchList;
}
