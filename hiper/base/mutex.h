#ifndef HIPER_MUTEX_H
#define HIPER_MUTEX_H

#include "noncopyable.h"

#include <atomic>
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>


namespace hiper {
class Semaphore {
public:
    Semaphore(size_t size);
    ~Semaphore();
    void wait();
    void notify();

private:
    sem_t sem_;
};

template<typename T> class ScopeLock : Noncopyable {
public:
    ScopeLock(T& t)
        : mutex_(t)
    {
        mutex_.lock();
        is_lock_ = true;
    }

    void unlock()
    {
        if (is_lock_) {
            mutex_.unlock();
        }
    }

    void lock()
    {
        if (!is_lock_) {
            mutex_.lock();
        }
    }

    ~ScopeLock()
    {
        if (is_lock_) {
            is_lock_ = false;
            mutex_.unlock();
        }
    }

private:
    T&   mutex_;
    bool is_lock_;
};

template<typename T> class RScopeLock : Noncopyable {
public:
    RScopeLock(T& t)
        : mutex_(t)
        , is_lock_(false)
    {
        mutex_.rdlock();
        is_lock_ = true;
    }

    ~RScopeLock()
    {
        if (is_lock_) {
            mutex_.unlock();
        }
    }

    void lock()
    {
        if (!is_lock_) {
            mutex_.rdlock();
            is_lock_ = true;
        }
    }

    void unlock()
    {
        if (is_lock_) {
            mutex_.unlock();
            is_lock_ = false;
        }
    }

private:
    T&   mutex_;
    bool is_lock_;
};


template<typename T> class WScopeLock : Noncopyable {
public:
    WScopeLock(T& t)
        : mutex_(t)
        , is_lock_(false)
    {
        mutex_.wrlock();
        is_lock_ = true;
    }

    ~WScopeLock()
    {
        if (is_lock_) {
            mutex_.unlock();
        }
    }

    void lock()
    {
        if (!is_lock_) {
            mutex_.wrlock();
            is_lock_ = true;
        }
    }

    void unlock()
    {
        if (is_lock_) {
            mutex_.unlock();
            is_lock_ = false;
        }
    }

private:
    T&   mutex_;
    bool is_lock_;
};


class Mutex : Noncopyable {
public:
    using Lock = ScopeLock<Mutex>;

    Mutex() { pthread_mutex_init(&mutex_, nullptr); };
    ~Mutex() { pthread_mutex_destroy(&mutex_); };
    void unlock() { pthread_mutex_unlock(&mutex_); };
    void lock() { pthread_mutex_lock(&mutex_); };

private:
    pthread_mutex_t mutex_;
};


class RWMutex : Noncopyable {
public:
    using ReadLock = RScopeLock<RWMutex>;
    using WriteLock = WScopeLock<RWMutex>;

    RWMutex() { pthread_rwlock_init(&rwlock_, nullptr); };
    ~RWMutex() { pthread_rwlock_destroy(&rwlock_); };
    void rdlock() { pthread_rwlock_rdlock(&rwlock_); };
    void wrlock() { pthread_rwlock_wrlock(&rwlock_); };
    void unlock() { pthread_rwlock_unlock(&rwlock_); };

private:
    pthread_rwlock_t rwlock_;
};





class SpinLock : Noncopyable {
public:
    using Lock = ScopeLock<SpinLock>;

    SpinLock() { pthread_spin_init(&spinlock_, 0); };
    ~SpinLock() { pthread_spin_destroy(&spinlock_); };
    void unlock() { pthread_spin_unlock(&spinlock_); };
    void lock() { pthread_spin_lock(&spinlock_); };

private:
    pthread_spinlock_t spinlock_;
};


class CASLock : Noncopyable {
public:
    using Lock = ScopeLock<CASLock>;
    CASLock() { flag_.clear(); };

    ~CASLock() {}

    void lock()
    {
        while (std::atomic_flag_test_and_set_explicit(&flag_, std::memory_order_acquire))
            ;
    }

    void unlock() { std::atomic_flag_clear_explicit(&flag_, std::memory_order_acquire); }

private:
    std::atomic_flag flag_;
};


}   // namespace hiper
#endif   // HIPER_MUTEX_H