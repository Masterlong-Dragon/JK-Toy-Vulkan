//
// Created by MasterLong on 2023/11/26.
//

#include "Descriptor.h"
#include "VulkanApp.h"

namespace jk {

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWriters.push_back(descriptorWrite);
        return *this;
    }

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo) {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = imageInfo;
        descriptorWriters.push_back(descriptorWrite);
        return *this;
    }

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::updateBuffer(size_t index, VkDescriptorBufferInfo *bufferInfo) {
        descriptorWriters[index].pBufferInfo = bufferInfo;
        return *this;
    }

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::updateImage(size_t index, VkDescriptorImageInfo *imageInfo) {
        descriptorWriters[index].pImageInfo = imageInfo;
        return *this;
    }

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::overwrite(int index) {
        if (index >= descriptorSets->descriptorSets.size()) {
            throw std::runtime_error("index out of range");
        }
        if (descriptorWriters.size() == 0) {
            throw std::runtime_error("no descriptor to write");
        }
        for (auto& descriptorWrite : descriptorWriters) {
            descriptorWrite.dstSet = descriptorSets->descriptorSets[index];
        }
        vkUpdateDescriptorSets(descriptorSets->poolWrapper->device, descriptorWriters.size(), descriptorWriters.data(), 0, nullptr);
        return *this;
    }

    DescriptorSets::DescriptorSetWriter& DescriptorSets::DescriptorSetWriter::flush() {
        descriptorWriters.clear();
        return *this;
    }

    DescriptorSets::DescriptorSets(DescriptorPool *poolWrapper) : poolWrapper(poolWrapper), writer(this), descriptorSets(VulkanApp::MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE) {}

    void DescriptorSets::createDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout) {
        std::vector<VkDescriptorSetLayout> layouts(VulkanApp::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolWrapper->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        auto device = poolWrapper->device;

        descriptorSets.resize(VulkanApp::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    void DescriptorSets::init(VkDescriptorSetLayout& descriptorSetLayout) {
        descriptorSets.clear();
        createDescriptorSets(descriptorSetLayout);
    }

    void DescriptorSets::cleanup(VkDevice& device) {
        // vkFreeDescriptorSets(device, poolWrapper->descriptorPool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());      
    }

    void DescriptorSets::bind(Shader& shader, VkCommandBuffer& commandBuffer, uint32_t currentFrame)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        shader.getPipelineLayout(), 0, 1, 
        &descriptorSets[currentFrame], 0, nullptr);
    }

    // pool

    DescriptorPool::DescriptorPool(VulkanApp *app, ResourceHelper& resourceHelper, uint32_t maxSets) : ResourceUser(app, resourceHelper) {
        this->device = app->getDevice();
        init(maxSets);
    }

    void DescriptorPool::init(uint32_t maxSets) {
        createDescriptorPool(maxSets);
    }

    void DescriptorPool::cleanup() {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    void DescriptorPool::createDescriptorPool(uint32_t maxSets) {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(VulkanApp::MAX_FRAMES_IN_FLIGHT) * maxSets;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(VulkanApp::MAX_FRAMES_IN_FLIGHT) * maxSets;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(VulkanApp::MAX_FRAMES_IN_FLIGHT) * maxSets;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void DescriptorSetLayout::createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo& createInfo, VkDescriptorSetLayout& descriptorSetLayout) {
        if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    VkDescriptorSetLayoutBinding DescriptorSetLayout::getDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = descriptorCount;

        layoutBinding.stageFlags = stageFlags;
        layoutBinding.pImmutableSamplers = nullptr;

        return layoutBinding;
    }

    void DescriptorSetLayout::createSingleDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout, const VkDescriptorSetLayoutBinding* bindings) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = bindings;

        DescriptorSetLayout::createDescriptorSetLayout(device, layoutInfo, descriptorSetLayout);
    }

}