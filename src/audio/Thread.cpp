/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "util/Platform.hpp"

#ifdef CERLIB_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <ctime>
#include <pthread.h>
#include <unistd.h>
#endif

#include "audio/Thread.hpp"

#ifdef CERLIB_PLATFORM_WINDOWS
namespace cer::thread
{
struct ThreadHandleData
{
    HANDLE thread;
};
} // namespace cer::thread

void* cer::thread::create_mutex()
{
    CRITICAL_SECTION* cs = new CRITICAL_SECTION;
    InitializeCriticalSectionAndSpinCount(cs, 100);
    return (void*)cs;
}

void cer::thread::destroy_mutex(void* aHandle)
{
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)aHandle;
    DeleteCriticalSection(cs);
    delete cs;
}

void cer::thread::lock_mutex(void* aHandle)
{
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)aHandle;
    if (cs)
    {
        EnterCriticalSection(cs);
    }
}

void cer::thread::unlock_mutex(void* aHandle)
{
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)aHandle;
    if (cs)
    {
        LeaveCriticalSection(cs);
    }
}

struct soloud_thread_data
{
    cer::thread::ThreadFunction mFunc;
    void*                       mParam;
};

static DWORD WINAPI threadfunc(LPVOID d)
{
    soloud_thread_data* p = (soloud_thread_data*)d;
    p->mFunc(p->mParam);
    delete p;
    return 0;
}

auto cer::thread::create_thread(ThreadFunction aThreadFunction, void* aParameter) -> ThreadHandle
{
    soloud_thread_data* d = new soloud_thread_data;
    d->mFunc              = aThreadFunction;
    d->mParam             = aParameter;
    HANDLE h              = CreateThread(nullptr, 0, threadfunc, d, 0, nullptr);
    if (0 == h)
    {
        return 0;
    }
    ThreadHandleData* thread_handle = new ThreadHandleData;
    thread_handle->thread           = h;
    return thread_handle;
}

void cer::thread::sleep(int aMSec)
{
    Sleep(aMSec);
}

void cer::thread::wait(ThreadHandle aThreadHandle)
{
    WaitForSingleObject(aThreadHandle->thread, INFINITE);
}

void cer::thread::release(ThreadHandle aThreadHandle)
{
    CloseHandle(aThreadHandle->thread);
    delete aThreadHandle;
}

int cer::thread::time_millis()
{
    return GetTickCount();
}

#else // pthreads
namespace cer::thread
{
struct ThreadHandleData
{
    pthread_t thread;
};
} // namespace cer::thread

auto cer::thread::create_mutex() -> void*
{
    auto* mutex = new pthread_mutex_t();

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    pthread_mutex_init(mutex, &attr);

    return mutex;
}

void cer::thread::destroy_mutex(void* handle)
{
    if (auto* mutex = static_cast<pthread_mutex_t*>(handle))
    {
        pthread_mutex_destroy(mutex);
        delete mutex;
    }
}

void cer::thread::lock_mutex(void* handle)
{
    if (auto* mutex = static_cast<pthread_mutex_t*>(handle))
    {
        pthread_mutex_lock(mutex);
    }
}

void cer::thread::unlock_mutex(void* aHandle)
{
    if (auto* mutex = static_cast<pthread_mutex_t*>(aHandle))
    {
        pthread_mutex_unlock(mutex);
    }
}

struct soloud_thread_data
{
    cer::thread::ThreadFunction func  = nullptr;
    void*                       param = nullptr;
};

static void* thread_func(void* d)
{
    const auto* p = static_cast<const soloud_thread_data*>(d);

    p->func(p->param);
    delete p;

    return nullptr;
}

auto cer::thread::create_thread(ThreadFunction aThreadFunction, void* aParameter) -> ThreadHandle
{
    auto* d  = new soloud_thread_data();
    d->func  = aThreadFunction;
    d->param = aParameter;

    auto* thread_handle = new ThreadHandleData();
    pthread_create(&thread_handle->thread, nullptr, thread_func, d);

    return thread_handle;
}

void cer::thread::sleep(int ms)
{
    // usleep(aMSec * 1000);
    timespec req = {};
    req.tv_sec   = 0;
    req.tv_nsec  = ms * 1000000L;

    nanosleep(&req, nullptr);
}

void cer::thread::wait(ThreadHandle thread_handle)
{
    pthread_join(thread_handle->thread, 0);
}

void cer::thread::release(ThreadHandle thread_handle)
{
    delete thread_handle;
}

auto cer::thread::time_millis() -> int
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + int(spec.tv_nsec / 1.0e6);
}
#endif

namespace cer::thread
{
static void pool_worker(void* param)
{
    auto* my_pool = static_cast<Pool*>(param);

    while (my_pool->m_running)
    {
        if (PoolTask* t = my_pool->get_work(); t == nullptr)
        {
            sleep(1);
        }
        else
        {
            t->work();
        }
    }
}

Pool::~Pool()
{
    m_running = 0;

    for (size_t i = 0; i < m_thread_count; ++i)
    {
        wait(m_thread[i]);
        release(m_thread[i]);
    }

    delete[] m_thread;

    if (m_work_mutex != nullptr)
    {
        destroy_mutex(m_work_mutex);
    }
}

void Pool::init(size_t thread_count)
{
    if (thread_count > 0)
    {
        m_max_task     = 0;
        m_work_mutex   = create_mutex();
        m_running      = 1;
        m_thread_count = thread_count;
        m_thread       = new ThreadHandle[thread_count];

        for (size_t i = 0; i < m_thread_count; ++i)
        {
            m_thread[i] = create_thread(pool_worker, this);
        }
    }
}

void Pool::add_work(PoolTask* aTask)
{
    if (m_thread_count == 0)
    {
        aTask->work();
    }
    else
    {
        if (m_work_mutex)
            lock_mutex(m_work_mutex);
        if (m_max_task == max_threadpool_tasks)
        {
            // If we're at max tasks, do the task on calling thread
            // (we're in trouble anyway, might as well slow down adding more work)
            if (m_work_mutex)
                unlock_mutex(m_work_mutex);
            aTask->work();
        }
        else
        {
            m_task_array[m_max_task] = aTask;
            m_max_task++;
            if (m_work_mutex)
                unlock_mutex(m_work_mutex);
        }
    }
}

PoolTask* Pool::get_work()
{
    PoolTask* t = nullptr;
    if (m_work_mutex)
    {
        lock_mutex(m_work_mutex);
    }

    if (m_max_task > 0)
    {
        const int r = m_robin % m_max_task;
        m_robin++;
        t               = m_task_array[r];
        m_task_array[r] = m_task_array[m_max_task - 1];
        m_max_task--;
    }

    if (m_work_mutex)
    {
        unlock_mutex(m_work_mutex);
    }
    return t;
}
} // namespace cer::thread