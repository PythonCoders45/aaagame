#include "TopLeftHUD.h"

std::vector<UISpriteQuad> TopLeftHUD::BuildTopLeftBountyFrame(const BountyCalculator& calculator) {
    std::vector<UISpriteQuad> topLeftBatch;
    
    int activeValue = calculator.GetDisplayBounty();
    if (activeValue <= 0) return topLeftBatch; // Keep the screen perfectly clean if no bounty exists

    std::string formattedText = "BOUNTY: $" + std::to_string(activeValue);

    // Top-Left Screen Layout Anchors
    float cursorX = 30.0f; 
    float cursorY = 40.0f; 
    float glyphWidth = 18.0f;
    float glyphHeight = 28.0f;
    float letterSpacing = 14.0f;

    float textureSliceWidth = 0.05f; // Layout grid resolution across your text atlas image

    for (char c : formattedText) {
        UISpriteQuad glyph;
        glyph.x = cursorX;
        glyph.y = cursorY;
        glyph.width = glyphWidth;
        glyph.height = glyphHeight;
        glyph.opacity = 1.0f;
        glyph.vVert = 0.20f;

        // Map character strings straight to the corresponding texture coordinates on your atlas row
        if (c == 'B') { glyph.u = 0.00f; glyph.v = 0.60f; }
        else if (c == 'O') { glyph.u = 0.05f; glyph.v = 0.60f; }
        else if (c == 'U') { glyph.u = 0.10f; glyph.v = 0.60f; }
        else if (c == 'N') { glyph.u = 0.15f; glyph.v = 0.60f; }
        else if (c == 'T') { glyph.u = 0.20f; glyph.v = 0.60f; }
        else if (c == 'Y') { glyph.u = 0.25f; glyph.v = 0.60f; }
        else if (c == ':') { glyph.u = 0.30f; glyph.v = 0.60f; }
        else if (c == '$') { glyph.u = 0.35f; glyph.v = 0.60f; }
        else if (c >= '0' && c <= '9') {
            int digitOffset = c - '0';
            glyph.u = 0.40f + (digitOffset * textureSliceWidth);
            glyph.v = 0.60f;
        } else {
            // Space bar or unhandled characters simply advance the cursor without printing a texture
            cursorX += letterSpacing;
            continue;
        }

        glyph.uWidth = textureSliceWidth;
        topLeftBatch.push_back(glyph);

        // Advance the cursor rightward for the next character
        cursorX += letterSpacing;
    }

    return topLeftBatch;
}
