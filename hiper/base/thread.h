/*
 * @Author: Leo
 * @Date: 2023-07-29 22:52:39
 * @LastEditTime: 2023-07-30 00:29:53
 * @Description: 线程类
 */

#ifndef HIPER_THREAD_H
#define HIPER_THREAD_H

#include "mutex.h"
#include "noncopyable.h"

#include <functional>
#include <memory>
#include <pthread.h>
#include <stdexcept>
#include <string>


namespace hiper {

class Thread : public Noncopyable {
public:
    using ptr      = std::shared_ptr<Thread>;
    using CallBack = std::function<void()>;

    Thread(const CallBack cb, const std::string& name);
    ~Thread();

    void join();

    pid_t              getId() const;
    const std::string& getName() const;

public:
    static Thread*            GetThis(const std::string& name = "main");
    static const std::string& GetName();
    static void               SetName(const std::string& name);

private:
    static void* run(void*);

private:
    pid_t       id_     = -1;
    pthread_t   thread_ = 0;
    std::string name_;
    CallBack    cb_;
    Semaphore   sem_;
};

}   // namespace hiper

#endif   // HIPER_THREAD_H
