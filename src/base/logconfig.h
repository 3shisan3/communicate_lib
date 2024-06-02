/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        logconfig.h
Version:     1.0
Author:      cjx
start date: 2023-9-21
Description: 常用方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-9-21      cjx        create

*****************************************************************/

#ifndef _LOGCONFIG_H_
#define _LOGCONFIG_H_

#include <iostream>
#include <stdio.h>

// 定义日志级别宏  
#define LOG_LEVEL_DEBUG 1  
#define LOG_LEVEL_INFO 2  
#define LOG_LEVEL_WARNING 3  
#define LOG_LEVEL_ERROR 4  

// 设置全局日志级别
#ifndef GLOBAL_LOG_LEVEL
    #define GLOBAL_LOG_LEVEL LOG_LEVEL_INFO
#endif

#ifndef PROJECT_NAME
    #define PROJECT_NAME ""
#endif

// 定义日志打印宏  
#define LOG_DEBUG(file, ...) do { \
    if (LOG_LEVEL_DEBUG >= GLOBAL_LOG_LEVEL) \
    { \
        fprintf(file, "[%sDEBUG]: ", PROJECT_NAME); \
        fprintf(file, __VA_ARGS__); \
    } } while (false)

#define LOG_INFO(file, ...) do { \
    if (LOG_LEVEL_INFO >= GLOBAL_LOG_LEVEL) \
    { \
        fprintf(file, "[%sINFO]: ", PROJECT_NAME); \
        fprintf(file, __VA_ARGS__); \
    } } while (false)

#define LOG_WARNING(file, ...) do { \
    if (LOG_LEVEL_WARNING >= GLOBAL_LOG_LEVEL) \
    { \
        fprintf(file, "[%sWARNING]: ", PROJECT_NAME); \
        fprintf(file, __VA_ARGS__); \
    } } while (false)

#define LOG_ERROR(file, ...) do { \
    if (LOG_LEVEL_ERROR >= GLOBAL_LOG_LEVEL) \
    { \
        fprintf(file, "[%sERROR]: ", PROJECT_NAME); \
        fprintf(file, __VA_ARGS__); \
    } } while (false)


#endif