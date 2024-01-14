//
// Created by MasterLong on 2023/11/29.
//

#include "Texture.h"
#include "Buffer.h"
#include "VulkanApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace jk {

    void TextureManager::createTextureImage(void *data, int width, int height, std::shared_ptr<Texture> &texture, bool useMipmap, MipMapSamplerInfo samplerInfo) {
        
        texture->useMipmap = useMipmap;
        texture->samplerInfo = samplerInfo;
        texture->mipLevels = useMipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;   

        VkDeviceSize imageSize = width * height * 4;
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        app->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* bufferData;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &bufferData);
        
        memcpy(bufferData, data, static_cast<size_t>(imageSize));
        
        vkUnmapMemory(device, stagingBufferMemory);

        auto useFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (useMipmap)
            useFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        
        app->createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                    useFlags,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->baseInfo.image, texture->baseInfo.imageMemory, texture->mipLevels);
        
        transitionImageLayout(texture->baseInfo.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->mipLevels);
        
        copyBufferToImage(stagingBuffer, texture->baseInfo.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        
        if (!useMipmap)
            transitionImageLayout(texture->baseInfo.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture->mipLevels);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        if (useMipmap)
            generateMipmaps(texture->baseInfo.image, VK_FORMAT_R8G8B8A8_SRGB, width, height, texture->mipLevels);
        
    }

    void TextureManager::createTextureImage(const std::string& filePath, std::shared_ptr<Texture>& texture, bool useMipmap, MipMapSamplerInfo samplerInfo) {

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        
        createTextureImage(pixels, texWidth, texHeight, texture, useMipmap, samplerInfo);
    }

    void TextureManager::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // 检查是否支持线性过滤
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(app->getPhysicalDevice(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        app->getCommandManager()->excuteCommand([&](VkCommandBuffer &commandBuffer)
                                                {
                                                    // 设置内存屏障

                                                    VkImageMemoryBarrier barrier{};
                                                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                                    barrier.image = image;
                                                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                                    barrier.subresourceRange.baseArrayLayer = 0;
                                                    barrier.subresourceRange.layerCount = 1;
                                                    barrier.subresourceRange.levelCount = 1;

                                                    int32_t mipWidth = texWidth;
                                                    int32_t mipHeight = texHeight;

                                                    for (uint32_t i = 1; i < mipLevels; i++)
                                                    {
                                                        barrier.subresourceRange.baseMipLevel = i - 1;
                                                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                                                        vkCmdPipelineBarrier(commandBuffer,
                                                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                                                            0, nullptr,
                                                                            0, nullptr,
                                                                            1, &barrier);

                                                        VkImageBlit blit{};
                                                        blit.srcOffsets[0] = {0, 0, 0};
                                                        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                                                        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                                        blit.srcSubresource.mipLevel = i - 1;
                                                        blit.srcSubresource.baseArrayLayer = 0;
                                                        blit.srcSubresource.layerCount = 1;
                                                        blit.dstOffsets[0] = {0, 0, 0};
                                                        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                                                        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                                        blit.dstSubresource.mipLevel = i;
                                                        blit.dstSubresource.baseArrayLayer = 0;
                                                        blit.dstSubresource.layerCount = 1;

                                                        vkCmdBlitImage(commandBuffer,
                                                                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                    1, &blit,
                                                                    VK_FILTER_LINEAR);

                                                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                                        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                                        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                                                        vkCmdPipelineBarrier(commandBuffer,
                                                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                                                            0, nullptr,
                                                                            0, nullptr,
                                                                            1, &barrier);

                                                        if (mipWidth > 1)
                                                            mipWidth /= 2;
                                                        if (mipHeight > 1)
                                                            mipHeight /= 2;
                                                    }

                                                    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
                                                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                                                    vkCmdPipelineBarrier(commandBuffer,
                                                                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                                                        0, nullptr,
                                                                        0, nullptr,
                                                                        1, &barrier);
                                                });
    }


    void TextureManager::createTextureImageView(std::shared_ptr<Texture>& texture, VkFormat format, VkImageAspectFlags aspectFlags) {
        texture->baseInfo.imageView = app->createImageView(texture->baseInfo.image, format, aspectFlags, texture->mipLevels);
    }

    void TextureManager::createTextureSampler(std::shared_ptr<Texture>& texture) {

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO; // 类型
        samplerInfo.magFilter = VK_FILTER_LINEAR; // 放大过滤器
        samplerInfo.minFilter = VK_FILTER_LINEAR; // 缩小过滤器
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // U轴寻址模式
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // V轴寻址模式
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // W轴寻址模式
        samplerInfo.anisotropyEnable = VK_TRUE; // 启用各向异性过滤
        samplerInfo.maxAnisotropy = 16; // 最大各向异性过滤
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // 边框颜色
        samplerInfo.unnormalizedCoordinates = VK_FALSE; // 标准化坐标
        samplerInfo.compareEnable = VK_FALSE; // 比较启用
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS; // 比较操作
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // mipmap模式
        samplerInfo.mipLodBias = texture->useMipmap ? texture->samplerInfo.mipLodBias : 0.0f; // mipmap偏移
        samplerInfo.minLod = texture->useMipmap ? static_cast<float>(texture->mipLevels) * texture->samplerInfo.minLodRate : 0.0f; // mipmap最小层级
        samplerInfo.maxLod = texture->useMipmap ? static_cast<float>(texture->mipLevels) * texture->samplerInfo.maxLodRate : 0.0f; // mipmap最大层级

        if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void TextureManager::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {

        app->getCommandManager()->excuteCommand([&](VkCommandBuffer& commandBuffer)
            {
                // 设置内存屏障
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; // 类型
                barrier.oldLayout = oldLayout; // 旧布局
                barrier.newLayout = newLayout; // 新布局
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // 源队列族索引
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // 目标队列族索引
                barrier.image = image; // 图像
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 图像方面
                barrier.subresourceRange.baseMipLevel = 0; // mipmap基层级
                barrier.subresourceRange.levelCount = mipLevels; // mipmap层级数量
                barrier.subresourceRange.baseArrayLayer = 0; // 图像基数组层
                barrier.subresourceRange.layerCount = 1; // 图像数组层数量

                // 设置屏障访问掩码
                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                // 未定义 -> 传输目标
                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    barrier.srcAccessMask = 0; // 源访问掩码
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // 目标访问掩码

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // 源阶段
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // 目标阶段
                }
                // 传输目标 -> 片段着色器只读
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // 源访问掩码
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // 目标访问掩码

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // 源阶段
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // 目标阶段
                }
                else {
                    throw std::invalid_argument("unsupported layout transition!");
                }

                // 执行屏障
                vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                                    &barrier); 
        });
    }


    void TextureManager::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        app->getCommandManager()->excuteCommand([&](VkCommandBuffer& commandBuffer)
            {
                // 设置拷贝区域
                VkBufferImageCopy region{};
                region.bufferOffset = 0; // 缓冲偏移
                region.bufferRowLength = 0; // 缓冲行长度
                region.bufferImageHeight = 0; // 缓冲图像高度
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 图像方面
                region.imageSubresource.mipLevel = 0; // mipmap层级
                region.imageSubresource.baseArrayLayer = 0; // 图像基数组层
                region.imageSubresource.layerCount = 1; // 图像数组层数量
                region.imageOffset = {0, 0, 0}; // 图像偏移
                region.imageExtent = {width, height, 1}; // 图像范围

                // 执行拷贝
                vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        });
    }

    // Texture::Texture() {
    //     imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //     imageInfo.imageView = textureImageView;
    //     imageInfo.sampler = textureSampler;
    // }

    void Texture::cleanup(VkDevice& device) {

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, baseInfo.imageView, nullptr);
        vkDestroyImage(device, baseInfo.image, nullptr);
        vkFreeMemory(device, baseInfo.imageMemory, nullptr);
    }

    TextureManager::TextureManager(VulkanApp *app, ResourceHelper& resourceHelper) : ResourceUser(app) {
        this->device = app->getDevice();
    }

    // load Texture

    void TextureManager::wrapTexture(std::shared_ptr<Texture>& texture) {
        createTextureImageView(texture);
        createTextureSampler(texture);

        texture->imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture->imageInfo.imageView = texture->baseInfo.imageView;
        texture->imageInfo.sampler = texture->textureSampler;
    }

    std::shared_ptr<Texture> TextureManager::loadTexture(const std::string& filePath, bool useMipmap, MipMapSamplerInfo samplerInfo) {
        auto texture = std::make_shared<Texture>();
        resourceHelper.createResource(std::static_pointer_cast<IResource>(texture));
        createTextureImage(filePath, texture, useMipmap, samplerInfo);
        wrapTexture(texture);
        return texture;
    }

    std::shared_ptr<Texture> TextureManager::loadTexture(void *data, int width, int height, bool useMipmap, MipMapSamplerInfo samplerInfo) {
        auto texture = std::make_shared<Texture>();
        resourceHelper.createResource(std::static_pointer_cast<IResource>(texture));
        createTextureImage(data, width, height, texture, useMipmap, samplerInfo);
        wrapTexture(texture);
        return texture;
    }

    std::shared_ptr<Texture> TextureManager::createFilledTexture(int width, int height, glm::vec3 color, bool useMipmap, MipMapSamplerInfo samplerInfo) {
        auto texture = std::make_shared<Texture>();
        resourceHelper.createResource(std::static_pointer_cast<IResource>(texture));
        auto data = new unsigned char[width * height * 4];
        for (int i = 0; i < width * height; i++) {
            data[i * 4] = static_cast<unsigned char>(color.r * 255);
            data[i * 4 + 1] = static_cast<unsigned char>(color.g * 255);
            data[i * 4 + 2] = static_cast<unsigned char>(color.b * 255);
            data[i * 4 + 3] = 255;
        }
        createTextureImage(data, width, height, texture, useMipmap, samplerInfo);
        wrapTexture(texture);
        delete[] data;
        return texture;
    }

    // get Texture
    std::shared_ptr<Texture> TextureManager::getTexture(uint32_t resID) {
        return std::static_pointer_cast<Texture>(resourceHelper.getResource(resID));
    }

    // destroy Texture
    void TextureManager::destroyTexture(uint32_t resID) {
        resourceHelper.destroyResource(resID, device);
    }

    void TextureManager::cleanup() {
        // 清空纹理
        // resourceHelper.cleanup(device);
    }

    TextureManager::~TextureManager() {
        cleanup();
    }

    VkDescriptorImageInfo& Texture::getImageInfo() {
        return imageInfo;
    }

}