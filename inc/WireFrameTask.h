#pragma once
#include "Utils.h"
#include "CameraUpdated.h"
#include <memory>

namespace Common
{
    class WireFrameTask
    {
    private:
        const VkQueue& m_graphicsQueue;
        const VkDevice& m_device;
        const VkPhysicalDevice& m_physicalDevice;

        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;
        VkPipelineLayout m_pipelineLayout;
        VkShaderModule m_shaderModule;

        VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;

        std::vector<VkImage> m_colorAttachments;
        std::vector<VkDeviceMemory> m_colorAttachmentMemory;
        std::vector<VkImageView> m_colorAttachmentViews;

        VkImage m_depthAttachment;
        VkDeviceMemory m_depthAttachmentMemory;
        VkImageView m_depthAttachmentViews;

        VkSampler m_immutableSampler;
        VkDescriptorSetLayout m_viewSetLayout;
        std::vector<VkDescriptorSet> m_viewSet;
        VkBuffer m_cameraUniforms;
        VkDeviceMemory m_cameraUniformMemory;
        std::unique_ptr<Common::Camera> m_pCamera;
        VkDescriptorPool m_descriptorPool;

        VkRenderingAttachmentInfo depthInfo{};
        std::vector<VkRenderingAttachmentInfo> colorInfoList;
        std::vector<VkRenderingInfo> m_renderInfoList;

        VkPipeline m_pipeline;
        uint32_t m_screenWidth;
        uint32_t m_screenHeight;
        uint32_t m_maxFrameInFlights;
        uint32_t m_numIndicies;

        void BuildCommandBuffers(const uint32_t& frameInFlight, bool changeImageLayout);

    public:

        WireFrameTask(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkQueue& graphicsQueue,
            uint32_t queueFamilyIndex, uint32_t maxFrameInFlight, uint32_t screenWidth, uint32_t screenHeight, const VkFormat& depthFormat);
        ~WireFrameTask();

        //Create quad draw specific resources
        void Update(const uint64_t& frameIndex, const uint32_t& frameInFlight,
            const VkSemaphore& timelineSem, uint64_t signalValue, uint64_t waitValue);
        const std::vector<VkImage>& GetColorAttachments() const;
        const std::vector<VkImageView>& GetColorAttachmentViews() const;
    };
}