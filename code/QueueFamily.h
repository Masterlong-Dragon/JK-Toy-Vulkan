//
// Created by MasterLong on 2023/9/17.
//

#ifndef VULKANTEST_QUEUEFAMILY_H
#define VULKANTEST_QUEUEFAMILY_H

#include <vulkan/vulkan.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>
#include <optional>
#include <vector>

namespace jk {

    /**
     * 队列族
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device);

}


#endif //VULKANTEST_QUEUEFAMILY_H
