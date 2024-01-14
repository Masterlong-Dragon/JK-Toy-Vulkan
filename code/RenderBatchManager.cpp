#include "RenderBatchManager.h"
#include "VulkanApp.h"

namespace jk {

    void RenderBatch::cleanup(VkDevice& device){
        for (auto& [resID, renderObject] : renderObjectPool.getResources()) {
            std::static_pointer_cast<RenderObject>(renderObject)->cleanup(device);
        }
    }

    RenderBatchManager::RenderBatchManager(VulkanApp* app, Shader& shader) : 
    ResourceUser(app),
    commandManager(*app->getCommandManager()), shader(shader) {
            device = app->getDevice();
        }

    RenderBatchManager::RenderBatchManager(VulkanApp *app, ResourceHelper& resourcePool, Shader& shader) :
    ResourceUser(app, resourcePool),
    commandManager(*app->getCommandManager()), shader(shader) {
            device = app->getDevice();
        }

    void RenderBatchManager::addRenderObject(std::shared_ptr<RenderObject> renderObject, std::shared_ptr<RenderBatch> renderBatch) {
        auto renderBatchID = renderBatch->getID();
        if (renderBatchMap.find(renderBatchID) == renderBatchMap.end()) {
            renderBatchMap[renderBatchID] = true;
        }
        renderObject->renderBatchID = renderBatchID;
        getRenderBatch(renderBatchID)->addRenderObject(renderObject);
    }

    void RenderBatchManager::removeRenderObject(std::shared_ptr<RenderObject> renderObject) {
        auto renderBatchID = renderObject->renderBatchID;
        auto renderBatch = getRenderBatch(renderBatchID);
        if (renderBatch == nullptr) {
            return;
        }
        renderBatch->destroyRenderObject(renderObject->getID(), device);
    }

    std::shared_ptr<RenderObject> RenderBatchManager::getRenderObject(uint32_t batchID, uint32_t resID) {
        auto renderBatch = getRenderBatch(batchID);
        if (renderBatch == nullptr) {
            return nullptr;
        }
        return renderBatch->getRenderObject(resID);
    }

    void RenderBatchManager::drawBatches(FrameInfo &frame) {
        shader.bind(frame.commandBuffer);
        for (auto& [batchID, pair] : renderBatchMap) {
            auto renderBatch = std::static_pointer_cast<RenderBatch>(resourceHelper.getResource(batchID));
            // auto bacthDescriptors = renderBatch->descriptorSets;
            // descriptorSetsGroup[1] = bacthDescriptors->getDescriptorSets()[frame.currentFrame];
            // vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            //                         shader.getPipelineLayout(), 0, descriptorSetsGroup.size(), descriptorSetsGroup.data(), 0, nullptr);
            renderBatch->drawBatch(commandManager, shader, frame);
        }
    }
}