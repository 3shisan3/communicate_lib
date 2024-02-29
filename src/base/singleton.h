/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
All rights reserved.
File:        singleton.h
Version:     1.0
Author:      cjx
start date: 2023-8-28
Description: 通用单例方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]
1             2023-8-28      cjx        create

*****************************************************************/

#ifndef _COMMON_SINGLETON_H_
#define _COMMON_SINGLETON_H_

#include <memory>
#include <mutex>

template<class SingletonClass>
class SingletonTemplate
{
public:
    // 构造，析构暂无特殊操作

    /**
     * @brief 获取单例
     *
     * @return 
     */
    static std::shared_ptr<SingletonClass> &getSingletonInstance()
    {
        std::call_once(m_s_singleFlag, [&] {
            m_s_pSingletonInstance.reset(new SingletonClass());
        });

        return m_s_pSingletonInstance;
    }

private:
    /**
     * @brief 单例的静态指针
     */
    static std::shared_ptr<SingletonClass> m_s_pSingletonInstance;

    /**
     * @brief 执行一次触发标志
     */
    static std::once_flag m_s_singleFlag;
};

template<class SingletonClass>
std::shared_ptr<SingletonClass> SingletonTemplate<SingletonClass>::m_s_pSingletonInstance = nullptr;

template<class SingletonClass>
std::once_flag SingletonTemplate<SingletonClass>::m_s_singleFlag;

#endif