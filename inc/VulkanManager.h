#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <iostream>
#include "ValidationManager.h"
#include "WindowManager.h"

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

    void CreateInstance();
    void GetPhysicalDevice();
    uint32_t GetQueuesFamilyIndex();
    void CreateLogicalDevice(const uint32_t & queueFamilyIndex);
    void GetMaxUsableVKSampleCount();
    void FindBestDepthFormat();

public:
    ~VulkanManager();
    VulkanManager(const uint32_t& screenWidth, const uint32_t& screenHeight);

    void Init();

    void DeInit();
    void Update();
    void CreateSurface(GLFWwindow* glfwWindow);
};
