//
// Created by MasterLong on 2023/10/4.
//

#ifndef VULKANTEST_COMMANDMANAGER_H
#define VULKANTEST_COMMANDMANAGER_H

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>

#include "Buffer.h"
#include "Descriptor.h"
#include "SwapChain.h"
#include "SyncManager.h"

#include "Shader.h"

namespace jk {

    class VulkanApp;

    struct FrameInfo {
        uint32_t currentFrame;
        uint32_t imageIndex;
        VkCommandBuffer commandBuffer;
    };

    class CommandManager {
    private:
        VulkanApp* app;
        VkDevice device;

        // 同步
        SyncManager syncManager;

        VkCommandPool commandPool;
        void createCommandPool();
        void createOneTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);
    public:
        CommandManager(VulkanApp* app);
        void init(std::vector<VkCommandBuffer>& commandBuffers);

        FrameInfo beginFrame(std::vector<VkCommandBuffer>& commandBuffers);
        void endFrame(FrameInfo& frameInfo);

        void reset(VkCommandBuffer& commandBuffer, VkCommandBufferResetFlags flags = 0);
        void renderModelBuffer(FrameInfo &frameInfo, 
                                std::shared_ptr<ModelBuffer>& vbuffer);
        void excuteCurrentFrame(VkCommandBuffer &commandBuffers);
        void excuteCommand(std::function<void(VkCommandBuffer&)> func);
        void cleanup();

        friend class SyncManager;
    };

}
#endif //VULKANTEST_COMMANDMANAGER_H
