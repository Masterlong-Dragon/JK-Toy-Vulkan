//
// Created by MasterLong on 2023/10/4.
//

#include "CommandManager.h"
#include "VulkanApp.h"

namespace jk {

    CommandManager::CommandManager(VulkanApp *app) : app(app), device(app->getDevice()), syncManager(this) {
        syncManager.init();
    }

    void CommandManager::createCommandPool() {
        // 获取队列族信息
        auto surface = app->getSurface();
        auto physicalDevice = app->getPhysicalDevice();
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(surface, physicalDevice);

        // 设置命令池信息
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(); // 设置队列族索引
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // 设置标志位

        // 创建命令池
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            // 创建失败
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void CommandManager::createCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers) {
        commandBuffers.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        // 设置命令缓冲信息
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        // 分配命令缓冲
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void CommandManager::renderModelBuffer(FrameInfo &frameInfo, 
                                            std::shared_ptr<ModelBuffer>& vbuffer) {
                                    
        // 绑定顶点缓冲
        vbuffer->bind(frameInfo.commandBuffer);
        vbuffer->draw(frameInfo.commandBuffer);
    }

    FrameInfo CommandManager::beginFrame(std::vector<VkCommandBuffer>& commandBuffers) {
        // 等待当前帧缓冲
        syncManager.waitFrame();
        syncManager.acquireNextImage();

        auto currentFrame = syncManager.getCurrentFrame();

        FrameInfo frame = {currentFrame, syncManager.getCurrentImageIndex(), commandBuffers[currentFrame]};

        reset(frame.commandBuffer, 0);

        // 设置命令缓冲信息
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        // 开始记录命令缓冲
        if (vkBeginCommandBuffer(frame.commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return frame;
    }

    void CommandManager::endFrame(FrameInfo& frameInfo) {

        // 结束记录命令缓冲
        if (vkEndCommandBuffer(frameInfo.commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // 提交命令缓冲
        syncManager.submit(frameInfo.commandBuffer);
        // 提交绘制结果
        syncManager.present();
        syncManager.nextFrame();
    }

    void CommandManager::init(std::vector<VkCommandBuffer> &commandBuffers) {
        createCommandPool();
        createCommandBuffers(commandBuffers);
    }

    void CommandManager::reset(VkCommandBuffer &commandBuffer, VkCommandBufferResetFlags flags) {
        vkResetCommandBuffer(commandBuffer, flags);
    }

    void CommandManager::cleanup() {
        syncManager.cleanup();
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    void CommandManager::excuteCurrentFrame(VkCommandBuffer &commandBuffer) {
        // 提交命令缓冲
        syncManager.submit(commandBuffer);
    }

    // 执行一条命令
    void CommandManager::excuteCommand(std::function<void(VkCommandBuffer &)> func) {

        // 分配命令缓冲
        VkCommandBuffer commandBuffer;
        createOneTimeCommandBuffer(commandBuffer);

        // 开始记录命令缓冲
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // 执行命令
        if (func) {
            func(commandBuffer);
        }

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        auto queue = app->getGraphicsQueue();

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void CommandManager::createOneTimeCommandBuffer(VkCommandBuffer &commandBuffer) {
        // 分配命令缓冲
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // 主要命令缓冲
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    }

}
