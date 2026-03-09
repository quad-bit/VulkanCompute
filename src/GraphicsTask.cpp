#include "..\inc\GraphicsTask.h"
#include "..\inc\Common.h"
#include <array>
#include <assert.h>

namespace
{
    void SubmitBufferToImageCopy(const VkDevice& device, uint32_t queueFamilyIndex, const VkQueue& queue, const VkBuffer& buffer, const VkImage& image, uint32_t width, uint32_t height)
    {
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo info{};
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        info.pNext = nullptr;
        info.queueFamilyIndex = queueFamilyIndex;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        ErrorCheck(vkCreateCommandPool(device, &info, nullptr, &pool));

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.pNext = nullptr;
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        ErrorCheck(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        beginInfo.pNext = nullptr;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        
        {
            VkImageMemoryBarrier image_barrier{};
            image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barrier.srcAccessMask = 0;
            image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barrier.image = image;
            image_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

            CopyBufferToImage(cmdBuffer, buffer, image, width, height);

            image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barrier.dstAccessMask = 0;
            image_barrier.image = image;
            image_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
        }

        vkEndCommandBuffer(cmdBuffer);

        VkFence fence = VK_NULL_HANDLE;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.pNext = nullptr;
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        ErrorCheck(vkCreateFence(device, &fenceInfo, nullptr, &fence));

        VkSubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        submitInfo.pNext = nullptr;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ErrorCheck(vkQueueSubmit(queue, 1, &submitInfo, fence));

        ErrorCheck(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(device, fence, nullptr);
        vkDestroyCommandPool(device, pool, nullptr);
    }
}


GraphicsTask::GraphicsTask(const VkDevice& device, const VkPhysicalDevice& physicalDevice,
    const VkQueue & graphicsQueue, uint32_t queueFamilyIndex, uint32_t maxFrameInFlight, const std::vector<VkBuffer>& sharedBuffers,
    uint32_t screenWidth, uint32_t screenHeight, const VkFormat& depthFormat) :
    m_graphicsQueue(graphicsQueue), m_device(device), m_screenWidth(screenWidth), m_screenHeight(screenHeight),
    m_maxFrameInFlights(maxFrameInFlight), m_vertexBuffers(sharedBuffers)
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

    {
        VkDescriptorSetLayoutBinding bindings[3]
        {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &m_immutableSampler}
        };

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.bindingCount = 1;
        createInfo.pBindings = &bindings[0];
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ErrorCheck(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_viewSetLayout));

        createInfo.pBindings = &bindings[1];
        ErrorCheck(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_transformSetLayout));

        createInfo.pBindings = &bindings[2];
        ErrorCheck(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_samplerLayout));

        VkDescriptorPoolSize pool_sizes[1] =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * maxFrameInFlight},
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = 0;
        poolInfo.maxSets = 3 * maxFrameInFlight;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = pool_sizes;
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ErrorCheck(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool));
    }

    VkDescriptorSetLayout layouts[3]{ m_viewSetLayout, m_transformSetLayout, m_samplerLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pipelineLayoutCreateInfo.pSetLayouts = layouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 3;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    ErrorCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

    // Render pass attachments
    m_colorAttachmentViews.resize(maxFrameInFlight);
    for (int i = 0; i < maxFrameInFlight; ++i)
    {
        auto[image, memory] = CreateImage(device, physicalDevice, screenWidth, screenHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
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

    //Depth
    {
        std::tie(m_depthAttachment, m_depthAttachmentMemory) = CreateImage(device, physicalDevice, screenWidth, screenHeight, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        VkImageViewCreateInfo createInfo{};
        createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
        createInfo.format = depthFormat;
        createInfo.image = m_depthAttachment;
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.layerCount = 1;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        ErrorCheck(vkCreateImageView(device, &createInfo, nullptr, &m_depthAttachmentViews));
    }

    ChangeImageLayout(m_device, m_colorAttachments, m_graphicsQueue, queueFamilyIndex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //Render pass
    VkClearValue clearValues{ VkClearColorValue{0.6033f, 0.6073f, 0.6133f, 1.0f}};
    colorInfoList.resize(maxFrameInFlight);
    for (uint32_t i = 0; i < maxFrameInFlight; i++)
    {
        colorInfoList[i].clearValue = clearValues;
        colorInfoList[i].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorInfoList[i].imageView = m_colorAttachmentViews[i];
        colorInfoList[i].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorInfoList[i].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        colorInfoList[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    }

    VkClearValue clearValuesDepth;
    clearValuesDepth.depthStencil = { VkClearDepthStencilValue{ 1.0f, 0u } };

    depthInfo.clearValue = clearValuesDepth;
    depthInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthInfo.imageView = m_depthAttachmentViews;
    depthInfo.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthInfo.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    for (uint32_t i = 0; i < maxFrameInFlight; i++)
    {
        VkRenderingInfo info{};
        info.colorAttachmentCount = (1);
        info.layerCount = (1);
        info.pColorAttachments = &colorInfoList[i];
        info.pDepthAttachment = &depthInfo;
        info.renderArea = VkRect2D{ {0, 0}, {(uint32_t)screenWidth, (uint32_t)screenHeight} };
        info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        m_renderInfoList.push_back(std::move(info));
    }

    VkFormat attachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &attachmentFormat;
    pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    // Create pipeline
    std::string vertSpvPath = std::string{ SPV_PATH } +"UnlitTexturedVert.spv";
    std::string fragSpvPath = std::string{ SPV_PATH } +"UnlitTexturedFrag.spv";

    auto[vertShaderModule, vertShaderStage] = CreateShaderModule(device, vertSpvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
    auto[fragShaderModule, fragShaderStage] = CreateShaderModule(device, fragSpvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

    m_vertexShaderModule = vertShaderModule;
    m_fragmentShaderModule = fragShaderModule;

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Describe the vertex input, i.e. two vertex input attributes in our case:
    VkVertexInputBindingDescription vertexBindings[2]
    {
        {0, sizeof(glm::vec4), VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(glm::vec2), VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX}
    };
    VkVertexInputAttributeDescription attributeDescriptions[2]
    {
        {0, 0, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0},
        {1, 1, VkFormat::VK_FORMAT_R32G32_SFLOAT, 0}
    };
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindings;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 2;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
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
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    /*pipelineDepthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
    pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_GREATER;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
    */

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkDynamicState dynamicStates[] = 
    {
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
        nullptr, &m_pipeline));

    // texture uv buffer (vertex buffer)
    {
        CreateBufferAndMemory(physicalDevice, device, m_uvBuffer, m_uvBufferMemory, sizeof(glm::vec2) * NUM_POINTS,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        {
            // stack over flow happening, quick fix. TODO correct this.
            glm::vec2 * uvs = new glm::vec2[NUM_POINTS];
            uint32_t row = 0, col = 0;
            float x = 0.0f;
            float y = 0.0f;
            float xOffset = 1.0f / (float)NUM_COLS;
            float yOffset = 1.0f / (float)NUM_ROWS;

            glm::vec2 topLeft{ x, y };
            for (uint32_t i = 0; i < NUM_POINTS; i++)
            {
                uvs[i] = (topLeft + glm::vec2((float)col * xOffset, (float)(row) * yOffset));

                col++;
                if (col % (NUM_COLS) == 0)
                {
                    row++;
                    col = 0;
                }
            }

            void* pData;
            ErrorCheck(vkMapMemory(m_device, m_uvBufferMemory, 0, NUM_POINTS * sizeof(glm::vec2), 0, &pData));
            memcpy(pData, &uvs[0], NUM_POINTS * sizeof(glm::vec2));
            vkUnmapMemory(m_device, m_uvBufferMemory);

            delete[] uvs;
        }
    }

    // index buffer
    {
        constexpr uint32_t numCols = NUM_COLS;
        constexpr uint32_t numRows = NUM_ROWS;

        // 4 points will make 2 tris. in one row num of tris = 2 * (numCols - 1)
        constexpr uint32_t numIndicies = 2 * (numCols - 1) * (numRows - 1) * 3;
        m_numIndicies = numIndicies;

        CreateBufferAndMemory(physicalDevice, device, m_indexBuffer, m_indexBufferMemory, numIndicies * sizeof(uint32_t),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        uint32_t* indicies = new uint32_t[numIndicies];
        uint32_t indexCounter = 0;

        indicies[0] = 0;
        indicies[1] = 1;
        indicies[2] = 0 + numCols;
        indicies[3] = 0 + numCols;
        indicies[4] = 1;
        indicies[5] = 0 + numCols + 1;

        indexCounter = 6;

        for (uint32_t i = 1; i < numCols * numRows; i++)
        {
            if (i%(numCols) <= numCols - 2 && i < numCols * (numRows - 1))
            {
                indicies[indexCounter++] = i;
                indicies[indexCounter++] = i + 1;
                indicies[indexCounter++] = i + numCols;

                indicies[indexCounter++] = i + numCols;
                indicies[indexCounter++] = i + 1;
                indicies[indexCounter++] = i + numCols + 1;
            }

            if ((i + 1) % (numCols) == numCols - 1)
                i++;
            assert(indexCounter <= numIndicies);
        }

        void* pData;
        ErrorCheck(vkMapMemory(m_device, m_indexBufferMemory, 0, numIndicies * sizeof(uint32_t), 0, &pData));
        memcpy(pData, &indicies[0], numIndicies * sizeof(uint32_t));
        vkUnmapMemory(m_device, m_indexBufferMemory);

        delete[] indicies;
    }

    // Camera Uniform
    {
        Transform camTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
        m_pCamera = std::make_unique<Camera>(camTransform, screenWidth / (float)screenHeight);
        
        m_viewSet.resize(maxFrameInFlight);

        m_cameraUniforms.resize(maxFrameInFlight);
        m_cameraUniformMemory.resize(maxFrameInFlight);
        for (uint32_t i = 0; i < maxFrameInFlight; i++)
        {
            CreateBufferAndMemory(physicalDevice, device, m_cameraUniforms[i], m_cameraUniformMemory[i], sizeof(SceneUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            SceneUniform uniform{m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMat(), m_pCamera->GetPosition()};

            void* pData;
            ErrorCheck(vkMapMemory(m_device, m_cameraUniformMemory[i], 0, sizeof(SceneUniform), 0, &pData));
            memcpy(pData, &uniform, sizeof(SceneUniform));
            vkUnmapMemory(m_device, m_cameraUniformMemory[i]);

            VkDescriptorSetAllocateInfo setAllocInfo{};
            setAllocInfo.descriptorPool = m_descriptorPool;
            setAllocInfo.descriptorSetCount = 1;
            setAllocInfo.pSetLayouts = &m_viewSetLayout;
            setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

            ErrorCheck(vkAllocateDescriptorSets(device, &setAllocInfo, &m_viewSet[i]));
            VkDescriptorBufferInfo bufferInfo{ m_cameraUniforms[i], 0, VK_WHOLE_SIZE };
            const VkWriteDescriptorSet writes
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_viewSet[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bufferInfo, nullptr
            };

            vkUpdateDescriptorSets(device, 1, &writes, 0, nullptr);
        }
    }

    // transform Uniform
    {
        m_transformUniforms.resize(maxFrameInFlight);
        m_transformUniformMemory.resize(maxFrameInFlight);
        m_transformSet.resize(maxFrameInFlight);

        Transform transform(glm::vec3(0.0f, 0.0f, 25.0f), glm::vec3(0.0f, glm::radians(180.0f), 0.0f), glm::vec3(1.0f));

        for (uint32_t i = 0; i < maxFrameInFlight; i++)
        {
            CreateBufferAndMemory(physicalDevice, device, m_transformUniforms[i], m_transformUniformMemory[i], sizeof(TransformUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            TransformUniform uniform{ transform.GetGlobalModelMatrix()};

            void* pData;
            ErrorCheck(vkMapMemory(m_device, m_transformUniformMemory[i], 0, sizeof(TransformUniform), 0, &pData));
            memcpy(pData, &uniform, sizeof(TransformUniform));
            vkUnmapMemory(m_device, m_transformUniformMemory[i]);

            VkDescriptorSetAllocateInfo setAllocInfo{};
            setAllocInfo.descriptorPool = m_descriptorPool;
            setAllocInfo.descriptorSetCount = 1;
            setAllocInfo.pSetLayouts = &m_transformSetLayout;
            setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ErrorCheck(vkAllocateDescriptorSets(device, &setAllocInfo, &m_transformSet[i]));

            VkDescriptorBufferInfo bufferInfo{ m_transformUniforms[i], 0, VK_WHOLE_SIZE };
            const VkWriteDescriptorSet writes
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_transformSet[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bufferInfo, nullptr
            };

            vkUpdateDescriptorSets(device, 1, &writes, 0, nullptr);
        }
    }

    // Load texture
    {
        std::string imagePath = std::string{ ASSETS_PATH } +"textures/lakers.jpg";
        auto [ pixelBuffer, pixelMemory, width, height] = LoadImageIntoHostCoherentMemory(physicalDevice,
            device, imagePath);

        m_samplerSet.resize(maxFrameInFlight);
        m_clothTextureViews.resize(maxFrameInFlight);

        for (int i = 0; i < maxFrameInFlight; ++i)
        {
            auto[image, memory] = CreateImage(device, physicalDevice, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            m_clothTexture.push_back(std::move(image));
            m_clothTextureMemory.push_back(std::move(memory));

            VkImageViewCreateInfo createInfo{};
            createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
            createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
            createInfo.image = m_clothTexture[i];
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

            ErrorCheck(vkCreateImageView(device, &createInfo, nullptr, &m_clothTextureViews[i]));

            SubmitBufferToImageCopy(device, queueFamilyIndex, graphicsQueue, pixelBuffer, m_clothTexture[i], width, height);

            VkDescriptorSetAllocateInfo setAllocInfo{};
            setAllocInfo.descriptorPool = m_descriptorPool;
            setAllocInfo.descriptorSetCount = 1;
            setAllocInfo.pSetLayouts = &m_samplerLayout;
            setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ErrorCheck(vkAllocateDescriptorSets(device, &setAllocInfo, &m_samplerSet[i]));

            VkDescriptorImageInfo imageInfo{ m_immutableSampler, m_clothTextureViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkWriteDescriptorSet writes
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_samplerSet[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo, nullptr, nullptr
            };

            vkUpdateDescriptorSets(device, 1, &writes, 0, nullptr);
        }

        vkFreeMemory(device, pixelMemory, nullptr);
        vkDestroyBuffer(device, pixelBuffer, nullptr);
    }
}

GraphicsTask::~GraphicsTask()
{
    m_pCamera.reset();
    m_pTransform.reset();

    for (uint32_t i = 0; i < m_maxFrameInFlights; i++)
    {
        vkFreeMemory(m_device, m_cameraUniformMemory[i], nullptr);
        vkDestroyBuffer(m_device, m_cameraUniforms[i], nullptr);
        vkFreeMemory(m_device, m_transformUniformMemory[i], nullptr);
        vkDestroyBuffer(m_device, m_transformUniforms[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroySampler(m_device, m_immutableSampler, nullptr);

    vkFreeMemory(m_device, m_uvBufferMemory, nullptr);
    vkDestroyBuffer(m_device, m_uvBuffer, nullptr);

    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);

    vkDestroyDescriptorSetLayout(m_device, m_viewSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_transformSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_samplerLayout, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyShaderModule(m_device, m_vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_device, m_fragmentShaderModule, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);

    for (uint32_t i = 0; i < m_colorAttachments.size(); i++)
    {
        vkDestroyImageView(m_device, m_colorAttachmentViews[i], nullptr);
        vkFreeMemory(m_device, m_colorAttachmentMemory[i], nullptr);
        vkDestroyImage(m_device, m_colorAttachments[i], nullptr);

        vkDestroyImageView(m_device, m_clothTextureViews[i], nullptr);
        vkFreeMemory(m_device, m_clothTextureMemory[i], nullptr);
        vkDestroyImage(m_device, m_clothTexture[i], nullptr);
    }

    vkDestroyImageView(m_device, m_depthAttachmentViews, nullptr);
    vkFreeMemory(m_device, m_depthAttachmentMemory, nullptr);
    vkDestroyImage(m_device, m_depthAttachment, nullptr);
}

void GraphicsTask::BuildCommandBuffers(const uint32_t & frameInFlight, bool changeImageLayout)
{
    VkViewport viewport = { 0.0f, 0.0f + static_cast<float>(m_screenHeight), static_cast<float>(m_screenWidth), -static_cast<float>(m_screenHeight), 0.0f, 1.0f };
    VkRect2D   scissor = { {0, 0}, {m_screenWidth, m_screenHeight} };

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

    /*VkClearValue clears = {};
    clears.color.float32[0] = 0.6033f;
    clears.color.float32[1] = 0.6073f;
    clears.color.float32[2] = 0.6133f;

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
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;*/

    vkCmdBeginRendering(m_commandBuffers[frameInFlight], &m_renderInfoList[frameInFlight]);
    {
        vkCmdSetViewport(m_commandBuffers[frameInFlight], 0, 1, &viewport);
        vkCmdSetScissor(m_commandBuffers[frameInFlight], 0, 1, &scissor);

        vkCmdBindPipeline(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        
        std::array<VkBuffer, 2> vertexBuffs{ m_vertexBuffers[frameInFlight], m_uvBuffer };
        std::array<VkDeviceSize, 2> offsets{ 0, 0};
        vkCmdBindVertexBuffers(m_commandBuffers[frameInFlight], 0, 2, vertexBuffs.data(), offsets.data());
        
        vkCmdBindIndexBuffer(m_commandBuffers[frameInFlight], m_indexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

        std::array<VkDescriptorSet, 3> sets{ m_viewSet[frameInFlight], m_transformSet[frameInFlight], m_samplerSet[frameInFlight] };
        vkCmdBindDescriptorSets(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 3, &sets[0], 0, nullptr);

        vkCmdDrawIndexed(m_commandBuffers[frameInFlight], m_numIndicies, 1, 0, 0, 0);
    }
    vkCmdEndRendering(m_commandBuffers[frameInFlight]);

    ErrorCheck(vkEndCommandBuffer(m_commandBuffers[frameInFlight]));
}

void GraphicsTask::Update(const uint64_t & frameIndex, const uint32_t & frameInFlight,
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
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;

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

const std::vector<VkImageView>& GraphicsTask::GetColorAttachmentViews() const
{
    return m_colorAttachmentViews;
}
