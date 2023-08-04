/*
 * @Author: Leo
 * @Date: 2023-07-29 22:51:10
 * @LastEditTime: 2023-07-29 23:24:28
 * @Description: 锁
 */

#include "mutex.h"

namespace hiper{

    Semaphore::Semaphore(size_t size){
        if(sem_init(&sem_,0,size)){
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore() {
        sem_destroy(&sem_);
    }

    void Semaphore::wait() {
        if(sem_wait(&sem_)){
            throw std::logic_error("sem_wait error");
        }
    }

    void  Semaphore::notify() {
        if(sem_post(&sem_)){
            throw std::logic_error("sem_post error");
        }
    }

}
