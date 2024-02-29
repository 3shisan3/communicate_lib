#include "mqttcomm_ways.h"

namespace communicate
{

MQTT_ERROR_CODE MqttCommWays::subMqttTopic(std::string& result, const ConnectInfo& addr, const TopicInfo& topic)
{
	auto ins = MqttClientIns::getInstance();
	if (!ins->toConnectBroker(addr.host.c_str(), addr.port, addr.port))
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
	if (!ins->toConnectBroker(addr.host.c_str(), addr.port, addr.port))
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

}
