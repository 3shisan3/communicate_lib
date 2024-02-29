#pragma once

// 忽略openssl3.0弃用了代码中的接口，产生的警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <cstring>
#include <string>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "base/logconfig.h"

#define RSA_KEYSUB_LEN    64

/*
@brief : 公钥格式调整标准
@para  : key -[i] 处理公钥
@return: 格式化后的公钥
**/
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

/*
@brief : 公钥解密
@para  : cipher_text -[i] 加密的密文
         pub_key     -[i] 公钥
@return: 解密后的数据
**/
/* std::string RsaPubDecrypt(const std::string & cipher_text, const std::string & pub_key)
{
	std::string decrypt_text;
	BIO *keybio = BIO_new_mem_buf((unsigned char *)pub_key.c_str(), -1);
	RSA *rsa = RSA_new();
	
	// 注意--------使用第1种格式的公钥进行解密
	// rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
	// 注意--------使用第2种格式的公钥进行解密（我们使用这种格式作为示例）
	rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
	if (!rsa)
	{
		unsigned long err= ERR_get_error(); //获取错误号
		char err_msg[1024] = { 0 };
        ERR_error_string(err, err_msg); // 格式：error:errId:库:函数:原因
		printf("err msg: err:%ld, msg:%s\n", err, err_msg);
		BIO_free_all(keybio);
        return decrypt_text;
	}
 
	int len = RSA_size(rsa);
	char *text = new char[len + 1];
	memset(text, 0, len + 1);
	// 对密文进行解密
	int ret = RSA_public_decrypt(cipher_text.length(), (const unsigned char*)cipher_text.c_str(), (unsigned char*)text, rsa, RSA_PKCS1_PADDING);
	if (ret >= 0) {
		decrypt_text.append(std::string(text, ret));
	}
 
	// 释放内存  
	delete text;
	BIO_free_all(keybio);
	RSA_free(rsa);
 
	return decrypt_text;
} */


/*
@brief : 公钥解密
@para  : cipher_text -[i] 加密的密文
         curPubkey   -[i] 公钥
@return: 解密后的数据
**/
std::string RsaPubDecrypt(const std::string & cipher_text, const std::string & curPubkey)
{
	std::string pub_key = curPubkey;
	// 判断公钥是否符合内部标准
	const std::string identifier = "PUBLIC KEY";
	if (curPubkey.substr(0, RSA_KEYSUB_LEN).find(identifier) == std::string::npos ||
		curPubkey.rfind(identifier, RSA_KEYSUB_LEN) == std::string::npos)
	{
		pub_key = FormatPubKey(curPubkey);
	}

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

		LOG_DEBUG(stderr, "err msg: err:%ld, msg:%s\n", err , err_msg);

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

			// LOG_DEBUG(stderr, "pos:%d, sub: %s\n", pos, sub_text);

			pos += len;
		}
	}
 
	// 释放内存  
	delete sub_text;
	BIO_free_all(keybio);
	RSA_free(rsa);
 
	return decrypt_text;
}

#pragma GCC diagnostic pop