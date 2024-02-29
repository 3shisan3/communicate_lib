#include "websocket_server.h"

#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <thread>  
#include <chrono>
#include <HttpMessage.h>
#include <HttpUtil.h>
#include <json_parser.h>
#include <WFHttpServer.h>
#include <WFServer.h>
#include <WFFacilities.h>

#include "json.hpp"

namespace communicate
{
using json = ::nlohmann::json;

void WebSocketServer::defTextProcessDealFn(WebSocketChannel *ws, protocol::WebSocketFrame *in)
{
    // 打印收到信息
    LOG_DEBUG(stderr, "get client msg size: %s \n", (char *)in->get_parser()->payload_length);
    LOG_DEBUG(stderr, "get client msg content: %s \n", (char *)in->get_parser()->payload_data);

    // 收到内容
    std::string inStr = (char *)in->get_parser()->payload_data;

    // 测试
    if (inStr == "personal internal testing instructions")
    {
        ws->set_process_text_fn(testChangeMsgFn);
        return;
    }

    // 发送回复， 等价于 send_text()
    ws->send_frame(
        (char *)in->get_parser()->payload_data, 
        in->get_parser()->payload_length,
        in->get_parser()->payload_length,
        WebSocketFrameText);

}

bool WebSocketServer::registerServer(unsigned short port, const char *cert_file, const char *key_file)
{
    WFFacilities::WaitGroup wait_group(1);

    WFWebSocketServer server;
    server.set_ping_interval(20 * 1000);
    server.set_process_text_fn(defTextProcessDealFn);

    int ret;
    if (cert_file && key_file)
    {
        ret = server.start(port, cert_file, key_file);
    }
    else
    {
        ret = server.start(port);
    }

    auto res = m_servers.insert({port, nullptr});
    return ret == 0 && res.second;
}

void WebSocketServer::testChangeMsgFn(WebSocketChannel *ws, protocol::WebSocketFrame *in)
{
    while (ws->open())
    {
        // 默认发三次
        static int dataTimes = 0;
        if (dataTimes > 2)
        {
            // 休眠五毫秒  
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        ws->send_text("change success !", 16);

        dataTimes++;
    }

    // 客户端主动断开连接，进入默认等待状态
    ws->set_process_text_fn(defTextProcessDealFn);
}

}