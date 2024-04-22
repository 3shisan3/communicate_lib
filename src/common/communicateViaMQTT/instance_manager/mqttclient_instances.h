/***************************************************************
Copyright (c) 2023-2030, shisan233@sszc.live.
All rights reserved.
File:        mqttclient_instances.h
Version:     1.0
Author:      cjx
start date: 2024-1-09
Description: mqtt客户端接口封装及实例管理
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2024-1-09      cjx            create

*****************************************************************/

#ifndef _MQTTCLIENT_INSTANCES_H_
#define _MQTTCLIENT_INSTANCES_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <mosquittopp.h>

// mqtt中需要的全局变量
extern std::string g_clientId;

// 继承enable_shared_from_this核心是保证类内和外部使用同块内存，共用计数器（保证智能指针内部计数器唯一性）
class MqttClient : public mosqpp::mosquittopp, public std::enable_shared_from_this<MqttClient>
{
public:
    friend class MqttClientIns;

    MqttClient(const char *name = nullptr) : mosqpp::mosquittopp(name) {};

    std::shared_ptr<MqttClient> getPtr()
    {
        return shared_from_this();
    }

    bool getClientUsability() const
    {
        return m_clientStatus != 0;
    }

protected:
    // rc 为 mqtt5_return_codes 可见mqtt_protocol.h
    void on_connect(int rc);
    void on_subscribe(int mid, int qos_count, const int *granted_qos);
    void on_publish(int mid);
    void on_message(const struct mosquitto_message *msg);
    void on_unsubscribe(int mid);
    void on_disconnect(int rc);

private:
    enum client_status_code
    {
        _FAILED = 0,
        CONNECTING,
        CONNECT_SUC,
        // SUBSCRIBE_SUC,
        // PUBLISH_SUC,
        // GETMSG_SUC
    };

    std::mutex m_mutex;
    client_status_code m_clientStatus;
    std::map<std::string, std::string &> m_topicMsg;  // key-topicname, value-msg
};

class MqttClientIns
{
public:
    static std::shared_ptr<MqttClientIns> getInstance();

    ~MqttClientIns();

    /**
     * @brief 连接到代理服务器
     *
     * @return 是否正常连接成功
     */
    bool toConnectBroker(const char *host, int port = 1883, int keepalive = 60);
    
    /**
     * @brief 订阅指定主题消息
     *
     * @return 订阅消息是否成功
     */
    bool toSubscribeTopic(std::string &result, const std::string &brokerAddr, int *mid, const char *sub, int qos = 0);

    /**
     * @brief 发布指定主题消息
     *
     * @return 发布消息是否成功
     */
    bool toPublishTopicMsg(const std::string &brokerAddr, const std::string& msgContent, int *mid, const char *sub, int qos = 0);

    /**
     * @brief 取消订阅指定主题消息
     *
     * @return 取消订阅消息是否成功
     */
    bool toUnsubscribeTopic(const std::string &brokerAddr, int *mid, const char *sub);

    /**
     * @brief 断开到代理服务器的连接
     *
     * @return 断开到代理服务器的连接
     */
    bool toDisconnectBroker(const std::string &brokerAddr);


private:
    MqttClientIns();


private:
    std::map<std::string, std::shared_ptr<MqttClient>> m_mqttClientIns;  // key-broker_addr, value-mqttclient
};





#endif