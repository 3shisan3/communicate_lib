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

#include <functional>
#include <map>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "httpcomm_structs.h"

namespace communicate::http
{

struct ReconnectCfg
{
    bool enable_features = false;   // 启用重新尝试功能
    uint32_t max_spaceTime = 60;    // 最大任务重连间隔（单位：秒） 
    uint8_t max_nums = 3;           // 最大失败任务重连次数
    uint32_t max_waitTime = 0;      // 整个任务流程最多花费时间（单位：秒）（超时直接结束并返回）
                                    //（0为不考虑任务花费时间，直到重连次数尝试完）
};

class HttpReqWays
{
public:
    // 控制全局默认的重试状态
    static void initHttpReqParams(const ReconnectCfg &cfg);
    static void initHttpReqParams(const std::string &cfgPath);  // 配置项不多，引入配置文件增加程序复杂度，暂不实现
    
    //
    static HTTP_ERROR_CODE reqToGetResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo = "", const std::string &headerInfo = "");

    static HTTP_ERROR_CODE reqToPostResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo = "", const std::string &headerInfo = "");

    /**
     * @brief 发送文件请求
     *
     * @param[out] result           上传请求后的返回信息
     * @param[in] filePathsStr      上传的所有文件路径集合（一次请求，传多个文件）(json字符串）
     * @param[in] reqAddr           请求访问的网址
     * @param[in] infoStr           请求上传文件时附带的信息内容(json字符串）
     * @param[in] headerInfoStr     请求需要额外增添的头信息(json字符串）
     *
     * @return 请求通讯的状态
     */
    static HTTP_ERROR_CODE reqToSendData(std::string &result, const std::string &filePathsStr, const std::string &reqAddr,
                                         const std::string &infoStr = "", const std::string &headerInfoStr = "");

public:
    /* The following functions are intended for task implementations only. */

    /**
     * @brief 获取任务接口主要入参描述
     *
     * @param[in] filePaths         以文件名为key，文件地址为value组成的json字符串
     * @param[in] reqInfo           请求时和云端商定所需要发送的信息（未强制要求json）
     * @param[in] headerInfoStr     请求需要额外增添的头信息(json字符串）
     * @param[in] infoStr           发送时和云端商定所需要除文件信息外额外要求附带的信息（此处混合文件数据的处理，要求必须为json）
     *
     * @return 生成的任务，可直接start启动，亦可添加到后续请求队列中
     */
    static WFHttpTask *getCommonReqTask(const std::string &reqAddr, const std::string &reqInfo = "", const ReconnectCfg &promiseReqSuc = {},
                                        const char *methodType = HttpMethodGet, const std::string &headerInfoStr = "");
    static WFHttpTask *getReqSendTask(MultipartParser &parser, const std::string &reqAddr, const ReconnectCfg &promiseReqSuc = {}, const std::string &headerInfoStr = "");
    static WFHttpTask *getCommonReqSendTask(const std::string &filePaths, const std::string &reqAddr, const std::string &infoStr = "",
                                            const ReconnectCfg &promiseReqSuc = {}, const std::string &headerInfoStr = "");

    static void startTask(WFHttpTask *task);    // 主要通过日志打印，判断任务执行，暂不提供其他接口（测试使用）
    
    // 以下为同步返回结果方法
    /**
     * @brief 有序请求任务
     *
     * @param[out] allResult        所有任务的云端回复，与入参vTasks序号一致
     *                              最后结果为失败前最后成功任务的返回
     * @param[out] failTaskIndex    失败任务序号
     * @param[in] vTasks            请求的任务集合
     *
     * @return 请求通讯状态码
     */
    static HTTP_ERROR_CODE reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::vector<std::string> &allResult, uint8_t &failTaskIndex);
    /**
     * @brief 有序请求任务（任务之间无依赖关系）
     *
     * @param[in] vTasks            请求的任务集合
     * @param[out] allResult        所有任务的云端回复
     *
     * @return 请求失败的任务序号，及状态码
     */
    static std::map<int, HTTP_ERROR_CODE> reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::vector<std::string> &allResult);
    /**
     * @brief 并行请求任务
     *
     * @param[out] allResult        所有任务的云端回复
     * @param[in] vTasks            请求的任务集合
     *
     * @return 请求失败的任务序号，及状态码
     */
    static std::map<int, HTTP_ERROR_CODE> reqGetRespByParallel(const std::vector<WFHttpTask *> &vTasks, std::vector<std::string> &allResult);

    // 设置多任务请求执行过程中处理任务状态的函数
    /**
     * @brief 设置每个任务执行后状态记录的处理方法
     *
     * @param[in] func              替换默认方法，外部输入
     * @param[in] func->tuple       first为请求url，second为taskId, 两者合为key；third为value
     * 
     */
    using noteTasksStatusCallback = std::function<void(const std::tuple<std::string, std::string, HTTP_ERROR_CODE> &)>;
    static void setTaskStatusFunc(const noteTasksStatusCallback &func);

protected:
    struct CommContext
    {
        uint8_t curTaskIndex = 0;
        HTTP_ERROR_CODE communicate_status = HTTP_ERROR_CODE::SUCCESS;
        std::vector<std::string> all_resp_data;

        virtual ~CommContext() {} // 主要目的保证dynamic_cast安全的向下类型转换
    };

    // 默认是通过请求信息使用sha256计算获得唯一标识
    static std::string obtain_task_id(WFHttpTask *task);

    static void base_callback_deal(WFHttpTask *task);

private:
    struct MultitaskCommContext;

    static inline void debugPrint(protocol::HttpRequest *req, protocol::HttpResponse *resp);
    static void wget_info_callback(WFHttpTask *task);
    // 尽可能保证请求成功的回调
    static void wget_promise_callback(WFHttpTask *task, int maxSpaceTime, int maxWaitTime, int maxRetryNum);

    // 网络请求执行过程中去获取任务状态，操作函数
    static noteTasksStatusCallback statusCallbackFunc;
};

}
#endif