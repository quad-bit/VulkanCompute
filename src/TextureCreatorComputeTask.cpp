#include "TextureCreatorComputeTask.h"
#include <math.h>

TextureCreatorComputeTask::TextureCreatorComputeTask(uint32_t queueFamilyIndex,
    const VkDevice& device, const VkQueue& computeQueue, const VkDescriptorSetLayout& storageLayout,
    const std::vector<VkDescriptorSet>& storageDescriptorSets, const std::vector<VkImage>& images,
    const uint32_t& imageWidth, const uint32_t& imageHeight, const uint32_t& maxFrameInFlights) : m_computeQueue(computeQueue),
    m_storageDescriptorSets(storageDescriptorSets), m_images(images), m_imageWidth(imageWidth), m_imageHeight(imageHeight),
    m_device(device), m_maxFrameInflight(maxFrameInFlights)
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    ErrorCheck(vkCreateCommandPool(device, &createInfo, nullptr, &m_commandPool));

    m_commandBuffers.resize(maxFrameInFlights);
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.commandBufferCount = maxFrameInFlights;
    alloc_info.commandPool = m_commandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    ErrorCheck(vkAllocateCommandBuffers(device, &alloc_info, &m_commandBuffers[0]));

    std::string spvPath = std::string{ SPV_PATH } +"Mandlebrot.spv";

    auto[shaderModule, shaderStage] = CreateShaderModule(device, spvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);

    m_shaderModule = shaderModule;

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(float);
    range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.pPushConstantRanges = &range;
    layoutCreateInfo.pSetLayouts = &storageLayout;
    layoutCreateInfo.pushConstantRangeCount = 1;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    ErrorCheck(vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &m_pipelineLayout));

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;
    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.stage = shaderStage;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

    ErrorCheck(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline));
}

TextureCreatorComputeTask::~TextureCreatorComputeTask()
{
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
}

void TextureCreatorComputeTask::Update(const uint32_t & frameIndex, const uint32_t & frameInFlight,
    const VkSemaphore& semaphore, uint64_t signalValue)
{
    BuildCommandBuffers(frameInFlight);

    VkSemaphoreSubmitInfo signalInfo
    {
        VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr, semaphore, signalValue, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0
    };

    VkCommandBufferSubmitInfo bufInfo{};
    bufInfo.commandBuffer = m_commandBuffers[frameInFlight];
    bufInfo.deviceMask = 0;
    bufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

    VkSubmitInfo2 submitInfo{};
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &bufInfo;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

    // If the threads are being killed, we need to skip the queue submission to allow the program to exit gracefully
    if (m_alive)
    {
        ErrorCheck(vkQueueSubmit2(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }
}

void TextureCreatorComputeTask::BuildCommandBuffers(const uint32_t& frameInFlight)
{
    float elapsed = 0.0f;
    static float counter = 0.0f;
    {
        elapsed = cosf(counter);
        counter += 0.0005f;

        if (counter > 2.0f * 3.14f)
            counter = 0.0f;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ErrorCheck(vkResetCommandBuffer(m_commandBuffers[frameInFlight], 0));
    ErrorCheck(vkBeginCommandBuffer(m_commandBuffers[frameInFlight], &beginInfo));

    VkImageMemoryBarrier image_barrier{};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    image_barrier.image = m_images[frameInFlight];
    image_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

    // The semaphore takes care of srcStageMask.
    vkCmdPipelineBarrier(m_commandBuffers[frameInFlight], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    vkCmdBindDescriptorSets(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_storageDescriptorSets[frameInFlight], 0, nullptr);
    vkCmdBindPipeline(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    vkCmdPushConstants(m_commandBuffers[frameInFlight], m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(elapsed), &elapsed);

    auto x = (uint32_t)ceil(m_imageWidth / (float)32);
    auto y = (uint32_t)ceil(m_imageHeight / (float)32);
    vkCmdDispatch(m_commandBuffers[frameInFlight], x, y, 1);

    image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    image_barrier.dstAccessMask = 0;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // The semaphore takes care of dstStageMask.
    vkCmdPipelineBarrier(m_commandBuffers[frameInFlight], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    ErrorCheck(vkEndCommandBuffer(m_commandBuffers[frameInFlight]));
}
