#pragma once
#include "Utils.h"
#include "Camera.h"
#include "Transform.h"
#include <memory>

class GraphicsTask
{
private:
    const VkQueue& m_graphicsQueue;
    const VkDevice& m_device;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkPipelineLayout m_pipelineLayout;
    VkShaderModule m_shaderModule;

    const std::vector<VkBuffer>& m_vertexBuffers;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_uvBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uvBufferMemory = VK_NULL_HANDLE;

    VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;

    std::vector<VkImage> m_colorAttachments;
    std::vector<VkDeviceMemory> m_colorAttachmentMemory;
    std::vector<VkImageView> m_colorAttachmentViews;

    VkImage m_depthAttachment;
    VkDeviceMemory m_depthAttachmentMemory;
    VkImageView m_depthAttachmentViews;

    VkSampler m_immutableSampler;
    VkDescriptorSetLayout m_viewSetLayout, m_transformSetLayout, m_samplerLayout;
    std::vector<VkDescriptorSet> m_viewSet, m_transformSet, m_samplerSet;
    std::vector<VkBuffer> m_cameraUniforms, m_transformUniforms;
    std::vector<VkDeviceMemory> m_cameraUniformMemory, m_transformUniformMemory;
    std::vector<VkImage> m_clothTexture;
    std::vector<VkDeviceMemory> m_clothTextureMemory;
    std::vector<VkImageView> m_clothTextureViews;


    VkDescriptorPool m_descriptorPool;

    VkRenderingAttachmentInfo depthInfo{};
    std::vector<VkRenderingAttachmentInfo> colorInfoList;
    std::vector<VkRenderingInfo> m_renderInfoList;

    VkPipeline m_pipeline;
    uint32_t m_screenWidth;
    uint32_t m_screenHeight;
    uint32_t m_maxFrameInFlights;
    uint32_t m_numIndicies;

    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Transform> m_pTransform;

    void BuildCommandBuffers(const uint32_t& frameInFlight, bool changeImageLayout);

public:

    GraphicsTask(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkQueue& graphicsQueue,
        uint32_t queueFamilyIndex, uint32_t maxFrameInFlight, const std::vector<VkBuffer>& sharedBuffers, uint32_t screenWidth, uint32_t screenHeight, const VkFormat& depthFormat);
    ~GraphicsTask();

    //Create quad draw specific resources
    void Init();
    void Update(const uint64_t& frameIndex, const uint32_t& frameInFlight,
        const VkSemaphore& timelineSem, uint64_t signalValue, uint64_t waitValue);
    const std::vector<VkImage>& GetColorAttachments();
};