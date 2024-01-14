//
// Created by MasterLong on 2023/12/3.
//

#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Buffer.h"
#include "Texture.h"
#include "ResourceHelper.hpp"
#include "CommandManager.h"
#include "Descriptor.h"

namespace jk {

    class VulkanApp;

    struct PointLight {
        alignas(16) glm::vec3 position{0.0f};
        alignas(16) glm::vec4 color{0.0f};
        glm::vec3 args{0.0f};
        // alignas(16) float constant{0.0f};
        // alignas(16) float linear{0.0f};
        // alignas(16) float quadratic{0.0f};
    };

    struct DirectionalLight {
        glm::vec3 position{0.0f};
        glm::vec3 direction{0.0f};
        glm::vec4 color{0.0f};
    };

    struct Material {
        glm::vec3 color {1.0f};
        glm::vec3 ambient{0.0f};
        glm::vec3 diffuse{0.0f};
        glm::vec4 specular{0.0f};
    };

    // 内存对齐
    struct PushData {
        alignas(16) glm::mat4 model {1.0f};
        alignas(16) glm::mat4 normal {1.0f};

        alignas(16) glm::vec3 color {1.0f};
        alignas(16) glm::vec3 ambient{0.0f};
        alignas(16) glm::vec3 diffuse{0.0f};
        alignas(16) glm::vec4 specular{0.0f};
        alignas(16) int args{0};
    };

    struct Transform {
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
    };

    class RenderObject : public IResource {
    private:
        std::shared_ptr<ModelBuffer> modelBuffer;
        uint32_t renderBatchID = 0;

        // Material material{};

        // 是否在绘制时push本地变换矩阵
        // bool enableLocalTransform;
        // std::function<void(Shader& shader, FrameInfo &frame)> pushFunc;
        std::function<glm::mat4()> modelMatrixFunc;
        Transform transform;
    protected:
        virtual void pushFunc(Shader& shader, FrameInfo &frame) {

        }
    public:

        RenderObject(std::shared_ptr<ModelBuffer> modelBuffer)// , bool enableLocalTransform = true)
                // :  enableLocalTransform(enableLocalTransform) {
                {
                    // modelBuffer不能为空
                    assert(modelBuffer != nullptr);
                    this->modelBuffer = std::move(modelBuffer);
                    modelMatrixFunc = std::bind(&RenderObject::defaultModelMatrix, this);
                    // if (enableLocalTransform) {
                    //     pushFunc = std::bind(&RenderObject::pushTransform, this, std::placeholders::_1, std::placeholders::_2);
                    // } else {
                    //     pushFunc = [](Shader& shader, FrameInfo &frame) {};
                    // }
                }
        inline std::shared_ptr<ModelBuffer> getModelBuffer() const { return modelBuffer; }

        inline RenderObject& setPosition(glm::vec3 position) {
            transform.position = position;
            return *this;
        }

        inline RenderObject& setPosX(float x) {
            transform.position.x = x;
            return *this;
        }

        inline RenderObject& setPosY(float y) {
            transform.position.y = y;
            return *this;
        }

        inline RenderObject& setPosZ(float z) {
            transform.position.z = z;
            return *this;
        }

        inline RenderObject& setRotation(glm::vec3 rotation) {
            transform.rotation = rotation;
            return *this;
        }

        inline RenderObject& setRotationX(float x) {
            transform.rotation.x = x;
            return *this;
        }

        inline RenderObject& setRotationY(float y) {
            transform.rotation.y = y;
            return *this;
        }

        inline RenderObject& setRotationZ(float z) {
            transform.rotation.z = z;
            return *this;
        }

        inline RenderObject& setScale(glm::vec3 scale) {
            transform.scale = scale;
            return *this;
        }

        inline RenderObject& setScaleX(float x) {
            transform.scale.x = x;
            return *this;
        }

        inline RenderObject& setScaleY(float y) {
            transform.scale.y = y;
            return *this;
        }

        inline RenderObject& setScaleZ(float z) {
            transform.scale.z = z;
            return *this;
        }

        inline Transform& getTransform() {
            return transform;
        }

        inline glm::vec3& getPosition() {
            return transform.position;
        }

        inline glm::vec3& getRotation() {
            return transform.rotation;
        }

        inline glm::vec3& getScale() {
            return transform.scale;
        }

        inline void setUseTexture(bool useTexture) {
            
        }


        // inline Material& getMaterial() {
        //     return material;
        // }

        void draw(CommandManager &commandManager, Shader &shader, FrameInfo &frame) {
            pushFunc(shader, frame);
            modelBuffer->bind(frame.commandBuffer);
            commandManager.renderModelBuffer(frame, modelBuffer);
        }

        void cleanup(VkDevice &device) override {
            // 不需要 因为全局的resource pool会处理好
        }

        void applyModelMatrix(std::function<glm::mat4()> modelMatrixFunc) {
            this->modelMatrixFunc = modelMatrixFunc;
        }

        glm::mat4 modelMatrix() {
            return modelMatrixFunc();
        }

        glm::mat4 translateMatrix() {
            glm::mat4 model = glm::mat4(1.0f);
            // model = glm::translate(model, transform.position);
            // 应课程要求改为手动求解
            model[3][0] = transform.position.x;
            model[3][1] = transform.position.y;
            model[3][2] = transform.position.z;
            return model;
        }

        glm::mat4 rotateMatrix() {
            glm::mat4 model = glm::mat4(1.0f);
            // model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            // model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            // model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            // 应课程要求改为手动求解
            float x = glm::radians(transform.rotation.x);
            float y = glm::radians(transform.rotation.y);
            float z = glm::radians(transform.rotation.z);
            // 手动计算绕X轴的旋转矩阵
            float cx = cos(x);
            float sx = sin(x);
            glm::mat4 rotationX = {
                1, 0, 0, 0,
                0, cx, sx, 0,
                0, -sx, cx, 0,
                0, 0, 0, 1
            };

            // 手动计算绕Y轴的旋转矩阵
            float cy = cos(y);
            float sy = sin(y);
            glm::mat4 rotationY = {
                cy, 0, -sy, 0,
                0, 1, 0, 0,
                sy, 0, cy, 0,
                0, 0, 0, 1
            };

            // 手动计算绕Z轴的旋转矩阵
            float cz = cos(z);
            float sz = sin(z);
            glm::mat4 rotationZ = {
                cz, sz, 0, 0,
                -sz, cz, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };

            // Z-Y-X是由于早期的错误导致的 由于场景参数调好了所以不改
            // 组合旋转矩阵
            model = rotationX * rotationY * rotationZ;
            
            return model;
        }

        glm::mat4 scaleMatrix() {
            glm::mat4 model = glm::mat4(1.0f);
            // 应课程要求改为手动求解
            // model = glm::scale(model, transform.scale);
            model[0][0] = transform.scale.x;
            model[1][1] = transform.scale.y;
            model[2][2] = transform.scale.z;
            return model;
        }

        glm::mat4 defaultModelMatrix() {
            return translateMatrix() * rotateMatrix() * scaleMatrix();
        }

        glm::mat4 normalMatrix() {
            return glm::transpose(glm::inverse(modelMatrix()));
        }

        friend class RenderBatch;
        friend class RenderBatchManager;
    };

    class MeshObject : public RenderObject {
    private:
        Material material{};

        #define USE_LIGHTING 1
        #define CAST_SHADOW 2
        #define USE_D_LIGHTING 4

        int useLighting = USE_LIGHTING;
        int castShadow = CAST_SHADOW;
        int useDLighting = USE_D_LIGHTING;
    protected:
        void pushFunc(Shader& shader, FrameInfo &frame) {
            int args = 0;
            args |= useLighting | castShadow | useDLighting;
            PushData pushData{modelMatrix(), normalMatrix(), material.color, material.ambient, material.diffuse, material.specular, args};
            // PPPPUSH!!!
            vkCmdPushConstants(
                                frame.commandBuffer,
                                shader.getPipelineLayout(),
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,
                                sizeof(PushData),
                                &pushData);
        }
    public:
        MeshObject(std::shared_ptr<ModelBuffer> modelBuffer) : RenderObject(std::move(modelBuffer)) {}

        inline Material& getMaterial() {
            return material;
        }

        inline void setUseLighting(bool useLighting) {
            this->useLighting = useLighting ? 1 : 0;
        }

        inline bool getUseLighting() {
            return useLighting;
        }

        inline void setCastShadow(bool castShadow) {
            this->castShadow = castShadow ? 2 : 0;
        }

        inline bool getCastShadow() {
            return castShadow;
        }

        inline void setUseDLighting(bool useDLighting) {
            this->useDLighting = useDLighting ? 4 : 0;
        }

        inline bool getUseDLighting() {
            return useDLighting;
        }

        inline static PushConstantInfo getPushConstantInfo(VkPushConstantRange& pushConstantRange) {
            pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(PushData);
            return PushConstantInfo{&pushConstantRange, 1};
        }
    };

    class PointLightObject : public MeshObject {
    private:
        PointLight pointLight{};
    public :
        PointLightObject(std::shared_ptr<ModelBuffer> modelBuffer) : MeshObject(std::move(modelBuffer)) { setUseLighting(false); }

        inline PointLight& getPointLight() {
            return pointLight;
        }

        inline void setLightPosition(glm::vec3 position) {
            pointLight.position = position;
            setPosition(position);
        }

        // inline static PushConstantInfo getPushConstantInfo(VkPushConstantRange& pushConstantRange) {
        //     pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        //     pushConstantRange.offset = 0;
        //     pushConstantRange.size = sizeof(PointLight);
        //     return PushConstantInfo{&pushConstantRange, 1};
        // }
    };
}

#endif //RENDER_OBJECT_H
