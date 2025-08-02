#include <sys/epoll.h>
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0; //空事件 
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false) {}

Channel::~Channel() {}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    // 被绑住的Channel, 需要检查绑定他的TcpConnection 是不是还活着（没有析构），因为Channel的Callback可能是TcpConnection的成员函数。
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();    //weak_ptr.lock() 返回shared_ptr if 这个对象还活着
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {    //如果没有绑住（例如计时，Acceptor 的监听 socket）就没关系
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents: %d\n", revents_);
    // 关闭-挂起且没有数据可读了，TcpConnection 通过shutdown关闭写端epoll触发EPOLLHUP
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        } else {
            LOG_WARN("EPOLLIN received but closeCallback_ not set!");
        }
    } 
    // 错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_) {
            errorCallback_();
        } else {
            LOG_WARN("EPOLLIN received but errorCallback_ not set!");
        }
    }
    // 读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_) {
            readCallback_(receiveTime);
        } else {
            LOG_WARN("EPOLLIN received but readCallback_ not set!");
        }
    }
    // 写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_){
            writeCallback_();
        } else {
            LOG_WARN("EPOLLIN received but writeCallback_ not set!");
        }
    }
}