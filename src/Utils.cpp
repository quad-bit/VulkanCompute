#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <optional>
#include <fstream>

void ErrorCheck(VkResult result)
{
#ifdef _DEBUG
    if (result < 0)
    {
        switch (result)
        {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            std::cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            std::cout << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
            break;
        case VK_ERROR_DEVICE_LOST:
            std::cout << "VK_ERROR_DEVICE_LOST" << std::endl;
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            std::cout << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            std::cout << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            std::cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            std::cout << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            std::cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            std::cout << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            std::cout << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            std::cout << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            std::cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
            break;
        case VK_SUBOPTIMAL_KHR:
            std::cout << "VK_SUBOPTIMAL_KHR" << std::endl;
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
            break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            std::cout << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
            break;
        case VK_ERROR_VALIDATION_FAILED_EXT:
            std::cout << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
            break;
        default:
            break;
        }

        assert(0);
    }
#endif
}

VkCommandBuffer AllocateCommandBuffer(const VkDevice & device, const VkCommandPool & commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = commandPool;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    VkCommandBuffer cmdBuffer;
    ErrorCheck( vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer));
    return cmdBuffer;
}

void FreeCommandBuffer(const VkDevice & device, const VkCommandPool & commandPool, VkCommandBuffer* commandBuffer)
{
    vkFreeCommandBuffers(device, commandPool, 1, commandBuffer);
}

VkDeviceMemory AllocateHostCoherentMemory(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkDeviceSize & bufferSize, const VkMemoryRequirements & memoryRequirements)
{
    // Get memory types supported by the physical device:
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    std::optional<uint32_t> memIndex;
    // In search for a suitable memory type INDEX:
    for (uint32_t i = 0u; i < memoryProperties.memoryTypeCount; ++i)
    {
        // Is this kind of memory suitable for our buffer?
        const auto bitmask = memoryRequirements.memoryTypeBits;
        const auto bit = 1 << i;
        if (0 == (bitmask & bit)) 
        {
            continue; // => nope
        }

        // Does this kind of memory support our usage requirements?
        if ((memoryProperties.memoryTypes[i].propertyFlags & (VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            != VkMemoryPropertyFlags{})
        {
            // Return the INDEX of a suitable memory type
            memIndex = i;
            break;
        }
    }

    assert(memIndex.has_value() == true);

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.allocationSize = std::max(bufferSize, memoryRequirements.size);
    memoryAllocInfo.memoryTypeIndex = memIndex.value();
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    ErrorCheck(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory));

    return memory;
}

//std::tuple<VkBuffer, VkDeviceMemory, int, int> LoadImageIntoHostCoherentMemory(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const std::string & pathToImageFile)
//{
//    return std::tuple<VkBuffer, VkDeviceMemory, int, int>();
//}

void FreeMemory(const VkDevice & device, const VkDeviceMemory& memory)
{
    vkFreeMemory(device, memory, nullptr);
}

void DestroyBuffer(const VkDevice & device, const VkBuffer& buffer)
{
    vkDestroyBuffer(device, buffer, nullptr);
}

void ChangeImageLayoutWithBarriers(const VkCommandBuffer & commandBuffer, const VkPipelineStageFlags & srcPipelineStage, const VkPipelineStageFlags & dstPipelineStage, const VkAccessFlags & srcAccessMask, const VkAccessFlags & dstAccessMask, const VkImage & image, const VkImageLayout & oldLayout, const VkImageLayout & newLayout)
{
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(commandBuffer, srcPipelineStage,
        dstPipelineStage, {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
   );
}

void CopyBufferToImage(const VkCommandBuffer & commandBuffer, const VkBuffer & buffer, const VkImage & image, const uint32_t width, const uint32_t height)
{
    VkBufferImageCopy bufImagCopy{
                0, width, height,
                VkImageSubresourceLayers{VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, VkOffset3D{0, 0, 0}, VkExtent3D{width, height, 1} };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufImagCopy);
}

std::tuple<VkImage, VkDeviceMemory> CreateImage(const VkDevice & device, const VkPhysicalDevice & physicalDevice, const uint32_t width, const uint32_t height, const VkFormat & format, const VkImageUsageFlags & usageFlags)
{
    VkImage image = VK_NULL_HANDLE;
    VkImageCreateInfo createInfo = {};
    createInfo.imageType = (VkImageType::VK_IMAGE_TYPE_2D);
    createInfo.extent = { width, height, 1u };
    createInfo.mipLevels = 1u;
    createInfo.arrayLayers = 1u;
    createInfo.format = format;
    createInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;			// We just create all images in optimal tiling layout
    createInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;	// Initially, the layout is undefined
    createInfo.usage = usageFlags;
    createInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    createInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ErrorCheck(vkCreateImage(device, &createInfo, nullptr, &image));

    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(device, image, &memReq);

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.allocationSize = memReq.size;
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.memoryTypeIndex = ([&]()
    {
        // Get memory types supported by the physical device:
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        // In search for a suitable memory type INDEX:
        for (uint32_t i = 0u; i < memoryProperties.memoryTypeCount; ++i)
        {

            // Is this kind of memory suitable for our buffer?
            const auto bitmask = memReq.memoryTypeBits;
            const auto bit = 1 << i;
            if (0 == (bitmask & bit))
            {
                continue; // => nope
            }

            // Does this kind of memory support our usage requirements?
            if ((memoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) // In contrast to our host-coherent buffers, we just assume that we want all our images to live in device memory
                != VkMemoryPropertyFlags{})
            {
                // Return the INDEX of a suitable memory type
                return i;
            }
        }
        throw std::runtime_error("Couldn't find suitable memory.");
    }());

    VkDeviceMemory memory;
    ErrorCheck(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory));
    ErrorCheck(vkBindImageMemory(device, image, memory, 0));

    return std::make_tuple(image, memory);
}

void DestroyImage(const VkDevice & device, VkImage image)
{
    vkDestroyImage(device, image, nullptr);
}

VkImageView CreateImageView(const VkDevice & device, const VkPhysicalDevice & physicalDevice, const VkImage & image, const VkFormat & format, const VkImageAspectFlags & imageAspectFlags)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.image = image;
    createInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange = { imageAspectFlags, 0u, 1u, 0u, 1u };

    VkImageView view = VK_NULL_HANDLE;
    ErrorCheck(vkCreateImageView(device, &createInfo, nullptr, &view));

    return view;
}

void DestroyImageView(const VkDevice & device, VkImageView imageView)
{
    vkDestroyImageView(device, imageView, nullptr);
}

std::tuple<VkShaderModule, VkPipelineShaderStageCreateInfo> CreateShaderModule(const VkDevice & device, const std::string & path, const VkShaderStageFlagBits & shaderStage)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.codeSize = buffer.size();
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    ErrorCheck(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.stage = shaderStage;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    return std::make_tuple(shaderModule, shaderStageCreateInfo);
}

void DestroyShaderModule(const VkDevice & device, VkShaderModule shaderModule)
{
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

std::tuple<VkBuffer, VkDeviceMemory> CreateBufferAndMemory(const VkDevice & device, const VkPhysicalDevice & physicalDevice, const size_t bufferSize, const VkBufferUsageFlags & bufferUsageFlags)
{
    return std::tuple<VkBuffer, VkDeviceMemory>();
}

void CopyDataIntoHostCoherentMemory(const VkDevice & device, const size_t & dataSize, const void * data, VkDeviceMemory & memory)
{
}

void ChangeImageLayout(const VkDevice& device, std::vector<VkImage>& imageList, const VkQueue& queue,
    uint32_t queueFamilyIndex, VkImageLayout oldLayout, VkImageLayout newLayout)
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
    std::vector<VkImageMemoryBarrier2> list;
    for (auto& image : imageList)
    {
        VkImageMemoryBarrier2 imgBarrier{};
        imgBarrier.dstAccessMask = 0;
        imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        imgBarrier.image = image;
        imgBarrier.newLayout = newLayout;
        imgBarrier.oldLayout = oldLayout;
        imgBarrier.pNext = nullptr;
        imgBarrier.srcAccessMask = 0;
        imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgBarrier.subresourceRange.baseArrayLayer = 0;
        imgBarrier.subresourceRange.baseMipLevel = 0;
        imgBarrier.subresourceRange.layerCount = 1;
        imgBarrier.subresourceRange.levelCount = 1;
        list.push_back(imgBarrier);
    }

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.imageMemoryBarrierCount = list.size();
    dependencyInfo.pImageMemoryBarriers = list.data();
    dependencyInfo.pNext = nullptr;
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
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