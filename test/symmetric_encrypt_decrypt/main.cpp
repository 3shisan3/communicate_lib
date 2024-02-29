#include <base64_trans.h>
#include <symmetric_encryption.h>

#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
    std::string content_path = "./encrypt_content";
    std::string content;

    if (argc > 3)
    {
        std::cout << "参数过多" << std::endl;
        return 1;
    }

    if (argc >= 2)
    {
        std::string temp = argv[1];
        if (temp == "--path")
        {
            content_path = argv[2];
        }
        else
        {
            content = temp;
        }
        std::cout << content << std::endl;
    }

    if (content.empty())
    {
        FILE *file = fopen(content_path.c_str(), "r"); // 打开文件，以只读方式

        if (file)
        {
            fseek(file, 0, SEEK_END);    // 将文件指针定位到文件末尾
            long fileSize = ftell(file); // 获取文件大小
            fseek(file, 0, SEEK_SET);    // 将文件指针重新定位到文件开头

            char *buffer = new char[fileSize + 1];       // 创建足够大的缓冲区来保存文件内容
            fread(buffer, sizeof(char), fileSize, file); // 读取文件内容到缓冲区
            buffer[fileSize] = '\0';                     // 在缓冲区末尾添加字符串结束符

            content = buffer; // 将缓冲区内容转换为字符串

            delete[] buffer; // 释放缓冲区内存
            fclose(file);    // 关闭文件

            std::cout << content << std::endl;
        }
    }

    // std::cout << "GetEncryptData1: " << GetEncryptData(content) << std::endl;
    std::string encrypt_result = base64_encode(GetEncryptData(content));
    std::cout << "encrypt_result: " << encrypt_result << std::endl;

    std::ofstream outfile("encrypt_result.txt"); // 创建输出文件流

    if (outfile.is_open())
    {                              // 检查文件是否成功打开
        outfile << encrypt_result; // 将字符串写入文件
        outfile.close();           // 关闭文件流
        std::cout << "String written to file successfully." << std::endl;
    }
    else
    {
        std::cout << "Unable to open file." << std::endl;
    }
    
    // std::cout << "GetEncryptData2: " << base64_decode(encrypt_result) << std::endl;
    std::string decrypt_result = GetDecryptData(base64_decode(encrypt_result));
    std::cout << "decrypt_result: " << decrypt_result << std::endl;

    return 0;
}