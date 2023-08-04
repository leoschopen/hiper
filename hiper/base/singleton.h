/*
 * @Author: Leo
 * @Date: 2023-07-30 10:49:23
 * @LastEditTime: 2023-07-30 10:49:35
 * @Description: 单例模式
 */

#ifndef HIPER_SINGLETON_H
#define HIPER_SINGLETON_H

#include <memory>

namespace hiper {

namespace {

template<class T, class X, int N> T& GetInstanceX()
{
    static T v;
    return v;
}

template<class T, class X, int N> std::shared_ptr<T> GetInstancePtr()
{
    static std::shared_ptr<T> v(new T);
    return v;
}


}   // namespace

/**
 * @brief 单例模式封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0> class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     */
    static T* GetInstance()
    {
        static T v;
        return &v;
        // return &GetInstanceX<T, X, N>();
    }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0> class SingletonPtr {
public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
        // return GetInstancePtr<T, X, N>();
    }
};

}   // namespace hiper

#endif   // HIPER_SINGLETON_H