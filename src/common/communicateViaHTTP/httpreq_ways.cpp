#include "httpreq_ways.h"

#include "logconfig.h"
#include "utils_method.h"

#define REDIRECT_MAX    5
#define RETRY_MAX       2

namespace communicate
{

WFHttpTask *HttpReqWays::getCommonReqTask(const std::string &reqAddr, const std::string &reqInfo, const char *methodType)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr,
												REDIRECT_MAX, RETRY_MAX,
												wget_info_callback);
	protocol::HttpRequest *req = http_task->get_req();

	req->set_method(methodType);
	req->add_header_pair("Accept", "*/*");
	// req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "close");

	/* Add req body info*/
	req->add_header_pair("Content-Type", "application/json");
	req->append_output_body(reqInfo);

	/* Limit the http response size to 20M. */
	// http_task->get_resp()->set_size_limit(20 * 1024 * 1024);

	/* no more than 30 seconds receiving http response. */
	http_task->set_receive_timeout(30 * 1000);

	return http_task;
}

WFHttpTask *HttpReqWays::getSpecialReqGetTask(const std::string &reqAddr, const std::string &reqInfo, const json_object_t *headerInfo)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr,
												REDIRECT_MAX, RETRY_MAX,
												wget_info_callback);
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
	/* Add req body info*/
	req->add_header_pair("Content-Type", "application/json");
	req->append_output_body(reqInfo);

	/* no more than 30 seconds receiving http response. */
	http_task->set_receive_timeout(30 * 1000);

	return http_task;
}

WFHttpTask *HttpReqWays::getReqSendTask(MultipartParser &parser, const std::string &reqAddr, const json_object_t *headerInfo)
{
	WFHttpTask *http_task;
	http_task = WFTaskFactory::create_http_task(reqAddr,
												REDIRECT_MAX, RETRY_MAX,
												wget_info_callback);
	protocol::HttpRequest *req = http_task->get_req();

	req->set_method(HttpMethodPost);
	req->add_header_pair("Accept", "*/*");
	// req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	// req->add_header_pair("Connection", "keep-alive");
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

	std::string boundary_res = parser.boundary();
	std::string body_res = parser.GenBodyContent();
	/* Add req body info*/
	req->add_header_pair("Content-Type", "multipart/form-data; boundary=" + boundary_res);
	req->append_output_body(body_res);

	return http_task;
}

WFHttpTask *HttpReqWays::getCommonReqSendTask(const json_object_t *filePaths, const std::string &reqAddr, const json_object_t *info)
{
	/* Parse file to Create body*/
	MultipartParser parser;
	const char *name;
	const json_value_t *val;

	if (info != nullptr)
	{
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

	json_object_for_each(name, val, filePaths)
	{
		parser.addFile((std::string)name, (std::string)json_value_string(val));
	}

	return getReqSendTask(parser, reqAddr, nullptr);
}

WFHttpTask *HttpReqWays::getSpecialReqSendTask(const json_object_t *filePaths, const std::string &reqAddr,
											   const json_object_t *info, const json_object_t *headerInfo)
{
	const char *name;
	const json_value_t *val;

	/* Parse file to Create body*/
	MultipartParser parser;
	if (info != nullptr)
	{
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
	json_object_for_each(name, val, filePaths)
	{
		parser.addFile((std::string)name, (std::string)json_value_string(val));
	}

	return getReqSendTask(parser, reqAddr, headerInfo);
}

bool HttpReqWays::reqToGetResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo)
{
	WFHttpTask *http_task = getCommonReqTask(reqAddr, reqInfo);

	uint8_t temp = 0;
	return reqGetRespBySeries({http_task}, result, temp);
}

bool HttpReqWays::reqToPostResp(std::string &result, const std::string &reqAddr, const std::string &reqInfo)
{
	WFHttpTask *http_task = getCommonReqTask(reqAddr, reqInfo, HttpMethodPost);

	uint8_t temp = 0;
	return reqGetRespBySeries({http_task}, result, temp);
}

bool HttpReqWays::reqToSendData(std::string &result, const json_object_t *filePaths, const std::string &reqAddr,
								const json_object_t *info, const json_object_t *headerInfo)
{
	WFHttpTask *http_task;
	if (headerInfo == nullptr)
	{
		http_task = getCommonReqSendTask(filePaths, reqAddr, info);
	}
	else
	{
		http_task = getSpecialReqSendTask(filePaths, reqAddr, info, headerInfo);
	}

	uint8_t temp = 0;
	return reqGetRespBySeries({http_task}, result, temp);
}

bool HttpReqWays::reqGetRespBySeries(const std::vector<WFHttpTask *> vTasks, std::string &finResult, uint8_t &failTaskIndex)
{
	WFFacilities::WaitGroup wait_group(1);
	CommContext context;
	context.curTaskIndex = 0;
	context.communicate_status = true;

	auto series_callback = [&wait_group, &finResult, &failTaskIndex](const SeriesWork *series)
	{
		CommContext *context = (CommContext *)series->get_context();

		if (context->communicate_status)
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

		finResult = std::move(context->resp_data);
		wait_group.done();
	};

	// 设置任务series
	SeriesWork *series = Workflow::create_series_work(vTasks.front(), series_callback);
	for (auto it = vTasks.begin() + 1; it != vTasks.end(); ++it)
	{
		series->push_back(*it);
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

std::vector<int> HttpReqWays::reqGetRespByParallel(const std::vector<WFHttpTask *> &vTasks, std::vector<std::string> &allResult)
{
	std::vector<int> sucTasks;

	ParallelWork *parallelWork = Workflow::create_parallel_work([&](const ParallelWork *pwork)
																{
		CommContext *context;

		for (size_t i = 0; i < pwork->size(); i++)
		{
			context = (CommContext *)pwork->series_at(i)->get_context();
			LOG_DEBUG(stderr, "ParallelWork task index is %d .\n", context->curTaskIndex);
			if (context->communicate_status)
			{
				LOG_DEBUG(stderr, "CurTask resp content is %s .\n", context->resp_data.c_str());
				sucTasks.emplace_back(context->curTaskIndex);
				allResult.emplace_back(context->resp_data);
			}
			else
			{
				// failTasks.emplace_back(context->curTaskIndex);
				allResult.emplace_back("");
			}

			delete context;
		}

		{	// 打印所有并行任务结束时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			LOG_DEBUG(stderr, "--- ParallelWork end at : %s ---\r\n", timestamp);
		} });

	SeriesWork *series;
	CommContext *ctx;
	for (size_t i = 0; i < vTasks.size(); ++i)
	{
		ctx = new CommContext;
		series = Workflow::create_series_work(vTasks[i], [](const SeriesWork *subSeries)
											  {
			// 打印并行任务启动时间戳
			char timestamp[32];
			getDateStamp(timestamp, sizeof(timestamp));
			CommContext *subctx = (CommContext *)subSeries->get_context();
			LOG_DEBUG(stderr, "--- ParallelWork subSeries %d end at : %s ---\r\n", subctx->curTaskIndex, timestamp); });
		series->set_context(ctx);
		parallelWork->add_series(series);
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

	return sucTasks;
}

void HttpReqWays::base_callback_deal(WFHttpTask *task)
{
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	switch (state)
	{
	case WFT_STATE_SYS_ERROR:
		LOG_ERROR(stderr, "system error: %s\n", strerror(error));
		break;
	case WFT_STATE_DNS_ERROR:
		LOG_ERROR(stderr, "DNS error: %s\n", gai_strerror(error));
		break;
	case WFT_STATE_SSL_ERROR:
		LOG_ERROR(stderr, "SSL error: %d\n", error);
		break;
	case WFT_STATE_TASK_ERROR:
		LOG_ERROR(stderr, "Task error: %d\n", error);
		break;
	case WFT_STATE_SUCCESS:
		break;
	}

	/* Print req and resp info*/
	if (LOG_LEVEL_DEBUG >= GLOBAL_LOG_LEVEL)
	{
		debugPrint(task->get_req(), resp);
	}
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
	if (!series)
	{
		LOG_ERROR(stderr, "This task have not bind context.\n");
	}
	else
	{
		context = (CommContext *)series->get_context();
		if (!context->communicate_status)
		{
			LOG_DEBUG(stderr, "This Series task running failed at taskIndex: %d .\r\n", context->curTaskIndex);
			return;
		}
		++(context->curTaskIndex);
	}

	if (task->get_state() != WFT_STATE_SUCCESS)
	{
		LOG_ERROR(stderr, "WFT_STATE Error.\n");
		context->communicate_status = false;
		series->cancel();
		return;
	}

	/* Get response body. */
	context->resp_data = protocol::HttpUtil::decode_chunked_body(task->get_resp());
	context->communicate_status = true;

	LOG_DEBUG(stderr, "Print resp data: %s \n", context->resp_data.c_str());
	{ // 打印当前task结束时间戳
		char timestamp[32];
		getDateStamp(timestamp, sizeof(timestamp));
		LOG_DEBUG(stderr, "--- Series task %d end at : %s ---\r\n", context->curTaskIndex, timestamp);
	}
	return;
}

}