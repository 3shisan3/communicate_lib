/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        specialsupply_req.h
Version:     1.0
Author:      cjx
start date: 2023-8-30
Description: 特供的处理方法封装
    处理需要做特殊限时，分包大小等请求场景
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-30      cjx        create

*****************************************************************/

#ifndef _SPECIAL_SUPPLY_REQ_H_
#define _SPECIAL_SUPPLY_REQ_H_

#include <fstream>

#include "common/data_manager.h"
#include "httpreq_ways.h"
#include "multipart_parser.h"

namespace communicate
{

class SpecialSupReq : HttpReqWays
{
public:
    /**
     * @brief 分段获取文件(按序下载)
     *
     * @param[out] fileName         存入的指定文件对象
     * @param[in] reqAddr           请求访问的网址
     * @param[in] chunkSize         指定分块大小
     * @param[in] reqInfo           极少可能的需要，get时带上数据
     * @param[in] resumeName        交互过程中存储信息的文件名
     *
     * @return 存入是否完成
     */ 
    static bool reqToGetChunkDataBySeries(const Resume_FileInfo *curInfo, const std::string &reqInfo = "");

    /**
     * @brief 获取文件拆分数据信息
     *
     * @param[in] paramName         文件数据的参数名
     * @param[in] filePath          上传的文件路径
     * @param[in] chunkSize         指定分块大小
     *
     * @return 文件拆分后结构集合
     */ 
    static std::vector<MultipartParser> getFileChunkStructs(const std::string &paramName, const std::string &filePath, uint32_t chunkSize = 0);

    /**
     * @brief 有序发送文件请求（分片传输）
     *
     * @param[out] failTaskIndex    任务失败停止的最后序号
     * @param[out] finResult        最终的云端回复
     * @param[in] chunksInfo        待上传数据信息集合
     * @param[in] reqAddr           请求访问的网址
     *
     * @return 所有上传任务是否通信成功
     */ 
    static bool reqToSendChunkDataBySeries(uint8_t &failTaskIndex, std::string &finResult,
                                           std::vector<MultipartParser> &chunksInfo, const std::string &reqAddr);

    /**
     * @brief 并行发送文件请求（分片传输）
     *
     * @param[out] failTaskIndexs   成功通信完成的任务序号集合
     * @param[out] allResult        所有任务的云端回复
     * @param[in] chunksInfo        待上传数据信息集合
     * @param[in] reqAddr           请求访问的网址
     *
     * @return 所有上传任务是否通信成功
     */ 
    static bool reqToSendChunkDataByParallel(std::vector<int> &failTaskIndexs, std::vector<std::string> &allResult,
                                             std::vector<MultipartParser> &chunksInfo, const std::string &reqAddr);

protected:
    static WFHttpTask *getSpecialReqGetTask(const std::string &reqAddr, const std::string &reqInfo = "", const json_object_t *headerInfo = nullptr);

private:
    struct CommContextAtChunk : public CommContext 
    {
        Resume_FileInfo resumeInfo;
        std::ofstream outFile;
    };

    static void wget_chunk_callback(WFHttpTask *task);

};

}
#endif