#include "specialsupply_req.h"

#include <cstring>

#include <HttpMessage.h>
#include <HttpUtil.h>
#include <json_parser.h>
#include <WFTaskFactory.h>
#include <WFFacilities.h>

#include "multipart_parser.h"
#include "Json.h"
#include "logconfig.h"
#include "utils_method.h"
#include "chunk_manager.h"

#define DEFAULT_CHUNK_SIZE    10485760      // 10M
#define CHECK_BYTES_NUMBER   10             // 重叠byte数来校验文件完整
#define REDIRECT_MAX    5
#define RETRY_MAX       2

// const std::string def_resp = "Too many replies, cancel printing";

namespace communicate::http
{

WFHttpTask *SpecialSupReq::getSpecialReqGetTask(const std::string &reqAddr, const std::string &reqInfo, const json_object_t *headerInfo)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr,
												REDIRECT_MAX, RETRY_MAX,
												wget_chunk_callback);
	protocol::HttpRequest *req = http_task->get_req();

	req->set_method(HttpMethodGet);
	req->add_header_pair("Accept", "*/*");
	// req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");

	if (headerInfo != nullptr)
	{
		const char *name;
		const json_value_t *val;

		/* Parse headerInfo to Add header_pair*/
		json_object_for_each(name, val, headerInfo)
		{
			req->add_header_pair((std::string)name, (std::string)json_value_string(val));
		}
	}
	/* Add req body info*/
	req->add_header_pair("Content-Type", "application/json");
	req->append_output_body(reqInfo);

	/* no more than 30 seconds receiving http response. */
	http_task->set_receive_timeout(30 * 1000);

	return http_task;
}

bool SpecialSupReq::reqToGetChunkDataBySeries(const Resume_FileInfo *curInfo, const std::string &reqInfo)
{
    std::ofstream out(curInfo->filePath, std::ios::app | std::ios::binary);
    // std::ofstream out(curInfo->filePath, std::ios::binary);
    // out.seekp(0, std::ios::end);
    if (!out.is_open())
    {
        LOG_ERROR(stderr, "can't open this file to append write, filePath: %s \n", curInfo->filePath);
        return false;
    }
    // 首次调整指针位置
    // if (curInfo->lastEndPos > curInfo->chunkSize && curInfo->lastEndPos < out.tellp())
    // {
    //     out.seekp(curInfo->lastEndPos, std::ios::beg);
    // }

    WFFacilities::WaitGroup wait_group(1);

    auto series_callback = [&wait_group](const SeriesWork *series)
	{
		CommContextAtChunk *context = (CommContextAtChunk *)series->get_context();

		if (context->communicate_status == HTTP_ERROR_CODE::SUCCESS)
		{
			LOG_INFO(stderr, "Series finished. all success!\n");
		}
		else
		{
			LOG_INFO(stderr, "Series finished. failed!\n");
			// failTaskIndex = context->curTaskIndex;
			LOG_WARNING(stderr, "Series failed at taskIndex: %d!\n", context->curTaskIndex);
		}

		{ // 打印访问结束时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			LOG_INFO(stderr, "--- Series end at : %s ---\r\n", timestamp);
		}

		wait_group.done();
	};

    // 设置首个任务的header信息
    uint32_t chunkSize = curInfo->chunkSize == 0 ? DEFAULT_CHUNK_SIZE : curInfo->chunkSize;
    std::string startPos = out.tellp() > CHECK_BYTES_NUMBER ?
        std::to_string(out.tellp() - (std::streampos)CHECK_BYTES_NUMBER) : std::to_string(out.tellp());
    std::string downRange = startPos + "-" + std::to_string((long long)out.tellp() + chunkSize);
    base::Json headerJson;
    headerJson.push_back("Range", "bytes=" + downRange);
    headerJson.push_back("Connection", "keep-alive");
    headerJson.push_back("FuncName", "dealDownloadDataReq");
    json_object_t *headerInfo = json_value_object(json_value_parse(headerJson.dump().c_str()));
    // 设置context信息
    CommContextAtChunk context;
    context.curTaskIndex = 0;
	context.communicate_status = HTTP_ERROR_CODE::SUCCESS;
    std::strncpy(context.resumeInfo.resumeName, curInfo->resumeName, sizeof(curInfo->resumeName));
    context.resumeInfo.lastEndPos = curInfo->lastEndPos == 0 ? (long long)out.tellp() : curInfo->lastEndPos;
    context.resumeInfo.chunkSize = chunkSize;
    std::strcpy(context.resumeInfo.commAddr, curInfo->commAddr);
    std::strcpy(context.resumeInfo.filePath, curInfo->filePath);
    std::strcpy(context.resumeInfo.fileHash, curInfo->fileHash);
    context.outFile = std::move(out);
    // 设置任务series
	SeriesWork *series = Workflow::create_series_work(
        SpecialSupReq::getSpecialReqGetTask(curInfo->commAddr, reqInfo, headerInfo),
        series_callback);
	series->set_context(&context);

	{ // 打印访问启动时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_INFO(stderr, "--- Series start at : %s ---\r\n", timestamp);
	}

	series->start();
	wait_group.wait();

    context.outFile.close();
    std::string mapKey = curInfo->fileHash[0] == '\0' ? curInfo->commAddr : curInfo->fileHash;
    ChunkManager::getInstance()->delDownTaskInfo(mapKey);
	return context.communicate_status == HTTP_ERROR_CODE::SUCCESS;
}

std::vector<MultipartParser> SpecialSupReq::getFileChunkStructs(const std::string &paramName, const std::string &filePath, uint32_t chunkSize)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return {};
    }

    uint32_t fileSize = file.tellg();
    file.close();

    std::vector<MultipartParser> result;
    uint32_t startPos = 0;
    if (chunkSize == 0)
    {
        chunkSize = DEFAULT_CHUNK_SIZE;
    }
    while (startPos < fileSize)
    {
        MultipartParser subParser;
        subParser.addChunkFile(paramName, filePath, startPos, startPos + chunkSize);
        startPos += chunkSize;

        result.emplace_back(std::move(subParser));
    }

    return result;
}

bool SpecialSupReq::reqToSendChunkDataBySeries(uint8_t &failTaskIndex, std::string &finResult,
                                               std::vector<MultipartParser> &chunksInfo, const std::string &reqAddr)
{
    std::vector<WFHttpTask *> vTasks;
    for (auto &mem : chunksInfo)
    {   // todo getReqSendTask此处需要修改
        vTasks.emplace_back(std::move(HttpReqWays::getReqSendTask(mem, reqAddr, true, nullptr)));
    }

    std::vector<std::string> resStrVec;
	auto res = HttpReqWays::reqGetRespBySeries(vTasks, resStrVec, failTaskIndex);
	finResult = resStrVec.back();
	return res == HTTP_ERROR_CODE::SUCCESS;
}

bool SpecialSupReq::reqToSendChunkDataByParallel(std::vector<int> &failTaskIndexs, std::vector<std::string> &allResult,
                                                 std::vector<MultipartParser> &chunksInfo, const std::string &reqAddr)
{
    std::vector<WFHttpTask *> vTasks;
    for (auto &mem : chunksInfo)
    {
        vTasks.emplace_back(std::move(HttpReqWays::getReqSendTask(mem, reqAddr, true, nullptr)));
    }

    auto failTaskMap = HttpReqWays::reqGetRespByParallel(vTasks, allResult);
    for (int i = 0; i < vTasks.size(); ++i)
    {
        if (failTaskMap.find(i) != failTaskMap.end())
        {
            failTaskIndexs.emplace_back(i);
        }
    }

    return failTaskIndexs.empty();
}

void SpecialSupReq::wget_chunk_callback(WFHttpTask *task)
{
	HttpReqWays::base_callback_deal(task);

	/* 数据传出 */
	SeriesWork *series = series_of(task);
	CommContextAtChunk *context;
	if (!series)
	{
		LOG_ERROR(stderr, "This resume task have not bind context.\n");
        return;
	}
	else
	{
        context = (CommContextAtChunk *)series->get_context();
		if (context->communicate_status != HTTP_ERROR_CODE::SUCCESS)
		{
			LOG_DEBUG(stderr, "This Series task running failed at taskIndex: %d .\r\n", context->curTaskIndex);
			return;
		}
		++(context->curTaskIndex);
	}

	if (task->get_state() != WFT_STATE_SUCCESS)
	{
		LOG_ERROR(stderr, "WFT_STATE Error.\n");
		series->cancel();
		return;
	}

    /* 获取指定header的值 */
    std::string contentRange = "";
    protocol::HttpHeaderCursor resp_cursor(task->get_resp());
    resp_cursor.find("Content-Range", contentRange);
    if (contentRange.empty())
    {   // 分段下载场景下，此处一定会有信息
        context->communicate_status = HTTP_ERROR_CODE::UNDEFINED_FAILED;
        LOG_ERROR(stderr, "The resume task has not content-range !\n");
        return;
    }
    long long rangeEnd = 0;
    try {
        rangeEnd = std::stoll(getStrAtSpecifyCharRfind(contentRange, '/'));
    } catch (const std::exception& e) {
        LOG_ERROR(stderr, "Failed to convert rangeEnd: %s\n", e.what());
        return;
    }
	/* Get response body. */
    // 不浪费空间，复用
	context->all_resp_data.back() = protocol::HttpUtil::decode_chunked_body(task->get_resp());
	// context->communicate_status = true;
    if (context->all_resp_data.back().empty())
    {
        LOG_ERROR(stderr, "The response to this request is empty !\n");
        // context->communicate_status = false;
        return;
    }

    // 进行磁盘写入 resp内容和当前内容重叠10 bit 来简单校验文件完整性
    if (context->resumeInfo.lastEndPos > context->outFile.tellp())
    {   // 此情况无法补救，验证
        LOG_WARNING(stderr, "The file for breakpoint continuation was modified during the writing process !\n");
        if (context->outFile.is_open())
        {
            context->outFile.close(); // 关闭文件
        }
        // 重新打开文件并清空内容
        context->outFile.open(context->resumeInfo.filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    }
    else if (context->resumeInfo.lastEndPos < context->outFile.tellp())
    {   // 少数情况存在这种情况（写入部分崩溃）
        if (context->outFile.tellp() > CHECK_BYTES_NUMBER)
        {   // 当前采用改lastEndPos，而非移动写入指针方案(当前方案相反2023.11.17)
            // context->outFile.seekp(context->resumeInfo.lastEndPos, std::ios::beg);
        }
        else
        {   // 重新打开文件并清空内容
            context->outFile.open(context->resumeInfo.filePath, std::ios::out | std::ios::binary | std::ios::trunc);
            LOG_INFO(stderr, "Download again due to insufficient written content !\n");
        } 
    }
    else
    {
        // 校验文件
        std::ifstream temp(context->resumeInfo.filePath, std::ios::binary);
        temp.seekg(context->resumeInfo.lastEndPos, std::ios::beg);
        if (temp.is_open())
        {
            if (context->outFile.tellp() >= CHECK_BYTES_NUMBER && !checkPreviousBytes(temp, CHECK_BYTES_NUMBER, context->all_resp_data.back().substr(0, CHECK_BYTES_NUMBER)))
            {
                temp.close();
                LOG_WARNING(stderr, "The file for breakpoint continuation was modified during the writing process !\n");
                if (context->outFile.is_open())
                {
                    context->outFile.close(); // 关闭文件
                }
                // 重新打开文件并清空内容
                context->outFile.open(context->resumeInfo.filePath, std::ios::out | std::ios::binary | std::ios::trunc);
            }
            else
            {
                temp.close();
                if (context->outFile.tellp() >= CHECK_BYTES_NUMBER)
                {
                    // std::ios::app 模式下，无法支持这样写入数据，依旧会在尾部添加，只能调整字符串
                    // context->outFile.seekp(context->outFile.tellp() - (std::streampos)CHECK_BYTES_NUMBER, std::ios::beg);
                    context->all_resp_data.back().erase(0, 10);
                }
                // 写入文件
                context->outFile.write(context->all_resp_data.back().c_str(), context->all_resp_data.back().size());
                context->outFile.flush();
                
                // todo 自旋锁进行读取生成hash并保存
            }
        }
        else
        {
            LOG_ERROR(stderr, "The file %s to read failed !\n", context->resumeInfo.filePath);
            return;
        }
    }

    // 记录当前状态
    context->resumeInfo.lastEndPos = context->outFile.tellp();
    // 采用阻塞形式存入当前下载状态（当前忽略篡改的影响，不记录hash）
    ChunkManager::getInstance()->saveDownTaskInfo(context->resumeInfo, context->resumeInfo.resumeName);

    LOG_DEBUG(stderr, "Print resp data: %s \n", context->all_resp_data.back().c_str());
	{ // 打印当前task结束时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_DEBUG(stderr, "--- Series task %d end at : %s ---\r\n", context->curTaskIndex, timestamp);
	}

    // 判断文件是否下载完成
    if (context->resumeInfo.lastEndPos == rangeEnd)
    {
        LOG_INFO(stderr, "complete this resume task . \r\n");
        return;
    }
    // 设置分段下载范围
    long long curEndPos = ((long long)context->resumeInfo.lastEndPos + context->resumeInfo.chunkSize) > rangeEnd ?
                          rangeEnd : ((long long)context->resumeInfo.lastEndPos + context->resumeInfo.chunkSize);
    long long curStartPos = context->resumeInfo.lastEndPos > CHECK_BYTES_NUMBER ?
                            (context->resumeInfo.lastEndPos - CHECK_BYTES_NUMBER) : context->resumeInfo.lastEndPos;
    std::string downRange = std::to_string(curStartPos) + "-" + std::to_string(curEndPos);
    base::Json headerJson;
    headerJson.push_back("Range", "bytes=" + downRange);
    headerJson.push_back("FuncName", "dealDownloadDataReq");
    rangeEnd == curEndPos ? headerJson.push_back("Connection", "close") :
                            headerJson.push_back("Connection", "keep-alive");
    json_object_t *headerInfo = json_value_object(json_value_parse(headerJson.dump().c_str()));

    context->curTaskIndex++;
    series->push_back(SpecialSupReq::getSpecialReqGetTask(context->resumeInfo.commAddr, "", headerInfo));
	return;
}

}