/***************************************************************
Copyright (c) 2023-2030, shisan233@sszc.live.
All rights reserved.
File:        mqttcomm_ways.h
Version:     1.0
Author:      cjx
start date: 2023-8-28
Description: mqtt客户端订阅行为相关内容
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef _MQTTCOMM_WAYS_H_
#define _MQTTCOMM_WAYS_H_

#include <string>

#include "instance_manager/mqttclient_instances.h"

namespace communicate
{

enum MQTT_ERROR_CODE
{
    SUCCESS = 0,
    CONNECT_FAILED,
    SUBSCRIBE_FAILED,
    PUBLISH_FAILED,
    UNSUBSCRIBE_FAILED,
    DISCONNECT_FAILED
};

struct ConnectInfo
{
    std::string host = "localhost"; // 要连接的代理的主机名或 IP 地址
    int port = 1883;                // 要连接的网络端口, mqtt通常是 1883
    int keepalive = 60;             // 如果在这段时间内（秒数）没有交换其他消息，代理应该向客户端发送 PING 消息
};

struct TopicInfo
{
    int * mid = nullptr;            // 如果不为 NULL，该函数会将其设置为此特定消息的消息ID，然后可以将其与subscribe 回调一起使用，以确定消息何时发送
    std::string topicName;          // 订阅消息对象
    int qos = 2;                    // 消息质量（0 最多传输一次，不保证消息一定会被接收  1 至少传输一次，但可能会接收多次
                                    // 2 只传输一次，确保消息只被接收一次）
};

class MqttCommWays
{
public:
    /**
     * @brief 订阅接口, 外部拉线程循环获取result处理
     *
     * @param[out] result           订阅的消息收到的结果
     * @param[in] addr              订阅目标的网络地址信息
     * @param[in] topic             订阅目标的标识信息
     *
     * @return 状态码
     */
    static MQTT_ERROR_CODE subMqttTopic(std::string& result, const ConnectInfo& addr, const TopicInfo& topic);

    /**
     * @brief 发布接口
     *
     * @param[in] addr              发布目标服务的网络地址信息
     * @param[in] topic             发布目标的标识信息
     * @param[in] msgContent        发布的具体内容
     *
     * @return 状态码
     */
    static MQTT_ERROR_CODE pubMqttTopicMsg(const ConnectInfo& addr, const TopicInfo& topic, const std::string& msgContent);

    /**
     * @brief 取消订阅接口
     *
     * @param[in] addr              订阅目标的网络地址信息
     * @param[in] topic             订阅目标的标识信息
     *
     * @return 状态码
     */
    static MQTT_ERROR_CODE unsubMqttTopic(const ConnectInfo& addr, const TopicInfo& topic);

    /**
     * @brief 直接断开代理服务器连接
     *
     * @param[in] addr              订阅目标的网络地址信息
     *
     * @return 状态码
     */
    static MQTT_ERROR_CODE disconnectMqttBroker(const ConnectInfo& addr);

};

}


#endif
