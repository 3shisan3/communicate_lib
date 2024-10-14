/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        register_funcs_base.h
Version:     1.0
Author:      cjx
start date: 2024-10-13
Description: 注册使用函数
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2024-10-13      cjx        create

*****************************************************************/

#ifndef _REGISTER_FUNCTION_BASE_H_
#define _REGISTER_FUNCTION_BASE_H_

#include <functional>
#include <string>
#include <unordered_map>

#include <WFFacilities.h>

namespace communicate::http
{

// 全局map来存储函数名和函数指针（extern此处非必要 模板类型本身遵循一次定义规则）
extern std::unordered_map<std::string, std::function<void(WFHttpTask *)>> g_funcsMap;

// 宏定义用于注册函数（name 为标识符，使用传入非字符串）
#define REGISTER_FUNCTION(name, func) \
    static struct Registrar##name { \
        Registrar##name() { \
            g_funcsMap[#name] = func; \
        } \
    } registrar##name;

} // communicate::http
#endif