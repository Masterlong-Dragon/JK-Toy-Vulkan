//
// Created by MasterLong on 2023/9/17.
//

#include "QueueFamily.h"

namespace jk {

    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // 物理设备队列族
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // 物理设备队列族列表
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // 检查队列族是否支持需要的队列族
        int i = 0;

        for (const auto &queueFamily: queueFamilies) {
            // 检查队列族是否支持图形操作
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // 检查队列族是否支持显示操作
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }


            // 如果找到所有需要的队列族，就停止搜索
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

}