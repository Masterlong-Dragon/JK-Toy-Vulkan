//
// Created by MasterLong on 2023/9/17.
//

#include "RenderProcess.h"
#include "fileHelper.hpp"
#include "VulkanApp.h"
#include "Vertex.h"

namespace jk {

    AbstractRenderProcess::AbstractRenderProcess(VulkanApp *app) {
        this->app = app;
        this->device = app->getDevice();
    }

    RenderProcess::RenderProcess(VulkanApp *app) : swapChain(app), AbstractRenderProcess(app) {}

    void RenderProcess::createRenderPass() {
        // 设置描述信息
        bool msaaEnabled = app->isEnableMSAA();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChain.getFormat();
        colorAttachment.samples = app->getMSAASamples();

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = msaaEnabled ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : 
                                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // 深度图
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = app->findDepthFormat();
        depthAttachment.samples = app->getMSAASamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // 颜色图
        VkAttachmentDescription colorAttachmentResolve{};
        if (msaaEnabled)
        {
            colorAttachmentResolve.format = app->getSwapChain()->getFormat();
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        
        // 窗口绘制
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 深度图
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // 从msaa采样恢复 
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        if (msaaEnabled)
            subpass.pResolveAttachments = &colorAttachmentResolveRef;

        // 支持深度测试 
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        auto attachmentCount = msaaEnabled ? attachments.size() : attachments.size() - 1;
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentCount);
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void RenderProcess::createGraphicsPipeline(Shader &shader) {

        // 创建着色器模块
        auto vertShaderModule = shader.getVertShaderModule();
        auto fragShaderModule = shader.getFragShaderModule();

        // 设置着色器阶段信息
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // 设置着色器阶段类型
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        // 设置着色器模块
        vertShaderStageInfo.module = vertShaderModule;
        // 设置着色器入口函数
        vertShaderStageInfo.pName = "main";

        // 设置着色器阶段信息
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // 设置着色器阶段类型
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        // 设置着色器模块
        fragShaderStageInfo.module = fragShaderModule;
        // 设置着色器入口函数
        fragShaderStageInfo.pName = "main";

        // 设置着色器阶段信息
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 设置顶点输入信息
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // 设置输入组装信息
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 设置视口信息
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 设置光栅化信息
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 设置多重采样信息
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = app->getMSAASamples();

        // 设置深度测试信息
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;


        // 设置颜色混合信息
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        // // 混合颜色方式
        // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        // // 混合透明度方式
        // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        // // 混合方式
        // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        

        // 设置颜色混合信息
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 设置动态状态信息
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 设置管线布局信息
        // pipelineLayoutInfo.pushConstantRangeCount = 0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = shader.getPipelineLayout();
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader.getGraphicsPipeline()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // 设置管线信息
        // vkDestroyShaderModule(device, fragShaderModule, nullptr);
        // vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void RenderProcess::setClearColor(VkClearColorValue clearColor) {
        clearValues[0].color = clearColor;
    }

    void RenderProcess::recreate() {
        swapChain.recreate();
        VkExtent2D swapChainExtent = swapChain.getExtent();
        renderPassInfo.renderArea.extent = swapChainExtent;
    }

    void RenderProcess::cleanup() {
        swapChain.cleanup();
        // vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    void RenderProcess::beginRenderPass(FrameInfo &frameInfo) {
        renderPassInfo.framebuffer = swapChain.getFramebuffers()[frameInfo.imageIndex];
        vkCmdBeginRenderPass(frameInfo.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        auto swapChainExtent = swapChain.getExtent();
        // 绘制
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(frameInfo.commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(frameInfo.commandBuffer, 0, 1, &scissor);
    }

    void RenderProcess::endRenderPass(FrameInfo &frameInfo) {
        vkCmdEndRenderPass(frameInfo.commandBuffer);
    }

    void RenderProcess::init() {
        swapChain.init();
        createRenderPass();

        swapChain.createFramebuffers();
        // createGraphicsPipeline();
        auto swapChainExtent = swapChain.getExtent();
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        // renderPassInfo.framebuffer = app->getSwapChain()->getFramebuffers()[frameInfo.imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        // 设置清除颜色
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
        renderPassInfo.pClearValues = clearValues;
    }

    // offscreen part
    OffscreenRenderProcess::OffscreenRenderProcess(VulkanApp *app) : AbstractRenderProcess(app) {}

    void OffscreenRenderProcess::createRenderPass() {
        VkAttachmentDescription attachmentDescription{};
        attachmentDescription.format = SHADOW_DEPTH_FORMAT;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // 由于我们不使用颜色信息，所以我们不需要加载任何内容
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthReference{};
        depthReference.attachment = 0;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthReference;

        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachmentDescription;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenPass.renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void OffscreenRenderProcess::createOffscreenFramebuffer() {
        offscreenPass.width = SHADOWMAP_DIM;
		offscreenPass.height = SHADOWMAP_DIM;

		// For shadow mapping we only need a depth attachment
        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
		image.extent.width = offscreenPass.width;
		image.extent.height = offscreenPass.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.format = VK_FORMAT_D16_UNORM;                                                         // Depth stencil attachment
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
		if (vkCreateImage(device, &image, nullptr, &offscreenPass.depth.image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen image!");
        }

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = app->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate offscreen image memory!");
        }
        
        if (vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.imageMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind offscreen image memory!");
        }

        VkImageViewCreateInfo depthStencilView{};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = VK_FORMAT_D16_UNORM;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;
		depthStencilView.image = offscreenPass.depth.image;
        if (vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen image view!");
        }

		// Create sampler to sample from to depth attachment
		// Used to sample in the fragment shader for shadowed rendering
		VkFilter shadowmap_filter = VK_FILTER_LINEAR;
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = shadowmap_filter;
		sampler.minFilter = shadowmap_filter;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (vkCreateSampler(device, &sampler, nullptr, &offscreenPass.depthSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen sampler!");
        }

        // Create frame buffer

		createRenderPass();

		// Create frame buffer
        VkFramebufferCreateInfo fbufCreateInfo{};
        fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.renderPass = offscreenPass.renderPass;
		fbufCreateInfo.attachmentCount = 1;
		fbufCreateInfo.pAttachments = &offscreenPass.depth.imageView;
		fbufCreateInfo.width = offscreenPass.width;
		fbufCreateInfo.height = offscreenPass.height;
		fbufCreateInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreen framebuffer!");
        }

    }

    void OffscreenRenderProcess::init() {
        createOffscreenFramebuffer();
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = offscreenPass.renderPass;
        renderPassInfo.framebuffer = offscreenPass.frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {offscreenPass.width, offscreenPass.height};
        clearValues[0].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues;

        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)offscreenPass.width;
        viewport.height = (float)offscreenPass.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        scissor.offset = {0, 0};
        scissor.extent = {offscreenPass.width, offscreenPass.height};
    }

    void OffscreenRenderProcess::cleanup() {
        vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
        vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);
        vkDestroySampler(device, offscreenPass.depthSampler, nullptr);
        vkDestroyImageView(device, offscreenPass.depth.imageView, nullptr);
        vkDestroyImage(device, offscreenPass.depth.image, nullptr);
        vkFreeMemory(device, offscreenPass.depth.imageMemory, nullptr);
    }

    void OffscreenRenderProcess::createGraphicsPipeline(Shader &shader) {
        // 对于shadowmap，我们只需要顶点着色器
        auto vertShaderModule = shader.getVertShaderModule();
         // 设置着色器阶段信息
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // 设置着色器阶段类型
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        // 设置着色器模块
        vertShaderStageInfo.module = vertShaderModule;
        // 设置着色器入口函数
        vertShaderStageInfo.pName = "main";

        // 设置着色器阶段信息
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo};

        // 设置顶点输入信息
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // 设置输入组装信息
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 设置视口信息
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 设置光栅化信息
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_TRUE;

        // 设置多重采样信息
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 设置深度测试信息
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // 设置颜色混合信息
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;

        // 设置动态状态信息
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
                VK_DYNAMIC_STATE_DEPTH_BIAS
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 设置管线布局信息
        // pipelineLayoutInfo.pushConstantRangeCount = 0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = shader.getPipelineLayout();
        pipelineInfo.renderPass = offscreenPass.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader.getGraphicsPipeline()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

    void OffscreenRenderProcess::beginRenderPass(FrameInfo &frameInfo) {
       vkCmdBeginRenderPass(frameInfo.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // 绘制
       vkCmdSetViewport(frameInfo.commandBuffer, 0, 1, &viewport);
       vkCmdSetScissor(frameInfo.commandBuffer, 0, 1, &scissor);

        // set bias
       vkCmdSetDepthBias(
            frameInfo.commandBuffer,
            depthBiasConstant,
            0.0f,
            depthBiasSlope);

    }

    void OffscreenRenderProcess::endRenderPass(FrameInfo &frameInfo) {
        vkCmdEndRenderPass(frameInfo.commandBuffer);
    }

    void OffscreenRenderProcess::fillImageDescriptorSets(std::shared_ptr<DescriptorSets> descriptorSets, uint32_t binding) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView = offscreenPass.depth.imageView;
        imageInfo.sampler = offscreenPass.depthSampler;

        uint32_t size = descriptorSets->getDescriptorSets().size();
        for (uint32_t i = 0; i < size; i++) {
            descriptorSets->writer.writeImage(binding, &imageInfo).overwrite(i).flush();
        }
    }
}