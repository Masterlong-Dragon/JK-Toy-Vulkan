//
// Created by MasterLong on 2023/11/27.
//

#ifndef VULKANTEST_SHADER_H
#define VULKANTEST_SHADER_H

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

#include "ResourceHelper.hpp"

namespace jk {

    class VulkanApp;

    struct PushConstantInfo {
        VkPushConstantRange *pushConstantRanges;
        uint32_t pushConstantRangeCount;
    };

    class Shader : public IResource {
    private:
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;

        static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

        virtual void cleanup(VkDevice& device);
    public:

        void bind(VkCommandBuffer& commandBuffer);

        inline VkShaderModule& getVertShaderModule() {
            return vertShaderModule;
        }

        inline VkShaderModule& getFragShaderModule() {
            return fragShaderModule;
        }

        inline VkPipeline& getGraphicsPipeline() {
            return graphicsPipeline;
        }

        inline VkPipelineLayout& getPipelineLayout() {
            return pipelineLayout;
        }

        inline VkPipelineLayoutCreateInfo& getPipelineLayoutInfo() {
            return pipelineLayoutInfo;
        }

        inline VkDescriptorSetLayout* getDescriptorSetLayouts(size_t& count) {
            count = _descriptorSetLayouts.size();
            return _descriptorSetLayouts.data();
        }

        friend class ShaderManager;
    };

    class ShaderManager : public ResourceUser {
    private:
        VkDevice device;

        std::shared_ptr<Shader>& wrapShader(std::shared_ptr<Shader>& shader, 
                                            VkDescriptorSetLayout* layouts, uint32_t layoutCount,
                                            PushConstantInfo pushConstantInfo);
    public:
        ShaderManager(VulkanApp *app, ResourceHelper& resourceHelper);

        std::shared_ptr<Shader> createShader(const std::vector<char> &vertexSource, const std::vector<char> &fragSource, 
                                                VkDescriptorSetLayout* layouts = nullptr, uint32_t layoutCount = 0,
                                                PushConstantInfo pushConstantInfo = {nullptr, 0}
                                                );
        std::shared_ptr<Shader> createShader(const std::string &vertexPath, const std::string &fragPath, 
                                                VkDescriptorSetLayout* layouts = nullptr, uint32_t layoutCount = 0,
                                                PushConstantInfo pushConstantInfo = {nullptr, 0}
                                                );

        std::shared_ptr<Shader> getShader(uint32_t resID);
        void destroyShader(uint32_t resID);

        void cleanup();
    };
}


#endif //VULKANTEST_SHADER_H