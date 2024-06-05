#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#include "base/logconfig.h"

// T 为实际的数据结构，容器这种结构不包含

template <typename T>
void saveStructsToFile(const std::vector<T> &data, const std::string &filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        for (const T &item : data)
        {
            file.write(reinterpret_cast<const char *>(&item), sizeof(T));
        }
        file.close();
        LOG_DEBUG(stderr, "data write file success, filePath: %s \n", filename.c_str());
    }
    else
    {
        LOG_ERROR(stderr, "can't open this file to write, filePath: %s \n", filename.c_str());
    }
}

template <typename T>
void loadStructsFromFile(std::vector<T> &data, const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        T item;
        while (file.read(reinterpret_cast<char *>(&item), sizeof(T)))
        {
            data.push_back(item);
        }
        file.close();
        LOG_DEBUG(stderr, "load file data success, filePath: %s \n", filename.c_str());
    }
    else
    {
        LOG_ERROR(stderr, "can't open this file to load, filePath: %s \n", filename.c_str());
    }
}

template <typename T>
void saveStructToFile(const T &data, const std::string &filename)
{
    std::vector<T> vStructs = {data};
    saveStructsToFile(vStructs, filename);
}

template <typename T>
void loadStructFromFile(T &data, const std::string &filename)
{
    std::vector<T> vStructs = {};
    loadStructsFromFile(vStructs, filename);
    data = vStructs.front();
}