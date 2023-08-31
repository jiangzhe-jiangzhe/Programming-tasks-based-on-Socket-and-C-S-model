#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class Sem
{
public:
    Sem();
    Sem(int num);
    ~Sem();
    bool Wait();
    bool Post();

private:
    sem_t sem_;
};

class Locker
{
public:
    Locker();
    ~Locker();
    bool Lock();
    bool Unlock();
    pthread_mutex_t *get();

private:
    pthread_mutex_t mutex_;
};
#endif
