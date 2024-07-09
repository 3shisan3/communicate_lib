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

#ifndef _CHUNK_MANAGER_H_
#define _CHUNK_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "logconfig.h"
#include "httpcomm_structs.h"

extern std::string g_saveFileDir;

namespace communicate
{

class ChunkManager
{
public:
    static std::shared_ptr<ChunkManager> getInstance();

    // ~ChunkManager();

private:
    ChunkManager();

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