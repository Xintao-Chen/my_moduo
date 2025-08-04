#include <sys/eventfd.h>
#include <unistd.h>   //接口函数， 如getpid(), read()
#include <fcntl.h>   //fd操作 如 open() 打开文件返回fd
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000; // 10000毫秒 = 10秒钟

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) { 
        //创建失败返回-1
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())                   //创建一个
    , wakeupChannel_(new Channel(this, wakeupFd_)) 
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread) {
        // 如果已有一个EventLoop在这个thread
        LOG_FATAL("Another EventLoop %p alread exists in this thread %d\n", t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this)    //成员函数时需要加&，给具体的地址
    );

    wakeupChannel_->enableReading(); // 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了
}


EventLoop::~EventLoop() {
    wakeupChannel_->disableAll(); // 给Channel移除所有感兴趣的事件
    wakeupChannel_->remove();     // 把Channel从EventLoop上删除掉
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}


void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); 
        for(Channel* channel : activeChannels_) {
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

//用wakeup来让epoll__wait 不再阻塞（因为有wakeupChannel有事件发生了），然后就会看到quit_ = true; 停止loop
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 当前EventLoop中执行回调
    {
        cb();
    }
    else // 在非当前EventLoop线程中执行cb，就需要唤醒EventLoop所在线程执行cb
    {
        queueInLoop(cb);
    }
}

// 把cb放进队列里，唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);    //锁上，离开作用域解锁
        pendingFunctors_.emplace_back(cb);
    }
    // 如果你不加 if，直接 wakeup()：
    // 每次 queueInLoop() 都会触发一次 eventfd 写操作；
    // 哪怕你就在 loop 线程里，下一行代码就能执行；
    // 或者没有在执行回调，只是闲着也会唤醒；
    // 这样相当于多做了很多没意义的系统调用，造成浪费。
    if (!isInLoopThread() || callingPendingFunctors_) {     //只要有新的回调进来，就需要唤醒防止延迟
        wakeup(); 
    }
}

// 读掉wakeupfd，重置它
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

// 用来唤醒loop所在线程 向wakeupFd_写一个数据 wakeupChannel就发生读事件 当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}


// EventLoop的方法 => Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}