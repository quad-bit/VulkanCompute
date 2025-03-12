#include "..\inc\ComputeTask.h"
#include <array>
#include <random>

namespace
{
    std::random_device rd;
    std::mt19937 gen(rd());

    // Define the range for the random numbers
    std::uniform_int_distribution<> dist(50, 100);

    
};



ComputeTask::ComputeTask(uint32_t queueFamilyIndex, const VkDevice & device, const VkPhysicalDevice& physicalDevice, const VkQueue & computeQueue, const std::vector<VkBuffer>& positionBuffer, const uint32_t& maxFrameInFlights) :
    m_device(device), m_computeQueue(computeQueue), m_maxFrameInflight(maxFrameInFlights), m_positionBuffer(positionBuffer)
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

    std::string spvPath = std::string{ SPV_PATH } +"ClothSim.spv";
    auto[shaderModule, shaderStage] = CreateShaderModule(device, spvPath, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
    m_shaderModule = shaderModule;

    {
        VkDescriptorSetLayoutBinding bindings[2]
        {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
        };

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.bindingCount = 2;
        createInfo.pBindings = bindings;
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ErrorCheck(vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_setLayout));
    }

    VkPushConstantRange range{};
    range.offset = 0;
    range.size = sizeof(PushConst);
    range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.pPushConstantRanges = &range;
    layoutCreateInfo.pSetLayouts = &m_setLayout;
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

    {
        m_pointBuffer.resize(maxFrameInFlights);
        m_pointBufferMemory.resize(maxFrameInFlights);

        size_t structSize = sizeof(Point) * NUM_POINTS;
        for (uint32_t i = 0; i < maxFrameInFlights; i++)
            CreateBufferAndMemory(physicalDevice, device, m_pointBuffer[i], m_pointBufferMemory[i], structSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // Descriptor pool
    {
        VkDescriptorPoolSize pool_sizes[1] =
        {
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * maxFrameInFlights},
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = 0;
        poolInfo.maxSets = 2 * maxFrameInFlights;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = pool_sizes;
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ErrorCheck(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool));
    }

    m_storageDescriptorSets.resize(maxFrameInFlights);

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.descriptorPool = m_descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &m_setLayout;
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    for (int i = 0; i < maxFrameInFlights; ++i)
    for (int i = 0; i < maxFrameInFlights; ++i)
    {
        ErrorCheck(vkAllocateDescriptorSets(device, &setAllocInfo, &m_storageDescriptorSets[i]));

        VkDescriptorBufferInfo pointBufferInfo{ m_pointBuffer[i], 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo positionBufferInfo{ m_positionBuffer[i], 0, VK_WHOLE_SIZE };

        const VkWriteDescriptorSet writes[2] =
        {
            {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_storageDescriptorSets[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &pointBufferInfo, nullptr},
            {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_storageDescriptorSets[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &positionBufferInfo, nullptr},
        };

        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }

    //upload data
    {
        std::vector<Point> points;
        uint32_t row = 0, col = 0;
        float springLength = 0.50f;
        float originX = NUM_COLS % 2 == 0 ? (NUM_COLS / 2) * springLength - springLength / 2.0f : (NUM_COLS / 2) * springLength;
        float originY = NUM_ROWS % 2 == 0 ? (NUM_ROWS / 2) * springLength - springLength / 2.0f : (NUM_ROWS / 2) * springLength;

        glm::vec3 topLeft{ -originX, originY, 0.0f };
        bool isPinned = false;
        for (uint32_t i = 0; i < NUM_POINTS; i++)
        {
            if (i == 0 || i == NUM_COLS - 1)
                isPinned = true;

            auto CreateSpiral = [&]() -> glm::vec4
            {
                float x = topLeft.x + col * springLength;
                float a = 0.5f;
                float b = 2.0f;
                float theta = glm::pi<float>() * (2.0f - (float)row/5.0f);
                float radius = (float)10.0f / (row + 1.0f);
                float z = radius * glm::cos(theta);
                float y = originY /*-(float)(row * springLength)/10.0f*/ + radius * glm::sin(theta);
                
                return glm::vec4(x, y, z, 1.0f);
            };

            auto position = glm::vec4(topLeft, 1.0f) + glm::vec4(col * springLength, -(float)(row * springLength), 0, 1.0f);
            //auto position = CreateSpiral();
            Point point(isPinned);
            point.m_currentPosition = glm::vec4(position.x, position.y, position.z, 1.0f);
            point.m_previousPosition = point.m_currentPosition;

            //Normal
            {
                glm::vec3 normal{ 0, 0, 1 };
                int randNum = dist(gen);
                randNum = randNum > 75 ? -randNum : randNum;
                normal.y = float(randNum % 50) / 50.0f;
                randNum = dist(gen);
                randNum = randNum > 75 ? -randNum : randNum;
                normal.x = float(randNum % 50) / 50.0f;

                normal = glm::normalize(normal);
                point.m_randomNormal = glm::vec4(normal.x, normal.y, normal.z, 0.0);
            }

            uint32_t neighbourCount = 0;

            auto GetNeighbours = [&i](int* neighbours)
            {
                bool right = false, left = false, top = false, bottom = false;

                //Structural constraints exist between a point mass and the 
                //point mass to its left as well as the point mass above it

                //Right
                if (i%NUM_COLS <= NUM_COLS - 2)
                {
                    neighbours[0] = i + 1;
                    right = true;
                }

                //Left
                if (i%NUM_COLS != 0)
                {
                    neighbours[1] = i - 1;
                    left = true;
                }

                //Top
                if (i / NUM_COLS != 0)
                {
                    neighbours[2] = i - NUM_COLS;
                    top = true;
                }

                //Bottom
                if (i / NUM_COLS != NUM_ROWS - 1)
                {
                    neighbours[3] = i + NUM_COLS;
                    bottom = true;
                }


                //Shearing constraints exist between a point mass and the point mass 
                //to its diagonal upper left as well as the point mass to its diagonal upper right.

                // right bottom diagonal
                if (right && bottom)
                {
                    neighbours[4] = i + NUM_COLS + 1;
                }

                // left bottom diagonal
                if (left && bottom)
                {
                    neighbours[5] = i + NUM_COLS - 1;
                }

                // right top diagonal
                if (right && top)
                {
                    neighbours[6] = i - NUM_COLS + 1;
                }

                // left top diagonal
                if (left && top)
                {
                    neighbours[7] = i - NUM_COLS - 1;
                }

                //Bending constraints exist between a point mass and the point mass two 
                //away to its left&right as well as the point mass two above & below it.

                if (i%NUM_COLS >= 2)
                {
                    neighbours[8] = i - 2;
                }

                if (i%NUM_COLS < NUM_COLS - 2)
                {
                    neighbours[10] = i + 2;
                }

                if (i/NUM_COLS >= 2)
                {
                    neighbours[9] = i - 2 * NUM_COLS;
                }

                if (i/NUM_COLS < NUM_ROWS - 2)
                {
                    neighbours[11] = i + 2 * NUM_COLS;
                }
            };

#if 0
            {
                // i+1 right
                // i-1 left
                // i+numCols, bottom
                // i-numCols, up

                bool right = false, left = false, top = false, bottom = false;

                //Right
                if (i%NUM_COLS <= NUM_COLS - 2)
                {
                    point.m_neighbourIndicies[0] = i + 1;
                    right = true;
                }

                //Left
                if (i%NUM_COLS != 0)
                {
                    point.m_neighbourIndicies[1] = i - 1;
                    left = true;
                }

                //Top
                if (i/NUM_COLS != 0)
                {
                    point.m_neighbourIndicies[2] = i - NUM_COLS;
                    top = true;
                }

                //Bottom
                if (i/NUM_COLS != NUM_ROWS - 1)
                {
                    point.m_neighbourIndicies[3] = i + NUM_COLS;
                    bottom = true;
                }

                /*
                // right bottom diagonal
                if (right && bottom)
                {
                    point.m_neighbourIndicies[4] = i + NUM_COLS + 1;
                }

                // left bottom diagonal
                if (left && bottom)
                {
                    point.m_neighbourIndicies[5] = i + NUM_COLS - 1;
                }

                // right top diagonal
                if (right && top)
                {
                    point.m_neighbourIndicies[6] = i - NUM_COLS + 1;
                }

                // left top diagonal
                if (left && top)
                {
                    point.m_neighbourIndicies[7] = i - NUM_COLS - 1;
                }
                */
            }
#else
            GetNeighbours(point.m_neighbourIndicies);
#endif
            points.push_back(point);

            col++;
            if (col % (NUM_COLS) == 0)
            {
                row++;
                col = 0;
            }
        }

        auto structSize = sizeof(Point);
        auto dataSize = points.size() * structSize;
        void* pData;
        ErrorCheck(vkMapMemory(m_device, m_pointBufferMemory[0], 0, dataSize, 0, &pData));
        memcpy(pData, points.data(), dataSize);
        vkUnmapMemory(m_device, m_pointBufferMemory[0]);
    }
}

ComputeTask::~ComputeTask()
{
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    for (auto mem : m_pointBufferMemory)
        FreeMemory(m_device, mem);

    for (auto& buf : m_pointBuffer)
        vkDestroyBuffer(m_device, buf, nullptr);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
}

void ComputeTask::Update(const uint64_t & frameIndex, const uint32_t & frameInFlight, const VkSemaphore & semaphore, uint64_t signalValue)
{
    if (frameIndex >= 2000)
    {
        pushConst.m_changeScene = 1;
        static int counter = 0;

        counter++;
        if (counter == 50)
        {
            //counter = 0;
            pushConst.m_windIntensity = 20.0f;// float(dist(gen));
        }
        else
        {
            pushConst.m_windIntensity = .0005f;
        }
    }

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

    {
        ErrorCheck(vkQueueSubmit2(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }
}

void ComputeTask::BuildCommandBuffers(const uint32_t & frameInFlight)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ErrorCheck(vkResetCommandBuffer(m_commandBuffers[frameInFlight], 0));
    ErrorCheck(vkBeginCommandBuffer(m_commandBuffers[frameInFlight], &beginInfo));

    vkCmdBindDescriptorSets(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_storageDescriptorSets[frameInFlight], 0, nullptr);
    vkCmdBindPipeline(m_commandBuffers[frameInFlight], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    vkCmdPushConstants(m_commandBuffers[frameInFlight], m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConst), &pushConst);

    auto x = (uint32_t)ceil(NUM_COLS / (float)32);
    auto y = (uint32_t)ceil(NUM_ROWS / (float)32);
    vkCmdDispatch(m_commandBuffers[frameInFlight], x, y, 1);

    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.buffer = m_pointBuffer[frameInFlight];
    barrier.offset = 0;
    barrier.size = sizeof(Point) * NUM_POINTS;

    // The semaphore takes care of srcStageMask.
    vkCmdPipelineBarrier(m_commandBuffers[frameInFlight], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

    // Copy the buffer being used to the one that will be used in the next frame
    {
        VkBufferCopy copyInfo{};
        copyInfo.dstOffset = 0;
        copyInfo.size = sizeof(Point) * NUM_POINTS;
        copyInfo.srcOffset = 0;

        vkCmdCopyBuffer(m_commandBuffers[frameInFlight], m_pointBuffer[frameInFlight], m_pointBuffer[(frameInFlight + 1) % m_maxFrameInflight], 1, &copyInfo);
    }

    ErrorCheck(vkEndCommandBuffer(m_commandBuffers[frameInFlight]));
}
