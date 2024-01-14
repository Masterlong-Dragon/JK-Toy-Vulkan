//
// Created by MasterLong on 2023/10/4.
//

#ifndef VULKANTEST_SYNCMANAGER_H
#define VULKANTEST_SYNCMANAGER_H

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

#include "RenderProcess.h"

namespace jk {

class CommandManager;

    class SyncManager {
    private:
        RenderProcess* renderProcess;
        CommandManager* commandManager;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        uint32_t currentFrame = 0;
        uint32_t imageIndex = 0;

        void createSyncObjects();
        void cleanupSyncObjects();
    public:
        SyncManager(CommandManager* commandManager);

        void init();
        void cleanup();

        inline uint32_t getCurrentFrame() const {
            return currentFrame;
        }

        inline uint32_t getCurrentImageIndex() const {
            return imageIndex;
        }

        void waitFrame();
        void acquireNextImage();
        void nextFrame();

        void submit(VkCommandBuffer& commandBuffer);
        void present();
    };

}

#endif //VULKANTEST_SYNCMANAGER_H
