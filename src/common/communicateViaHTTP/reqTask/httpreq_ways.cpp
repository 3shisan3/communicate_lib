#include "httpreq_ways.h"

#include <deque>
#include <memory>

#include <HttpMessage.h>
#include <HttpUtil.h>
#include <json_parser.h>
#include <WFTaskFactory.h>
#include <WFFacilities.h>

#include "logconfig.h"
#include "utils_method.h"
#include "multipart_parser.h"
#include "hash_create.h"

#define REDIRECT_MAX    5
#define RETRY_MAX       2
#define FIRST_WAIT_TIME 5	// 秒
#define DELIMITER		","

namespace communicate::http
{

ReconnectCfg g_reconnectCfg = {};
struct ReTaskManager
{
	// uint8_t reCount = 0;
	std::deque<int> taskIndexVec;
	SeriesWork *reQueuePtr = nullptr;
};


int calculateSumWaitTime(int firstTime, int maxSpendTime, int retryNum)
{
	// 计算获取当前任务以耗费时间，目前内部设计等待时长指数增长
	int totalWaitTime = 0;
	int i = 1;
	for (; i <= retryNum; ++i)
	{
		int increment = firstTime * static_cast<int>(pow(2, i - 1)); // 计算本次应该增加的量
		if (increment > maxSpendTime)
		{
			break;
		}
		totalWaitTime += increment; // 累加到总和中
	}
	for (; i <= retryNum; ++i)
	{
		totalWaitTime += maxSpendTime; // 时间超过设置的最长间隔，按最长间隔算
	}

	return totalWaitTime;
}

void HttpReqWays::initHttpReqParams(const ReconnectCfg &cfg)
{
	g_reconnectCfg = cfg;
}

void HttpReqWays::initHttpReqParams(const GlobalSetting &cfg)
{
	WFGlobalSettings settings = GLOBAL_SETTINGS_DEFAULT;
	settings.handler_threads = cfg.handler_threads;
	settings.poller_threads = cfg.poller_threads;
	settings.hosts_path = cfg.hosts_path;
	settings.resolv_conf_path = cfg.resolv_conf_path;
	settings.dns_server_params.connect_timeout = cfg.dns_outTime.connect_timeout;
	settings.dns_server_params.response_timeout = cfg.dns_outTime.response_timeout;
	settings.dns_server_params.ssl_connect_timeout = cfg.dns_outTime.ssl_connect_timeout;
	settings.endpoint_params.connect_timeout = cfg.global_outTime.connect_timeout;
	settings.endpoint_params.response_timeout = cfg.global_outTime.response_timeout;
	settings.endpoint_params.ssl_connect_timeout = cfg.global_outTime.ssl_connect_timeout;

	WORKFLOW_library_init(&settings);
}

void HttpReqWays::initHttpReqParams(const std::string &cfgPath)
{
	// todo 判断输入配置文件为yaml还是json
}

void HttpReqWays::startTask(WFHttpTask *task)
{
	task->start();
}

/* WFHttpTask 本身构造时不涉及SeriesWork，但在start()时会进行构造 */
WFHttpTask *HttpReqWays::getCommonReqTask(const std::string &reqAddr, const std::string &reqInfo,
										  const ReconnectCfg &promiseReqSuc, const char *methodType, const std::string &headerInfoStr)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr, REDIRECT_MAX, RETRY_MAX, [&promiseReqSuc](WFHttpTask *inTask) {
		if (promiseReqSuc.enable_features)
		{
			wget_promise_callback(inTask, promiseReqSuc.max_spaceTime, promiseReqSuc.max_waitTime, promiseReqSuc.max_nums);
		}
		else
		{
			wget_info_callback(inTask);
		}
	});
	protocol::HttpRequest *req = http_task->get_req();

	req->set_method(methodType);
	req->add_header_pair("Accept", "*/*");
	// req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "close");

	if (!headerInfoStr.empty())
	{	
		auto headerValue = json_value_parse(headerInfoStr.c_str());
		auto headerInfo = headerValue == nullptr ? nullptr : json_value_object(headerValue);
		if (headerInfo != nullptr)
		{
			const char *name;
			const json_value_t *val;

			/* Parse headerInfo to Add header_pair*/
			json_object_for_each(name, val, headerInfo)
			{
				std::string val_str = "";
				switch (json_value_type(val))
				{
				case JSON_VALUE_STRING:
					val_str = json_value_string(val);
					break;
				case JSON_VALUE_NUMBER:
				{
					double tempNum = json_value_number(val);
					val_str = hasDecimal(tempNum) ? std::to_string(tempNum) :	
													std::to_string(static_cast<int>(tempNum));
				}
				break;
				case JSON_VALUE_TRUE:
					val_str = "true";
					break;
				case JSON_VALUE_FALSE:
					val_str = "false";
					break;
				default:
					LOG_WARNING(stderr, "this value handles an exception, "
										"the type cannot be written to the request content, "
										"type value %d", json_value_type(val));
					break;
				}
				req->add_header_pair((std::string)name, val_str);
			}
		}
	}

	/* Add req body info*/
	if (json_value_parse(reqInfo.c_str()) != nullptr)
	{
		req->add_header_pair("Content-Type", "application/json");
	}
	else
	{	// 默认作为二进制流
		req->add_header_pair("Content-Type", "application/octet-stream");
	}
	
	req->append_output_body(reqInfo);

	/* Limit the http response size to 20M. */
	// http_task->get_resp()->set_size_limit(20 * 1024 * 1024);

	/* no more than 30 seconds receiving http response. */
	http_task->set_receive_timeout(30 * 1000);

	return http_task;
}

WFHttpTask *HttpReqWays::getReqSendTask(MultipartParser &parser, const std::string &reqAddr, const ReconnectCfg &promiseReqSuc, const std::string &headerInfoStr)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr, REDIRECT_MAX, RETRY_MAX, [&promiseReqSuc](WFHttpTask *inTask) {
		if (promiseReqSuc.enable_features)
		{
			wget_promise_callback(inTask, promiseReqSuc.max_spaceTime, promiseReqSuc.max_waitTime, promiseReqSuc.max_nums);
		}
		else
		{
			wget_info_callback(inTask);
		}
	});
	protocol::HttpRequest *req = http_task->get_req();

	req->set_method(HttpMethodPost);
	req->add_header_pair("Accept", "*/*");
	// req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "keep-alive");
	if (!headerInfoStr.empty())
	{	
		auto headerValue = json_value_parse(headerInfoStr.c_str());
		auto headerInfo = headerValue == nullptr ? nullptr : json_value_object(headerValue);
		if (headerInfo != nullptr)
		{
			const char *name;
			const json_value_t *val;

			/* Parse headerInfo to Add header_pair*/
			json_object_for_each(name, val, headerInfo)
			{
				std::string val_str = "";
				switch (json_value_type(val))
				{
				case JSON_VALUE_STRING:
					val_str = json_value_string(val);
					break;
				case JSON_VALUE_NUMBER:
				{
					double tempNum = json_value_number(val);
					val_str = hasDecimal(tempNum) ? std::to_string(tempNum) : std::to_string(static_cast<int>(tempNum));
				}
				break;
				case JSON_VALUE_TRUE:
					val_str = "true";
					break;
				case JSON_VALUE_FALSE:
					val_str = "false";
					break;
				default:
					LOG_WARNING(stderr, "this value handles an exception, "
										"the type cannot be written to the request content, "
										"type value %d", json_value_type(val));
					break;
				}
				req->add_header_pair((std::string)name, val_str);
			}
		}
	}

	// std::string boundary_res = parser.boundary();	// 有RVO/NRVO/拷贝省略，但
	// std::string body_res = parser.GenBodyContent();	// 可能存在不确定的拷贝情况，忽略
	/* Add req body info*/
	req->add_header_pair("Content-Type", "multipart/form-data; boundary=" + parser.boundary());
	req->append_output_body_nocopy(parser.GenBodyContent());

	return http_task;
}

WFHttpTask *HttpReqWays::getCommonReqSendTask(const std::string &filePaths, const std::string &reqAddr, const std::string &infoStr,
											  const ReconnectCfg &promiseReqSuc, const std::string &headerInfoStr)
{
	const char *name;
	const json_value_t *val;

	/* Parse file to Create body*/
	MultipartParser parser;

	if (json_value_parse(infoStr.c_str()) != nullptr)
	{
		auto info = json_value_object(json_value_parse(infoStr.c_str()));

		json_object_for_each(name, val, info)
		{
			std::string val_str = "";
			switch (json_value_type(val))
			{
			case JSON_VALUE_STRING:
				val_str = json_value_string(val);
				break;
			case JSON_VALUE_NUMBER:
				{
					double tempNum = json_value_number(val);
					val_str = hasDecimal(tempNum) ? std::to_string(tempNum) :
													std::to_string(static_cast<int>(tempNum));
				}
				break;
			case JSON_VALUE_TRUE:
				val_str = "true";
				break;
			case JSON_VALUE_FALSE:
				val_str = "false";
				break;
			default:
				LOG_WARNING(stderr, "this value handles an exception, "
					"the type cannot be written to the request content, "
					"type value %d", json_value_type(val));
				break;
			}
			parser.addParameter((std::string)name, val_str);
		}
	}

	if (json_value_parse(filePaths.c_str()) != nullptr)
	{
		auto filePathsObj = json_value_object(json_value_parse(filePaths.c_str()));
		json_object_for_each(name, val, filePathsObj)
		{
			parser.addFile((std::string)name, (std::string)json_value_string(val));
		}
	}

	WFHttpTask *task = getReqSendTask(parser, reqAddr, promiseReqSuc, headerInfoStr);
	std::pair<std::string, std::string> noteInfo = {filePaths, infoStr};
	task->user_data = &noteInfo;
	return task;
}

HTTP_ERROR_CODE HttpReqWays::reqToGetResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo, const std::string &headerInfoStr)
{
	WFHttpTask *http_task;
	if (headerInfoStr.empty())
	{
		http_task = getCommonReqTask(reqAddr, reqInfo, g_reconnectCfg);
	}
	else
	{
		http_task = getCommonReqTask(reqAddr, reqInfo, g_reconnectCfg, HttpMethodGet, headerInfoStr);
	}

	uint8_t temp = 0;
	std::vector<std::string> resStrVec;
	auto res = reqGetRespBySeries({http_task}, resStrVec, temp);
	result = resStrVec.back();
	return res;
}

HTTP_ERROR_CODE HttpReqWays::reqToPostResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo, const std::string &headerInfoStr)
{
	WFHttpTask *http_task;
	if (headerInfoStr.empty())
	{
		http_task = getCommonReqTask(reqAddr, reqInfo, g_reconnectCfg, HttpMethodPost);
	}
	else
	{
		json_object_t * headerInfo = json_value_object(json_value_parse(headerInfoStr.c_str()));
		http_task = getCommonReqTask(reqAddr, reqInfo, g_reconnectCfg, HttpMethodPost, headerInfoStr);
	}

	uint8_t temp = 0;
	std::vector<std::string> resStrVec;
	auto res = reqGetRespBySeries({http_task}, resStrVec, temp);
	result = resStrVec.back();
	return res;
}

HTTP_ERROR_CODE HttpReqWays::reqToSendData(std::string &result, const std::string &filePathsStr, const std::string &reqAddr,
										   const std::string &infoStr, const std::string &headerInfoStr)
{
	WFHttpTask *http_task;
	if (headerInfoStr.empty())
	{
		http_task = getCommonReqSendTask(filePathsStr, reqAddr, infoStr, g_reconnectCfg);
	}
	else
	{
		http_task = getCommonReqSendTask(filePathsStr, reqAddr, infoStr, g_reconnectCfg, headerInfoStr);
	}

	uint8_t temp = 0;
	std::vector<std::string> resStrVec;
	auto res = reqGetRespBySeries({http_task}, resStrVec, temp);
	result = resStrVec.back();
	return res;
}

struct HttpReqWays::MultitaskCommContext : public HttpReqWays::CommContext
{
	// bool taskHasDependency;		// 使用此结构，默认任务之间不存在依赖

	std::map<int, HTTP_ERROR_CODE> failTaskStatus;
	std::map<std::string, std::shared_ptr<ReTaskManager>> reQueueMap;	// key为url
};

HttpReqWays::noteTasksStatusCallback HttpReqWays::statusCallbackFunc = nullptr;

void HttpReqWays::noteTaskStatus(WFHttpTask *task, HTTP_ERROR_CODE status)
{
	if (statusCallbackFunc != nullptr)
	{
		std::string key_first = task->get_req()->get_request_uri();
		std::string key_second = obtain_task_id(task);
		HTTP_ERROR_CODE value_first = status;
		std::string value_second = protocol::HttpUtil::decode_chunked_body(task->get_resp());

		statusCallbackFunc({key_first, key_second, value_first, value_second});
	}
}

HTTP_ERROR_CODE HttpReqWays::reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::vector<std::string> &allResult, uint8_t &failTaskIndex)
{
	WFFacilities::WaitGroup wait_group(1);
	CommContext context;

	auto series_callback = [&wait_group, &allResult, &failTaskIndex](const SeriesWork *series) {
		CommContext *context = (CommContext *)series->get_context();

		if (context->communicate_status == HTTP_ERROR_CODE::SUCCESS)
		{
			LOG_INFO(stderr, "Series finished. all success!\n");
		}
		else
		{
			LOG_INFO(stderr, "Series finished. failed!\n");
			failTaskIndex = context->curTaskIndex;
			LOG_WARNING(stderr, "Series failed at taskIndex: %d!\n", context->curTaskIndex);
		}

		{ // 打印访问结束时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			LOG_INFO(stderr, "--- Series end at : %s ---\r\n", timestamp);
		}

		allResult = std::move(context->all_resp_data);
		wait_group.done();
	};

	// 设置任务series
	SeriesWork *series = Workflow::create_series_work(vTasks.front(), series_callback);
	for (auto it = vTasks.begin() + 1; it != vTasks.end(); ++it)
	{
		series->push_back(*it);
		noteTaskStatus(*it, HTTP_ERROR_CODE::UNDEFINED);
	}
	series->set_context(&context);

	{ // 打印访问启动时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_INFO(stderr, "--- Series start at : %s ---\r\n", timestamp);
	}
	series->start();

	wait_group.wait();

	return context.communicate_status;
}

std::map<int, HTTP_ERROR_CODE> HttpReqWays::reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::vector<std::string> &allResult)
{
	WFFacilities::WaitGroup wait_group(1);
	MultitaskCommContext context;
	
	auto series_callback = [&](const SeriesWork *series) {	
		// CommContext *context = (CommContext *)series->get_context();
		// allResult = context->all_resp_data;		// 如果拓展promise，异步等，需要提前得到结果可能需要

		{ // 打印访问结束时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			LOG_INFO(stderr, "--- Main series end at : %s ---\r\n", timestamp);
		}
	};

	// 设置任务series
	SeriesWork *series = Workflow::create_series_work(vTasks.front(), series_callback);
	for (auto it = vTasks.begin() + 1; it != vTasks.end(); ++it)
	{
		series->push_back(*it);
		noteTaskStatus(*it, HTTP_ERROR_CODE::UNDEFINED);
	}
	series->set_context(&context);

	{ // 打印访问启动时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_INFO(stderr, "--- Series start at : %s ---\r\n", timestamp);
	}
	// 使用ParallelWork, 便于存在重传等任务时，等待这些任务的结果返回, 存在依赖关系的任务，放在一个SeriesWork中
	ParallelWork *parallelWork = Workflow::create_parallel_work([&wait_group](const ParallelWork *pwork) {
		wait_group.done();
	});
	parallelWork->add_series(series);
	parallelWork->start();

	wait_group.wait();

	allResult = std::move(context.all_resp_data);
	return context.failTaskStatus;
}

std::map<int, HTTP_ERROR_CODE> HttpReqWays::reqGetRespByParallel(const std::vector<WFHttpTask *> &vTasks, std::vector<std::string> &allResult)
{
	std::map<int, HTTP_ERROR_CODE> failTasks;

	ParallelWork *parallelWork = Workflow::create_parallel_work([&](const ParallelWork *pwork) {
		CommContext *context;

		for (size_t i = 0; i < pwork->size(); i++)
		{
			context = (CommContext *)pwork->series_at(i)->get_context();
			LOG_DEBUG(stderr, "ParallelWork task index is %d .\n", context->curTaskIndex);
			if (context->communicate_status == HTTP_ERROR_CODE::SUCCESS)
			{
				LOG_DEBUG(stderr, "CurTask resp content is %s .\n", context->all_resp_data.back().c_str());
				allResult.emplace_back(context->all_resp_data.back());
			}
			else
			{
				failTasks.insert({context->curTaskIndex, context->communicate_status});
				allResult.emplace_back("");
			}

			delete context;
		}

		{	// 打印所有并行任务结束时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			LOG_DEBUG(stderr, "--- ParallelWork end at : %s ---\r\n", timestamp);
		}
	});

	SeriesWork *series;
	CommContext *ctx;
	for (size_t i = 0; i < vTasks.size(); ++i)
	{
		series = Workflow::create_series_work(vTasks[i], [](const SeriesWork *subSeries) {
			// 打印并行任务启动时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			CommContext *subctx = (CommContext *)subSeries->get_context();
			LOG_DEBUG(stderr, "--- ParallelWork subSeries %d end at : %s ---\r\n", subctx->curTaskIndex, timestamp);
		});

		ctx = new CommContext;
		ctx->curTaskIndex = i;
		series->set_context(ctx);
		parallelWork->add_series(series);

		noteTaskStatus(vTasks[i], HTTP_ERROR_CODE::UNDEFINED);
	}

	WFFacilities::WaitGroup wait_group(1);

	Workflow::start_series_work(parallelWork, [&wait_group](const SeriesWork *)
								{ wait_group.done(); });

	{ // 打印并行任务启动时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_DEBUG(stderr, "--- ParallelWork start at : %s ---\r\n", timestamp);
	}
	wait_group.wait();

	/*	// 另一种阻塞方案
	std::mutex mutex;
	std::condition_variable cond;
	bool finished = false;

	Workflow::start_series_work(parallelWork,
		[&mutex, &cond, &finished](const SeriesWork *)
	{
		mutex.lock();
		finished = true;
		cond.notify_one();
		mutex.unlock();
	});

	std::unique_lock<std::mutex> lock(mutex);
	while (!finished)
		cond.wait(lock);
	lock.unlock(); */

	return failTasks;
}

void HttpReqWays::setTaskStatusFunc(const noteTasksStatusCallback &func)
{
	statusCallbackFunc = func;
}

void HttpReqWays::base_callback_deal(WFHttpTask *task)
{
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	// 自己的调用流程，此处触发，必定task已经start，series_of默认不为空
	auto context = (CommContext *)series_of(task)->get_context();
	switch (state)
	{
	case WFT_STATE_SYS_ERROR:
		LOG_ERROR(stderr, "system error: %s\n", strerror(error));
		context->communicate_status = HTTP_ERROR_CODE::SYSTEM_ERROR;
		break;
	case WFT_STATE_DNS_ERROR:
		LOG_ERROR(stderr, "DNS error: %s\n", gai_strerror(error));
		context->communicate_status = HTTP_ERROR_CODE::DNS_ERROR;
		break;
	case WFT_STATE_SSL_ERROR:
		LOG_ERROR(stderr, "SSL error: %d\n", error);
		context->communicate_status = HTTP_ERROR_CODE::SSL_ERROR;
		break;
	case WFT_STATE_TASK_ERROR:
		LOG_ERROR(stderr, "Task error: %d\n", error);
		context->communicate_status = HTTP_ERROR_CODE::TASK_ERROR;
		break;
	case WFT_STATE_SUCCESS:
		context->communicate_status = HTTP_ERROR_CODE::SUCCESS;
		break;
	default:
		LOG_ERROR(stderr, "Req error, unmanaged error types: %d\n", error);
		context->communicate_status = HTTP_ERROR_CODE::UNDEFINED;
	}

	noteTaskStatus(task, context->communicate_status);

	/* Print req and resp info*/
	if (LOG_LEVEL_DEBUG >= GLOBAL_LOG_LEVEL)
	{
		debugPrint(task->get_req(), resp);
	}
}

std::string HttpReqWays::obtain_task_id(WFHttpTask *task)
{
	std::string taskKey;
	if (task->user_data != nullptr)
	{
		std::string reqInfo = protocol::HttpUtil::decode_chunked_body(task->get_req());
		taskKey = get_sha256Hex(reqInfo);
	}
	else
	{
		std::pair<std::string, std::string> *pair = static_cast<std::pair<std::string, std::string>*>(task->user_data);
		taskKey = pair->first + DELIMITER + pair->second;
	}
	
	LOG_DEBUG(stderr, "Print task key: %s \n", taskKey.c_str());
	return taskKey;
}

void HttpReqWays::debugPrint(protocol::HttpRequest *req, protocol::HttpResponse *resp)
{
	// 打印相关信息(test)
	std::string name;
	std::string value;
	/* Print request. */
	LOG_DEBUG(stderr, "Print request info: %s %s %s\r\n", req->get_method(),
			 req->get_http_version(),
			 req->get_request_uri());

	protocol::HttpHeaderCursor req_cursor(req);

	while (req_cursor.next(name, value))
		LOG_DEBUG(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	LOG_DEBUG(stderr, "\r\n");

	/* Print response header. */
	LOG_DEBUG(stderr, "Print response info: %s %s %s\r\n", resp->get_http_version(),
			 resp->get_status_code(),
			 resp->get_reason_phrase());

	protocol::HttpHeaderCursor resp_cursor(resp);

	while (resp_cursor.next(name, value))
		LOG_DEBUG(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	LOG_DEBUG(stderr, "\r\n");
}

void HttpReqWays::wget_info_callback(WFHttpTask *task)
{
	base_callback_deal(task);	

	/* 数据传出*/
	SeriesWork *series = series_of(task);
	CommContext *context;
	/* Get response body. */
	context->all_resp_data.emplace_back(protocol::HttpUtil::decode_chunked_body(task->get_resp()));
	LOG_DEBUG(stderr, "Print resp data: %s \n", context->all_resp_data.back().c_str());

	if (!series)	// 基本不存在此情况
	{
		LOG_ERROR(stderr, "BUG !!! This task have not bind context.\n");
	}
	else
	{
		context = (CommContext *)series->get_context();
		if (context->communicate_status != HTTP_ERROR_CODE::SUCCESS)
		{
			LOG_ERROR(stderr, "Http Req Error, Error Code: %d.\n", task->get_state());
			LOG_DEBUG(stderr, "This Series task running failed at taskIndex: %d .\r\n", context->curTaskIndex);

			MultitaskCommContext *ctx = dynamic_cast<MultitaskCommContext *>(context);
			if (ctx == nullptr)
			{ // 区分场景，SeriesWork中task访问服务器不一定一致，状态并非本地错误，有时不应直接取消后续任务
				series->cancel();
				return;
			}
			else
			{
				ctx->failTaskStatus.insert({context->curTaskIndex, context->communicate_status});
			}
		}
		++(context->curTaskIndex);
	}

	{ // 打印当前task结束时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_DEBUG(stderr, "--- Series task %d end at : %s ---\r\n", context->curTaskIndex, timestamp);
	}
	return;
}

void HttpReqWays::wget_promise_callback(WFHttpTask *task, int maxSpaceTime, int maxWaitTime, int maxRetryNum)
{
	base_callback_deal(task);	

	/* 数据传出*/
	SeriesWork *series = series_of(task);
	CommContext *context;
	context = (CommContext *)series->get_context();
	/* Get response body. */
	context->all_resp_data.emplace_back(protocol::HttpUtil::decode_chunked_body(task->get_resp()));
	LOG_DEBUG(stderr, "Print resp data: %s \n", context->all_resp_data.back().c_str());

	WFCounterTask *noteTask = dynamic_cast<WFCounterTask *>(series->get_last_task());
	std::string curTaskUrl = task->get_req()->get_request_uri();

	if (context->communicate_status != HTTP_ERROR_CODE::SUCCESS)
	{	// 重传任务，启动
		LOG_ERROR(stderr, "Http Req Error, Error Code: %d.\n", task->get_state());
		LOG_DEBUG(stderr, "This Series task running failed at taskIndex: %d, start req again.\r\n", context->curTaskIndex);
		
		MultitaskCommContext *ctx = dynamic_cast<MultitaskCommContext *>(context);
		if (ctx == nullptr)
		{	// 任务存在依赖, 直接在本身Series中添加再次请求，与等待任务
			if (noteTask == nullptr)
			{	// 当前任务还未进行过重传, 增加如果每个任务有单独设置重传次数，则入参随之更改
				int *intPtr = new int(1);
				series->set_last_task(WFTaskFactory::create_counter_task(maxRetryNum, [&series, &intPtr, maxRetryNum](WFCounterTask *countTask){
					LOG_WARNING(stderr, "This task has reached the maximum number: %d of reconnections\n", maxRetryNum);
					series->cancel();
					
					countTask->user_data = nullptr;
					delete intPtr;
				}));

				noteTask = (WFCounterTask *)series->get_last_task();
				noteTask->user_data = static_cast<void *>(intPtr);
			}
			else
			{
				noteTask->count();
				++(*(static_cast<int *>(noteTask->user_data)));
			}

			if (series->is_canceled())
			{ 
				return;
			}
			int retryNum = *(static_cast<int *>(noteTask->user_data));
			int waitTime = calculateSumWaitTime(FIRST_WAIT_TIME, maxSpaceTime, retryNum);
			if (maxWaitTime != 0 && waitTime > maxWaitTime)
			{
				LOG_WARNING(stderr, "This task has taken too long and exceeded the limit %d s\n", maxWaitTime);
				series->cancel();
			}
			series->push_front(task);	// 极为罕见会出现，排除服务端问题，在同一环境同一设备上访问相同网址会出现不同状态，故使用push_front尝试一个任务即可
			series->push_front(WFTaskFactory::create_timer_task(waitTime, 0, [waitTime, retryNum](WFTimerTask *timeTask) {
				LOG_INFO(stderr, "Complete the %d second wait and start the %d retry of the task\n", waitTime, retryNum);
			}));
		}
		else
		{	// 任务不存在依赖，新创建队列，并行进行再次请求
			int curDealTaskIndex = context->curTaskIndex;
			ctx->failTaskStatus.insert({curDealTaskIndex, HTTP_ERROR_CODE::WAIT_LOCAL_RE});	// 先改状态
			// 列出新建任务队列情况, 此时任务来源必为main series, curDealTaskIndex值有效
			if (ctx->reQueueMap.find(curTaskUrl) == ctx->reQueueMap.end())
			{
				// 新的任务队列回调
				auto newSeries = Workflow::create_series_work(task, [&ctx, &curTaskUrl](const SeriesWork *series){
					CommContext *subContext = (CommContext *)series->get_context();
					if (subContext->communicate_status != HTTP_ERROR_CODE::SUCCESS)
					{
						for (const auto &index : ctx->reQueueMap.at(curTaskUrl)->taskIndexVec)
						{
							ctx->failTaskStatus[index] = subContext->communicate_status;
						}
						ctx->reQueueMap[curTaskUrl]->reQueuePtr = nullptr;	// 便于主series中又有同样的任务在此周期内仍去尝试reconnect
					}
					else
					{
						for (const auto &index : ctx->reQueueMap.at(curTaskUrl)->taskIndexVec)
						{
							ctx->failTaskStatus.erase(index);
						}
						ctx->reQueueMap.erase(curTaskUrl);
					}
				});
				// 创建新的任务队列
				CommContext subContext;
				newSeries->set_context(&subContext);	// task再次触发回调就会走上面的if
				int *intPtr = new int(1);
				newSeries->set_last_task(WFTaskFactory::create_counter_task(maxRetryNum, [&newSeries, &intPtr, maxRetryNum](WFCounterTask *countTask){
					LOG_WARNING(stderr, "This sub task has reached the maximum number: %d of reconnections\n", maxRetryNum);
					newSeries->cancel();
					
					countTask->user_data = nullptr;
					delete intPtr;
				}));
				noteTask = (WFCounterTask *)series->get_last_task();
				noteTask->user_data = static_cast<void *>(intPtr);
				// 将任务注册到父的任务队列中，方便阻塞返回最终的回调
				const ParallelTask *parallel = series->get_in_parallel();	// 获取当前task所执行的parallelwork
				auto parallelPtr = dynamic_cast<ParallelWork *>(const_cast<ParallelTask *>(parallel));
				if (parallelPtr != nullptr)
				{
					parallelPtr->add_series(newSeries);
				}

				ctx->reQueueMap[curTaskUrl] = std::make_shared<ReTaskManager>();
				ctx->reQueueMap[curTaskUrl]->reQueuePtr = newSeries;
				ctx->reQueueMap[curTaskUrl]->taskIndexVec.emplace_back(curDealTaskIndex);
			}
			else if (ctx->reQueueMap[curTaskUrl]->reQueuePtr == nullptr /* && !ctx->reQueueMap[curTaskUrl]->taskIndexVec.empty() */)
			{
				ctx->failTaskStatus.insert({curDealTaskIndex, ctx->failTaskStatus[ctx->reQueueMap[curTaskUrl]->taskIndexVec.front()]});
			}
			else
			{
				ctx->reQueueMap[curTaskUrl]->reQueuePtr->push_back(task);
				ctx->reQueueMap[curTaskUrl]->taskIndexVec.emplace_back(curDealTaskIndex);
			}
			
			++(context->curTaskIndex);	// 任务序号加一，主series继续下一个任务
		}

		return;
	}
	
	if (noteTask != nullptr)
	{	// 重新计数, 且避免自己加入的count任务阻塞series
		series->unset_last_task();
	}

	++(context->curTaskIndex);

	{ // 打印当前task结束时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_DEBUG(stderr, "--- Series task %d end at : %s ---\r\n", context->curTaskIndex, timestamp);
	}
	return;
}

}