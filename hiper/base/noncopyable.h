/*
 * @Author: Leo
 * @Date: 2023-07-29 23:14:58
 * @LastEditTime: 2023-07-30 00:11:14
 * @Description: 不可拷贝类
 */

#ifndef HIPER_NOCOPYABLE_H
#define HIPER_NOCOPYABLE_H

namespace hiper {

class Noncopyable {
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};

}   // namespace hiper

#endif   // HIPER_NOCOPYABLE_H