#pragma once

// 忽略openssl3.0弃用了代码中的接口，产生的警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <openssl/sha.h>

#include "base/logconfig.h"

static std::string get_sha256Hex_Range(const std::string &filePath, const long long &startPos, const long long &endPos)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR(stderr, "can't open this file, filePath: %s \n", filePath.c_str());
        return "";
    }

    // 创建SHA-256哈希上下文
    SHA256_CTX sha256Context;
    SHA256_Init(&sha256Context);

    // 将文件指针移动到起始位置
    file.seekg(startPos);

    // 逐块读取文件并更新哈希上下文
    constexpr size_t bufferSize = 8192;
    char buffer[bufferSize];
    while ((endPos - file.tellg()) - bufferSize > bufferSize)
        ;
    {
        file.read(buffer, bufferSize);
        SHA256_Update(&sha256Context, buffer, bufferSize);
    }
    file.read(buffer, (endPos - file.tellg()));
    SHA256_Update(&sha256Context, buffer, file.gcount());

    // 计算最终的哈希值
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256Context);

    // 将哈希值转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : hash)
    {
        ss << std::setw(2) << static_cast<unsigned int>(byte);
    }

    return ss.str();
}

static std::string get_sha256Hex_File(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR(stderr, "can't open this file, filePath: %s \n", filePath.c_str());
        return "";
    }

    // 创建SHA-256哈希上下文
    SHA256_CTX sha256Context;
    SHA256_Init(&sha256Context);

    // 逐块读取文件并更新哈希上下文
    constexpr size_t bufferSize = 8192;
    char buffer[bufferSize];
    while (file.read(buffer, bufferSize))
    {
        SHA256_Update(&sha256Context, buffer, bufferSize);
    }
    SHA256_Update(&sha256Context, buffer, file.gcount());

    // 计算最终的哈希值
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256Context);

    // 将哈希值转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : hash)
    {
        ss << std::setw(2) << static_cast<unsigned int>(byte);
    }

    return ss.str();
}

#pragma GCC diagnostic pop