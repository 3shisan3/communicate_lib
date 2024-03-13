/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        websocket_server.h
Version:     1.0
Author:      cjx
start date: 2024-01-28
Description: 通用的服务器构建方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef _WEBSOCKET_SERVER_H_
#define _WEBSOCKET_SERVER_H_

#include <memory>
#include <WFWebSocketServer.h>

#include "logconfig.h"
#include "singleton.h"

namespace communicate
{

class WebSocketServer
{
public:
    friend class SingletonTemplate<WebSocketServer>;
    ~WebSocketServer() = default;

    /**
     * @brief 指定端口注册一个web服务器
     *
     * @param[in] port              服务器监听的端口
     * @param[in] cert_file         ssl证书文件地址
     * @param[in] key_file          ssl私钥文件地址
     *
     * @return 服务器注册是否成功
     */
    bool registerServer(unsigned short port, const char *cert_file = nullptr, const char *key_file = nullptr);

protected:
    /**
     * @brief 测试篡改收到信号行为场景的处理函数
     *
     * @param[in] ws                服务端的输出socket
     * @param[in] in                客户端的传入信息帧
     *
     * @return 服务器注册是否成功
     */
    static void testChangeMsgFn(WebSocketChannel *ws, protocol::WebSocketFrame *in);

private:
    WebSocketServer() = default;

    static void defTextProcessDealFn(WebSocketChannel *ws, protocol::WebSocketFrame *in);
    // todo WFWebSocketServer 替换为继承后自定义类型，用智能指针来管理
    std::map<unsigned short, WFWebSocketServer *> m_servers;  // key-port, value-WFWebSocketServer
};

}



#endif