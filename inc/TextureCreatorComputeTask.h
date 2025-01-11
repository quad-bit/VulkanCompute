#pragma once
#include "Utils.h"
#include <atomic>

class TextureCreatorComputeTask
{
private:
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkPipelineLayout m_pipelineLayout;
    VkShaderModule m_shaderModule;
    VkPipeline m_pipeline;
    const VkQueue& m_computeQueue;
    const VkDevice& m_device;
    const std::vector<VkDescriptorSet>& m_storageDescriptorSets;
    const std::vector<VkImage>& m_images;

    const uint32_t& m_imageWidth;
    const uint32_t& m_imageHeight;
    const uint32_t& m_maxFrameInflight;

public:

    TextureCreatorComputeTask(uint32_t queueFamilyIndex, const VkDevice& device, const VkQueue& computeQueue,
        const VkDescriptorSetLayout& storageLayout, const std::vector<VkDescriptorSet>& storageDescriptorSets,
        const std::vector<VkImage>& images, const uint32_t& imageWidth, const uint32_t& imageHeight, const uint32_t& maxFrameInFlights);
    ~TextureCreatorComputeTask();

    std::atomic_bool m_alive;
    //Create compute specific resources
    void Init();
    void Update(const uint32_t& frameIndex, const uint32_t& frameInFlight,
        const VkSemaphore& semaphore, uint64_t signalValue);
    void BuildCommandBuffers(const uint32_t& frameInFlight);
};