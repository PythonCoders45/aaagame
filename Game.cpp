#pragma once

#include "../../engine/graphics/VulkanRenderer.h"
#include "../../engine/physics/PhysicsManager.h"
#include "../../engine/audio/AudioManager.h"
#include "../control/ControlManager.h"

class Game {
public:
    Game() = default;
    ~Game() = default;

    bool Initialize();
    void Run();
    void Shutdown();

private:
    VulkanRenderer mRenderer;
    PhysicsManager mPhysics;
    AudioManager   mAudio;
    ControlManager mControls;

    bool mIsRunning = false;
};