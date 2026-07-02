#include "ControlManager.h"

void ControlManager::Initialize(GLFWwindow* window) {
    mWindow = window;
    std::cout << "Control Manager Initialized." << std::endl;
}

void ControlManager::Update() {
    if (!mWindow) return;

    // Reset states
    mThrottle = 0.0f;
    mSteering = 0.0f;
    mActionKey = false;

    // Forward / Backward (Throttle)
    if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS) mThrottle += 1.0f;
    if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS) mThrottle -= 1.0f;

    // Left / Right (Steering)
    if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS) mSteering -= 1.0f;
    if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS) mSteering += 1.0f;

    // Action Key (e.g., Enter Car, Open Door - standard GTA 'F' key)
    if (glfwGetKey(mWindow, GLFW_KEY_F) == GLFW_PRESS) {
        mActionKey = true;
    }
}