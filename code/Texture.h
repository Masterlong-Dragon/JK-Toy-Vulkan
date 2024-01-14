//
// Created by MasterLong on 2023/11/29.
//

#ifndef VULKANTEST_TEXTURE_H
#define VULKANTEST_TEXTURE_H

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
#include "Descriptor.h"

namespace jk {

    class VulkanApp;

    // 0到1
    struct MipMapSamplerInfo {
        float mipLodBias;
        float minLodRate;
        float maxLodRate;
    };

    struct TextureBaseInfo {
        VkImage image;
        VkImageView imageView;
        VkDeviceMemory imageMemory;
    };

    class Texture : public IResource {
    public:
        VkDescriptorImageInfo& getImageInfo();

        inline void fillImageDescriptorSets(std::shared_ptr<DescriptorSets> descriptorSets, uint32_t binding) {
            uint32_t size = descriptorSets->getDescriptorSets().size();
            for (uint32_t i = 0; i < size; i++) {
                descriptorSets->writer.writeImage(binding, &imageInfo).overwrite(i).flush();
            }
        }

        inline bool isUseMipmap() const {
            return useMipmap;
        }

        inline uint32_t getMipLevels() const {
            return mipLevels;
        }

        inline MipMapSamplerInfo& getSamplerInfo() {
            return samplerInfo;
        }

        inline TextureBaseInfo& getBaseInfo() {
            return baseInfo;
        }

        inline VkSampler& getTextureSampler() {
            return textureSampler;
        }

        inline int getWidth() const {
            return width;
        }

        inline int getHeight() const {
            return height;
        }

        virtual void cleanup(VkDevice &device);
    private:
        bool useMipmap = false;
        uint32_t mipLevels;
        MipMapSamplerInfo samplerInfo;

        TextureBaseInfo baseInfo;
        VkSampler textureSampler;
        VkDescriptorImageInfo imageInfo;

        int width;
        int height;

        // Texture(const std::string& filePath);
        // ~Texture();
        friend class TextureManager;
    };

    class TextureManager : public ResourceUser {
    public:
        TextureManager(VulkanApp* app, ResourceHelper& resourceHelper);
        ~TextureManager();

        std::shared_ptr<Texture> createTexture();
        std::shared_ptr<Texture> loadTexture(const std::string& filePath, bool useMipmap = false, MipMapSamplerInfo samplerInfo = {0.0f, 0.0f, 1.0f});
        std::shared_ptr<Texture> loadTexture(void *data, int width, int height, bool useMipmap = false, MipMapSamplerInfo samplerInfo = {0.0f, 0.0f, 1.0f});
        std::shared_ptr<Texture> createFilledTexture(int width, int height, glm::vec3 color, bool useMipmap = false, MipMapSamplerInfo samplerInfo = {0.0f, 0.0f, 1.0f});
        std::shared_ptr<Texture> getTexture(uint32_t resID);
        void destroyTexture(uint32_t resID);

        void cleanup();

        friend class SwapChain;
    private:

        VkDevice device;

        void initDepthResource();
        void destroyDepthResource();

        void createTextureImage(void *data, int width, int height, std::shared_ptr<Texture> &texture, bool useMipmap, MipMapSamplerInfo samplerInfo);
        void createTextureImage(const std::string& filePath, std::shared_ptr<Texture>& texture, bool useMipmap, MipMapSamplerInfo samplerInfo);
        void createTextureImageView(std::shared_ptr<Texture>& texture, 
                                    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, 
                                    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
        void createTextureSampler(std::shared_ptr<Texture>& texture);
        void wrapTexture(std::shared_ptr<Texture>& texture);

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
        void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    };

}

#endif //VULKANTEST_TEXTURE_H