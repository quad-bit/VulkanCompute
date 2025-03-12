#pragma once
#include "Utils.h"
#include "Common.h"

constexpr auto EFFECTIVE_NEIGHBOURS = 12;

struct Point
{
    glm::vec4 m_currentPosition;
    glm::vec4 m_previousPosition;
    glm::vec4 m_velocity;
    glm::vec4 m_randomNormal;
    int m_neighbourIndicies[EFFECTIVE_NEIGHBOURS]{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};


    Point(const bool& pinned)// : m_isPinned(pinned)
    {
        m_currentPosition = glm::vec4(0.0f);
        m_previousPosition = glm::vec4(0.0f);
        m_velocity = glm::vec4(0.0f);
        m_randomNormal = glm::vec4(0.0f);
    }
};

class ComputeTask
{
private:
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkPipelineLayout m_pipelineLayout;
    VkShaderModule m_shaderModule;
    VkPipeline m_pipeline;
    const VkQueue& m_computeQueue;
    const VkDevice& m_device;

    VkDescriptorPool m_descriptorPool;
    // binding 0 with storage buffer for cloth point struct
    // binding 1 with storage buffer for position
    std::vector<VkDescriptorSet> m_storageDescriptorSets;
    VkDescriptorSetLayout m_setLayout;
    const std::vector<VkBuffer>& m_positionBuffer;
    std::vector<VkBuffer> m_pointBuffer;
    std::vector<VkDeviceMemory> m_pointBufferMemory;
    uint32_t m_maxFrameInflight;

    struct PushConst
    {
        int m_changeScene = 0;
        float m_windIntensity = 0.0f;
    }pushConst;


public:

    ComputeTask(uint32_t queueFamilyIndex, const VkDevice& device, const VkPhysicalDevice& physicalDevice,
        const VkQueue& computeQueue, const std::vector<VkBuffer>& positionBuffer, const uint32_t& maxFrameInFlights);
    ~ComputeTask();

    //Create compute specific resources
    void Init();
    void Update(const uint64_t& frameIndex, const uint32_t& frameInFlight,
        const VkSemaphore& semaphore, uint64_t signalValue);
    void BuildCommandBuffers(const uint32_t& frameInFlight);
};