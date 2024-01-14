//
// Created by MasterLong on 2023/10/5.
//

#ifndef VULKANTEST_VERTEX_H
#define VULKANTEST_VERTEX_H

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace jk {

    struct Vertex{
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription(){
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0; // 顶点数据绑定索引
            bindingDescription.stride = sizeof(Vertex); // 步长
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 顶点数据输入速率

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions(){
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
            // 顶点位置属性
            attributeDescriptions[0].binding = 0; // 顶点数据绑定索引
            attributeDescriptions[0].location = 0; // 顶点着色器中的位置属性索引
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // 顶点数据格式
            attributeDescriptions[0].offset = offsetof(Vertex, pos); // 顶点数据在结构体中的偏移量
            // 顶点法线属性
            attributeDescriptions[1].binding = 0; // 顶点数据绑定索引
            attributeDescriptions[1].location = 1; // 顶点着色器中的法线属性索引
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // 顶点数据格式
            attributeDescriptions[1].offset = offsetof(Vertex, normal); // 顶点数据在结构体中的偏移量
            // 顶点颜色属性
            attributeDescriptions[2].binding = 0; // 顶点数据绑定索引
            attributeDescriptions[2].location = 2; // 顶点着色器中的颜色属性索引
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // 顶点数据格式
            attributeDescriptions[2].offset = offsetof(Vertex, color); // 顶点数据在结构体中的偏移量
            // 顶点纹理坐标属性
            attributeDescriptions[3].binding = 0; // 顶点数据绑定索引
            attributeDescriptions[3].location = 3; // 顶点着色器中的纹理坐标属性索引
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT; // 顶点数据格式
            attributeDescriptions[3].offset = offsetof(Vertex, texCoord); // 顶点数据在结构体中的偏移量

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const {
            return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
        }
    };

}

#endif //VULKANTEST_VERTEX_H
