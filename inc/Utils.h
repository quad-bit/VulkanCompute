#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <tuple>
#include <string>

void ErrorCheck(VkResult result);

VkCommandBuffer AllocateCommandBuffer(
    const VkDevice& device,
    const VkCommandPool& commandPool
);

void FreeCommandBuffer(
    const VkDevice& device,
    const VkCommandPool& commandPool,
    VkCommandBuffer* commandBuffer
);

VkDeviceMemory AllocateHostCoherentMemory(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& device,
    const VkDeviceSize& bufferSize,
    const VkMemoryRequirements& memoryRequirements
);

// Load an image from a file, and copy it into a newly created buffer (backed with memory already):
// Returns a tuple with: <0> the buffer handle, <1> the memory handle, <2> width, <3> height
//std::tuple<VkBuffer, VkDeviceMemory, int, int> LoadImageIntoHostCoherentMemory(
//    const VkPhysicalDevice& physicalDevice,
//    const VkDevice& device,
//    const std::string& pathToImageFile
//);

// Free memory that has been allocated with AllocateHostCoherentMemoryForBuffer
void FreeMemory(
    const VkDevice& device,
    const VkDeviceMemory& memory
);

void DestroyBuffer(
    const VkDevice& device,
    const VkBuffer& buffer
);

void ChangeImageLayoutWithBarriers(
    const VkCommandBuffer& commandBuffer,
    const VkPipelineStageFlags& srcPipelineStage, const VkPipelineStageFlags& dstPipelineStage,
    const VkAccessFlags& srcAccessMask, const VkAccessFlags& dstAccessMask,
    const VkImage& image,
    const VkImageLayout& oldLayout, const VkImageLayout& newLayout
);

void CopyBufferToImage(
    const VkCommandBuffer& commandBuffer,
    const VkBuffer& buffer,
    const VkImage& image, const uint32_t width, const uint32_t height
);

std::tuple<VkImage, VkDeviceMemory> CreateImage(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const uint32_t width, const uint32_t height,
    const VkFormat& format, const VkImageUsageFlags& usageFlags
);

// Destroy an image that has been created using the helper functions
void DestroyImage(
    const VkDevice& device,
    VkImage image
);

// Create an image view to an image
VkImageView CreateImageView(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const VkImage& image, const VkFormat& format, const VkImageAspectFlags& imageAspectFlags
);

// Destroy an image view that has been created using the helper functions
void DestroyImageView(
    const VkDevice& device,
    VkImageView imageView
);

std::tuple<VkShaderModule, VkPipelineShaderStageCreateInfo> CreateShaderModule(
    const VkDevice& device,
    const std::string& path,
    const VkShaderStageFlagBits& shaderStage
);

void DestroyShaderModule(
    const VkDevice& device,
    VkShaderModule shaderModule
);

std::tuple<VkBuffer, VkDeviceMemory> CreateBufferAndMemory(
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const size_t bufferSize,
    const VkBufferUsageFlags& bufferUsageFlags
);

// Copy data of the gifen size into the buffer
void CopyDataIntoHostCoherentMemory(
    const VkDevice& device,
    const size_t& dataSize,
    const void* data,
    VkDeviceMemory& memory
);

