//
// Created by MasterLong on 2023/9/17.
//

#ifndef VULKANTEST_SWAPCHAIN_H
#define VULKANTEST_SWAPCHAIN_H

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>

namespace jk {

    class VulkanApp;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device);

    class SwapChain {
    public:
        SwapChain(VulkanApp *app);
        // 交换链
        static const std::vector<const char *> deviceExtensions;
        void createFramebuffers();
        void recreate() {
            recreateSwapChain();
        };

    //    void setRenderPass(VkRenderPass renderPass) {
    //        this->renderPass = renderPass;
    //        createFramebuffers();
    //    };

        void cleanup() {
            cleanupSwapChain();
        };

        inline auto getExtent() {
            return swapChainExtent;
        }

        inline auto getFormat() {
            return swapChainImageFormat;
        };

        inline auto getSwapChain() {
            return swapChain;
        }

        inline auto getFramebuffers() {
            return swapChainFramebuffers;
        }

        void init(){
            createSwapChain();
            createImageViews();
        }

    private:
        VulkanApp *app;
        VkDevice device;
        // VkRenderPass renderPass;
        // device and physical device
        // 显示

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        void createSwapChain();

        void cleanupSwapChain();

        void recreateSwapChain();

        void createImageViews();

        void cleanupImageViews();

    //    void createFramebuffers();

        void cleanupFramebuffers();
    };

}

#endif //VULKANTEST_SWAPCHAIN_H
