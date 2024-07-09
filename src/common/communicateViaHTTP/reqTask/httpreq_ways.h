/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        httpreq_ways.h
Version:     1.0
Author:      cjx
start date: 2023-8-28
Description: 通用的请求方式封装
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef HTTPREQ_WAYS_H
#define HTTPREQ_WAYS_H

#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "httpcomm_structs.h"

namespace communicate
{

class MultipartParser;
struct CommContext
{
    uint8_t curTaskIndex;
    bool communicate_status;
    std::string resp_data;
};

class HttpReqWays
{
public:
    //
    static bool reqToGetResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo = "", const std::string &headerInfo = "");

    static bool reqToPostResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo = "", const std::string &headerInfo = "");

    /**
     * @brief 发送文件请求
     *
     * @param[out] result           上传请求后的返回信息
     * @param[in] filePathsStr      上传的所有文件路径集合（一次请求，传多个文件）(json字符串）
     * @param[in] reqAddr           请求访问的网址
     * @param[in] infoStr           请求上传文件时附带的信息内容(json字符串）
     * @param[in] headerInfoStr     请求需要额外增添的头信息(json字符串）
     *
     * @return 请求通讯是否成功
     */
    static bool reqToSendData(std::string &result, const std::string &filePathsStr, const std::string &reqAddr,
							  const std::string &infoStr = "", const std::string &headerInfoStr = "");

protected:
    static WFHttpTask *getCommonReqTask(const std::string &reqAddr, const std::string &reqInfo = "",
                                        const char *methodType = "GET", const json_object_t *headerInfo = nullptr);

    static WFHttpTask *getReqSendTask(MultipartParser &parser, const std::string &reqAddr, const json_object_t *headerInfo = nullptr);
    static WFHttpTask *getCommonReqSendTask(const json_object_t *filePaths, const std::string &reqAddr,
                                            const json_object_t *info = nullptr, const json_object_t *headerInfo = nullptr);

    /**
     * @brief 排序请求任务
     *
     * @param[out] finResult        最终的云端回复
     * @param[out] failTaskIndex    失败任务序号
     * @param[in] vTasks            请求的任务集合
     *
     * @return 请求通讯是否成功
     */
    static bool reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::string &finResult, uint8_t &failTaskIndex);
    /**
     * @brief 并行请求任务
     *
     * @param[out] allResult        所有任务的云端回复
     * @param[in] vTasks            请求的任务集合
     *
     * @return 通讯成功任务序号
     */
    static std::vector<int> reqGetRespByParallel(const std::vector<WFHttpTask *> &vTasks, std::vector<std::string> &allResult);

    static void base_callback_deal(WFHttpTask *task);
private:
    static inline void debugPrint(protocol::HttpRequest *req, protocol::HttpResponse *resp);
    static void wget_info_callback(WFHttpTask *task);
};

}
#endif