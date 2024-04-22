#include "mqttclient_instances.h"

#include <cstring>
#include <thread>

#include "logconfig.h"

std::string g_clientId = "";

void MqttClient::on_connect(int rc)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (rc)
    {
        LOG_ERROR(stderr, "connect to broker failed, failed reason code is %d\n", rc);
        m_clientStatus = _FAILED;
    }
    else
    {
        m_clientStatus = CONNECT_SUC;
    }
}

void MqttClient::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
    LOG_DEBUG(stderr, "topic subscribe success, mid: %d, qos_count %d, granted_qos %d\n", mid, qos_count, *granted_qos);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_clientStatus = CONNECT_SUC;
}

void MqttClient::on_publish(int mid)
{
    LOG_DEBUG(stderr, "topic publish msg status, mid: %d\n", mid);
}

void MqttClient::on_message(const struct mosquitto_message *msg)
{
    LOG_INFO(stderr, "get message content: mid: %d, qos: %d, topicName %s, retain %d\n", msg->mid, msg->qos, msg->topic, msg->retain);

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string topicName = (std::string)msg->topic;
    auto iter = m_topicMsg.find(topicName);
    if (iter == m_topicMsg.end())
    {
        LOG_ERROR(stderr, "invalid topic (%s)\n", msg->topic);
    }
    else
    {
        char str[msg->payloadlen];
        std::memcpy(str, msg->payload, msg->payloadlen);
        iter->second = str;
    }
}

void MqttClient::on_unsubscribe(int mid)
{
    LOG_DEBUG(stderr, "topic unsubscribe success, mid: %d\n", mid);
}

void MqttClient::on_disconnect(int rc)
{
    LOG_DEBUG(stderr, "client disconnect status, rc: %d\n", rc);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (rc)
    {
        reconnect();
    }
    else
    {
        m_clientStatus = _FAILED;
        // 进行资源释放
    }
}


static std::shared_ptr<MqttClientIns> s_mqttclients = nullptr;
static std::once_flag s_singleFlag;

bool g_pubRetain = true;

std::shared_ptr<MqttClientIns> MqttClientIns::getInstance()
{
    std::call_once(s_singleFlag, [&] {
        s_mqttclients = std::shared_ptr<MqttClientIns>(new MqttClientIns());
    });
    return s_mqttclients;
}

MqttClientIns::MqttClientIns()
{
    LOG_INFO(stderr, "mosquitto version is %d\n", mosqpp::lib_version(nullptr, nullptr, nullptr));
    mosqpp::lib_init();

}

MqttClientIns::~MqttClientIns()
{
    // mosquittopp析构会自己调mosquitto_destroy
    m_mqttClientIns.erase(m_mqttClientIns.begin(), m_mqttClientIns.end());  // erase才会调析构
    mosqpp::lib_cleanup();
}

bool MqttClientIns::toConnectBroker(const char *host, int port, int keepalive)
{
    std::string brokerAddr = (std::string)host + ":" + std::to_string(port);

    auto iter = m_mqttClientIns.find(brokerAddr);
    if (iter != m_mqttClientIns.end())
    {
        // std::lock_guard<std::mutex> lock(iter->second->m_mutex);
        if (iter->second->m_clientStatus == MqttClient::client_status_code::CONNECTING)
        {
            LOG_DEBUG(stderr, "wait loop sync time ~");
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待loop_start网络回调的操作，休眠1秒
        }
        if (!iter->second->getClientUsability())
        {
            // 会有默认的重连操作
            LOG_ERROR(stderr, "abnormal connection status with the broker, broker addr: %s\n", brokerAddr.c_str());
            // return iter->second->reconnect() == mosq_err_t::MOSQ_ERR_SUCCESS;
            return false;
        }
        return true;
    }

    std::shared_ptr<MqttClient> newClient = g_clientId.empty() ? std::make_shared<MqttClient>() : std::make_shared<MqttClient>(g_clientId.c_str());
    int status = newClient->connect(host, port, keepalive);
    {
        std::lock_guard<std::mutex> lock(newClient->m_mutex);
        newClient->m_clientStatus = MqttClient::client_status_code::CONNECTING;
    }
    if (status == mosq_err_t::MOSQ_ERR_SUCCESS)
    {
        status = newClient->loop_start();

        m_mqttClientIns.insert({brokerAddr, newClient});
    }

    return status == mosq_err_t::MOSQ_ERR_SUCCESS;
}

bool MqttClientIns::toSubscribeTopic(std::string &result, const std::string &brokerAddr, int *mid, const char *sub, int qos)
{
    auto iter = m_mqttClientIns.find(brokerAddr);
    if (iter == m_mqttClientIns.end())
    {
        LOG_ERROR(stderr, "The subscription message: %s did not find the specified server: %s\n", sub, brokerAddr.c_str());
        return false;
    }

    std::shared_ptr<MqttClient> curClient = iter->second;
    std::string topicName = (std::string)sub;
    if (curClient->m_topicMsg.find(topicName) != curClient->m_topicMsg.end())
    {
        return curClient->getClientUsability();
    }

    curClient->m_topicMsg.insert({topicName, result});
    return curClient->subscribe(mid, sub, qos) == mosq_err_t::MOSQ_ERR_SUCCESS;
}

bool MqttClientIns::toPublishTopicMsg(const std::string &brokerAddr, const std::string& msgContent, int *mid, const char *sub, int qos)
{
    auto iter = m_mqttClientIns.find(brokerAddr);
    if (iter == m_mqttClientIns.end())
    {
        LOG_ERROR(stderr, "The publish message: %s did not find the specified server: %s\n", sub, brokerAddr.c_str());
        return false;
    }

    // retain 是否保留消息，已确保必定被订阅收到(每个Client订阅Topic后会立即读取到retain消息，不必要等待发送)
    return iter->second->publish(mid, sub, sizeof(msgContent), msgContent.c_str(), qos, g_pubRetain) == mosq_err_t::MOSQ_ERR_SUCCESS;
}

bool MqttClientIns::toUnsubscribeTopic(const std::string &brokerAddr, int *mid, const char *sub)
{
    auto iter = m_mqttClientIns.find(brokerAddr);
    if (iter == m_mqttClientIns.end())
    {
        LOG_ERROR(stderr, "The unsubscription message: %s did not find the specified server: %s\n", sub, brokerAddr.c_str());
        return false;
    }

    std::shared_ptr<MqttClient> curClient = iter->second;
    std::string topicName = (std::string)sub;
    if (curClient->m_topicMsg.find(topicName) == curClient->m_topicMsg.end())
    {
        LOG_WARNING(stderr, "The unsubscription message: %s did not subscribe at the broker: %s\n", sub, brokerAddr.c_str());
        return false;
    }

    bool res = iter->second->unsubscribe(mid, sub) == mosq_err_t::MOSQ_ERR_SUCCESS;
    res = res && curClient->m_topicMsg.erase(topicName);
    // if (curClient->m_topicMsg.empty())
    // {
    //     toDisconnectBroker(brokerAddr);
    // }
    return res;
}

bool MqttClientIns::toDisconnectBroker(const std::string &brokerAddr)
{
    auto iter = m_mqttClientIns.find(brokerAddr);
    if (iter == m_mqttClientIns.end())
    {
        LOG_ERROR(stderr, "The client disconnect broker: %s failed\n", brokerAddr.c_str());
        return false;
    }

    bool res = iter->second->disconnect() == mosq_err_t::MOSQ_ERR_SUCCESS;
    res = res && (iter->second->loop_stop() == mosq_err_t::MOSQ_ERR_SUCCESS);
    res = res && m_mqttClientIns.erase(brokerAddr);   // erase 返回的是删除的数量
    return res;
}