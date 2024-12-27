#include "VulkanManager.h"

int main()
{
    constexpr uint32_t screenWidth = 600;
    constexpr uint32_t screenHeight = 600;

    std::unique_ptr<WindowManager> windowManagerObj = std::make_unique<WindowManager>(screenWidth, screenHeight);
    windowManagerObj->Init();

    std::unique_ptr<VulkanManager> vulkanManager = std::make_unique<VulkanManager>(screenWidth, screenHeight);
    vulkanManager->Init();
    vulkanManager->CreateSurface(windowManagerObj->glfwWindow);

    while (windowManagerObj->Update())
    {

    }

    vulkanManager->DeInit();
    windowManagerObj->DeInit();

    return 0;
}