#include "VulkanApp.h"
#include "SimpleObjLoader.hpp"
#include "RenderBatchManager.h"
#include "Camera.hpp"

class MyVulkanApp : public jk::VulkanApp {
private:
    std::shared_ptr<jk::Shader> shader;
    std::shared_ptr<jk::Shader> offscreenShader;

    std::unique_ptr<jk::OffscreenRenderProcess> offscreenRenderProcess;

    std::vector<VkDescriptorSetLayout> layouts;

    std::shared_ptr<jk::MeshObject> myObj;
    std::shared_ptr<jk::MeshObject> myObj2;
    std::shared_ptr<jk::MeshObject> myObj3;
    std::shared_ptr<jk::MeshObject> cube;
    std::shared_ptr<jk::MeshObject> lightSign;
    std::shared_ptr<jk::MeshObject> plane;
    std::shared_ptr<jk::MeshObject> plane2;
    std::shared_ptr<jk::MeshObject> earth;
    std::shared_ptr<jk::PointLightObject> sun;

    // 平行光标志模型的变换矩阵
    glm::mat4 lightDirMat{1.0f};
    // 地球模型的变换矩阵
    glm::mat4 earthMat{1.0f};

    std::unique_ptr<jk::Camera> camera;
    std::unique_ptr<jk::CameraController> cameraController;

    std::shared_ptr<jk::RenderBatchManager> renderBatchManager;
    std::shared_ptr<jk::RenderBatch> batchShadow;

    std::shared_ptr<jk::UniformBuffer> globalBuf;
    std::shared_ptr<jk::UniformBuffer> offscreenBuf;

    std::shared_ptr<jk::DescriptorSets> depthMVPDescriptor;
    std::shared_ptr<jk::DescriptorSets> shadowMapDescriptor;

    glm::mat4 depthProjectionMatrix{glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f)};
    jk::DepthVP depthVP{};
    jk::GlobalBufferObject ubo{};
    // pos dir color
    jk::DirectionalLight directionalLight{glm::vec3(0.0f, 40.0f, 0.0f),
                                        glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f)), glm::vec4(1.0f, 1.0f, 1.0f, 0.8f)};
    jk::PointLight pointLights[4];

    // 调试用
    bool enableShadow = true;
    bool enableLights[2]{true, true};
    bool enableDirLight = true;
    bool enableDirRotate = true;
    bool enableERev = true;
    bool enableERot = true;

public:
    MyVulkanApp() : VulkanApp(800, 600, nullptr, true, VK_SAMPLE_COUNT_8_BIT) {}

    void mouseUpdate(double offsetX, double offsetY) {
        cameraController->processMouseMovement(offsetX, offsetY);
    }

    void singleKeyPressed(int key) {
        switch(key) {
            // 是否开启阴影
            case GLFW_KEY_Y:
                enableShadow = !enableShadow;
                break;
            // 是否开启平行光
            case GLFW_KEY_P:
                enableDirLight = !enableDirLight;
                if (enableDirLight) {
                    directionalLight.color.a = 0.8f;
                } else {
                    directionalLight.color.a = 0.0f;
                }
                break;
            // 是否开启平行光旋转
            case GLFW_KEY_R:
                enableDirRotate = !enableDirRotate;
                break;
            // 是否开启点光源1
            case GLFW_KEY_1:
                enableLights[0] = !enableLights[0];
                if (enableLights[0]) {
                    pointLights[0].color.a = 1.0f;
                } else {
                    pointLights[0].color.a = 0.0f;
                }
                break;
            // 是否开启点光源2
            case GLFW_KEY_2:
                enableLights[1] = !enableLights[1];
                if (enableLights[1]) {
                    pointLights[1].color.a = 1.0f;
                } else {
                    pointLights[1].color.a = 0.0f;
                }
                break;
            // 是否开启地球自转
            case GLFW_KEY_Z:
                enableERot = !enableERot;
                break;
            // 是否开启地球公转
            case GLFW_KEY_G:
                enableERev = !enableERev;
                break;
        }
    }

    void prepareResources() override {
        /////////////////////////// 以下是资源初始化 ///////////////////////////
        // 这里的设计不好 其实应该把global uniform再单独抽出一个类管理的
        // 默认单个相机所以让它持有全局ubo很合理叭
        camera = std::make_unique<jk::Camera>(*globalBufManager, 0);// uniform binding at 0

        auto uniformBinding = camera->getViewLayoutBinding();
        auto imageBinding = jk::DescriptorSetLayout::imageDescriptorLayoutBinding(0);
        // 创建两个layout
        globalDescriptorPool->fillLayoutsByBindings(layouts, {uniformBinding, imageBinding, imageBinding});
        globalBuf = camera->initDescriptorSets(*globalDescriptorPool, layouts[0]);

        VkPushConstantRange pushConstantRange{};
        shader = shaderManager->createShader("shaders/vert.spv", "shaders/frag.spv",
                                             layouts.data(), layouts.size(),
                                             jk::MeshObject::getPushConstantInfo(pushConstantRange));
        // 使用对应的renderprocess进行最后的pipeline创建
        renderProcess->createGraphicsPipeline(*shader);
        renderProcess->setClearColor({0.1f, 0.1f, 1.0f, 1.0f});

        // 准备offscreen部分做shadow mapping
        // 不需要片元着色器
        offscreenShader = shaderManager->createShader("shaders/offscreen.spv", "",
                                                      layouts.data(), 1,
                                                      jk::MeshObject::getPushConstantInfo(pushConstantRange));

        // renderprocess这一块因为最初只有单个renderpass 设计得也不好 没时间改了已经 
        // 暂时用继承抽象基类的方式解决
        offscreenRenderProcess = std::make_unique<jk::OffscreenRenderProcess>(this);
        offscreenRenderProcess->init();
        offscreenRenderProcess->createGraphicsPipeline(*offscreenShader);

        /////////////////////////// 其它资源初始化 ///////////////////////////
        std::vector<std::shared_ptr<jk::DescriptorSets>> d;
        for (int i = 0; i < 7; i++) {
            d.push_back(globalDescriptorPool->createDescriptorSets());
            d[i]->init(layouts[1]);
        }
        // 默认空白纹理
        textureManager->createFilledTexture(1, 1, glm::vec3(1.0f, 1.0f, 1.0f))->fillImageDescriptorSets(d[0], 0);
        // 小屋纹理
        // 填充纹理描述符
        textureManager->loadTexture("viking_room.png", true)->fillImageDescriptorSets(d[1], 0);
        // 测试平面贴图纹理
        textureManager->loadTexture("portrait.png", true)->fillImageDescriptorSets(d[2], 0);
        textureManager->loadTexture("building.jpeg", true)->fillImageDescriptorSets(d[3], 0);
        textureManager->loadTexture("earth.jpg", true)->fillImageDescriptorSets(d[4], 0);
        textureManager->loadTexture("college.png", true)->fillImageDescriptorSets(d[5], 0);
        textureManager->loadTexture("sun.jpg", true)->fillImageDescriptorSets(d[6], 0);
        // shadowmap descriptor
        shadowMapDescriptor = globalDescriptorPool->createDescriptorSets();
        shadowMapDescriptor->init(layouts[2]);

        // 准备创建渲染批处理
        // 将一类具有相同资源描述的渲染对象放在同一个渲染批处理中
        // 起因是我觉得descriptor应当尽量复用
        // 虽然但是我最后做下来感觉压根没啥意义 小场景作用不大 而且增加复杂度和后续拓展难度例如透明度排序
        // 主场景渲染批处理管理
        renderBatchManager = std::make_shared<jk::RenderBatchManager>(this, *shader);
        std::vector<std::shared_ptr<jk::RenderBatch>> batches;
        for (int i = 0; i < d.size(); i++) {
            batches.push_back(renderBatchManager->createRenderBatch(d[i]->getID()));
            batches[i]->setGlobalDescriptorSet(camera->getViewDescriptorSets());
            auto &des = batches[i]->getDescriptorSets();
            des.push_back(d[i]);
            des.push_back(shadowMapDescriptor);
            batches[i]->updateDescriptorSets();
        }

        // 加入渲染对象

        // 模型加载
        auto myBuf = jk::SimpleObj::load(*globalBufManager, "viking_room.obj");
        auto myBuf2 = jk::SimpleObj::load(*globalBufManager, "smooth_vase.obj");
        auto cubeBuf = globalBufManager->genCube();
        auto planeBuf = globalBufManager->genDoublePlane();
        auto sphereBuf = globalBufManager->genSphere();
        myObj = std::make_shared<jk::MeshObject>(myBuf);
        myObj2 = std::make_shared<jk::MeshObject>(myBuf2);
        myObj3 = std::make_shared<jk::MeshObject>(jk::SimpleObj::load(*globalBufManager, "teapot.obj"));
        cube = std::make_shared<jk::MeshObject>(cubeBuf);
        lightSign = std::make_shared<jk::MeshObject>(jk::SimpleObj::load(*globalBufManager, "lamp.obj"));
        plane = std::make_shared<jk::MeshObject>(planeBuf);
        plane2 = std::make_shared<jk::MeshObject>(planeBuf);
        earth = std::make_shared<jk::MeshObject>(sphereBuf);
        sun = std::make_shared<jk::PointLightObject>(sphereBuf);

        renderBatchManager->addRenderObject(myObj, batches[1]);
        renderBatchManager->addRenderObject(myObj2, batches[0]);
        renderBatchManager->addRenderObject(myObj3, batches[0]);
        renderBatchManager->addRenderObject(lightSign, batches[0]);
        renderBatchManager->addRenderObject(cube, batches[5]);
        renderBatchManager->addRenderObject(plane, batches[2]);
        renderBatchManager->addRenderObject(plane2, batches[3]);
        renderBatchManager->addRenderObject(earth, batches[4]);
        renderBatchManager->addRenderObject(sun, batches[6]);

        // offscreen部分
        offscreenBuf = globalBufManager->createUniformBuffer(sizeof(jk::DepthVP));
        depthMVPDescriptor = globalDescriptorPool->createDescriptorSets();
        depthMVPDescriptor->init(layouts[0]);
        offscreenBuf->fillUniformDescriptorSets(depthMVPDescriptor, 0);
        offscreenRenderProcess->fillImageDescriptorSets(shadowMapDescriptor, 0);

        // 加入阴影渲染对象
        batchShadow = renderBatchManager->createRenderBatch(0); // 这个0也挺不严谨的 一般不会跟其它batch的ID冲突 因为是自增ID
        batchShadow->setGlobalDescriptorSet(depthMVPDescriptor);
        batchShadow->updateDescriptorSets();

        batchShadow->addRenderObject(myObj);
        batchShadow->addRenderObject(myObj2);
        batchShadow->addRenderObject(myObj3);
        batchShadow->addRenderObject(cube);
        batchShadow->addRenderObject(plane);
        batchShadow->addRenderObject(plane2);
        // batchShadow->addRenderObject(earth);
    }

    void init() override {

        // 鼠标控制器初始化
         glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
                                 {
            auto app = reinterpret_cast<MyVulkanApp*>(glfwGetWindowUserPointer(window));

            static double lastX = xpos;
            static double lastY = ypos;

            double xoffset = xpos - lastX;
            double yoffset = lastY - ypos;
            lastX = xpos;
            lastY = ypos;
            if (!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))return;
            app->mouseUpdate(xoffset, yoffset);
        });


        // 键盘控制器初始化
         glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                            {
            auto app = reinterpret_cast<MyVulkanApp*>(glfwGetWindowUserPointer(window));
            const double keyPressDelay = 0.2; // 调整按键响应速度的延迟时间（以秒为单位）
            static double lastKeyPressTime = 0.0;
            double currentTime = glfwGetTime();

            if (action == GLFW_PRESS && currentTime - lastKeyPressTime >= keyPressDelay) {
                // 更新上一次按键时间
                app->singleKeyPressed(key);
                lastKeyPressTime = currentTime;
            } });

         /////////////////////////// 以下是场景属性初始化 ///////////////////////////

         camera->setPerspective(glm::radians(45.0f), width / (float)height, 0.1f, 100.0f);
         camera->setPosition(glm::vec3(2.0f, 2.0f, -2.0f));
         camera->setLookDirection(glm::vec3(-2.0f, -2.0f, 2.0f));
         camera->update();

         // 修正灯光投影 vulkan坐标系与OpenGL的反转y轴
         depthProjectionMatrix[1][1] *= -1;

         cameraController = std::make_unique<jk::CameraController>(window, *camera);

         myObj->setRotationX(-90);
         myObj->setPosY(-0.08f);
         std::static_pointer_cast<jk::MeshObject>(myObj);
         auto &mat2 = std::static_pointer_cast<jk::MeshObject>(myObj)->getMaterial();
         mat2.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
         mat2.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
         mat2.specular = glm::vec4(1.0f, 1.0f, 1.0f, 16.0f);

         // myObj2->setScale(glm::vec3(0.04f, 0.04f, 0.04f));
         myObj2->setPosition(glm::vec3(1.0f, 0.0f, 1.5f));
         myObj2->setRotationX(180);
         auto &mat = std::static_pointer_cast<jk::MeshObject>(myObj2)->getMaterial();
         mat.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
         mat.diffuse = glm::vec3(0.75164f, 0.60648f, 0.22648f);
         mat.specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 512.0f);

         myObj3->getMaterial() = mat;
         myObj3->getMaterial().color = glm::vec3(0.33f, 0.18f, 0.18f);
         myObj3->getMaterial().diffuse = glm::vec3(0.33f, 0.18f, 0.18f);
         myObj3->setScale(glm::vec3(0.04f));
         myObj3->setRotationX(-90);
         myObj3->setPosition(glm::vec3(3.0f, 0.5f, -3.0f));

         lightSign->setUseLighting(false);
         lightSign->setCastShadow(false);
         lightSign->setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
         lightSign->setScale(glm::vec3(0.01f));
         lightSign->setRotationX(90);
         lightSign->getMaterial().color = glm::vec3(0.94f, 0.75f, 0.38f);

         // 解除renderobject默认变换 以实现自定义效果
         auto lightSignTransformM = [&]()
         {
             return lightDirMat;
         };
         lightSign->applyModelMatrix(lightSignTransformM);

         cube->setPosition(glm::vec3(0.0f, -0.2f, 0.0f));
         cube->setScale(glm::vec3(10.0f, 0.1f, 10.0f));
         auto &cubeMat = cube->getMaterial();
         cubeMat.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
         cubeMat.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
         cubeMat.specular = glm::vec4(1.0f, 1.0f, 1.0f, 64.0f);

         plane->setPosY(0.42f);
         plane->setRotationX(90);
         plane->setPosZ(0.72f);
         plane->setPosX(0.2f);
         plane->getMaterial() = cubeMat;

         plane2->setScale(glm::vec3(1.28f, 1.0f, 0.72f) * glm::vec3(1.5f));
         plane2->setPosition(glm::vec3(-0.52f, 0.42f, 0.0f));
         plane2->setRotationX(90);
         plane2->setRotationZ(90);
         plane2->getMaterial() = cubeMat;

         earth->getMaterial() = cubeMat;
         earth->getMaterial().ambient = glm::vec3(0.04f);
         earth->setPosition(glm::vec3(2.0f, 2.0f, 6.0f));
         earth->setScale(glm::vec3(0.1f));
         earth->setCastShadow(false);
         earth->setUseDLighting(false);

         // 解除renderobject默认变换 以实现自定义效果
         auto earthTransformM = [&]()
         {
             return earthMat;
         };
         earth->applyModelMatrix(earthTransformM);

         sun->setLightPosition(glm::vec3(2.0f, 2.0f, 10.0f));
         sun->setScale(glm::vec3(0.5f));
         sun->setCastShadow(false);

         auto &sunLight = sun->getPointLight();
         // 偏黄色
         sunLight.color = glm::vec4(0.94f, 0.75f, 0.38f, 1.0f);
         sunLight.args = glm::vec3(0.8f, 0.09f, 0.256f);

         pointLights[0] = jk::PointLight{glm::vec3(-0.1f, 0.65f, 0.46f), glm::vec4(0.94f, 0.75f, 0.38f, 1.0f), glm::vec3(0.8f, 0.72f, 1.024f)};
         pointLights[1] = jk::PointLight{glm::vec3(-0.1f, 0.65f, -0.70f), glm::vec4(0.94f, 0.75f, 0.38f, 1.0f), glm::vec3(0.8f, 0.72f, 1.024f)};
         pointLights[2] = sunLight;
    }

    void renderFrame(jk::FrameInfo& frame) override {
        // first pass
        offscreenRenderProcess->beginRenderPass(frame);
        if (enableShadow) {
            offscreenShader->bind(frame.commandBuffer);
            batchShadow->drawBatch(*commandManager, *offscreenShader, frame);
        }
        offscreenRenderProcess->endRenderPass(frame);

        // second pass
        renderProcess->beginRenderPass(frame);
        renderBatchManager->drawBatches(frame);
        renderProcess->endRenderPass(frame);
    } 

    void frameResized(VkExtent2D& swapChainExtent) {
        VulkanApp::frameResized(swapChainExtent);
        float aspect = swapChainExtent.width / (float) swapChainExtent.height;
        // 检查窗口如果最小化则不更新
        if (std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0))
        {
            camera->setPerspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        }
    }

    void processUpdate(jk::FrameInfo& frame) override {

        // auto &rotationY = myObj->transform.rotation.x;
        // rotationY = (rotationY + 2.0f * deltaTime());
        // if (rotationY > 360.0f) {
        //     rotationY -= 360.0f;
        // }

        float deltaTime = this->deltaTime();
        float elapsedTime = this->elapsedTime();

        // 控制平行光视角高度
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            directionalLight.position.y += 2.0f * deltaTime;
            directionalLight.direction = glm::normalize(-directionalLight.position);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            directionalLight.position.y -= 2.0f * deltaTime;
            directionalLight.direction = glm::normalize(-directionalLight.position);
        }

        // 茶壶变换
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            myObj3->setScale(myObj3->getScale() + glm::vec3(0.01f) * deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            myObj3->setScale(myObj3->getScale() - glm::vec3(0.01f) * deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
            myObj3->setPosition(myObj3->getPosition() + glm::vec3(0.1f, 0.0f, 0.0f) * deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
            myObj3->setPosition(myObj3->getPosition() - glm::vec3(0.1f, 0.0f, 0.0f) * deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
            myObj3->setPosition(myObj3->getPosition() + glm::vec3(0.0f, 0.0f, 0.1f) * deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
            myObj3->setPosition(myObj3->getPosition() - glm::vec3(0.0f, 0.0f, 0.1f) * deltaTime);
        }

         // 茶壶旋转 注意茶壶原本是躺倒的
        myObj3->setRotationZ(elapsedTime * 10.0f);


        cameraController->processInput(deltaTime);
        cameraController->processMouseMovement();

        // 令平行光源于xz平面运动
        if (enableDirRotate) {
            float tx = cos(elapsedTime / 2), tz = sin(elapsedTime / 2);
            directionalLight.position.x = 40.0f * tx;
            directionalLight.position.z = 40.0f * tz;
            directionalLight.direction = glm::normalize(-directionalLight.position);
            // 设置平行灯光标志模型跟随运动
            glm::vec3 n = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -directionalLight.direction);
            float theta = acos(glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), -directionalLight.direction));
            n = glm::normalize(n);
            lightDirMat = lightSign->translateMatrix() * glm::rotate(theta, n) * lightSign->rotateMatrix() * lightSign->scaleMatrix();
        }
        // 令地球模型围绕太阳模型于xz平面内运动
        // 同时 地球模型以23.5度的角度倾斜并绕轴旋转
        if (enableERev) {
            float ex = cos(elapsedTime / 2), ez = sin(elapsedTime / 2);
            earth->setPosition(glm::vec3(ex, 0.0f, ez) + sun->getPosition());
        }

        static glm::mat4 earthSpinMatrix = glm::mat4(1.0f);
        static glm::vec3 earthSpinAxis = glm::normalize(glm::vec3(sin(glm::radians(23.5f)), cos(glm::radians(23.5f)), 0.0f)); // 自转轴

        if (enableERot) {
            float earthRotationAngle = elapsedTime;
            earthSpinMatrix = glm::rotate(earthRotationAngle, earthSpinAxis);
        }
        earthMat = earth->translateMatrix() * earthSpinMatrix * earth->scaleMatrix();

        camera->update();
        /////////////////////////// 以下ubo更新 ///////////////////////////

        // 更新阴影场景的uniform buffer
        // 阴影场景使用directional light
        // 正交投影
		// 创建一个与光照方向对齐的视图矩阵
        glm::mat4 depthViewMatrix = glm::lookAt(directionalLight.position, // 将"观察点"设置为沿负光照方向的一个点
                                                glm::vec3(0.0f), // 目标点通常设置为原点或场景中心
                                                glm::vec3(0.0, 1.0, 0.0)); // 上方向通常保持不变
        depthVP.depthVP = depthProjectionMatrix * depthViewMatrix;
        offscreenBuf->updateUniformBuffer(frame.currentFrame, &depthVP);

        // 更新主场景的uniform buffer
        ubo.proj = camera->getProjection();
        ubo.view = camera->getView();
        ubo.depthVP = depthVP.depthVP;
        ubo.directionalLightDirection = directionalLight.direction;
        ubo.directionalLightColor = directionalLight.color;
        // ubo.pointLights[0] = lightObject->getPointLight();
        ubo.lightNum = 3;
        memcpy(ubo.pointLights, pointLights, sizeof(jk::PointLight) * 3);
        globalBuf->updateUniformBuffer(frame.currentFrame, &ubo);
    }

    void clean() override {
        offscreenRenderProcess->cleanup();
    }
};

int main() {
    auto app = MyVulkanApp();
    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}