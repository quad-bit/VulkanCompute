#pragma once
#include "Utils.h"

class GraphicsTask
{
private:
    const VkQueue& m_graphicsQueue;
    const VkDevice& m_device;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkPipelineLayout m_pipelineLayout;
    VkShaderModule m_shaderModule;

    VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;

    VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;

    std::vector<VkImage> m_colorAttachments;
    std::vector<VkDeviceMemory> m_colorAttachmentMemory;
    std::vector<VkImageView> m_colorAttachmentViews;
    
    VkPipeline m_pipeline;
    uint32_t m_screenWidth;
    uint32_t m_screenHeight;
    uint32_t m_maxFrameInFlights;

    void BuildCommandBuffers(const uint32_t& frameInFlight, bool changeImageLayout);

public:

    GraphicsTask(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkQueue& graphicsQueue,
        uint32_t queueFamilyIndex, uint32_t maxFrameInFlight, uint32_t screenWidth, uint32_t screenHeight);
    ~GraphicsTask();

    //Create quad draw specific resources
    void Init();
    void Update(const uint32_t& frameIndex, const uint32_t& frameInFlight,
        const VkSemaphore& timelineSem, uint64_t signalValue, uint64_t waitValue);
    const std::vector<VkImage>& GetColorAttachments();
};