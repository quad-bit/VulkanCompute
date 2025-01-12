#include "..\inc\GraphicsTask.h"
#include <array>


GraphicsTask::GraphicsTask(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkQueue & graphicsQueue, uint32_t queueFamilyIndex,
    uint32_t maxFrameInFlight, uint32_t screenWidth, uint32_t screenHeight):
    m_graphicsQueue(graphicsQueue), m_device(device), m_screenWidth(screenWidth), m_screenHeight(screenHeight),
    m_maxFrameInFlights(maxFrameInFlight)
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    ErrorCheck(vkCreateCommandPool(device, &createInfo, nullptr, &m_commandPool));

    m_commandBuffers.resize(maxFrameInFlight);
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.commandBufferCount = maxFrameInFlight;
    alloc_info.commandPool = m_commandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    ErrorCheck(vkAllocateCommandBuffers(device, &alloc_info, &m_commandBuffers[0]));

    /*VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pipelineLayoutCreateInfo.pSetLayouts = &sampledLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    ErrorCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));*/

    // Render pass attachments
    m_colorAttachmentViews.resize(maxFrameInFlight);
    for (int i = 0; i < maxFrameInFlight; ++i)
    {
        auto[image, memory] = CreateImage(device, physicalDevice, screenWidth, screenHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT| VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        m_colorAttachments.push_back(std::move(image));
        m_colorAttachmentMemory.push_back(std::move(memory));

        VkImageViewCreateInfo createInfo{};
        createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
        createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        createInfo.image = m_colorAttachments[i];
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.layerCount = 1;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        ErrorCheck(vkCreateImageView(device, &createInfo, nullptr, &m_colorAttachmentViews[i]));
    }

    ChangeImageLayout(m_device, m_colorAttachments, m_graphicsQueue, queueFamilyIndex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //Render pass
    VkClearValue clearValues{ VkClearColorValue{0.7f, 0.2f, 0.5f, 1.0f} };

    std::vector<VkRenderingAttachmentInfo> colorInfoList(maxFrameInFlight);
    for (uint32_t i = 0; i < maxFrameInFlight; i++)
    {
        colorInfoList[i].clearValue = clearValues;
        colorInfoList[i].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorInfoList[i].imageView = m_colorAttachmentViews[i];
        colorInfoList[i].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorInfoList[i].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    }

    std::vector<VkRenderingInfo> renderInfoList;
    for (uint32_t i = 0; i < maxFrameInFlight; i++)
    {
        VkRenderingInfo info{};
        info.colorAttachmentCount = (1);
        info.layerCount = (1);
        info.pColorAttachments = &colorInfoList[i];
        info.renderArea = VkRect2D{ {0, 0}, {(uint32_t)screenWidth, (uint32_t)screenHeight} };
        renderInfoList.push_back(std::move(info));
    }

    VkFormat attachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &attachmentFormat;
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    // Create pipeline
    /*std::string vertSpvPath = std::string{ SPV_PATH } +"FullScreenQuadVert.spv";
    std::string fragSpvPath = std::string{ SPV_PATH } +"FullScreenQuadFrag.spv";

    auto[vertShaderModule, vertShaderStage] = CreateShaderModule(device, vertSpvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
    auto[fragShaderModule, fragShaderStage] = CreateShaderModule(device, fragSpvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

    m_vertexShaderModule = vertShaderModule;
    m_fragmentShaderModule = fragShaderModule;

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkPipelineInputAssemblyStateCreateInfo  pipelineInputAssemblyStateCreateInfo = {};
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
    pipelineColorBlendAttachmentState.colorWriteMask = 0xF;
    pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
    pipelineDepthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
    pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_GREATER;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2] = {};
    pipelineShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[0].module = m_vertexShaderModule;
    pipelineShaderStageCreateInfos[0].pName = "main";
    pipelineShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

    pipelineShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[1].module = m_fragmentShaderModule;
    pipelineShaderStageCreateInfos[1].pName = "main";
    pipelineShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.layout = m_pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

    ErrorCheck( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo,
        nullptr, &m_pipeline));*/
}

GraphicsTask::~GraphicsTask()
{
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    //vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    //vkDestroyShaderModule(m_device, m_vertexShaderModule, nullptr);
    //vkDestroyShaderModule(m_device, m_fragmentShaderModule, nullptr);
    //vkDestroyPipeline(m_device, m_pipeline, nullptr);

    for (uint32_t i = 0; i < m_colorAttachments.size(); i++)
    {
        vkDestroyImageView(m_device, m_colorAttachmentViews[i], nullptr);
        vkFreeMemory(m_device, m_colorAttachmentMemory[i], nullptr);
        vkDestroyImage(m_device, m_colorAttachments[i], nullptr);
    }
}

void GraphicsTask::BuildCommandBuffers(const uint32_t & frameInFlight, bool changeImageLayout)
{
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight), 0.0f, 1.0f };
    VkRect2D   scissor = { {0, 0}, {m_screenWidth, m_screenHeight} };

    // Simple fix for 1:1 pixel aspect ratio.
    if (viewport.width > viewport.height)
    {
        viewport.x += 0.5f * (viewport.width - viewport.height);
        viewport.width = viewport.height;
    }
    else if (viewport.height > viewport.width)
    {
        viewport.y += 0.5f * (viewport.height - viewport.width);
        viewport.height = viewport.width;
    }

    ErrorCheck(vkResetCommandBuffer(m_commandBuffers[frameInFlight], 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ErrorCheck(vkBeginCommandBuffer(m_commandBuffers[frameInFlight], &beginInfo));

    if (changeImageLayout)
    {
        VkImageMemoryBarrier image_barrier2{};
        image_barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_barrier2.dstAccessMask = 0;
        image_barrier2.image = m_colorAttachments[frameInFlight];
        image_barrier2.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        image_barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_barrier2.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // The semaphore takes care of srcStageMask.
        vkCmdPipelineBarrier(m_commandBuffers[frameInFlight],
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &image_barrier2);
    }

    VkClearValue clears = {};
    clears.color.float32[0] = 0.033f;
    clears.color.float32[1] = 0.073f;
    clears.color.float32[2] = 0.133f;

    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.clearValue = clears;
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.imageView = m_colorAttachmentViews[frameInFlight];
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    VkRenderingInfo renderingInfo{};
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.layerCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.renderArea = { {0,0}, {m_screenWidth, m_screenHeight} };
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    vkCmdBeginRendering(m_commandBuffers[frameInFlight], &renderingInfo);

    //vkCmdBindPipeline(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdSetViewport(m_commandBuffers[frameInFlight], 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffers[frameInFlight], 0, 1, &scissor);

    //vkCmdBindDescriptorSets(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_sampledDescriptorSets[frameInFlight], 0, nullptr);
    //vkCmdDraw(m_commandBuffers[frameInFlight], 3, 1, 0, 0);

    vkCmdEndRendering(m_commandBuffers[frameInFlight]);

    ErrorCheck(vkEndCommandBuffer(m_commandBuffers[frameInFlight]));
}

void GraphicsTask::Update(const uint32_t & frameIndex, const uint32_t & frameInFlight,
    const VkSemaphore& timelineSem, uint64_t signalValue, uint64_t waitValue)
{
    if(frameIndex > 1)
        BuildCommandBuffers(frameInFlight, true);
    else
        BuildCommandBuffers(frameInFlight, false);

    VkSemaphoreSubmitInfo waitInfo
    { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr, timelineSem, waitValue, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0 };

    VkSemaphoreSubmitInfo signalInfo
    { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, nullptr, timelineSem, signalValue, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 };

    VkCommandBufferSubmitInfo bufInfo{};
    bufInfo.commandBuffer = m_commandBuffers[frameInFlight];
    bufInfo.deviceMask = 0;
    bufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

    VkSubmitInfo2 submitInfo{};
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &bufInfo;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    //submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    //submitInfo.waitSemaphoreInfoCount = 1;

    // If the threads are being killed, we need to skip the queue submission to allow the program to exit gracefully
    //if (m_alive)
    {
        ErrorCheck(vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }
}

const std::vector<VkImage>& GraphicsTask::GetColorAttachments()
{
    return m_colorAttachments;
}
