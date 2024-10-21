/*
SoLoud audio engine
Copyright (c) 2013-2014 Jari Komppa

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

#pragma once

#include <array>

// TODO: replace this entire file by std::jthread and std::mutex

namespace cer::thread
{
struct ThreadHandleData;

using ThreadFunction = void (*)(void*);

using ThreadHandle = ThreadHandleData*;

auto create_mutex() -> void*;

void destroy_mutex(void* handle);

void lock_mutex(void* handle);

void unlock_mutex(void* handle);

auto create_thread(ThreadFunction thread_function, void* parameter) -> ThreadHandle;

void sleep(int ms);

void wait(ThreadHandle thread_handle);

void release(ThreadHandle thread_handle);

auto time_millis() -> int;

static constexpr auto max_threadpool_tasks = size_t(1024);

class PoolTask
{
  public:
    virtual ~PoolTask() noexcept = default;

    virtual void work() = 0;
};

class Pool
{
  public:
    // Initialize and run thread pool. For thread count 0, work is done at addWork call.
    void init(size_t thread_count);

    Pool() = default;

    // Dtor. Waits for the threads to finish. Work may be unfinished.
    ~Pool();

    // Add work to work list. Object is not automatically deleted when work is done.
    void add_work(PoolTask* task);

    // Called from worker thread to get a new task. Returns null if no work available.
    auto get_work() -> PoolTask*;

    size_t        m_thread_count = 0; // number of threads
    ThreadHandle* m_thread       = nullptr; // array of thread handles
    void*         m_work_mutex   = nullptr; // mutex to protect task array/maxtask
    std::array<PoolTask*, max_threadpool_tasks> m_task_array = {}; // pointers to tasks
    size_t                                      m_max_task   = 0; // how many tasks are pending
    int          m_robin   = 0; // cyclic counter, used to pick jobs for threads
    volatile int m_running = 0; // running flag, used to flag threads to stop
};
} // namespace cer::thread
