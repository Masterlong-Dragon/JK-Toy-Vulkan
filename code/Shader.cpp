//
// Created by MasterLong on 2023/11/27.
//

#include "Shader.h"
#include "fileHelper.hpp"
#include "VulkanApp.h"

namespace jk {

    VkShaderModule Shader::createShaderModule(VkDevice device, const std::vector<char> &code) {

        if (code.size() == 0) {
            return VK_NULL_HANDLE;
        }

        // 设置着色器模块信息
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // 设置字节码大小
        createInfo.codeSize = code.size();
        // 设置字节码
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        // 创建着色器模块
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            // 创建失败
            throw std::runtime_error("failed to create shader module!");
        }

        // 返回着色器模块
        return shaderModule;
    }

    void Shader::cleanup(VkDevice& device) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
    }

    void Shader::bind(VkCommandBuffer& commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    ShaderManager::ShaderManager(VulkanApp *app, ResourceHelper& resourceHelper) : ResourceUser(app, resourceHelper) {
        device = app->getDevice();
    }

    std::shared_ptr<Shader>& ShaderManager::wrapShader(std::shared_ptr<Shader>& shader, 
                                                        VkDescriptorSetLayout* layouts, uint32_t layoutCount,
                                                        PushConstantInfo pushConstantInfo) {
        
        shader->pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        shader->pipelineLayoutInfo.setLayoutCount = layoutCount;
        shader->pipelineLayoutInfo.pSetLayouts = layouts;
        shader->pipelineLayoutInfo.pushConstantRangeCount = 0;

        shader->_descriptorSetLayouts = std::vector<VkDescriptorSetLayout>(layouts, layouts + layoutCount);

        if (pushConstantInfo.pushConstantRanges != nullptr) {
            // 创建push constant

            shader->pipelineLayoutInfo.pushConstantRangeCount = pushConstantInfo.pushConstantRangeCount;
            shader->pipelineLayoutInfo.pPushConstantRanges = pushConstantInfo.pushConstantRanges;
        }

        if (vkCreatePipelineLayout(device, &shader->pipelineLayoutInfo, nullptr, &shader->pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // app->getRenderProcess()->createGraphicsPipeline(*shader);
        return shader;
    }

    std::shared_ptr<Shader> ShaderManager::createShader(const std::vector<char> &vertexSource, const std::vector<char> &fragSource, 
                                                        VkDescriptorSetLayout* layouts, uint32_t layoutCount,
                                                        PushConstantInfo pushConstantInfo) {
        auto shader = std::make_shared<Shader>();
        shader->fragShaderModule = Shader::createShaderModule(device, fragSource);
        shader->vertShaderModule = Shader::createShaderModule(device, vertexSource);
        resourceHelper.createResource(std::static_pointer_cast<IResource>(shader));
        return wrapShader(shader, layouts, layoutCount, pushConstantInfo);
    }

    std::shared_ptr<Shader> ShaderManager::createShader(const std::string &vertexPath, const std::string &fragPath,
                                                        VkDescriptorSetLayout* layouts, uint32_t layoutCount,
                                                        PushConstantInfo pushConstantInfo) {
        auto shader = std::make_shared<Shader>();
        
        // 考虑path为空 就不生成shaderModule
        if (vertexPath.size() != 0) {
            shader->vertShaderModule = Shader::createShaderModule(device, readFile(vertexPath));
        }
        if (fragPath.size() != 0) {
            shader->fragShaderModule = Shader::createShaderModule(device, readFile(fragPath));
        }
        resourceHelper.createResource(std::static_pointer_cast<IResource>(shader));
        return wrapShader(shader, layouts, layoutCount, pushConstantInfo);
    }

    std::shared_ptr<Shader> ShaderManager::getShader(uint32_t resID) {
        return std::static_pointer_cast<Shader>(resourceHelper.getResource(resID));
    }

    void ShaderManager::destroyShader(uint32_t resID) {
        resourceHelper.destroyResource(resID, device);
    }

    void ShaderManager::cleanup() {
        // resourceHelper.cleanup(device);
    }

}