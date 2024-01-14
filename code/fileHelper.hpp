//
// Created by MasterLong on 2023/9/16.
//

#ifndef VULKANTEST_FILEHELPER_H
#define VULKANTEST_FILEHELPER_H

#include <iostream>
#include <fstream>
#include <vector>
namespace jk {

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}
#endif //VULKANTEST_FILEHELPER_H
