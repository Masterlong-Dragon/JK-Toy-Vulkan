//
// Created by MasterLong on 2023/10/4.
//

#include "SyncManager.h"
#include "VulkanApp.h"

namespace jk {

    SyncManager::SyncManager(CommandManager* commandManager) : commandManager(commandManager) {
        renderProcess = commandManager->app->getRenderProcess();
    }

    void SyncManager::createSyncObjects() {
        // 设置信号量信息
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // 创建信号量
        imageAvailableSemaphores.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < VulkanApp::MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(commandManager->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(commandManager->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(commandManager->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void SyncManager::cleanupSyncObjects() {
        for (size_t i = 0; i < VulkanApp::MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(commandManager->device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(commandManager->device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(commandManager->device, inFlightFences[i], nullptr);
        }
    }

    void SyncManager::init() {
        createSyncObjects();
    }

    void SyncManager::cleanup() {
        cleanupSyncObjects();
    }

    void SyncManager::waitFrame() {
        vkWaitForFences(commandManager->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    }

    void SyncManager::acquireNextImage() {
        VkResult result = vkAcquireNextImageKHR(commandManager->device,
                                                renderProcess->getSwapChain().getSwapChain(), UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        // 检查是否需要重新创建交换链
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            renderProcess->recreate();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(commandManager->device, 1, &inFlightFences[currentFrame]);
    }

    void SyncManager::nextFrame() {
        // 更新当前帧
        currentFrame = (currentFrame + 1) % VulkanApp::MAX_FRAMES_IN_FLIGHT;
    }

    void SyncManager::submit(VkCommandBuffer &commandBuffer) {
        // 提交命令缓冲
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // 设置等待信号量
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // 设置信号信号量
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // 提交命令缓冲
        if (vkQueueSubmit(commandManager->app->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

    void SyncManager::present() {
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        // 设置交换链
        VkSwapchainKHR swapChains[] = {renderProcess->getSwapChain().getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        // 提交绘制结果
        VkResult result = vkQueuePresentKHR(commandManager->app->getPresentQueue(), &presentInfo);

        // 检查是否需要重新创建交换链
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            renderProcess->recreate();
            // vkResetFences(device, 1, &inFlightFences[currentFrame]);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            // 获取图像索引失败
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

}