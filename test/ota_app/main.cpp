#include "dpi_ota_req_api.h"

#include <chrono>
#include <iostream>
#include <string>

#define MaxTryTimes 3

int main(int argc, char *argv[])
{
    std::string otaCfgPath = "./dpi_ota_config.json";
    if (argc >= 2)
    {
        otaCfgPath = argv[1];
    }
    std::cout << "ota config path: " << otaCfgPath << std::endl;

    int num = 0;
    while (num <= MaxTryTimes)
    {
        if (dpi::ota::initialize(otaCfgPath))
        {
            break;
        }
        num++;
    }
    if (num > MaxTryTimes)
    {
        return 1;
    }

    dpi::ota::LicenseResult licenseRes;
    if (!dpi::ota::dealLicenseStatus(licenseRes, dpi::ota::ReqUser::All))
    {
        return -1;
    }
    for (const auto& pair : licenseRes) {
        std::cout << "键: " << (int)pair.first << ", 值: " << pair.second << std::endl;
    }


    std::string verPath;
    std::pair<std::string, std::string> testVer = {"mapengine", "98"};
    if (!dpi::ota::checkMapEngineVersion(verPath, testVer))
    {
        return 2;
    }
    std::cout << "ota update filepath: " << verPath << std::endl;

    std::string respInfo;
    dpi::ota::GatherData testGather;
    testGather.filepath = verPath;
    testGather.seq = 0;
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为时间戳
    testGather.stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    if (!dpi::ota::sendGatherData(respInfo, testGather))
    {
        return 3;
    }
    std::cout << "gather data resp: " << respInfo << std::endl;

    dpi::ota::CompositionData testSendData;
    testSendData.filePath = otaCfgPath;
    if (!dpi::ota::sendCompositionData(respInfo, testSendData))
    {
        return 4;
    }
    
    return 0;
}