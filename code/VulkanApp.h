//
// Created by MasterLong on 2023/9/16.
//

#ifndef VULKANTEST_VULKANAPP_H
#define VULKANTEST_VULKANAPP_H
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vulkan/vulkan.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/gtx/transform.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>
#include <mutex>

#include "QueueFamily.h"
#include "SwapChain.h"
#include "RenderProcess.h"
#include "CommandManager.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceHelper.hpp"

namespace jk {

    class VulkanApp {
    public:
        static const int MAX_FRAMES_IN_FLIGHT = 2;
    private:
        ResourceHelper globalResourcePool;

        // 帧间隔计算
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;

        float _deltaTime = 0.0f;
        float _elapsedTime = 0.0f;

        std::string tittle;

    protected:
        GLFWwindow *window;
        int width;
        int height;

        VkInstance instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        // 超采样
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    //    struct QueueFamilyIndices;
    //    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        // 逻辑设备
        VkDevice device;
        VkQueue graphicsQueue;

        // surface
        VkSurfaceKHR surface;
        // 显示
        VkQueue presentQueue;

        // 深度图
        TextureBaseInfo depthResource;
        // MSAA
        TextureBaseInfo colorResource;
        bool enableMSAA = false;

        // 图形管线
        std::unique_ptr<ShaderManager> shaderManager;
        std::unique_ptr<RenderProcess> renderProcess;

        // 命令池
        std::unique_ptr<CommandManager> commandManager;
        std::vector<VkCommandBuffer> commandBuffers;

        // descriptor
        std::unique_ptr<DescriptorPool> globalDescriptorPool;

        // texture manager
        std::unique_ptr<TextureManager> textureManager;

        // buffer manager
        std::unique_ptr<GeneralBufferManager> globalBufManager;

        // 验证层
        const std::vector<const char*> validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

        // debug扩展
        VkDebugUtilsMessengerEXT debugMessenger;

        void initWindow();
        void initVulkan();
        void mainLoop();
        void cleanup();

        void createInstance();
        void pickPhysicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);

        void createLogicalDevice();

        void createSurface();

        bool checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions();

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void setupDebugMessenger();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        VkSampleCountFlagBits getMaxUsableSampleCount();

        std::function<void()> _initImageResources;
        std::function<void()> _destroyImageResources;

        void initDepthResource();
        void destroyDepthResource();

        void initColorResource();
        void destroyColorResource();
    public:
        inline void initImageResources() {
            _initImageResources();
        }

        inline void destroyImageResources() {
            _destroyImageResources();
        }

    public:
        void run();
        void drawFrame();
        VulkanApp(int width = 800, int height = 600, const char* tittle = nullptr, bool enableMSAA = false, VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

        virtual void prepareResources() = 0;
        virtual void init() = 0;
        virtual void clean() = 0;
        virtual void renderFrame(FrameInfo& frameInfo) = 0;
        virtual void processUpdate(FrameInfo& frame) = 0;
        virtual void frameResized(VkExtent2D &swapChainExtent) {
            renderProcess->recreate();
            drawFrame();
        }

    public:
        // static std::shared_ptr<VulkanApp> context();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        inline VkFormat findDepthFormat() {
            return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }

        static bool hasStencilComponent(VkFormat format) {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                        VkDeviceMemory &imageMemory, uint32_t mipLevels = 1, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);

        inline VkImageView getDepthResource() {
            return depthResource.imageView;
        }

        inline VkImageView getColorResource() {
            return colorResource.imageView;
        }

        inline bool isEnableMSAA() const{
            return enableMSAA;
        }

        inline VkSampleCountFlagBits getMSAASamples() {
            return msaaSamples;
        }

        inline float deltaTime() const {
            return _deltaTime;
        }

        inline float elapsedTime() const {
            return _elapsedTime;
        }

        inline CommandManager *getCommandManager() const {
            return commandManager.get();
        }

        inline SwapChain *getSwapChain() const{
            return &renderProcess->getSwapChain();
        }

        inline RenderProcess *getRenderProcess() const {
            return renderProcess.get();
        }

        inline DescriptorPool *getDescriptorPool() const {
            return globalDescriptorPool.get();
        }

        inline TextureManager *getTextureManager() const {
            return textureManager.get();
        }

        inline GeneralBufferManager *getGlobalBufManager() const {
            return globalBufManager.get();
        }

        inline GLFWwindow *getWindow() {
            return window;
        }

        inline VkDevice& getDevice() {
            return device;
        }

        inline VkQueue& getGraphicsQueue() {
            return graphicsQueue;
        }

        inline VkQueue& getPresentQueue() {
            return presentQueue;
        }

        inline VkPhysicalDevice& getPhysicalDevice() {
            return physicalDevice;
        }

        inline VkSurfaceKHR& getSurface() {
            return surface;
        }

        inline void setTittle(const char* tittle) {
            this->tittle = tittle;
            glfwSetWindowTitle(window, tittle);
        }

        inline const char* getTittle() const {
            return tittle.c_str();
        }

        friend class ResourceUser;
    };

}

#endif //VULKANTEST_VULKANAPP_H
