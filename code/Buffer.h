//
// Created by MasterLong on 2023/10/5.
//

#ifndef VULKANTEST_BUFFER_H
#define VULKANTEST_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include "Vertex.h"
#include "ResourceHelper.hpp"
#include "Descriptor.h"

namespace jk {

    class VulkanApp;

    class UniformBuffer : public IResource{
    private:
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        VkDeviceSize bufferSize;

        std::vector<VkDescriptorBufferInfo> uniformBufferInfo;

        void createUniformBuffers(VulkanApp *app, VkDeviceSize bufferSize);
    public:
        virtual void cleanup(VkDevice& device);

        void updateUniformBuffer(uint32_t currentImage, void* ubo);

        std::vector<VkBuffer>& getUniformBuffers();
        std::vector<VkDescriptorBufferInfo>& getUniformBufferInfo();

        inline void fillUniformDescriptorSets(std::shared_ptr<DescriptorSets> descriptorSets, uint32_t binding) {
            uint32_t size = descriptorSets->getDescriptorSets().size();
            for (uint32_t i = 0; i < size; i++) {
                descriptorSets->writer.writeBuffer(binding, &uniformBufferInfo[i]).overwrite(i).flush();
            }
        }

        friend class GeneralBufferManager;
    };

    class ModelBuffer : public IResource{
    private:
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        uint32_t vertexCount = 0;     // 记录顶点数量
        uint32_t indexCount = 0;      // 记录索引数量

        bool isIndexed = false;

        std::function<void(VkCommandBuffer& commandBuffer)> bindFunc;
        std::function<void(VkCommandBuffer& commandBuffer)> drawFunc;
        std::function<void(VkDevice& device)> cleanEndFunc;

        void defaultBind(VkCommandBuffer& commandBuffer);
        void defaultDraw(VkCommandBuffer& commandBuffer);
        void indexedBind(VkCommandBuffer& commandBuffer);
        void indexedDraw(VkCommandBuffer& commandBuffer);
        void cleanIndexBuffer(VkDevice& device);

        void createVertexBuffer(VulkanApp* app, std::vector<Vertex>& vertices);
        void createIndexBuffer(VulkanApp* app, std::vector<uint32_t>& indices);

    public:
        ModelBuffer();

        void setIndexed(bool indexed);
        bool indexed() const;

        void bind(VkCommandBuffer& commandBuffer);
        void draw(VkCommandBuffer& commandBuffer);

        void loadVertices(VulkanApp* app, std::vector<Vertex>& vertices);
        void loadIndices(VulkanApp* app, std::vector<uint32_t>& indices);
        virtual void cleanup(VkDevice& device);

        VkBuffer getVertexBuffer();
        uint32_t getVertexCount() const;
        uint32_t getIndexCount() const;
        VkBuffer getIndexBuffer();

        friend class GeneralBufferManager;
        friend class SimpleObj;
    };

    class GeneralBufferManager : public ResourceUser {
    private:
        VkDevice device;
    public:
        GeneralBufferManager(VulkanApp* app, ResourceHelper& resourceHelper);

        std::shared_ptr<UniformBuffer> createUniformBuffer(VkDeviceSize bufferSize);

        std::shared_ptr<ModelBuffer> createModelBuffer();
        std::shared_ptr<jk::ModelBuffer> genCube(float size = 1.0f);
        std::shared_ptr<jk::ModelBuffer> genDoublePlane(float size = 1.0f);
        std::shared_ptr<jk::ModelBuffer> genSphere(float radius = 1.0f, uint32_t rings = 32, uint32_t sectors = 32);

        inline void loadVerticesOntoBuffer(std::shared_ptr<ModelBuffer>& buf, std::vector<Vertex>& vertices) {
            buf->loadVertices(app, vertices);
        }

        inline void loadIndicesOntoBuffer(std::shared_ptr<ModelBuffer>& buf, std::vector<uint32_t>& indices) {
            buf->loadIndices(app, indices);
        }

        std::shared_ptr<UniformBuffer> getUniformBuffer(uint32_t resID);
        std::shared_ptr<ModelBuffer> getModelBuffer(uint32_t resID);

        void destroyUniformBuffer(uint32_t resID);
        void destroyModelBuffer(uint32_t resID);

        void cleanup();
    };
}
#endif //VULKANTEST_BUFFER_H
