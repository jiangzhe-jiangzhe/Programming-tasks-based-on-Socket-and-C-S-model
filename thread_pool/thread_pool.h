#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <set>
#include <iostream>
#include "../lock/locker.h"

template <typename T>
class ThreadPool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    ThreadPool(int thread_number = 6, int max_request = 10000);
    ~ThreadPool();
    bool Append(T *request, int &res);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *Worker(void *arg);
    void Run();

private:
    int thread_number_;         // 线程池中的线程数
    int max_requests_;          // 请求队列中允许的最大请求数
    pthread_t *threads_;        // 描述线程池的数组，其大小为thread_number_
    std::list<T *> work_queue_; // 请求队列
    std::set<T *> set_work_;    // 判断是否重复,发现UDP单播/组播会重复的把连接加入请求队列，导致其他连接不能响应
    Locker queue_locker_;       // 保护请求队列的互斥锁
    Sem queue_stat_;            // 是否有任务需要处理
};

template <typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests) : thread_number_(thread_number), max_requests_(max_requests), threads_(NULL)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    threads_ = new pthread_t[thread_number_];
    if (!threads_)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(threads_ + i, NULL, Worker, this) != 0)
        {
            delete[] threads_;
            throw std::exception();
        }
        if (pthread_detach(threads_[i]))
        {
            delete[] threads_;
            throw std::exception();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] threads_;
}

template <typename T>
bool ThreadPool<T>::Append(T *request, int &res)
{
    queue_locker_.Lock();
    if (work_queue_.size() >= max_requests_)
    {
        queue_locker_.Unlock();
        return false;
    }
    if (set_work_.find(request) == set_work_.end())
    {
        res = request->HandleRecv();
        if (res <= 0)
        {
            queue_locker_.Unlock();
            return false;
        }
        work_queue_.push_back(request);
        set_work_.insert(request);
        queue_locker_.Unlock();
        queue_stat_.Post();
    }
    else
    {
        queue_locker_.Unlock();
    }

    return true;
}

template <typename T>
void *ThreadPool<T>::Worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    pool->Run();
    return pool;
}

template <typename T>
void ThreadPool<T>::Run()
{
    while (true)
    {
        queue_stat_.Wait();
        queue_locker_.Lock();
        if (work_queue_.empty())
        {
            queue_locker_.Unlock();
            continue;
        }
        T *request = work_queue_.front();
        work_queue_.pop_front();
        set_work_.erase(request);
        queue_locker_.Unlock();
        if (!request)
            continue;
        request->HandleSend();
    }
}
#endif