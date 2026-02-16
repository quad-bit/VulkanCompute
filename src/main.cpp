#include "VulkanManager.h"
#include "GraphicsTask.h"
#include "ComputeTask.h"
#include <optional>
#include "Timer.h"

struct SharedResources
{
    std::vector<VkBuffer> m_buffers;
    std::vector<VkDeviceMemory> m_bufferMemories;

    const VkDevice& m_device;
    SharedResources(const VkPhysicalDevice& physicalDevice, const VkDevice& device,
        const uint32_t& maxFramesInFlight) : m_device(device)
    {
        m_buffers.resize(maxFramesInFlight);
        m_bufferMemories.resize(maxFramesInFlight);

        size_t structSize = sizeof(glm::vec4) * NUM_POINTS;
        for(uint32_t i = 0; i < maxFramesInFlight; i++)
            CreateBufferAndMemory(physicalDevice, device, m_buffers[i], m_bufferMemories[i], structSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    ~SharedResources()
    {
        for (auto mem : m_bufferMemories)
            FreeMemory(m_device, mem);

        for (auto& buf : m_buffers)
            vkDestroyBuffer(m_device, buf, nullptr);
    }
};

int main()
{
    constexpr uint32_t screenWidth = 1024;
    constexpr uint32_t screenHeight = 1024;

    //constexpr uint32_t imageWidth = 1024;
    //constexpr uint32_t imageHeight = 1024;

    Timer timer(60);

    std::unique_ptr<WindowManager> windowManagerObj = std::make_unique<WindowManager>(screenWidth, screenHeight);
    windowManagerObj->Init();

    std::unique_ptr<VulkanManager> vulkanManager = std::make_unique<VulkanManager>(screenWidth, screenHeight);
    vulkanManager->Init(windowManagerObj->glfwWindow);

    uint32_t maxFramesInFlight = vulkanManager->GetMaxFramesInFlight();

    std::unique_ptr<SharedResources> pSharedResources = std::make_unique<SharedResources>(
        vulkanManager->GetPhysicalDevice(), vulkanManager->GetLogicalDevice(), maxFramesInFlight);

    std::unique_ptr<GraphicsTask> pGraphicsTask = std::make_unique<GraphicsTask>(
        vulkanManager->GetLogicalDevice(), vulkanManager->GetPhysicalDevice(), vulkanManager->GetGraphicsQueue(),
        vulkanManager->GetQueueFamilyIndex(), vulkanManager->GetMaxFramesInFlight(), pSharedResources->m_buffers,
        screenWidth, screenHeight, vulkanManager->GetDepthFormat());

    std::unique_ptr<ComputeTask> pComputeTask = std::make_unique<ComputeTask>(vulkanManager->GetQueueFamilyIndex(),
        vulkanManager->GetLogicalDevice(), vulkanManager->GetPhysicalDevice(), vulkanManager->GetComputeQueue(), pSharedResources->m_buffers, maxFramesInFlight);

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

    uint64_t frameIndex = 0;

    //timer.Sleep(10);

    while (windowManagerObj->Update())
    {
        timer.StartFrame();

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

        // Trigger compute
        {
            uint64_t signalValue = timelineSemaphores[currentFrameInFlight]->GetTimelineValue(TimelineStages::COMPUTE_FINISHED);
            pComputeTask->Update(frameIndex, currentFrameInFlight, timelineSemaphores[currentFrameInFlight]->GetSemaphore(), signalValue);
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

        timer.EndFrame();

    }

    if (vulkanManager->AreTheQueuesIdle())
    {
        //vkDestroySemaphore(vulkanManager->GetLogicalDevice(), timelineSemaphore, nullptr);

        for(auto& sem : swapchainImageAcquiredSemaphores)
            vkDestroySemaphore(vulkanManager->GetLogicalDevice(), sem, nullptr);

        for (auto& sem : timelineSemaphores)
            sem.reset();
        timelineSemaphores.clear();

        pSharedResources.reset();
        pSharedResources = nullptr;

        pGraphicsTask.reset();
        pGraphicsTask = nullptr;

        pComputeTask.reset();
        pComputeTask = nullptr;
    }

    vulkanManager->DeInit();
    windowManagerObj->DeInit();

    return 0;
}