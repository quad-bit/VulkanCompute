#pragma once
#include <memory>
#include <iostream>
#include "ValidationManager.h"
#include "WindowManager.h"
#include "Utils.h"

class VulkanManager
{
private:
    VulkanManager(VulkanManager const&) = delete;
    VulkanManager const& operator= (VulkanManager const&) = delete;

    std::unique_ptr<ValidationManager> m_validationManagerObj;
    std::unique_ptr<WindowManager> m_windowManagerObj;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkInstance m_instanceObj = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE, m_computeQueue = VK_NULL_HANDLE;

    uint32_t m_queueFamilyIndex;

    VkFormat m_depthFormat;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkSurfaceCapabilitiesKHR m_surfaceCapabilities;

    size_t m_surfaceWidth, m_surfaceHeight;
    uint32_t m_swapchainImageCount = 0;
    uint32_t m_maxFrameInFlight, m_frameInFlight = 0;
    VkSwapchainKHR m_swapchainObj = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImageList;
    std::vector<VkImageView> m_swapChainImageViewList;

    void CreateInstance();
    void GetPhysicalDevice();
    uint32_t GetQueuesFamilyIndex();
    void CreateLogicalDevice(const uint32_t & queueFamilyIndex);
    void GetMaxUsableVKSampleCount();
    void FindBestDepthFormat();
    void CreateSurface(GLFWwindow* glfwWindow);

    void CreateSwapchain();
    void DestroySwapChain();

public:
    ~VulkanManager();
    VulkanManager(const uint32_t& screenWidth, const uint32_t& screenHeight);

    void Init(GLFWwindow* glfwWindow);

    void DeInit();
    void Update();
};
