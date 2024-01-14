//
// Created by MasterLong on 2023/9/17.
//

#include "SwapChain.h"
#include "QueueFamily.h"
#include "VulkanApp.h"

namespace jk {

    const std::vector<const char *> SwapChain::deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    SwapChain::SwapChain(VulkanApp *app) {
        this->app = app;
        device = app->getDevice();
    }

    /**
     * 交换链支持细节
     */
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        // 获取交换链支持细节
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // 获取交换链支持格式
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        // 如果支持格式数量大于 0
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            // 获取支持格式
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // 获取交换链支持显示模式
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        // 如果支持显示模式数量大于 0
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            // 获取支持显示模式
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        // 更正 VK_FORMAT_B8G8R8A8_SRGB

        // 如果只有一个可用的格式，且格式为undefined，表示所有格式都可用
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
            // 返回默认格式
            return {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }

        // 如果有多个可用的格式，检查是否有需要的格式
        for (const auto &availableFormat: availableFormats) {
            // 如果有需要的格式
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                // 返回需要的格式
                return availableFormat;
            }
        }

        // 如果没有需要的格式，返回第一个可用的格式
        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        // 检查是否支持三重缓冲
        for (const auto &availablePresentMode: availablePresentModes) {
            // 如果支持三重缓冲
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // 返回三重缓冲
                return availablePresentMode;
            }
        }

        // 如果不支持三重缓冲，返回双重缓冲
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        // 如果当前分辨率不受限制
        if (capabilities.currentExtent.width != UINT32_MAX) {
            // 返回当前分辨率
            return capabilities.currentExtent;
        } else {
            // 获取窗口大小
            int width, height;
            auto window = app->getWindow();
            glfwGetFramebufferSize(window, &width, &height);

            // 创建分辨率
            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            // 限制分辨率
            actualExtent.width = std::max(capabilities.minImageExtent.width,
                                        std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                                        std::min(capabilities.maxImageExtent.height, actualExtent.height));

            // 返回分辨率
            return actualExtent;
        }
    }

    void SwapChain::createSwapChain() {
        // 获取交换链支持信息

        auto surface = app->getSurface();
        auto physicalDevice = app->getPhysicalDevice();

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, physicalDevice);

        // 获取交换链surface格式
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        // 获取交换链显示模式
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        // 获取交换链分辨率
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // 获取交换链图片数量
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        // 如果交换链图片数量超过最大值，或者小于最小值
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // 创建交换链
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // 获取队列族句柄
        QueueFamilyIndices indices = findQueueFamilies(surface, physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        // 如果队列族不同
        if (indices.graphicsFamily != indices.presentFamily) {
            // 设置共享模式
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            // 设置独占模式
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        // 设置变换方式
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        // 设置旧交换链
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // 创建交换链
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // 获取交换链图片
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // 保存交换链信息
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void SwapChain::cleanupSwapChain() {

        // 清除深度资源
        app->destroyImageResources();

        cleanupFramebuffers();
        cleanupImageViews();
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void SwapChain::recreateSwapChain() {
        int width = 0, height = 0;
        auto window = app->getWindow();
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwWaitEvents();
            glfwGetFramebufferSize(window, &width, &height);
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();

        createFramebuffers();
    }

    void SwapChain::createImageViews() {
        // 为每张交换链图片创建视图
        swapChainImageViews.resize(swapChainImages.size());
        // 遍历交换链图片
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            // 创建视图
            swapChainImageViews[i] = app->createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void SwapChain::cleanupImageViews() {
        // 销毁交换链视图
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }

    void SwapChain::createFramebuffers() {
        // 深度资源
        app->initImageResources();

        swapChainFramebuffers.resize(swapChainImageViews.size());

        // 遍历所有的交换链视图
        auto renderPass = app->getRenderProcess()->getRenderPass();
        auto depthImageView = app->getDepthResource();
        auto colorImageView = app->getColorResource();
        int attrSwapIndex = 0;
        std::vector<VkImageView> attachments = {
                swapChainImageViews[0],
                depthImageView
        };
        if (app->isEnableMSAA()) {
            attachments = {
                    colorImageView,
                    depthImageView,
                    swapChainImageViews[0]
            };
            attrSwapIndex = 2;
        }
        auto attachmentCount = static_cast<uint32_t>(attachments.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            // 设置附件信息
            attachments[attrSwapIndex] = swapChainImageViews[i];
            // 设置帧缓冲信息
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

            framebufferInfo.renderPass = renderPass; // 设置渲染流程
            framebufferInfo.attachmentCount = attachmentCount; // 设置附件数量
            framebufferInfo.pAttachments = attachments.data(); // 设置附件
            framebufferInfo.width = swapChainExtent.width; // 设置宽度
            framebufferInfo.height = swapChainExtent.height; // 设置高度
            framebufferInfo.layers = 1; // 设置层数

            // 创建帧缓冲
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                // 创建失败
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void SwapChain::cleanupFramebuffers() {
        // 遍历所有的帧缓冲
        for (auto framebuffer : swapChainFramebuffers) {
            // 销毁帧缓冲
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

}