/*
 * @Author: Leo
 * @Date: 2023-07-29 22:52:47
 * @LastEditTime: 2023-07-30 00:29:41
 * @Description:
 */


#include "thread.h"

#include "log.h"
#include "util.h"

namespace hiper {

static thread_local Thread*     g_cur_thread      = nullptr;
static thread_local std::string g_cur_thread_name = "unnamed";

Thread::Thread(const CallBack cb, const std::string& name)
    : name_(name)
    , cb_(cb)
    , sem_(0)
{
    if (name_.empty()) {
        name_ = "unnamed";
    }

    if (pthread_create(&thread_, nullptr, &Thread::run, this)) {
        throw std::logic_error("pthread_create error");
    }

    sem_.wait();
}

Thread::~Thread()
{
    if (thread_) {
        pthread_detach(thread_);
    }
}

Thread* Thread::GetThis(const std::string& name)
{
    return g_cur_thread;
}

pid_t Thread::getId() const
{
    return id_;
}

const std::string& Thread::getName() const
{
    return g_cur_thread_name;
}

void Thread::join()
{
    if (thread_) {
        if (pthread_join(thread_, nullptr)) {
            throw std::logic_error("pthread_join error");
        }
        thread_ = 0;
    }
}


const std::string& Thread::GetName()
{
    return g_cur_thread_name;
}

void* Thread::run(void* arg)
{
    Thread* thread = (Thread*)arg;

    g_cur_thread      = thread;
    g_cur_thread_name = g_cur_thread->name_;

    pthread_setname_np(pthread_self(), g_cur_thread_name.substr(0, 15).c_str());
    g_cur_thread->id_ = GetThreadId();
    CallBack cb;
    cb.swap(g_cur_thread->cb_);

    g_cur_thread->sem_.notify();
    cb();

    return 0;
}


void Thread::SetName(const std::string& name)
{
    if (g_cur_thread) g_cur_thread->name_ = name;
    g_cur_thread_name = name;
    pthread_setname_np(pthread_self(), g_cur_thread_name.substr(0, 15).c_str());
    return;
}

}   // namespace hiper