// #include "license_parse.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>


#define RSA_KEYSUB_LEN    64
 
std::string FormatPubKey(const std::string & key)
{
    std::string pub_key = "-----BEGIN PUBLIC KEY-----\n";
	
    auto pos = key.length();
    pos = 0;
    while (pos < key.length()) {
		pub_key.append(key.substr(pos, RSA_KEYSUB_LEN));
		pub_key.append("\n");
		pos += RSA_KEYSUB_LEN;
	}
 
	pub_key.append("-----END PUBLIC KEY-----");
	return pub_key;

}

std::string base64_decode(const std::string& encoded) {  
    BIO* b64 = BIO_new(BIO_f_base64());  
    BIO* mem = BIO_new(BIO_s_mem());  
    BIO_push(b64, mem);  
    BIO_write(b64, encoded.c_str(), encoded.size());  
    BIO_flush(b64);  
    char* decoded;  
    long len = BIO_get_mem_data(mem, &decoded);  
    std::string result(decoded, len);  
    BIO_free_all(b64);  
    return result;  
}
std::string base64Decode(const std::string& encodedString) {
    const std::string base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<unsigned char> decodedBytes;
    decodedBytes.reserve(encodedString.size());

    size_t padding = 0;
    for (char c : encodedString) {
        if (c == '=')
            padding++;
        else {
            size_t index = base64Chars.find(c);
            if (index != std::string::npos)
                decodedBytes.push_back(static_cast<unsigned char>(index));
        }
    }

    const size_t outputSize = (decodedBytes.size() / 4) * 3 - padding;
    std::string decodedString;
    decodedString.reserve(outputSize);

    for (size_t i = 0; i < decodedBytes.size(); i += 4) {
        unsigned char a = decodedBytes[i];
        unsigned char b = decodedBytes[i + 1];
        unsigned char c = decodedBytes[i + 2];
        unsigned char d = decodedBytes[i + 3];

        unsigned char byte1 = (a << 2) | (b >> 4);
        unsigned char byte2 = (b << 4) | (c >> 2);
        unsigned char byte3 = (c << 6) | d;

        decodedString.push_back(byte1);

        if (i + 2 < decodedBytes.size())
            decodedString.push_back(byte2);

        if (i + 3 < decodedBytes.size())
            decodedString.push_back(byte3);
    }

    return decodedString;
}

const std::string pubKey = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJkPEAjOipY1CoT0UwDcilhbJ7XumQ7Pdihw4PTb5HGnYt02E+6KZuiIYq+2XDg2c29O5tez+M5ZjnForeffgg8CAwEAAQ==";

const std::string dataContent = "DFsQVjA/cy9TDvLStVdd9+mjVbMJ1OKxSgGzq53I3nM1y1ENrZL/gD8t7imlY2xzFkkndJKbnBCKok6rZ6OX4xpMqu+hQThGDgUg0xX3TpJ8rqQtJsEvDfvETwZ3G29A2Y/itA/rN3yKnw1AHsCEUB7sMSlx+uCa6wLsAL2m9+oaFPtmDwHjiVUtaplXaeNJ3m/kieKntZ5ezq3Sf/AruoKo4+nUUGCU0LpGN2CeVT/uDIs7SmWWG3e824hwA1M9R4M8t9mwWuvqp7deXvWA+NeXXdJbyA8q/Bw0bQv45j6Ga6eghX+Eh2ol7xy2aLThMNKou59QgeB/WXAihV6hCwHztHjf/5nzL25cvBYn0cP5XyaA4MaQ8OC1+zC5vVHDJNECEqgb/pxjihzhTMcqP4t6gzUD98MjKE6mF4Xs32g=";

const std::string test_base64 = "MjE0MTI0MTI0";

// 公钥解密    
std::string rsa_pub_decrypt(const std::string& clearText, std::string& pubKey)
{
    std::string strRet;
    BIO* keybio = BIO_new_mem_buf((unsigned char*)pubKey.c_str(), -1);
    //keybio = BIO_new_mem_buf((unsigned char *)strPublicKey.c_str(), -1);  
    // 此处有三种方法  
    // 1, 读取内存里生成的密钥对，再从内存生成rsa  
    // 2, 读取磁盘里生成的密钥对文本文件，在从内存生成rsa  
    // 3，直接从读取文件指针生成rsa  
    //RSA* pRSAPublicKey = RSA_new();  
    RSA* rsa = RSA_new();
    rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL); // rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);如果要私钥解密，函数要变化。
    if (!rsa)
    {
        BIO_free_all(keybio);
        return std::string("");
    }

    int len = RSA_size(rsa);
    //int len = 1028;
    char* encryptedText = (char*)malloc(len + 1);
    memset(encryptedText, 0, len + 1);

    //解密
    int ret = RSA_public_decrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedText, rsa, RSA_PKCS1_PADDING);
    if (ret >= 0)
        strRet = std::string(encryptedText, ret);

    // 释放内存  
    free(encryptedText);
    BIO_free_all(keybio);
    RSA_free(rsa);

    return strRet;
}


/*
@brief : 公钥解密
@para  : cipher_text -[i] 加密的密文
         pub_key     -[i] 公钥
@return: 解密后的数据
**/
std::string RsaPubDecrypt(const std::string & cipher_text, const std::string & pub_key)
{
	std::string decrypt_text;
	BIO *keybio = BIO_new_mem_buf((unsigned char *)pub_key.c_str(), -1);
	RSA* rsa = RSA_new();
	
	// 注意-------使用第1种格式的公钥进行解密
	//rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
	// 注意-------使用第2种格式的公钥进行解密（我们使用这种格式作为示例）
	rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
	if (!rsa)
	{
		unsigned long err = ERR_get_error(); //获取错误号
		char err_msg[1024] = { 0 };
		ERR_error_string(err, err_msg); // 格式：error:errId:库:函数:原因
		printf("err msg: err:%ld, msg:%s\n", err , err_msg);
		BIO_free_all(keybio);
		
        return decrypt_text;
	}
 
	// 获取RSA单次处理的最大长度
	int len = RSA_size(rsa);
	char *sub_text = new char[len + 1];
	memset(sub_text, 0, len + 1);
	int ret = 0;
	std::string sub_str;
	int pos = 0;
	// 对密文进行分段解密
	while (pos < cipher_text.length()) {
		sub_str = cipher_text.substr(pos, len);
		memset(sub_text, 0, len + 1);
		ret = RSA_public_decrypt(sub_str.length(), (const unsigned char*)sub_str.c_str(), (unsigned char*)sub_text, rsa, RSA_PKCS1_PADDING);
		if (ret >= 0) {
			decrypt_text.append(std::string(sub_text, ret));
			printf("pos:%d, sub: %s\n", pos, sub_text);
			pos += len;
		}
	}
 
	// 释放内存  
	delete sub_text;
	BIO_free_all(keybio);
	RSA_free(rsa);
 
	return decrypt_text;
}

int main(int argc, char *argv[])
{
    std::string pubKeyPath = "./data.pem";

    std::string testBase64Ori = base64Decode(test_base64);

    if (argc < 2)
    {
        std::string truPub = FormatPubKey(pubKey);
        std::string truContent= base64Decode(dataContent);
        std::string result = RsaPubDecrypt(truContent, truPub);
        // std::string result = rsa_pub_decrypt(truContent, truPub);
        std::cout << result << std::endl;
        
        return 0;
    }
    
    FILE* file = fopen(argv[1], "r");  // 打开文件，以只读方式

    if (file) {
        fseek(file, 0, SEEK_END);  // 将文件指针定位到文件末尾
        long fileSize = ftell(file);  // 获取文件大小
        fseek(file, 0, SEEK_SET);  // 将文件指针重新定位到文件开头

        char* buffer = new char[fileSize + 1];  // 创建足够大的缓冲区来保存文件内容
        fread(buffer, sizeof(char), fileSize, file);  // 读取文件内容到缓冲区
        buffer[fileSize] = '\0';  // 在缓冲区末尾添加字符串结束符

        std::string fileContent(buffer);  // 将缓冲区内容转换为字符串

        delete[] buffer;  // 释放缓冲区内存
        fclose(file);  // 关闭文件

        std::cout << fileContent << std::endl;
    }

    return 0;
}