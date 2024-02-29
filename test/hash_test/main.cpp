#include "hash_create.h"

#include <string>

#include <iostream>
#include <cstring>
#include <vector>
#include <openssl/md5.h>  
#include <iomanip>
#include <filesystem>

using namespace std;

// 定义 MD5 算法常量
const int MD5_BLOCK_SIZE = 64;
const int MD5_DIGEST_SIZE = 16;

// 定义 MD5 算法函数
/* void md5(const unsigned char *data, size_t len, unsigned char digest[MD5_DIGEST_SIZE]) {
  // 初始化 MD5 算法变量
  unsigned int A = 0x67452301;
  unsigned int B = 0xefcdab89;
  unsigned int C = 0x98badcfe;
  unsigned int D = 0x10325476;

  // 循环处理数据
  for (size_t i = 0; i < len; i += MD5_BLOCK_SIZE) {
    // 获取当前块数据
    unsigned char block[MD5_BLOCK_SIZE];
    memcpy(block, data + i, MD5_BLOCK_SIZE);

    // 填充数据
    if (len - i < MD5_BLOCK_SIZE) {
      block[len - i] = 0x80;
      for (size_t j = len - i + 1; j < MD5_BLOCK_SIZE; j++) {
        block[j] = 0;
      }
    }

    // 计算 MD5 值
    unsigned int a = A;
    unsigned int b = B;
    unsigned int c = C;
    unsigned int d = D;
    for (int j = 0; j < 64; j++) {
      unsigned int f = 0;
      unsigned int g = 0;
      if (j < 16) {
        f = (b & c) | (~b & d);
        g = j;
      } else if (j < 32) {
        f = (d & b) | (~d & c);
        g = (5 * j + 1) % 16;
      } else if (j < 48) {
        f = b ^ c ^ d;
        g = (3 * j + 5) % 16;
      } else {
        f = c ^ (b | ~d);
        g = (7 * j) % 16;
      }
      f = f + a + ((block[g] << 24) | (block[g + 1] << 16) | (block[g + 2] << 8) | block[g + 3]);
      a = d;
      d = c;
      c = b;
      b = b + ((f << 3) | (f >> 29)) + ((a & b) | (~a & c)) + 0x5a827999;
    }

    // 更新 MD5 算法变量
    A = A + a;
    B = B + b;
    C = C + c;
    D = D + d;
  }

  // 将 MD5 值保存到 digest 数组中
  for (int i = 0; i < MD5_DIGEST_SIZE; i++) {
    digest[i] = (A >> (24 - i * 8)) & 0xff;
  }
} */

int main(int argc, char *argv[])
{
    std::string filePath = "./cloud-map-20231120105959twsjo_ndsCompile.zip";

    std::string result1 = get_sha256Hex_File(filePath);
    std::cout << "result1: " << result1 << std::endl;
    std::uintmax_t fileSize = std::filesystem::file_size(filePath);
    result1 = get_sha256Hex_Range(filePath, 0, fileSize);
    std::cout << "fileSize: " << fileSize << std::endl;
    std::cout << "result1: " << result1 << std::endl;

    // 定义要计算 MD5 值的文件
    string filename = filePath;
    // 打开文件
    ifstream file(filename, ios::binary);
    // 计算文件的 MD5 值
    unsigned char digest[MD5_DIGEST_SIZE];
    MD5((unsigned char *)file.rdbuf(), file.tellg(), digest);
    // 关闭文件
    file.close();
    // 输出 MD5 值
    std::ostringstream sout;  
    sout << std::hex << std::setfill('0');  
    for (long long c: digest) {  
        sout << std::setw(2) << c;  
    } 

    std::string result2 = sout.str();
    std::cout << "result2: " << result2 << std::endl;
    

    return 0;
}