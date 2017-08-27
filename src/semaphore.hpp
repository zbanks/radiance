#ifndef __UTIL_SEMAPHORE_H__
#define __UTIL_SEMAPHORE_H__

#include <QtGlobal>

#if defined(Q_OS_MAC)
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <mach/mach_time.h>
#elif defined(Q_OS_UNIX)
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#else
#include <QSemaphore>
#endif

namespace radiance {
#if defined(Q_OS_MAC)
class RSemaphore {
    semaphore_t m_d;
    public:
        RSemaphore( int value = 0)
        {
            auto task = mach_task_self();
            auto res  = semaphore_create(task, &m_d, SYNC_POLICY_FIFO, value);
            if(res != KERN_SUCCESS) {
                throw std::system_error(res,std::system_category(), "sem_init failed.");
            }
        }
        ~RSemaphore() {
            auto task = mach_task_self();
            semaphore_deestroy(task, m_d);
            m_d = 0;
        }
        bool tryAcquire()
        {
            kern_return_t res;
            mach_timespec_t wait_time = mach_timespec_t{0,0};
            while((res = semaphore_timedwait(m_d,wait_time)) != KERN_SUCCESS && res != KERN_OPERATION_TIMED_OUT) { }
            return res == KERN_SUCCESS;
        }
        void acquire()
        {
            kern_return_t res;
            while((res = semaphore_wait(m_d)) != KERN_SUCCESS) { }
        }
        void release(int n = 1) { for(auto i = 0; i < n; ++i ) semaphore_signal(m_d); }
};

////////////////////////////// Unix //////////////////////////////
//
#elif defined(Q_OS_UNIX)

class RSemaphore {
    sem_t   m_d;
    public:
        RSemaphore( int value = 0)
        {
            if(sem_init(&m_d, 0, value) < 0) {
                throw std::system_error(errno,std::system_category(), "sem_init failed.");
            }
        }
        ~RSemaphore() { sem_destroy(&m_d); }
        bool tryAcquire()
        {
            while(sem_trywait(&m_d) < 0) {
                auto err = errno;
                if(err == EAGAIN)
                    return false;
                else if(err != EINTR)
                    throw std::system_error(err,std::system_category(), "Trywait failed.");
            }
            return true;
        }
        void acquire()
        {
            while(sem_wait(&m_d) < 0) {
                auto err = errno;
                if(err != EINTR && err != EAGAIN)
                    throw std::system_error(err,std::system_category(), "Invalid.");
            }
        }
        void release(int n = 1) { for(auto i = 0; i < n; ++i) sem_post(&m_d); }
};

////////////////////////////// Default //////////////////////////////
#else

using RSemaphore = QSemaphore;

#endif
}

#endif /* __UTIL_SEMAPHORE_H__ */
