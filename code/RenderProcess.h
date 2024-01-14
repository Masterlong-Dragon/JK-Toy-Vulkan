//
// Created by MasterLong on 2023/9/17.
//

#ifndef VULKANTEST_RENDERPROCESS_H
#define VULKANTEST_RENDERPROCESS_H

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>

#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"

namespace jk {

    struct FrameInfo;

    class VulkanApp;

    class AbstractRenderProcess {
    protected:
        VulkanApp *app;
        VkDevice device;

        VkRenderPassBeginInfo renderPassInfo{};
        VkClearValue clearValues[2] = {};

        virtual void createRenderPass() = 0;

    public:
        AbstractRenderProcess(VulkanApp *app);

        virtual void init() = 0;

        virtual void cleanup() = 0;

        virtual void beginRenderPass(FrameInfo& frameInfo) = 0;

        virtual void endRenderPass(FrameInfo& frameInfo) = 0;

        virtual void createGraphicsPipeline(Shader& shader) = 0;
    };

    class RenderProcess : public AbstractRenderProcess {
    private:
        SwapChain swapChain;

        VkRenderPass renderPass;

    protected:
        virtual void createRenderPass();

    public:
        RenderProcess(VulkanApp *app);

        virtual void init();
        virtual void cleanup();
        virtual void createGraphicsPipeline(Shader& shader);

        void setClearColor(VkClearColorValue clearColor);

        virtual void beginRenderPass(FrameInfo& frameInfo);
        virtual void endRenderPass(FrameInfo& frameInfo);

        inline VkRenderPass getRenderPass() {
            return renderPass;
        }

        inline SwapChain &getSwapChain() {
            return swapChain;
        }

        void recreate();
    };


    #define SHADOWMAP_DIM 2048
    #define SHADOW_DEPTH_FORMAT VK_FORMAT_D16_UNORM


    typedef struct {
        glm::mat4 depthVP;
    } DepthVP;


    class OffscreenRenderProcess : public AbstractRenderProcess {
    private:
        // offscreen 为阴影准备
        struct OffscreenPass {
            unsigned int width, height;
            TextureBaseInfo depth;
            VkFramebuffer frameBuffer;
            VkRenderPass renderPass;
            VkSampler depthSampler;
            VkDescriptorImageInfo descriptor;
        } offscreenPass;

        // shadow mapping
	    bool filterPCF = true;

        // Depth bias (and slope) are used to avoid shadowing artifacts
        // Constant depth bias factor (always applied)
        float depthBiasConstant = 1.25f;
        // Slope depth bias factor, applied depending on polygon's slope
        float depthBiasSlope = 1.75f;

        VkViewport viewport{};
        VkRect2D scissor{};

        void createOffscreenFramebuffer();
    protected:
        virtual void createRenderPass();
    public:
        OffscreenRenderProcess(VulkanApp *app);

        virtual void init();
        virtual void cleanup();
        virtual void createGraphicsPipeline(Shader& shader);

        virtual void beginRenderPass(FrameInfo& frameInfo);
        virtual void endRenderPass(FrameInfo& frameInfo);

        void fillImageDescriptorSets(std::shared_ptr<DescriptorSets> descriptorSets, uint32_t binding);

        inline VkRenderPass getRenderPass() {
            return offscreenPass.renderPass;
        }

        inline VkSampler getDepthSampler() {
            return offscreenPass.depthSampler;
        }

        inline VkDescriptorImageInfo& getDescriptor() {
            return offscreenPass.descriptor;
        }

        inline TextureBaseInfo& getDepth() {
            return offscreenPass.depth;
        }

        inline VkFramebuffer getFramebuffer() {
            return offscreenPass.frameBuffer;
        }

        // void recreate();
    };

}
#endif //VULKANTEST_RENDERPROCESS_H
