#ifndef VULKANTEST_RENDERBATCHMANAGER_H
#define VULKANTEST_RENDERBATCHMANAGER_H

#include "RenderObject.hpp"

namespace jk {
    class RenderBatch : public IResource{
    protected:
        ResourceHelper renderObjectPool;
        std::vector<std::shared_ptr<DescriptorSets>> descriptorSets;
        std::array<std::vector<VkDescriptorSet>, 2> descriptorSetsGroup;
        std::shared_ptr<jk::DescriptorSets> globalDescriptorSet;
        uint32_t descriptorCount = 2;
        uint32_t label;
    public:
        RenderBatch(uint32_t label) { this->label = label; }

        inline void addRenderObject(std::shared_ptr<RenderObject> renderObject) {
            renderObjectPool.createResource(std::static_pointer_cast<IResource>(renderObject));
        }

        inline std::shared_ptr<RenderObject> getRenderObject (uint32_t resID) {
            return std::static_pointer_cast<RenderObject>(renderObjectPool.getResource(resID));
        }

        inline void destroyRenderObject(uint32_t resID, VkDevice& device) {
            renderObjectPool.destroyResource(resID, device);
        }

        inline std::vector<std::shared_ptr<DescriptorSets>>& getDescriptorSets() {
            return descriptorSets;
        }

        inline void setGlobalDescriptorSet(std::shared_ptr<jk::DescriptorSets> globalDescriptorSet) {
            this->globalDescriptorSet = globalDescriptorSet;
        }

        inline std::shared_ptr<jk::DescriptorSets> getGlobalDescriptorSet() {
            return globalDescriptorSet;
        }

        inline uint32_t getLabel() const{
            return label;
        }

        inline void updateDescriptorSets() {
            for (int i = 0; i < descriptorCount; i++) {
                auto& descriptorSetGroup = descriptorSetsGroup[i];
                descriptorSetGroup.clear();
                if (globalDescriptorSet != nullptr)
                    descriptorSetGroup.push_back(globalDescriptorSet->getDescriptorSets()[i]);
                for (auto& set: descriptorSets) {
                    descriptorSetGroup.push_back(set->getDescriptorSets()[i]);
                }
            }
        }

        void drawBatchInternal(CommandManager &commandManager, Shader &shader, FrameInfo &frame) {
            for (auto& [resID, renderObject] : renderObjectPool.getResources()) {
                auto obj = std::static_pointer_cast<RenderObject>(renderObject);
                obj->draw(commandManager, shader, frame);
            }
        }

        // 如果不使用BatchManager，需要手动绑定descriptorSets
        void drawBatch(CommandManager &commandManager, Shader &shader, FrameInfo &frame) {
            // std::array<VkDescriptorSet, 2> descriptorSetsGroup = {globalDescriptorSets->getDescriptorSets()[frame.currentFrame], descriptorSets->getDescriptorSets()[frame.currentFrame]};
            vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                    shader.getPipelineLayout(), 0, descriptorSetsGroup[frame.currentFrame].size(), 
                                    descriptorSetsGroup[frame.currentFrame].data(), 0, nullptr);
            drawBatchInternal(commandManager, shader, frame);
        }

        void cleanup(VkDevice &device);

        friend class RenderBatchManager;
    };

    class RenderBatchManager : public ResourceUser {
    private:
        VkDevice device;
        CommandManager& commandManager;
        Shader& shader;
        std::unordered_map<uint32_t, bool> renderBatchMap;
        std::array<VkDescriptorSet, 2> descriptorSetsGroup;

        inline std::shared_ptr<RenderBatch> getRenderBatch(uint32_t resID) {
            return std::static_pointer_cast<RenderBatch>(resourceHelper.getResource(resID));
        }

        inline void destroyRenderBatch(uint32_t resID) {
            resourceHelper.destroyResource(resID, device);
        }
    public:
        RenderBatchManager(VulkanApp *app, Shader& shader);
        RenderBatchManager(VulkanApp *app, ResourceHelper& resourcePool, Shader& shader);

        inline std::shared_ptr<RenderBatch> createRenderBatch(uint32_t label) {
            auto renderBatch = std::make_shared<RenderBatch>(label);
            resourceHelper.createResource(std::static_pointer_cast<IResource>(renderBatch));
            return renderBatch;
        }

        void addRenderObject(std::shared_ptr<RenderObject> renderObject, std::shared_ptr<RenderBatch> renderBatch);
        void removeRenderObject(std::shared_ptr<RenderObject> renderObject);
        std::shared_ptr<RenderObject> getRenderObject(uint32_t batchID, uint32_t resID);

        void drawBatches(FrameInfo &frame);
    };


    // struct LightBufferObject {
    //     PointLight pointLights[16];
    //     alignas(16) int numLights;
    // };

    // class LightRenderBatch : public RenderBatch {
    // private:
    //     std::shared_ptr<UniformBuffer> lightBuffer;
    //     VkDescriptorSetLayoutBinding lightLayoutBinding;
    // public:
    //     LightRenderBatch(GeneralBufferManager& bufferAllocator, uint32_t binding = 0) {
    //         lightBuffer = bufferAllocator.createUniformBuffer(sizeof(LightBufferObject));
    //         lightLayoutBinding = jk::DescriptorSetLayout::uniformDescriptorLayoutBinding(binding);
    //     }

    //     void initDescriptorSets(DescriptorPool& descriptorPool, VkDescriptorSetLayout layout) {
    //         descriptorSets = descriptorPool.createDescriptorSets();
    //         descriptorSets->init(layout);
    //         lightBuffer->fillUniformDescriptorSets(descriptorSets, lightLayoutBinding.binding);
    //     }

    //     void updateLightBuffer(FrameInfo& frame) {
    //         LightBufferObject ubo{};
    //         ubo.numLights = 0;
    //         for (auto& [resID, renderObject] : renderObjectPool.getResources()) {
    //             auto light = std::static_pointer_cast<PointLightObject>(renderObject);
    //             if (light != nullptr) {
    //                 ubo.pointLights[ubo.numLights++] = light->getPointLight();
    //             }
    //         }
    //         lightBuffer->updateUniformBuffer(frame.currentFrame, &ubo);
    //     }
    // };
}

#endif //VULKANTEST_RENDERBATCHMANAGER_H