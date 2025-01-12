#include "VulkanManager.h"
#include "GraphicsTask.h"
#include <optional>

int main()
{
    constexpr uint32_t screenWidth = 600;
    constexpr uint32_t screenHeight = 600;

    constexpr uint32_t imageWidth = 1024;
    constexpr uint32_t imageHeight = 1024;

    std::unique_ptr<WindowManager> windowManagerObj = std::make_unique<WindowManager>(screenWidth, screenHeight);
    windowManagerObj->Init();

    std::unique_ptr<VulkanManager> vulkanManager = std::make_unique<VulkanManager>(screenWidth, screenHeight);
    vulkanManager->Init(windowManagerObj->glfwWindow);

    uint32_t maxFramesInFlight = vulkanManager->GetMaxFramesInFlight();
    std::unique_ptr<GraphicsTask> pGraphicsTask = std::make_unique<GraphicsTask>(
        vulkanManager->GetLogicalDevice(), vulkanManager->GetPhysicalDevice(), vulkanManager->GetGraphicsQueue(),
        vulkanManager->GetQueueFamilyIndex(), vulkanManager->GetMaxFramesInFlight(),
        screenWidth, screenHeight);

    std::vector<VkSemaphore> swapchainImageAcquiredSemaphores;
    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        ErrorCheck(vkCreateSemaphore(vulkanManager->GetLogicalDevice(), &info, nullptr, &semaphore));
        swapchainImageAcquiredSemaphores.push_back(semaphore);
    }

    std::vector<std::unique_ptr<TimelineSemaphore>> timelineSemaphores;
    {
        timelineSemaphores.emplace_back(std::make_unique<TimelineSemaphore>(vulkanManager->GetLogicalDevice()));
        timelineSemaphores.emplace_back(std::make_unique<TimelineSemaphore>(vulkanManager->GetLogicalDevice()));
    }

    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    {
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphoreTypeCreateInfoKHR typeCreateInfo{};
        typeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
        typeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
        typeCreateInfo.initialValue = 0;
        createInfo.pNext = &typeCreateInfo;

        ErrorCheck(vkCreateSemaphore(vulkanManager->GetLogicalDevice(), &createInfo, nullptr, &timelineSemaphore));
    }

    uint64_t frameIndex = 0;
    while (windowManagerObj->Update())
    {
        auto currentFrameInFlight = vulkanManager->GetFrameInFlightIndex();
        if (timelineSemaphores[currentFrameInFlight]->GetFrameIndex() > 0)
        {
            // wait for previous frame's (corresponding frameInFlight) presentation to complete
            // this acts as a fence

            uint64_t value = (timelineSemaphores[currentFrameInFlight]->GetFrameIndex() - 1) * (TimelineStages::NUM_STAGES - 1) + TimelineStages::SAFE_TO_PRESENT;

            VkSemaphoreWaitInfo waitInfo{};
            waitInfo.pSemaphores = &timelineSemaphores[currentFrameInFlight]->GetSemaphore();
            waitInfo.pValues = &value;
            waitInfo.semaphoreCount = 1;
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;

            ErrorCheck(vkWaitSemaphores(vulkanManager->GetLogicalDevice(), &waitInfo, UINT64_MAX));
        }

        // Trigger graphics tasks
        {
            uint64_t signalValue = timelineSemaphores[currentFrameInFlight]->GetTimelineValue(TimelineStages::GRAPHICS_FINISHED);
            uint64_t waitValue = timelineSemaphores[currentFrameInFlight]->GetTimelineValue(TimelineStages::COMPUTE_FINISHED);
            pGraphicsTask->Update(frameIndex, currentFrameInFlight, timelineSemaphores[currentFrameInFlight]->GetSemaphore(), signalValue, waitValue);
        }

        // Get the active swapchain index
        uint32_t activeSwapchainImageindex = vulkanManager->GetActiveSwapchainImageIndex(swapchainImageAcquiredSemaphores[currentFrameInFlight]);

        // End the frame (increments index counters)
        vulkanManager->CopyAndPresent(pGraphicsTask->GetColorAttachments()[currentFrameInFlight], *timelineSemaphores[currentFrameInFlight], swapchainImageAcquiredSemaphores[currentFrameInFlight]);

        frameIndex++;
        timelineSemaphores[currentFrameInFlight]->IncrementFrameIndex();
    }

    if (vulkanManager->AreTheQueuesIdle())
    {
        vkDestroySemaphore(vulkanManager->GetLogicalDevice(), timelineSemaphore, nullptr);

        for(auto& sem : swapchainImageAcquiredSemaphores)
            vkDestroySemaphore(vulkanManager->GetLogicalDevice(), sem, nullptr);

        for (auto& sem : timelineSemaphores)
            sem.reset();
        timelineSemaphores.clear();

        pGraphicsTask.reset();
        pGraphicsTask = nullptr;
    }

    vulkanManager->DeInit();
    windowManagerObj->DeInit();

    return 0;
}