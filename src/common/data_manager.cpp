#include "data_manager.h"

#include "base/utils_method.h"
#include "utils/memory_access.h"

std::string g_saveFileDir = "./temporary_cache/";

namespace communicate
{

const std::string saveResumeFileDir = g_saveFileDir + "resume_note/";

static std::shared_ptr<DataManager> s_ota_datamanager = nullptr;
static std::once_flag s_singleFlag;

std::shared_ptr<DataManager> DataManager::getInstance()
{
    std::call_once(s_singleFlag, [&] {
        s_ota_datamanager = std::shared_ptr<DataManager>(new DataManager());
    });
    return s_ota_datamanager;
}

DataManager::DataManager()
    : m_downResume_() , m_uploadResume_()
{
    std::filesystem::create_directories(saveResumeFileDir);
    
    getAllResumeTasks();
}

// DataManager::~DataManager()
// {
//     m_downResume_.clear();
//     m_uploadResume_.clear();
// }

void DataManager::getAllResumeTasks()
{
    Resume_FileInfo tempCache;

    std::vector<std::string> allDownResumeTasks = getFilesWithExtension(saveResumeFileDir, "_down");
    for (const std::string &fileName : allDownResumeTasks)
    {
        loadStructFromFile(tempCache, fileName);
        std::string key = tempCache.fileHash[0] == '\0' ? tempCache.commAddr : tempCache.fileHash;
        m_downResume_.insert({key, tempCache});
    }

    std::vector<std::string> allSendResumeTasks = getFilesWithExtension(saveResumeFileDir, "_send");
    for (const std::string &fileName : allSendResumeTasks)
    {
        loadStructFromFile(tempCache, fileName);
        std::string key = tempCache.fileHash[0] == '\0' ? tempCache.commAddr : tempCache.fileHash;
        m_uploadResume_.insert({key, tempCache});
    }
}

const Resume_FileInfo *DataManager::getDownTaskInfo(const std::string &key) const
{
    if (m_downResume_.find(key) == m_downResume_.end())
    {
        return nullptr;
    }

    return &m_downResume_.at(key);
}

const std::vector<const Resume_FileInfo *> DataManager::getSendTaskInfo(const std::string &taskAddr) const
{
    std::vector<const Resume_FileInfo *> result = {};

    auto range = m_uploadResume_.equal_range(taskAddr);
    for (auto it = range.first; it != range.second; ++it)
    {
        result.emplace_back(&(it->second));
    }

    return result;
}

void DataManager::saveDownTaskInfo(const Resume_FileInfo &taskInfo, std::string fileName)
{
    if (fileName == "")
    {
        fileName = saveResumeFileDir + taskInfo.commAddr + "_down";
    }
    else
    {
        fileName = saveResumeFileDir + fileName + "_down";
    }
    
    saveStructToFile(taskInfo, fileName);
    std::string key = taskInfo.fileHash[0] == '\0' ? taskInfo.commAddr : taskInfo.fileHash;
    m_downResume_.insert({key, taskInfo});
}

void DataManager::delDownTaskInfo(const std::string &key)
{
    if (m_downResume_.find(key) == m_downResume_.end())
    {
        return;
    }
    std::string filePath = saveResumeFileDir + m_downResume_.at(key).resumeName + "_down";
    std::remove(filePath.c_str());
    m_downResume_.erase(key);
}

}