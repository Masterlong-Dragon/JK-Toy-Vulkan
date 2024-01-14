//
// Created by MasterLong on 2023/10/5.
//

#include "Buffer.h"
#include "Descriptor.h"
#include "VulkanApp.h"

#include <glm/gtx/transform.hpp>

namespace jk {

    std::shared_ptr<ModelBuffer> GeneralBufferManager::genCube(float size) {
        std::vector<Vertex> cubeVertices;

        auto addVertex = [&](glm::vec3 pos, glm::vec3 norm, glm::vec3 col, glm::vec2 tex) {
            cubeVertices.push_back({pos, norm, col, tex});
        };

        float halfSize = size / 2.0f;

        // 前面
        addVertex({-halfSize, -halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, -halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 后面
        addVertex({-halfSize, -halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, -halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 左面
        addVertex({-halfSize, -halfSize, -halfSize}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({-halfSize, -halfSize, halfSize}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({-halfSize, halfSize, halfSize}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, halfSize, -halfSize}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 右面
        addVertex({halfSize, -halfSize, -halfSize}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, -halfSize, halfSize}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, halfSize, halfSize}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({halfSize, halfSize, -halfSize}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 上面
        addVertex({-halfSize, halfSize, -halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, halfSize, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, halfSize, halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, halfSize, halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 下面
        addVertex({-halfSize, -halfSize, -halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, -halfSize, -halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, -halfSize, halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, -halfSize, halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});
        
        // indices
        std::vector<uint32_t> indices =
            {
                0, 1, 2, 2, 3, 0,
                6, 5, 4, 4, 7, 6,
                8, 9, 10, 10, 11, 8,
                14, 13, 12, 12, 15, 14,
                // 上下翻转 vulkan坐标系的问题
                18, 17, 16, 16, 19, 18,
                20, 21, 22, 22, 23, 20
        };

        auto cube = createModelBuffer();
        cube->setIndexed(true);
        cube->loadVertices(app, cubeVertices);
        cube->loadIndices(app, indices);
        return cube;
    }

    std::shared_ptr<ModelBuffer> GeneralBufferManager::genDoublePlane(float size) {
        std::vector<Vertex> cubeVertices;

        auto addVertex = [&](glm::vec3 pos, glm::vec3 norm, glm::vec3 col, glm::vec2 tex) {
            cubeVertices.push_back({pos, norm, col, tex});
        };

        float halfSize = size / 2.0f;

        // 上面
        addVertex({-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // 下面
        addVertex({-halfSize, 0.0f, -halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
        addVertex({halfSize, 0.0f, -halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
        addVertex({halfSize, 0.0f, halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});
        addVertex({-halfSize, 0.0f, halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f});

        // indices
        std::vector<uint32_t> indices = {
                2, 1, 0, 0, 3, 2,
                4, 5, 6, 6, 7, 4
        };

        auto cube = createModelBuffer();
        cube->setIndexed(true);
        cube->loadVertices(app, cubeVertices);
        cube->loadIndices(app, indices);
        return cube;
    }

    std::shared_ptr<jk::ModelBuffer> GeneralBufferManager::genSphere(float radius, uint32_t rings, uint32_t sectors) {
        std::vector<Vertex> sphereVertices;

        auto addVertex = [&](glm::vec3 pos, glm::vec3 norm, glm::vec3 col, glm::vec2 tex) {
            sphereVertices.push_back({pos, norm, col, tex});
        };

        float const R = 1.0f / (float) (rings - 1);
        float const S = 1.0f / (float) (sectors - 1);
        uint32_t r, s;

        for (r = 0; r < rings; r++) {
            for (s = 0; s < sectors; s++) {
                float const y = sin(-M_PI_2 + M_PI * r * R);
                float const x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
                float const z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);

                addVertex({x * radius, y * radius, z * radius}, {x, y, z}, {1.0f, 1.0f, 1.0f}, {1 - s * S, 1.0f - r * R});
            }
        }


        std::vector<uint32_t> indices;
        uint32_t r1, r2;
        for (r = 0; r < rings - 1; r++) {
            for (s = 0; s < sectors - 1; s++) {
                r1 = r * sectors + s;
                r2 = r1 + sectors;

                indices.push_back(r1);
                indices.push_back(r2);
                indices.push_back(r1 + 1);

                indices.push_back(r1 + 1);
                indices.push_back(r2);
                indices.push_back(r2 + 1);
            }
        }

        

        auto sphere = createModelBuffer();
        sphere->setIndexed(true);
        sphere->loadVertices(app, sphereVertices);
        sphere->loadIndices(app, indices);
        return sphere;
    }


    void ModelBuffer::createVertexBuffer(VulkanApp* app, std::vector<Vertex> &vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        app->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                    stagingBufferMemory);

        void *data;
        auto device = app->getDevice();
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        app->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        app->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void ModelBuffer::cleanup(VkDevice& device) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        cleanEndFunc(device);
    }

    void ModelBuffer::cleanIndexBuffer(VkDevice& device) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
    }

    void ModelBuffer::loadVertices(VulkanApp* app, std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        createVertexBuffer(app, vertices);
    }

    VkBuffer ModelBuffer::getVertexBuffer() {
        return vertexBuffer;
    }

    uint32_t ModelBuffer::getVertexCount() const {
        return vertexCount;
    }

    void ModelBuffer::createIndexBuffer(VulkanApp* app, std::vector<uint32_t> &indices) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        app->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                    stagingBufferMemory);

        void *data;
        auto device = app->getDevice();
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        app->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        app->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    VkBuffer ModelBuffer::getIndexBuffer() {
        return indexBuffer;
    }

    void ModelBuffer::loadIndices(VulkanApp* app, std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        createIndexBuffer(app, indices);
    }

    uint32_t ModelBuffer::getIndexCount() const {
        return indexCount;
    }

    void UniformBuffer::cleanup(VkDevice &device) {
        for (size_t i = 0; i < VulkanApp::MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }

    void UniformBuffer::createUniformBuffers(VulkanApp *app, VkDeviceSize bufferSize) {

        this->bufferSize = bufferSize;

        uniformBuffers.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        uniformBufferInfo.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);

        auto device = app->getDevice();

        for (size_t i = 0; i < VulkanApp::MAX_FRAMES_IN_FLIGHT; i++) {
            app->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

            // 创建描述符
            uniformBufferInfo[i].buffer = uniformBuffers[i];
            uniformBufferInfo[i].offset = 0;
            uniformBufferInfo[i].range = bufferSize;
        }
    }

    void UniformBuffer::updateUniformBuffer(uint32_t currentImage, void* ubo) {
        memcpy(uniformBuffersMapped[currentImage], ubo, bufferSize);
    }

    std::vector<VkBuffer> &UniformBuffer::getUniformBuffers() {
        return uniformBuffers;
    }

    std::vector<VkDescriptorBufferInfo> &UniformBuffer::getUniformBufferInfo() {
        return uniformBufferInfo;
    }

    void ModelBuffer::setIndexed(bool indexed) {
        isIndexed = indexed;
        if (indexed) {
            bindFunc = std::bind(&ModelBuffer::indexedBind, this, std::placeholders::_1);
            drawFunc = std::bind(&ModelBuffer::indexedDraw, this, std::placeholders::_1);
            cleanEndFunc = std::bind(&ModelBuffer::cleanIndexBuffer, this, std::placeholders::_1);
        } else {
            bindFunc = std::bind(&ModelBuffer::defaultBind, this, std::placeholders::_1);
            drawFunc = std::bind(&ModelBuffer::defaultDraw, this, std::placeholders::_1);
            cleanEndFunc = [](VkDevice& device) {};
        }
    }

    bool ModelBuffer::indexed() const {
        return isIndexed;
    }

    void ModelBuffer::defaultBind(VkCommandBuffer& commandBuffer) {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    }

    void ModelBuffer::defaultDraw(VkCommandBuffer& commandBuffer) {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }

    void ModelBuffer::indexedBind(VkCommandBuffer& commandBuffer) {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void ModelBuffer::indexedDraw(VkCommandBuffer& commandBuffer) {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    }

    void ModelBuffer::bind(VkCommandBuffer& commandBuffer) {
        bindFunc(commandBuffer);
    }

    void ModelBuffer::draw(VkCommandBuffer& commandBuffer) {
        drawFunc(commandBuffer);
    }

    ModelBuffer::ModelBuffer() {
        bindFunc = std::bind(&ModelBuffer::defaultBind, this, std::placeholders::_1);
        drawFunc = std::bind(&ModelBuffer::defaultDraw, this, std::placeholders::_1);
        // 空操作
        cleanEndFunc = [](VkDevice& device) {};
    }

    GeneralBufferManager::GeneralBufferManager(VulkanApp *app, ResourceHelper& resourceHelper) : ResourceUser(app, resourceHelper) {
        device = app->getDevice();
    }

    std::shared_ptr<UniformBuffer> GeneralBufferManager::createUniformBuffer(VkDeviceSize bufferSize) {
        auto uniformBuffer = std::make_shared<UniformBuffer>();
        uniformBuffer->createUniformBuffers(app, bufferSize);
        resourceHelper.createResource(std::static_pointer_cast<IResource>(uniformBuffer));
        return uniformBuffer;
    }

    std::shared_ptr<ModelBuffer> GeneralBufferManager::createModelBuffer() {
        auto modelBuffer = std::make_shared<ModelBuffer>();
        resourceHelper.createResource(std::static_pointer_cast<IResource>(modelBuffer));
        return modelBuffer;
    }

    std::shared_ptr<UniformBuffer> GeneralBufferManager::getUniformBuffer(uint32_t resID) {
        return std::static_pointer_cast<UniformBuffer>(resourceHelper.getResource(resID));
    }

    std::shared_ptr<ModelBuffer> GeneralBufferManager::getModelBuffer(uint32_t resID) {
        return std::static_pointer_cast<ModelBuffer>(resourceHelper.getResource(resID));
    }

    void GeneralBufferManager::cleanup() {
        // resourceHelper.cleanup(device);
    }

}