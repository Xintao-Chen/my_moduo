#include "_Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);  //() {} 是调用构造函数赋值

Thread::Thread(ThreadFunc func, const std::string &name = std::string())
    : started_(false)
    , joined_(false)
    , name_(name)
    , tid_(0)    //还没启动thread呢
    , func_(std::move(func))
    {
        setDefaultName();
    }   

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);  //sem_init(sem_t *sem, int pshared, unsigned int value)

    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();                                  
        sem_post(&sem);
        func_();   
    }));
    
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
