#include "mqttcomm_ways.h"

#include "instance_manager/mqttclient_instances.h"

namespace communicate::mqtt
{

void MqttCommWays::initMqttData(const std::string &clientID)
{
	g_clientId = clientID;
}

MQTT_ERROR_CODE MqttCommWays::subMqttTopic(std::string& result, const ConnectInfo& addr, const TopicInfo& topic)
{
	auto ins = MqttClientIns::getInstance();
	if (!ins->toConnectBroker(addr.host.c_str(), addr.port, addr.keepalive))
	{
		return MQTT_ERROR_CODE::CONNECT_FAILED;
	}
	std::string brokerAddr = addr.host + ":" + std::to_string(addr.port);
	if (!ins->toSubscribeTopic(result, brokerAddr, topic.mid, topic.topicName.c_str(), topic.qos))
	{
		return MQTT_ERROR_CODE::SUBSCRIBE_FAILED;
	}

	return MQTT_ERROR_CODE::SUCCESS;
}


MQTT_ERROR_CODE MqttCommWays::pubMqttTopicMsg(const ConnectInfo& addr, const TopicInfo& topic, const std::string& msgContent)
{
	auto ins = MqttClientIns::getInstance();
	if (!ins->toConnectBroker(addr.host.c_str(), addr.port, addr.keepalive))
	{
		return MQTT_ERROR_CODE::CONNECT_FAILED;
	}
	std::string brokerAddr = addr.host + ":" + std::to_string(addr.port);
	if (!ins->toPublishTopicMsg(brokerAddr, msgContent, topic.mid, topic.topicName.c_str(), topic.qos))
	{
		return MQTT_ERROR_CODE::PUBLISH_FAILED;
	}
	
	return MQTT_ERROR_CODE::SUCCESS;
}

MQTT_ERROR_CODE MqttCommWays::unsubMqttTopic(const ConnectInfo& addr, const TopicInfo& topic)
{
	auto ins = MqttClientIns::getInstance();
	std::string brokerAddr = addr.host + ":" + std::to_string(addr.port);
	if (!ins->toUnsubscribeTopic(brokerAddr, topic.mid, topic.topicName.c_str()))
	{
		return MQTT_ERROR_CODE::UNSUBSCRIBE_FAILED;
	}

	return MQTT_ERROR_CODE::SUCCESS;
}

MQTT_ERROR_CODE MqttCommWays::disconnectMqttBroker(const ConnectInfo& addr)
{
	auto ins = MqttClientIns::getInstance();
	std::string brokerAddr = addr.host + ":" + std::to_string(addr.port);
	if (!ins->toDisconnectBroker(brokerAddr))
	{
		return MQTT_ERROR_CODE::DISCONNECT_FAILED;
	}

	return MQTT_ERROR_CODE::SUCCESS;
}

bool MqttCommWays::setClientRecCfg(const ConnectInfo& addr, const ReconnectCfg &cfg)
{
	auto ins = MqttClientIns::getInstance();
	if (!ins->toConnectBroker(addr.host.c_str(), addr.port, addr.keepalive))
	{
		return false;
	}
	std::string brokerAddr = addr.host + ":" + std::to_string(addr.port);
	if (!ins->setReconnectCfg(brokerAddr, cfg.interval_time, cfg.interval_time_max, cfg.growthIntervval_add))
	{
		return false;
	}

	return true;
}

}
