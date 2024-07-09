#pragma once

template<class REQ, class RESP>
class WFNetworkTask;

namespace protocol
{
    class HttpRequest;
    class HttpResponse;
} // namespace protocol
using WFHttpTask = WFNetworkTask<protocol::HttpRequest, protocol::HttpResponse>;
typedef struct __json_object json_object_t;

namespace communicate
{

enum HTTP_ERROR_CODE
{
    SUCCESS = 0,
    UNDEFINED_FAILED,
    /* 客户端错误 */
    SSL
    /* 服务端错误 */
    
};

// 存入读取，使用sizeof需要明确结构体大小，std::string 不符合
struct Resume_FileInfo
{
    char resumeName[198];       // 断电续传信息的记录文件名
    char filePath[328];         // 记录信息的目标文件路径
    char fileHash[68];          // 锁定文件的hash值
    long long lastEndPos = 0;   // 最终处理结束的下标
    uint32_t chunkSize = 0;     // 分块尺寸
    char commAddr[648];         // 交互的地址   “作为任务划分的key”
};

}