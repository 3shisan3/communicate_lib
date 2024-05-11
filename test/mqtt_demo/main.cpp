#include "mqttcomm_ways.h"

#include <atomic>
#include <iostream>
#include <thread>

int main(int argc, char *argv[])
{
	communicate::ConnectInfo param1;
	communicate::TopicInfo param2;
	param2.topicName = "test";
	param2.qos = 0;

	std::string sub_result = "";
	int rescode_sub = communicate::MqttCommWays::subMqttTopic(sub_result, param1, param2);
	std::cout << "rescode_sub: " << rescode_sub << std::endl;

	std::atomic_bool hadPubMsg = false;
	std::thread subInfoPrint([&](){
		while (!hadPubMsg)
		{
			std::cout << "sub_result: " << sub_result << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1)); // 休眠1秒 
		}
	});
	subInfoPrint.detach();

	int rescode_pub1 = communicate::MqttCommWays::pubMqttTopicMsg(param1, param2, "this test content");
	std::cout << "rescode_pub1: " << rescode_pub1 << std::endl;
	
	communicate::TopicInfo param3;
	param3.topicName = "test2";
	int rescode_pub2 = communicate::MqttCommWays::pubMqttTopicMsg(param1, param3, "this test2 content");
	std::cout << "rescode_pub2: " << rescode_pub2 << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds(5)); // 休眠5秒 
	hadPubMsg = true;

	return 0;
}