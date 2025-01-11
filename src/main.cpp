#include "VulkanManager.h"
#include "TextureCreatorComputeTask.h"
#include "QuadRenderTask.h"
#include <optional>



class SharedResources
{
private:
    VkDescriptorSetLayout m_storageLayout;
    VkDescriptorSetLayout m_sampledLayout;
    std::vector<VkDescriptorSet> m_storageDescriptorSets;
    std::vector<VkDescriptorSet> m_sampledDescriptorSets;
    VkDescriptorPool m_descriptorPool;

    VkSampler m_immutableSampler;
    std::vector<VkImage> m_images; std::vector<VkDeviceMemory> m_imageMemories;
    std::vector<VkImageView> m_imageViews;

    const uint32_t& m_imageWidth;
    const uint32_t& m_imageHeight;

    const VkDevice& m_device;
    uint32_t m_maxFrameInFlight;

public:
    SharedResources() = delete;
    SharedResources(const uint32_t& maxFrameInFlights, const VkDevice& device,
        const VkPhysicalDevice& physicalDevice, const uint32_t& imageWidth, const uint32_t& imageHeight,
        uint32_t queueFamilyIndex) : m_device(device), m_maxFrameInFlight(maxFrameInFlights), m_imageWidth(imageWidth), m_imageHeight(imageHeight)
    {
        m_storageDescriptorSets.resize(maxFrameInFlights);
        m_sampledDescriptorSets.resize(maxFrameInFlights);
        m_images.resize(maxFrameInFlights);
        m_imageViews.resize(maxFrameInFlights);
        m_imageMemories.resize(maxFrameInFlights);

        // Descriptor pool
        {
            VkDescriptorPoolSize pool_sizes[2] =
            {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxFrameInFlights},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxFrameInFlights},
            };

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.flags = 0;
            poolInfo.maxSets = 2 * maxFrameInFlights;
            poolInfo.poolSizeCount = 2;
            poolInfo.pPoolSizes = pool_sizes;
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

            ErrorCheck(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool));
        }

        // Sampler
        {
            VkSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            
            ErrorCheck(vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_immutableSampler));
        }

        // Images and image views
        {
            for (int i = 0; i < maxFrameInFlights; ++i)
            {
                auto [image, memory] = CreateImage(device, physicalDevice, m_imageWidth, m_imageHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
                m_images[i] = std::move(image);
                m_imageMemories[i] = std::move(memory);

                VkImageViewCreateInfo createInfo{};
                createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
                createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
                createInfo.image = m_images[i];
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.layerCount = 1;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

                ErrorCheck(vkCreateImageView(device, &createInfo, nullptr, &m_imageViews[i]));
            }
        }

        // Descriptor layouts
        {
            VkDescriptorSetLayoutBinding storageBinding{};
            storageBinding.binding = 0;
            storageBinding.descriptorCount = 1;
            storageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            storageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding sampledBinding{};
            sampledBinding.binding = 0;
            sampledBinding.descriptorCount = 1;
            sampledBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sampledBinding.pImmutableSamplers = &m_immutableSampler;
            sampledBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo storageSetLayoutInfo{};
            storageSetLayoutInfo.bindingCount = 1;
            storageSetLayoutInfo.pBindings = &storageBinding;
            storageSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

            VkDescriptorSetLayoutCreateInfo sampledSetLayoutInfo{};
            sampledSetLayoutInfo.bindingCount = 1;
            sampledSetLayoutInfo.pBindings = &sampledBinding;
            sampledSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

            ErrorCheck(vkCreateDescriptorSetLayout(device, &storageSetLayoutInfo, nullptr, &m_storageLayout));
            ErrorCheck(vkCreateDescriptorSetLayout(device, &sampledSetLayoutInfo, nullptr, &m_sampledLayout));
        }

        // Descriptor sets
        {
            VkDescriptorSetAllocateInfo storageAllocInfo{};
            storageAllocInfo.descriptorPool = m_descriptorPool;
            storageAllocInfo.descriptorSetCount = 1;
            storageAllocInfo.pSetLayouts = &m_storageLayout;
            storageAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

            VkDescriptorSetAllocateInfo sampledAllocInfo{};
            sampledAllocInfo.descriptorPool = m_descriptorPool;
            sampledAllocInfo.descriptorSetCount = 1;
            sampledAllocInfo.pSetLayouts = &m_sampledLayout;
            sampledAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

            for (int i = 0; i < maxFrameInFlights; ++i)
            {
                ErrorCheck(vkAllocateDescriptorSets(device, &storageAllocInfo, &m_storageDescriptorSets[i]));
                ErrorCheck(vkAllocateDescriptorSets(device, &sampledAllocInfo, &m_sampledDescriptorSets[i]));

                VkDescriptorImageInfo storageImageInfo{VK_NULL_HANDLE, m_imageViews[i], VK_IMAGE_LAYOUT_GENERAL };
                VkDescriptorImageInfo sampledImageInfo{m_immutableSampler, m_imageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

                const VkWriteDescriptorSet writes[2] =
                {
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_storageDescriptorSets[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageInfo, nullptr, nullptr},
                    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_sampledDescriptorSets[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &sampledImageInfo, nullptr, nullptr},
                };

                vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
            }
        }
    }

    ~SharedResources()
    {
        vkDestroyDescriptorSetLayout(m_device, m_storageLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_sampledLayout, nullptr);
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

        for (uint32_t i = 0; i < m_maxFrameInFlight; i++)
        {
            vkDestroyImageView(m_device, m_imageViews[i], nullptr);
            vkFreeMemory(m_device, m_imageMemories[i], nullptr);
            vkDestroyImage(m_device, m_images[i], nullptr);
        }

        vkDestroySampler(m_device, m_immutableSampler, nullptr);
    }

    const std::vector<VkDescriptorSet>& GetStorageDescSet() const
    {
        return m_storageDescriptorSets;
    }

    const std::vector<VkDescriptorSet>& GetSampledDescSet() const
    {
        return m_sampledDescriptorSets;
    }

    const VkDescriptorSetLayout& GetStorageImageDescSetLayout() const
    {
        return m_storageLayout;
    }

    const VkDescriptorSetLayout& GetSampledImageDescSetLayout() const
    {
        return m_sampledLayout;
    }

    const std::vector<VkImage>& GetImages() const
    {
        return m_images;
    }
};

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
    std::unique_ptr<SharedResources> pSharedResources = std::make_unique<SharedResources>(maxFramesInFlight, vulkanManager->GetLogicalDevice(), vulkanManager->GetPhysicalDevice(),
        imageWidth, imageHeight, vulkanManager->GetQueueFamilyIndex());
    std::unique_ptr<TextureCreatorComputeTask> pComputeTask = std::make_unique<TextureCreatorComputeTask>( vulkanManager->GetQueueFamilyIndex(),
        vulkanManager->GetLogicalDevice(), vulkanManager->GetComputeQueue(), pSharedResources->GetStorageImageDescSetLayout(), pSharedResources->GetStorageDescSet(), pSharedResources->GetImages(),
        imageWidth, imageHeight, vulkanManager->GetMaxFramesInFlight());
    std::unique_ptr<QuadRenderTask> pGraphicsTask = std::make_unique<QuadRenderTask>(
        vulkanManager->GetLogicalDevice(), vulkanManager->GetPhysicalDevice(), vulkanManager->GetGraphicsQueue(),
        vulkanManager->GetQueueFamilyIndex(), pSharedResources->GetSampledImageDescSetLayout(), vulkanManager->GetMaxFramesInFlight(),
        screenWidth, screenHeight, pSharedResources->GetSampledDescSet(), pSharedResources->GetImages());

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

            //uint64_t currentVal = 0;
            //vkGetSemaphoreCounterValue(vulkanManager->GetLogicalDevice(), timelineSemaphores[currentFrameInFlight]->GetSemaphore(), &currentVal);

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

        pComputeTask.reset();
        pComputeTask = nullptr;

        pSharedResources.reset();
        pSharedResources = nullptr;
    }

    vulkanManager->DeInit();
    windowManagerObj->DeInit();

    return 0;
}