#include "register_funcs_base.h"

#include <HttpUtil.h>

#include "logconfig.h"

namespace communicate::http
{

std::unordered_map<std::string, std::function<void(WFHttpTask *)>> g_funcsMap;  // 定义

// 使用示例
static void echoTaskInfo(WFHttpTask *task)
{
    protocol::HttpRequest *req = task->get_req();
    protocol::HttpResponse *resp = task->get_resp();

    // 获取请求的url
    const std::string url = req->get_request_uri();
    LOG_DEBUG(stderr, "task req url: %s", url.c_str());

    // 获取请求的数据内容
    const std::string request_body = protocol::HttpUtil::decode_chunked_body(req);
    LOG_DEBUG(stderr, "task req body: %s", request_body.c_str());

    // 设置回复
    resp->append_output_body("triggered successfully ~");
}

REGISTER_FUNCTION(echoTaskInfo, &echoTaskInfo)

} // communicate::http