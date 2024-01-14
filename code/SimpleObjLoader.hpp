//
// Created by MasterLong on 2023/12/2.
//

#ifndef VULKANTEST_SIMPLEOBJ_H
#define VULKANTEST_SIMPLEOBJ_H

#include <iostream>
#include <unordered_map>
#include "thirdparty/rapidobj/rapidobj.hpp"
#include "Buffer.h"

namespace jk {

class SimpleObj {
    public:
        // 加载 obj 到ModelBuffer
        static std::shared_ptr<ModelBuffer> load(GeneralBufferManager& allocator, const std::string& path) {
            // 不考虑材质问题 这里简单地只加载顶点信息
            rapidobj::Result result = rapidobj::ParseFile(path, rapidobj::MaterialLibrary::Default(rapidobj::Load::Optional));

            if (result.error) {
                std::cerr << "Failed to load obj file: " << result.error.code.message() << std::endl;
                return nullptr;
            }

            bool success = rapidobj::Triangulate(result);

            if (!success) {
                std::cerr << result.error.code.message() << '\n';
                return nullptr;
            }

            std::vector<Vertex> vertices;
            // std::vector<uint32_t> indices;
            for (const auto& shape : result.shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};
                    vertex.pos = {
                            result.attributes.positions[index.position_index * 3 + 0],
                            result.attributes.positions[index.position_index * 3 + 1],
                            result.attributes.positions[index.position_index * 3 + 2]
                    };


                    if (result.attributes.texcoords.empty()) {
                        vertex.texCoord = {0.0f, 0.0f};
                    } else
                    vertex.texCoord = {
                        result.attributes.texcoords[2 * index.texcoord_index + 0],
                        1.0f - result.attributes.texcoords[2 * index.texcoord_index + 1]
                    };

                    if (result.attributes.normals.empty()) {
                        vertex.normal = {0.0f, 0.0f, 0.0f};
                    } else
                    vertex.normal = {
                            result.attributes.normals[index.normal_index * 3 + 0],
                            result.attributes.normals[index.normal_index * 3 + 1],
                            result.attributes.normals[index.normal_index * 3 + 2]
                    };

                    vertex.color = {1.0f, 1.0f, 1.0f};

                    vertices.push_back(vertex);
                }
            }

            // std::cout << vertices.size() << std::endl;

            auto model = allocator.createModelBuffer();
            allocator.loadVerticesOntoBuffer(model, vertices);
            return model;
        }
    };
}

#endif //VULKANTEST_SIMPLEOBJ_H