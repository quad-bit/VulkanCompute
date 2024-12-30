#include "VulkanManager.h"

int main()
{
    constexpr uint32_t screenWidth = 600;
    constexpr uint32_t screenHeight = 600;

    std::unique_ptr<WindowManager> windowManagerObj = std::make_unique<WindowManager>(screenWidth, screenHeight);
    windowManagerObj->Init();

    std::unique_ptr<VulkanManager> vulkanManager = std::make_unique<VulkanManager>(screenWidth, screenHeight);
    vulkanManager->Init(windowManagerObj->glfwWindow);

    uint32_t maxFramesInFlight = vulkanManager->GetMaxFramesInFlight();
    std::vector<VkSemaphore> swapchainImageAcquiredSemaphores;

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        ErrorCheck(vkCreateSemaphore(vulkanManager->GetLogicalDevice(), &info, nullptr, &semaphore));
        swapchainImageAcquiredSemaphores.push_back(semaphore);
    }

    uint64_t frameIndex = 0;
    while (windowManagerObj->Update())
    {
        auto currentFrameInFlight = vulkanManager->GetFrameInFlightIndex();
        //vulkanManager->Update(frameIndex);

        // Trigger compute


        // Get the active swapchain index and related fence
        auto [activeSwapchainImageindex, imageAcquireFence] = vulkanManager->GetActiveSwapchainImageIndex(swapchainImageAcquiredSemaphores[currentFrameInFlight]);
        // Trigger graphics tasks

        // End the frame (increments index counters)
        vulkanManager->CopyAndPresent(VK_NULL_HANDLE, swapchainImageAcquiredSemaphores[currentFrameInFlight], imageAcquireFence);
        frameIndex++;
    }

    vulkanManager->DeInit(swapchainImageAcquiredSemaphores);
    windowManagerObj->DeInit();

    return 0;
}