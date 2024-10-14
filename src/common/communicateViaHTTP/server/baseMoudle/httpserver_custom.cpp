#include "httpserver_custom.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include "logconfig.h"
#include "utils_method.h"
#include "register_funcs_base.h"

namespace communicate::http
{

std::string HttpServerCustom::getApiNameByHeader(WFHttpTask *task)
{
    protocol::HttpHeaderMap headerMap(task->get_req());
    return headerMap.get("FuncName");
}

std::string HttpServerCustom::getApiNameByUrl(WFHttpTask *task)
{
    std::string url = task->get_req()->get_request_uri();
    return getStrAtSpecifyCharRfind(url, '/');
}

std::string getApiNameByCustom(WFHttpTask *task)
{
    LOG_DEBUG(stderr, "call base api name func, to test . \n");
    return "echoTaskInfo";
}

HttpServerCustom::HttpServerCustom(const cfg_data *cfgData)
{
    size_t bindFuncsNum = cfgData->vBindFuncs.size();
    if (bindFuncsNum == 0)
    {
        m_server = new WFHttpServer(default_processFunc);
    }
    else
    {
        auto self = shared_from_this();
        auto processFunc = [cfgData, self](WFHttpTask *task) {
            // 与客户端约定好，按什么条件触发接口
            std::string funcName = self->getApiNameByCustom(task);
            // 查找是否有映射的处理函数
            auto apiRegister = g_funcsMap.find(funcName);
            bool hasBind = (apiRegister != g_funcsMap.end()) &&
                           (std::find(cfgData->vBindFuncs.begin(), cfgData->vBindFuncs.end(), funcName) != cfgData->vBindFuncs.end());
            // 只有接口被绑定和其回调注册，才执行其对应的回调处理函数，否则调用默认的处理函数
            auto process = hasBind ? apiRegister->second : default_processFunc;
            (void)process(task);
        };
        m_server = new WFHttpServer(processFunc);
    }

    if (cfgData->cert_path != "" && cfgData->key_path != "")
    {
        m_server->start(cfgData->port, (cfgData->cert_path).c_str(), (cfgData->key_path).c_str());
    }
    else
    {
        m_server->start(cfgData->port);
    }
}

HttpServerCustom::~HttpServerCustom()
{
    m_server->stop();
}

void HttpServerCustom::default_processFunc(WFHttpTask *server_task)
{
    protocol::HttpRequest *req = server_task->get_req();
	protocol::HttpResponse *resp = server_task->get_resp();
	long long seq = server_task->get_task_seq();
	protocol::HttpHeaderCursor cursor(req);
	std::string name;
	std::string value;
	char buf[8192];
	int len;

	/* Set response message body. */
	resp->append_output_body_nocopy("<html>", 6);
	len = snprintf(buf, 8192, "<p>%s %s %s</p>", req->get_method(),
				   req->get_request_uri(), req->get_http_version());
	resp->append_output_body(buf, len);

	while (cursor.next(name, value))
	{
		len = snprintf(buf, 8192, "<p>%s: %s</p>", name.c_str(), value.c_str());
		resp->append_output_body(buf, len);
	}

	resp->append_output_body_nocopy("</html>", 7);

	/* Set status line if you like. */
	resp->set_http_version("HTTP/1.1");
	resp->set_status_code("200");
	resp->set_reason_phrase("OK");

	resp->add_header_pair("Content-Type", "text/html");
	resp->add_header_pair("Server", "This is a test server");
	if (seq == 9) /* no more than 10 requests on the same connection. */
		resp->add_header_pair("Connection", "close");

	/* print some log */
	char addrstr[128];
	struct sockaddr_storage addr;
	socklen_t l = sizeof addr;
	unsigned short port = 0;

	server_task->get_peer_addr((struct sockaddr *)&addr, &l);
	if (addr.ss_family == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
		inet_ntop(AF_INET, &sin->sin_addr, addrstr, 128);
		port = ntohs(sin->sin_port);
	}
	else if (addr.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr;
		inet_ntop(AF_INET6, &sin6->sin6_addr, addrstr, 128);
		port = ntohs(sin6->sin6_port);
	}
	else
		strcpy(addrstr, "Unknown");

	LOG_INFO(stderr, "default_processFunc, Peer address: %s:%d, seq: %lld.\n", addrstr, port, seq);
}

}   // communicate::http