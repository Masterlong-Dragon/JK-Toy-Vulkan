//
// Created by MasterLong on 2023/9/16.
//

#include <cstring>
#include "VulkanApp.h"

#include <set>
#include <optional>

namespace jk {

    /**
     * 检查验证层是否被支持
     * @param instance
     * @param pCreateInfo
     * @param pAllocator
     * @param pDebugMessenger
     * @return
     */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                        const VkAllocationCallbacks *pAllocator,
                                        VkDebugUtilsMessengerEXT *pDebugMessenger) {
        // 获取函数指针
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            // 调用函数
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    /**
     * 销毁验证层
     * @param instance
     * @param debugMessenger
     * @param pAllocator
     */
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks *pAllocator) {
        // 获取函数指针
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            // 调用函数
            func(instance, debugMessenger, pAllocator);
        }
    }

    /*
    * VulkanApp Class
    * */

    VulkanApp::VulkanApp(int width, int height, const char* tittle, bool enableMSAA, VkSampleCountFlagBits msaaSamples) : width(width), height(height), enableMSAA(enableMSAA) {
        
        this->tittle = "Vulkan Framework powered by JK.L";

        if (tittle != nullptr) {
            this->tittle = std::string(tittle);
        }
        
        if (enableMSAA) {
            this->msaaSamples = msaaSamples;
            _initImageResources = [this]() {
                initColorResource();
                initDepthResource();
            };

            _destroyImageResources = [this]() {
                destroyDepthResource();
                destroyColorResource();
            };

            return;
        }
        _initImageResources = std::bind(initDepthResource, this);
        _destroyImageResources = std::bind(destroyDepthResource, this);

    }

    void VulkanApp::run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
        VkExtent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        app->frameResized(extent);
        // app->drawFrame();
        // app->drawFrame();
    }

    void VulkanApp::initWindow() {
        glfwInit();

        // glfw 初始化窗口
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // 创建窗口
        window = glfwCreateWindow(width, height, tittle.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void VulkanApp::initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();

        // 资源池利用相关
        textureManager = std::make_unique<TextureManager>(this, globalResourcePool);

        globalBufManager = std::make_unique<GeneralBufferManager>(this, globalResourcePool);

        shaderManager = std::make_unique<ShaderManager>(this, globalResourcePool);

        // 核心组件
        renderProcess = std::make_unique<RenderProcess>(this);
        renderProcess->init();

        // descriptorPool = std::make_unique<DescriptorPool>(this);

        commandManager = std::make_unique<CommandManager>(this);
        commandManager->init(commandBuffers);

        globalDescriptorPool = std::make_unique<jk::DescriptorPool>(this, globalResourcePool, 50);

        prepareResources();

        init();
    //    createCommandPool();
    //    createCommandBuffers();
        // syncManager = std::make_unique<SyncManager>(this);
        // syncManager->init();
    //    createSyncObjects();
    }

    void VulkanApp::mainLoop() {
        // 主循环
        lastFrameTime = startTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            _deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
            _elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            glfwPollEvents();
            drawFrame();
            lastFrameTime = currentTime;
        }
        vkDeviceWaitIdle(device);
    }

    void VulkanApp::cleanup() {

        clean();

        // 清理
        // textureManager->cleanup();

        // shaderManager->cleanup();

        // globalBufManager->cleanup();
        globalResourcePool.cleanup(device);

        renderProcess->cleanup();

        globalDescriptorPool->cleanup();

        commandManager->cleanup();

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void VulkanApp::createInstance() {
        // 创建 Vulkan 实例

        // 检查验证层是否被支持
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // appInfo 用于向驱动程序表明应用程序的一些基本信息
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanTest";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // 创建 Vulkan 实例
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // 使用GLFW扩展
        auto extensions = getRequiredExtensions();
        // 指定扩展
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // debug 支持
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // 指定验证层
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // 指定debug回调函数
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        // 创建实例
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    bool VulkanApp::checkValidationLayerSupport() {
        uint32_t layerCount;
        // 获取验证层数量
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // 获取验证层列表
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // 检查validationLayers中的每一层是否都被支持
        for (const char *layerName: validationLayers) {
            bool layerFound = false;

            // 遍历所有可用的验证层
            for (const auto &layerProperties: availableLayers) {
                // 如果验证层被支持
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            // 如果验证层没有被支持
            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> VulkanApp::getRequiredExtensions() {
        // 获取GLFW需要的扩展
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // 将GLFW需要的扩展转换为vector
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // 如果启用了验证层，添加VK_EXT_debug_utils扩展
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void VulkanApp::setupDebugMessenger() {
        if (!enableValidationLayers) return;

        // 创建debugMessenger
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        // 创建debugMessenger
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void VulkanApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void VulkanApp::pickPhysicalDevice() {
        // 获取物理设备数量
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        // 如果没有可用的物理设备
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // 获取所有可用的物理设备
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // 检查所有可用的物理设备，找到第一个满足需求的物理设备
        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                if (enableMSAA) {
                    VkSampleCountFlagBits tmp = getMaxUsableSampleCount();
                    if (msaaSamples == VK_SAMPLE_COUNT_1_BIT || msaaSamples > tmp) {
                        msaaSamples = tmp;
                    }
                }
                break;
            }
        }

        // 如果没有找到满足需求的物理设备
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // 获取物理设备支持的扩展数量
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        // 获取物理设备支持的扩展列表
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // 将需要的扩展转换为set 便于查找
        auto deviceExtensions = SwapChain::deviceExtensions;
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // 检查物理设备支持的扩展是否包含需要的扩展
        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        // 如果所有需要的扩展都被支持
        return requiredExtensions.empty();
    }

    bool VulkanApp::isDeviceSuitable(VkPhysicalDevice device) {
        // 检查物理设备是否支持需要的队列族
        QueueFamilyIndices indices = findQueueFamilies(surface, device);

        // 检查物理设备是否支持需要的扩展
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // 检查物理设备是否支持交换链
        bool swapChainAdequate = false;
        // 如果物理设备支持交换链
        if (extensionsSupported) {
            // 获取交换链支持的细节
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, device);
            // 检查交换链支持的细节是否满足需求
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // 支持特性 包括 纹理各向异性滤波 
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    void VulkanApp::createLogicalDevice() {
        // 检查物理设备是否支持需要的队列族
        QueueFamilyIndices indices = findQueueFamilies(surface, physicalDevice);

        // 创建队列族
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        // 获取所有需要的队列族
        std::set<uint32_t> uniqueQueueFamilies = {
                indices.graphicsFamily.value(),
                indices.presentFamily.value()
        };

        // 指定队列族的优先级
        float queuePriority = 1.0f;

        // 创建队列族信息
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // 指定需要的设备特性
        VkPhysicalDeviceFeatures deviceFeatures{};
        // 纹理各向异性过滤
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        // 创建逻辑设备
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        auto deviceExtensions = SwapChain::deviceExtensions;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // 如果启用了验证层，添加验证层
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        // 创建逻辑设备
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // 获取队列族句柄
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void VulkanApp::createSurface() {
        // 创建surface
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void VulkanApp::drawFrame() {
        FrameInfo frame = commandManager->beginFrame(commandBuffers);

        // 重置命令缓冲
        // 设置渲染流程信息
        // renderProcess->beginRenderPass(frame);
        renderFrame(frame);
        // 结束渲染流程
        // renderProcess->endRenderPass(frame);
        // 提交命令缓冲
        commandManager->endFrame(frame);
        processUpdate(frame);
    }


    VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void VulkanApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                    VkBuffer &buffer, VkDeviceMemory &bufferMemory) {

        // 设置缓冲信息
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // 创建缓冲
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        // 获取缓冲内存需求
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        // 设置内存信息
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // 分配内存
        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        // 绑定内存
        vkBindBufferMemory(device, buffer, bufferMemory, 0);

    }

    uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // 获取内存属性
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        // 获取内存类型索引
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // 复制缓冲
        commandManager->excuteCommand([&](VkCommandBuffer& commandBuffer) {
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        });

    }

    VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                VkFormatFeatureFlags features) {
        // 检查候选格式是否被支持
        for (VkFormat format: candidates) {
            // 获取格式属性
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            // 检查格式是否被支持
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                    (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                            VkDeviceMemory& imageMemory, uint32_t mipLevels, VkSampleCountFlagBits numSamples) {                    

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D; // 图像类型
        imageInfo.extent.width = width; // 图像宽度
        imageInfo.extent.height = height; // 图像高度
        imageInfo.extent.depth = 1; // 图像深度
        imageInfo.mipLevels = mipLevels; // mipmap层级
        imageInfo.arrayLayers = 1; // 图像数组层数
        imageInfo.format = format; // 图像格式
        imageInfo.tiling = tiling; // 图像布局
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 图像初始布局
        imageInfo.usage = usage; // 图像使用
        imageInfo.samples = numSamples; // 采样数
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // 共享模式

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // 分配内存
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; // 类型
        allocInfo.allocationSize = memRequirements.size; // 分配大小
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties); // 内存类型索引

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        // 绑定内存
        vkBindImageMemory(device, image, imageMemory, 0);
    }

    // 深度图
    void VulkanApp::initDepthResource() {
        VkFormat depthFormat = findDepthFormat();
        auto swapChainExtent = getSwapChain()->getExtent();
        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                    depthResource.image, depthResource.imageMemory, 1, msaaSamples);
        depthResource.imageView = createImageView(depthResource.image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void VulkanApp::destroyDepthResource() {
        vkDestroyImageView(device, depthResource.imageView, nullptr);
        vkDestroyImage(device, depthResource.image, nullptr);
        vkFreeMemory(device, depthResource.imageMemory, nullptr);
    }

    // 超采样
    void VulkanApp::initColorResource() {
        VkFormat colorFormat = getSwapChain()->getFormat();
        auto swapChainExtent = getSwapChain()->getExtent();
        createImage(swapChainExtent.width, swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                    colorResource.image, colorResource.imageMemory, 1, msaaSamples);
        colorResource.imageView = createImageView(colorResource.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void VulkanApp::destroyColorResource() {
        vkDestroyImageView(device, colorResource.imageView, nullptr);
        vkDestroyImage(device, colorResource.image, nullptr);
        vkFreeMemory(device, colorResource.imageMemory, nullptr);
    }

    VkSampleCountFlagBits VulkanApp::getMaxUsableSampleCount() {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    ResourceUser::ResourceUser(VulkanApp* app) : app(app), resourceHelper(app->globalResourcePool) {}

}

// // 单例
// static std::shared_ptr<VulkanApp> singleton = nullptr;
// static std::once_flag singletonFlag;

// std::shared_ptr<VulkanApp> VulkanApp::context() {
//     std::call_once(singletonFlag, [&] {
//         singleton = std::shared_ptr<VulkanApp>(new VulkanApp());
//     });
//     return singleton;
// }
