/*
 * @Author: Leo
 * @Date: 2023-07-28 10:57:45
 * @LastEditTime: 2023-07-30 00:33:14
 * @Description: main
 */

#include "../hiper/base/mutex.h"

#include "../hiper/base/thread.h"
#include "../hiper/base/util.h"

#include <atomic>
#include <bits/types/clock_t.h>
#include <bits/types/time_t.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>

// int count = 0;
std::atomic<int>  count = 0;
std::shared_mutex shared_mutex;
std::mutex        mutex;
hiper::RWMutex    rwmutex;

hiper::SpinLock spin_mutex;
hiper::CASLock  cas_mutex;
void            fun1()
{
    for (int i = 0; i < 1000; ++i) {
        // std::unique_lock<std::shared_mutex> lock(shared_mutex);
        // std::lock_guard<std::mutex> lock(mutex);
        // std::lock_guard<hiper::SpinLock> lock(spin_mutex);
        hiper::SpinLock::Lock lock(spin_mutex);
        // hiper::CASLock::Lock lock(cas_mutex);
        // hiper::RWMutex::WriteLock lock(rwmutex);
        ++count;
    }
}

int main()
{
    clock_t avg = 0;
    for (int m = 0; m < 10; ++m) {

        auto start = clock();

        std::vector<hiper::Thread::ptr> threads;

        for (int i = 0; i < 10000; ++i) {
            threads.push_back(std::make_shared<hiper::Thread>(
                []() {
                    // std::lock_guard<std::mutex> lock(mutex);
                    // std::cout << "thread name: " << hiper::Thread::GetName() << std::endl;
                    // std::cout << "thread id: " << hiper::Thread::GetThis()->getId() << std::endl;
                    fun1();
                },
                "name_" + std::to_string(i)));
        }
        for (int i = 0; i < 5; ++i) {
            threads[i]->join();
        }
        auto end = clock();
        avg += (end - start) * 1000 / CLOCKS_PER_SEC;
    }
    std::cout << "avg: " << avg / 10 << std::endl;
    return 0;
}