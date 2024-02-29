/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        multipart_parser.h
Version:     1.0
Author:      cjx
start date: 2023-8-28
Description: 文件打包为multipart方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef _MULTIPART_PARSER_H_
#define _MULTIPART_PARSER_H_

#include <limits>
#include <string>
#include <tuple>
#include <vector>

namespace communicate
{

struct ChunkFileInfo
{
    std::string fileName;
    uint32_t startPos;
    uint32_t endPos;
};

class MultipartParser
{
public:
    MultipartParser();
    inline const std::string &body_content()
    {
        return body_content_;
    }
    inline const std::string &boundary()
    {
        return boundary_;
    }
    inline void addParameter(const std::string &name, const std::string &value)
    {
        params_.push_back(std::move(std::pair<std::string, std::string>(name, value)));
    }
    inline void addFile(const std::string &name, const std::string &value)
    {
        files_.push_back(std::move(std::pair<std::string, std::string>(name, value)));
    }
    inline void addChunkFile(const std::string &name, const ChunkFileInfo &chunkInfo)
    {
        chunkFiles_.push_back(std::move(std::pair<std::string, ChunkFileInfo>(name, chunkInfo)));
    }
    inline void addChunkFile(const std::string &name, const std::string &value,
        const uint32_t s = 0, const uint32_t e = std::numeric_limits<uint32_t>::max())
    {
        addChunkFile(name, ChunkFileInfo{value, s, e});
    }
    const std::string &GenBodyContent();

private:
    void _get_file_name_type(const std::string &file_path, std::string *filenae, std::string *content_type);

private:
    static const std::string boundary_prefix_;
    static const std::string rand_chars_;
    std::string boundary_;
    std::string body_content_;
    std::vector<std::pair<std::string, std::string>> params_;
    std::vector<std::pair<std::string, std::string>> files_;
    std::vector<std::pair<std::string, ChunkFileInfo>> chunkFiles_;
};

}

#endif