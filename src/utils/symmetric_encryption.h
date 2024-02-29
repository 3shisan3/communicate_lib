#pragma once

// 忽略openssl3.0弃用了代码中的接口，产生的警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/aes.h>
#include <openssl/err.h>

#include <string>
#include <string.h>

#define DEFAULT_LOCAL_KEY "内部特殊加密"

/**
 * @brief 密匙加密
 *
 * @param[in] content    加密内容
 * @param[in] key_str    密匙内容
 *
 * @return 加密结果
 */
static std::string GetEncryptData(const std::string &content, const std::string &key_str = DEFAULT_LOCAL_KEY)
{
    std::string result(content.length(), '\0'); // 预分配与明文相同长度的密文空间  
    // 创建 AES 密钥
    AES_KEY key_aes;
    // 密匙长度bit为单位
    int key_len = key_str.size() * 8 > 128 ? 256 : 128; // 只允许128, 192, 256
    std::string rel_str = key_str;
    rel_str.append(key_len - key_str.length(), '3');
    const unsigned char *key = (const unsigned char *)rel_str.c_str();
    // 生成 AES 密钥
    if (AES_set_encrypt_key(key, key_len, &key_aes) != 0)
    {
        ERR_print_errors_fp(stderr);
        return "";
    }

    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE); // 使用全0的初始化向量
    int len = content.length();
    int blocks = len / AES_BLOCK_SIZE;
    if (len % AES_BLOCK_SIZE != 0)
    {
        blocks++; // 如果明文长度不是块大小的倍数，则需要增加一个填充块
    }
    result.resize(blocks * AES_BLOCK_SIZE); // 预分配与密文相同长度的空间

    for (int i = 0; i < blocks; i++)
    {
        if (i == blocks - 1)
        { // 最后一个块需要填充
            int padding = AES_BLOCK_SIZE - (len - i * AES_BLOCK_SIZE) % AES_BLOCK_SIZE;
            for (int j = 0; j < padding; j++)
            {
                result[i * AES_BLOCK_SIZE + j] = padding;
            }
        }
        AES_cbc_encrypt((unsigned char *)content.c_str() + i * AES_BLOCK_SIZE, (unsigned char *)result.c_str() + i * AES_BLOCK_SIZE, AES_BLOCK_SIZE, &key_aes, iv, AES_ENCRYPT);
    }

    return result;
}

/**
 * @brief 密匙解密
 *
 * @param[in] content    解密内容
 * @param[in] key_str    密匙内容
 *
 * @return 解密结果
 */
static std::string GetDecryptData(const std::string &content, const std::string &key_str = DEFAULT_LOCAL_KEY)
{
    std::string result(content.length(), '\0'); // 预分配与密文相同长度的解密文本空间 
    // 创建 AES 密钥
    AES_KEY key_aes;
    // 密匙长度bit为单位
    int key_len = key_str.size() * 8 > 128 ? 256 : 128; // 只允许128, 192, 256
    std::string rel_str = key_str;
    rel_str.append(key_len - key_str.length(), '3');
    const unsigned char *key = (const unsigned char *)rel_str.c_str();
    // 生成 AES 密钥
    if (AES_set_decrypt_key(key, key_len, &key_aes) != 0)
    {
        ERR_print_errors_fp(stderr);
        return "";
    }

    unsigned char iv[AES_BLOCK_SIZE];  
    memset(iv, 0, AES_BLOCK_SIZE); // 使用与加密相同的初始化向量

    int len = content.length();  
    int blocks = len / AES_BLOCK_SIZE;

    result.resize(blocks * AES_BLOCK_SIZE); // 预分配与明文相同长度的空间  

    for (int i = 0; i < blocks; i++) {  
        AES_cbc_encrypt((unsigned char*)content.c_str() + i * AES_BLOCK_SIZE, (unsigned char*)result.c_str() + i * AES_BLOCK_SIZE, AES_BLOCK_SIZE, &key_aes, iv, AES_DECRYPT);  
    } 

    return result;
}

#pragma GCC diagnostic pop
