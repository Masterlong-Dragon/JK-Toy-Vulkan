cmake_minimum_required(VERSION 3.20)
project(vulkanTest)

set(CMAKE_CXX_STANDARD 17)



# Vulkan
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
link_directories(${Vulkan_LIBRARY})

# GLFW
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

# GLM
find_package(glm REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
link_directories(${GLM_LIBRARY_DIRS})

# executable
# add_executable(vulkanTest main.cpp)
# add_executable(vulkanTest main.cpp VulkanApp.cpp VulkanApp.h fileHelper.h SwapChain.cpp SwapChain.h QueueFamily.cpp QueueFamily.h RenderProcess.cpp RenderProcess.h CommandManager.cpp CommandManager.h SyncManager.cpp SyncManager.h Vertex.h Buffer.cpp Buffer.h Descriptor.h Descriptor.cpp Shader.cpp Shader.h Texture.h Texture.cpp)
aux_source_directory(. SRC_FILES)
add_executable(vulkanTest ${SRC_FILES})
target_link_libraries(vulkanTest ${Vulkan_LIBRARIES} glfw glm)