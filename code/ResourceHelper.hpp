//
// Created by MasterLong on 2023/12/1.
//

#ifndef VULKANTEST_RESOURCEHELPER_H
#define VULKANTEST_RESOURCEHELPER_H

#include <iostream>
#include <memory>
#include <unordered_map>
#include <functional>

#include <vulkan/vulkan.h>

namespace jk {

// 简单的资源管理器

// 软件设计中的继承转使用关系 现学现用了属于是

    class IResource {
    private:
        uint32_t resID;
        bool available = false;
    public:
        virtual void cleanup(VkDevice &device) = 0;
        inline uint32_t getID() const {
            return resID;
        }
        
        inline bool isAvailable() const {
            return available;
        }

        friend class ResourceHelper;
    };

    class ResourceHelper {
    private:
        std::unordered_map<uint32_t, std::shared_ptr<IResource>> resources;
        uint32_t resID = 0;
        std::function<uint32_t(std::shared_ptr<IResource> res)> genID;
    public:

        ResourceHelper() {
            genID = std::bind(&ResourceHelper::genNewResourceID, this, std::placeholders::_1);
        }

        ResourceHelper(std::function<uint32_t(std::shared_ptr<IResource> res)> genID) : genID(genID) {}

        uint32_t genNewResourceID(std::shared_ptr<IResource> res) {
            return resID++;
        }

        std::shared_ptr<IResource> createResource(std::shared_ptr<IResource> resource) {
            uint32_t resID = genID(resource);
            resource->resID = resID;
            resource->available = true;
            resources[resID] = resource;
            return resource;
        }

        std::shared_ptr<IResource> getResource(uint32_t resID) {
            auto it = resources.find(resID);
            if (it == resources.end())
                return nullptr;
            return it->second;
        }

        void destroyResource(uint32_t resID, VkDevice& device) {
            auto it = resources.find(resID);
            if (it == resources.end())
                return;
            it->second->cleanup(device);
            it->second->available = false;
            resources.erase(it);
        }

        void cleanup(VkDevice& device) {
            for (auto& resource : resources) {
                resource.second->cleanup(device);
                resource.second->available = false;
            }
            resources.clear();
        }

        inline std::unordered_map<uint32_t, std::shared_ptr<IResource>>& getResources() {
            return resources;
        }
    };

    class VulkanApp;

    class ResourceUser {
    protected:
        ResourceHelper& resourceHelper;
        VulkanApp *app;
    public:
        ResourceUser(VulkanApp *app);
        ResourceUser(VulkanApp *app, ResourceHelper& resourceHelper) : app(app), resourceHelper(resourceHelper) {}
    };

}

#endif //VULKANTEST_RESOURCEHELPER_H