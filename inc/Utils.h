#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <tuple>
#include <vector>
#include <string>

void ErrorCheck(VkResult result);

VkCommandBuffer AllocateCommandBuffer(
    const VkDevice& device,
    const VkCommandPool& commandPool
);

void FreeCommandBuffer(
    const VkDevice& device,
    const VkCommandPool& commandPool,
    VkCommandBuffer* commandBuffer
);

VkDeviceMemory AllocateHostCoherentMemory(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& device,
    const VkDeviceSize& bufferSize,
    const VkMemoryRequirements& memoryRequirements
);

// Load an image from a file, and copy it into a newly created buffer (backed with memory already):
// Returns a tuple with: <0> the buffer handle, <1> the memory handle, <2> width, <3> height
//std::tuple<VkBuffer, VkDeviceMemory, int, int> LoadImageIntoHostCoherentMemory(
//    const VkPhysicalDevice& physicalDevice,
//    const VkDevice& device,
//    const std::string& pathToImageFile
//);

// Free memory that has been allocated with AllocateHostCoherentMemoryForBuffer
void FreeMemory(
    const VkDevice& device,
    const VkDeviceMemory& memory
);

void DestroyBuffer(
    const VkDevice& device,
    const VkBuffer& buffer
);

void ChangeImageLayoutWithBarriers(
    const VkCommandBuffer& commandBuffer,
    const VkPipelineStageFlags& srcPipelineStage, const VkPipelineStageFlags& dstPipelineStage,
    const VkAccessFlags& srcAccessMask, const VkAccessFlags& dstAccessMask,
    const VkImage& image,
    const VkImageLayout& oldLayout, const VkImageLayout& newLayout
);

void CopyBufferToImage(
    const VkCommandBuffer& commandBuffer,
    const VkBuffer& buffer,
    const VkImage& image, const uint32_t width, const uint32_t height
);

std::tuple<VkImage, VkDeviceMemory> CreateImage(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const uint32_t width, const uint32_t height,
    const VkFormat& format, const VkImageUsageFlags& usageFlags
);

// Destroy an image that has been created using the helper functions
void DestroyImage(
    const VkDevice& device,
    VkImage image
);

// Create an image view to an image
VkImageView CreateImageView(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const VkImage& image, const VkFormat& format, const VkImageAspectFlags& imageAspectFlags
);

// Destroy an image view that has been created using the helper functions
void DestroyImageView(
    const VkDevice& device,
    VkImageView imageView
);

std::tuple<VkShaderModule, VkPipelineShaderStageCreateInfo> CreateShaderModule(
    const VkDevice& device,
    const std::string& path,
    const VkShaderStageFlagBits& shaderStage
);

void DestroyShaderModule(
    const VkDevice& device,
    VkShaderModule shaderModule
);

std::tuple<VkBuffer, VkDeviceMemory> CreateBufferAndMemory(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const size_t bufferSize,
    const VkBufferUsageFlags& bufferUsageFlags
);

// Copy data of the gifen size into the buffer
void CopyDataIntoHostCoherentMemory(
    const VkDevice& device,
    const size_t& dataSize,
    const void* data,
    VkDeviceMemory& memory
);

void ChangeImageLayout(const VkDevice& device, std::vector<VkImage>& imageList, const VkQueue& queue,
    uint32_t queueFamilyIndex, VkImageLayout oldLayout, VkImageLayout newLayout);


enum TimelineStages
{
    UNINITIALIZED = 0,
    COMPUTE_FINISHED = 1,
    GRAPHICS_FINISHED = 2,
    SAFE_TO_PRESENT = 3,
    NUM_STAGES = 4
};

class TimelineSemaphore
{
private:
    VkSemaphore m_semaphore;
    TimelineStages m_currentStage{ UNINITIALIZED };
    uint64_t m_frameIndex = 0;

public:
    const VkDevice& m_device;
    TimelineSemaphore() = delete;
    TimelineSemaphore(TimelineSemaphore const&) = delete;
    TimelineSemaphore& operator=(TimelineSemaphore const&) = delete;

    TimelineSemaphore(const VkDevice& device) : m_device(device)
    {
        VkSemaphoreTypeCreateInfoKHR typeCreateInfo{};
        typeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
        typeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
        typeCreateInfo.initialValue = 0;

        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = &typeCreateInfo;
        ErrorCheck(vkCreateSemaphore(device, &info, nullptr, &m_semaphore));
    }

    ~TimelineSemaphore()
    {
        vkDestroySemaphore(m_device, m_semaphore, nullptr);
    }

    void SetTimelineStage(const TimelineStages& stage)
    {
        m_currentStage = stage;
    }

    void IncrementFrameIndex()
    {
        m_frameIndex++;
    }

    const VkSemaphore& GetSemaphore() const
    {
        return m_semaphore;
    }

    uint64_t GetFrameIndex() const
    {
        return m_frameIndex;
    }

    uint64_t GetTimelineValue(const TimelineStages& stage)
    {
        return m_frameIndex * (NUM_STAGES - 1) + stage;
    }

};

