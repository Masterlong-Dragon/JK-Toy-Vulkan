//
// Created by MasterLong on 2023/11/26.
//

#ifndef VULKANTEST_DESCRIPTOR_H
#define VULKANTEST_DESCRIPTOR_H

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>

#include "Shader.h"
#include "ResourceHelper.hpp"

namespace jk {

    class VulkanApp;

    class DescriptorPool;

    class DescriptorSets : public IResource {
    private:
        std::vector<VkDescriptorSet> descriptorSets;
        DescriptorPool *poolWrapper;

        void createDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout);
    public:
        void init(VkDescriptorSetLayout& descriptorSetLayout);
        void bind(Shader& shader, VkCommandBuffer& commandBuffer, uint32_t currentFrame);
        
        inline std::vector<VkDescriptorSet>& getDescriptorSets() {
            return descriptorSets;
        }

        void cleanup(VkDevice &device);

        // 我觉得我写得毫无设计模式可言（
        // 不能怪我 vulkan tutorial教得顺序导致我没有空间进行工程化重构设计 
        class DescriptorSetWriter{
        private:
            std::vector<VkWriteDescriptorSet> descriptorWriters;
            DescriptorSets* descriptorSets;
        public:
            DescriptorSetWriter(DescriptorSets* descriptorSets) : descriptorSets(descriptorSets) {}

            DescriptorSetWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
            DescriptorSetWriter& writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);
            DescriptorSetWriter& overwrite(int index);
            DescriptorSetWriter& flush();
            DescriptorSetWriter& updateBuffer(size_t index, VkDescriptorBufferInfo *bufferInfo);
            DescriptorSetWriter& updateImage(size_t index, VkDescriptorImageInfo *imageInfo);           
        };

        friend class DescriptorSetWriter;

        DescriptorSetWriter writer;

        DescriptorSets(DescriptorPool *poolWrapper);
    };

    class DescriptorSetLayout : public IResource {
    private:

        VkDescriptorSetLayout descriptorSetLayout;

        static void createSingleDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout, const VkDescriptorSetLayoutBinding *binding);
        static void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo& createInfo, VkDescriptorSetLayout& descriptorSetLayout);
    public:    
        static VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags);
        static inline VkDescriptorSetLayoutBinding uniformDescriptorLayoutBinding(uint32_t binding) {
            return getDescriptorSetLayoutBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }
        static inline VkDescriptorSetLayoutBinding imageDescriptorLayoutBinding(uint32_t binding) {
            return getDescriptorSetLayoutBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        void cleanup(VkDevice& device) override {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }

        inline VkDescriptorSetLayout& getDescriptorSetLayout() {
            return descriptorSetLayout;
        }

        friend class DescriptorPool;
    };

    class DescriptorPool : ResourceUser{
    public:
        DescriptorPool(VulkanApp *app, ResourceHelper& resourceHelper, uint32_t maxSets = 50);
        void init(uint32_t maxSets);
        void cleanup();
        
        inline void fillLayoutsByBindings(std::vector<VkDescriptorSetLayout>& layouts, std::initializer_list<VkDescriptorSetLayoutBinding> bindings) {
            for (auto& binding : bindings) {
                layouts.push_back(createSingleDescriptorSetLayout(&binding)->getDescriptorSetLayout());
            }         
        }

        inline void fillLayoutsByBindings(std::vector<VkDescriptorSetLayout>& layouts, std::vector<VkDescriptorSetLayoutBinding>& bindings) {
            for (auto& binding : bindings) {
                layouts.push_back(createSingleDescriptorSetLayout(&binding)->getDescriptorSetLayout());
            }
        }

        inline std::shared_ptr<DescriptorSetLayout> createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) {
            auto descriptorSetLayout = std::make_shared<DescriptorSetLayout>();
            DescriptorSetLayout::createDescriptorSetLayout(device, createInfo, descriptorSetLayout->descriptorSetLayout);
            resourceHelper.createResource(std::static_pointer_cast<IResource>(descriptorSetLayout));
            return descriptorSetLayout;
        }

        inline std::shared_ptr<DescriptorSetLayout> createSingleDescriptorSetLayout(const VkDescriptorSetLayoutBinding *binding) {
            auto descriptorSetLayout = std::make_shared<DescriptorSetLayout>();
            DescriptorSetLayout::createSingleDescriptorSetLayout(device, descriptorSetLayout->descriptorSetLayout, binding);
            resourceHelper.createResource(std::static_pointer_cast<DescriptorSetLayout>(descriptorSetLayout));
            return descriptorSetLayout;
        }

        inline VkDescriptorPool& getDescriptorPool() {
            return descriptorPool;
        }

        inline std::shared_ptr<DescriptorSets> createDescriptorSets() {
            auto descriptorSets = std::make_shared<DescriptorSets>(this);
            resourceHelper.createResource(std::static_pointer_cast<IResource>(descriptorSets));
            return descriptorSets;
        } 

        friend class DescriptorSets;
    private:
        VkDevice device;
        VkDescriptorPool descriptorPool;
        void createDescriptorPool(uint32_t maxSets);
    };

}
#endif