/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        data_manager.h
Version:     1.0
Author:      cjx
start date: 2023-10-16
Description: 文件管理层
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-10-16      cjx        create

*****************************************************************/

#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/logconfig.h"

extern std::string g_saveFileDir;

namespace communicate
{

// 存入读取，使用sizeof需要明确结构体大小，std::string 不符合
struct Resume_FileInfo
{
    char resumeName[198];       // 断电续传信息的记录文件名
    char filePath[328];         // 记录信息的目标文件路径
    char fileHash[68];          // 锁定文件的hash值
    long long lastEndPos = 0;   // 最终处理结束的下标
    uint32_t chunkSize = 0;     // 分块尺寸
    char commAddr[648];         // 交互的地址   “作为任务划分的key”
};

class DataManager
{
public:
    static std::shared_ptr<DataManager> getInstance();

    // ~DataManager();

private:
    DataManager();

    /**
     * @brief 断点续传上传, 及下载状态获取
     *
     * @return 
     */
    void getAllResumeTasks();

public:
    /**
     * @brief 查询指定断点续传下载任务状态
     * 
     * @param[in] taskAddr         任务请求地址
     *
     * @return 获取下载任务信息
     */
    const Resume_FileInfo *getDownTaskInfo(const std::string &taskAddr) const;
    /**
     * @brief 查询指定断点续传上传任务状态
     * 
     * @param[in] taskAddr         任务请求地址
     *
     * @return 获取上传任务信息集合
     */
    const std::vector<const Resume_FileInfo *> getSendTaskInfo(const std::string &taskAddr) const;

    /**
     * @brief 存储断点续传下载任务状态
     * 
     * @param[in] taskAddr         存储的下载任务信息
     * @param[in] fileName         指定存入信息的名字
     *
     * @return 
     */
    void saveDownTaskInfo(const Resume_FileInfo &taskInfo, std::string fileName = "");

    /**
     * @brief 删除指定断点续传下载任务记录
     * 
     * @param[in] taskAddr         存储的下载任务信息
     *
     * @return 
     */
    void delDownTaskInfo(const std::string &taskAddr);


private:
    // 下载任务只会一对一
    std::map<std::string, Resume_FileInfo> m_downResume_;
    // 上传任务存在一对多
    std::multimap<std::string, Resume_FileInfo> m_uploadResume_;

};

}
#endif