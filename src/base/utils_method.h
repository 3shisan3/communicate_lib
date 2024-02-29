/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        utils_method.h
Version:     1.0
Author:      cjx
start date: 2023-8-28
Description: 常用方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef _UTILS_METHOD_H_
#define _UTILS_METHOD_H_

#include <cmath>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <vector>

#include "logconfig.h"

// 获取当前时间
inline void getDateStamp(char * buf, size_t n)
{
	auto tt = std::time(nullptr);
	std::tm cur{};
	// gmtime_r(&tt, &cur);
	localtime_r(&tt, &cur);
	strftime(buf, n, "%a, %d %b %Y %H:%M:%S %Z", &cur);
}

// linux上获取指定网卡mac地址
static std::string getMACaddrAtLinux(uint8_t n = 0)
{
	// 获取当前网卡的名称
	std::string folderPath = "/sys/class/net"; // 指定文件夹路径
	std::string prefix = "e";				   // 指定文件名前缀
	std::string interface = "eth0";			   // 根据实际情况设置正确的网卡名称
	for (const auto &entry : std::filesystem::directory_iterator(folderPath))
	{
		std::string filename = entry.path().filename().string();
		if (filename.substr(0, prefix.size()) == prefix)
		{
			interface = filename;
			break;
		}
	}

	// 获取网卡的 MAC 地址
	std::string mac_address = "";
	std::string command = "";
	switch (n)
	{
	case 0:
		command = "ifconfig " + interface + " | grep ether | awk '{print $2}'";
		break;
	case 1:
		command = "ip addr show " + interface + " | grep ether | awk '{print $2}'";
		break;
	case 2:
		command = "cat " + folderPath + "/" + interface + "/address"; 
		// command = "netstat -rn | grep " + interface + " | awk '{print $2}'";
		break;
	case 3:
		command = "ss -ln | grep " + interface + " | awk '{print $2}'";
		break;
	case 4:
		{
			// 获取ip来锁定网卡，获取MAC (arp命令)
			// 搭载域控时常换，暂不实现
		}
		break;
	default:
		LOG_ERROR(stderr, "all commands for querying MAC addresses are unavailable .");
		return "";
	}

	FILE *pipe = popen(command.c_str(), "r");
	if (pipe)
	{
		char buffer[128];
		while (!feof(pipe))
		{
			if (fgets(buffer, 128, pipe) != nullptr)
			{
				mac_address += buffer;
			}
		}
		pclose(pipe);
	}

	if (mac_address.empty())
	{
		return getMACaddrAtLinux(++n);
	}

	return mac_address;
}

// 字符串查找指定字符第一次出现后字符串内容
inline std::string getStrAtSpecifyChar(const std::string &target, const char symbol)
{
	size_t pos = target.find(symbol);  // 顺着查找指定符号的位置
	if (pos != std::string::npos)
	{
		return target.substr(pos + 1); // 获取指定符号后的子字符串
	}

	LOG_INFO(stderr, "don't find this symbol '%c' at string %s .", symbol, target.c_str());
	return target;
}

// 字符串查找指定字符最后一次出现后字符串内容
inline std::string getStrAtSpecifyCharRfind(const std::string &target, const char symbol)
{
	size_t pos = target.rfind(symbol); // 倒着查找指定符号的位置
	if (pos != std::string::npos)
	{
		return target.substr(pos + 1); // 获取指定符号后的子字符串
	}

	LOG_INFO(stderr, "don't find this symbol '%c' at string %s .", symbol, target.c_str());
	return target;
}

// 获取文件夹下带有指定后缀的文件集合
inline std::vector<std::string> getFilesWithExtension(const std::string &directory, const std::string &extension)
{
	std::vector<std::string> filePaths;

	for (const auto &entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			std::string fileName = entry.path().filename().string();
			if (fileName.rfind(extension) == fileName.size() - extension.size())
			{
				filePaths.push_back(entry.path().string());
			}
		}
	}

	return filePaths;
}

// 校验文件中内容指定字节数和输入字符串的差异
inline bool checkPreviousBytes(std::ifstream &file, uint16_t checkBytesNumber, const std::string &str)
{
	if (file.is_open())
	{
		file.seekg(-checkBytesNumber, std::ios::cur); // 将读取位置移动到前n个字节的位置

		char buffer[checkBytesNumber + 1];
		file.read(buffer, checkBytesNumber);		  // 读取前n个字节

		buffer[checkBytesNumber] = '\0';			  // 添加字符串结尾符

		std::string previousBytes(buffer, checkBytesNumber);

		file.close();

		return (previousBytes == str); 				  // 比较前十个字节与指定字符串
	}

	return false;
}

// 判断浮点类型是否有小数部分
inline bool hasDecimal(double value)
{  
    return std::abs(value - std::floor(value)) > std::numeric_limits<double>::epsilon();  
}  

#endif