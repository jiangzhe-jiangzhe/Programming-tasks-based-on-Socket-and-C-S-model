#include "locker.h"

Sem::Sem()
{
    if (sem_init(&sem_, 0, 0) != 0)
    {
        throw std::exception();
    }
}

Sem::Sem(int num)
{
    if (sem_init(&sem_, 0, num) != 0)
    {
        throw std::exception();
    }
}

Sem::~Sem()
{
    sem_destroy(&sem_);
}

bool Sem::Wait()
{
    return sem_wait(&sem_) == 0;
}

bool Sem::Post()
{
    return sem_post(&sem_) == 0;
}

Locker::Locker()
{
    if (pthread_mutex_init(&mutex_, NULL) != 0)
    {
        throw std::exception();
    }
}

Locker::~Locker()
{
    pthread_mutex_destroy(&mutex_);
}

bool Locker::Lock()
{
    return pthread_mutex_lock(&mutex_) == 0;
}

bool Locker::Unlock()
{
    return pthread_mutex_unlock(&mutex_) == 0;
}

pthread_mutex_t *Locker::get()
{
    return &mutex_;
}