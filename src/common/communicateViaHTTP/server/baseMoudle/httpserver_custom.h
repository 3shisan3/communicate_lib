/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        httpserver_custom.h
Version:     1.0
Author:      cjx
start date: 2024-10-13
Description: 通用服务器自定义
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2024-10-13      cjx        create

*****************************************************************/

#ifndef _HTTPCOMM_SERVER_CUSTOM_H_
#define _HTTPCOMM_SERVER_CUSTOM_H_

#include <string>
#include <vector>

#include <Workflow.h>
#include <HttpMessage.h>
#include <HttpUtil.h>
#include <WFFacilities.h>
#include <WFHttpServer.h>

namespace communicate::http
{

struct cfg_data
{
    int port;
    std::string cert_path;
    std::string key_path;
    std::vector<std::string> vBindFuncs;
};

class HttpServerCustom : public std::enable_shared_from_this<HttpServerCustom> 
{
public:
    explicit HttpServerCustom(const cfg_data *cfgData);
    ~HttpServerCustom();

private:
    std::string getApiNameByHeader(WFHttpTask *task);
    std::string getApiNameByUrl(WFHttpTask *task);
    virtual std::string getApiNameByCustom(WFHttpTask *task);

    static void default_processFunc(WFHttpTask *server_task);

    WFHttpServer *m_server;
};

}
#endif