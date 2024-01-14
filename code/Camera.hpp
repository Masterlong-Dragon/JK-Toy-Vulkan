//
// Created by MasterLong on 2023/12/4.
//

#ifndef VULKANTEST_CAMERA_H
#define VULKANTEST_CAMERA_H

#include "Buffer.h"
#include "Descriptor.h"
#include "CommandManager.h"
#include "RenderObject.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace jk {

    struct GlobalBufferObject {
        // glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;

        alignas(16) glm::vec3 directionalLightDirection;
        alignas(16) glm::vec4 directionalLightColor;
        alignas(16) glm::mat4 depthVP;

        alignas(16) int lightNum;
        alignas(16) PointLight pointLights[4];
    };

    class Camera {
    private:
        glm::mat4 view;
        glm::mat4 projection;

        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;

        bool updated = false;
        bool lastFrameUpdated = false;

        std::shared_ptr<UniformBuffer> uniformBuffer; 
        std::shared_ptr<DescriptorSets> viewDescriptorSets;

        VkDescriptorSetLayoutBinding viewLayoutBinding;
        uint32_t binding;

        // void updateUBO(FrameInfo& frame) {
        //     GlobalBufferObject ubo{};
        //     ubo.view = view;
        //     ubo.proj = projection;
        //     uniformBuffer->updateUniformBuffer(frame.currentFrame, &ubo);
        // }

        void updateProjection() {
            projection[1][1] *= -1;
        }

        void updateView() {
            view = glm::lookAt(position, position + front, up);
        } 
    public:

        Camera(GeneralBufferManager& bufferAllocator, uint32_t binding = 0) {
            uniformBuffer = bufferAllocator.createUniformBuffer(sizeof(GlobalBufferObject));
            viewLayoutBinding = jk::DescriptorSetLayout::uniformDescriptorLayoutBinding(this->binding = binding);

            position = glm::vec3(0.0f, 0.0f, 0.0f);
            up = glm::vec3(0.0f, 1.0f, 0.0f);
            front = glm::vec3(0.0f, 0.0f, -1.0f);
            right = glm::vec3(1.0f, 0.0f, 0.0f);
            
            updateView();
        }

        std::shared_ptr<UniformBuffer> initDescriptorSets(DescriptorPool& descriptorPool, VkDescriptorSetLayout layout) {
            viewDescriptorSets = descriptorPool.createDescriptorSets();
            viewDescriptorSets->init(layout);
            uniformBuffer->fillUniformDescriptorSets(viewDescriptorSets, binding);
            return uniformBuffer;
        }

        void setOrtho(float left, float right, float bottom, float top, float _near, float _far) {
            projection = glm::ortho(left, right, bottom, top, _near, _far);
            updated = true;
            updateProjection();
        }

        void setPerspective(float fov, float aspect, float _near, float _far) {
            projection = glm::perspective(fov, aspect, _near, _far);
            updated = true;
            updateProjection();
        }

        Camera& move(glm::vec3 offset) {
            position += offset;
            updated = true;
            return *this;
        }

        Camera& setPosition(glm::vec3 position) {
            this->position = position;
            updated = true;
            return *this;
        }

        Camera& setUp(glm::vec3 up) {
            this->up = up;
            updated = true;
            return *this;
        }

        Camera& setRight(glm::vec3 right) {
            this->right = right;
            updated = true;
            return *this;
        }

        Camera& setLookDirection(glm::vec3 lookDirection) {
            this->front = glm::normalize(lookDirection);
            updated = true;
            return *this;
        }

        void update() {
            if (updated || lastFrameUpdated) {
                updateView();
                // std::cout << "position " << position.x << " " << position.y << " " << position.z << std::endl;
                // updateUBO(frame);
                // 因为MAX_FRAMES_IN_FLIGHT = 2，所以这里需要判断一下
                // 如果上一帧更新了 这一帧也需要更新
                lastFrameUpdated = updated;
                updated = false;
            }
        }

        glm::mat4& getView() {
            return view;
        }

        glm::mat4& getProjection() {
            return projection;
        }

        VkDescriptorSetLayoutBinding getViewLayoutBinding() {
            return viewLayoutBinding;
        }

        std::shared_ptr<DescriptorSets> getViewDescriptorSets() {
            return viewDescriptorSets;
        }

        friend class CameraController;
    };

    class CameraController {
    private:
        GLFWwindow* window;
        Camera& camera;

        float yaw = -90.0f;
        float pitch = 0.0f;

        float movementSpeed = 2.5f;
        float sensitivity = 0.1f;

    public:
        CameraController(GLFWwindow* window, Camera& camera) : window(window), camera(camera) {
            // 根据camera的front和right计算yaw和pitch
            glm::vec3 front = camera.front;
            yaw = glm::degrees(atan2(front.z, front.x));
            pitch = glm::degrees(asin(front.y));
        }

        void processInput(float deltaTime) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera.move(camera.front * movementSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera.move(-camera.front * movementSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camera.move(-camera.right * movementSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camera.move(camera.right * movementSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                camera.move(camera.up * movementSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                camera.move(-camera.up * movementSpeed * deltaTime);
        }

        void processMouseMovement(float xoffset = 0.0f, float yoffset = 0.0f) {

            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;

            glm::vec3 front, right, up;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(front);
            right = glm::normalize(glm::cross(front, glm::vec3(0, 1 ,0)));
            up = glm::normalize(glm::cross(right, front));  
            camera.setLookDirection(front).setUp(up).setRight(right);
        } 
    };
}

#endif