#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr; //还未实现，返回poll的实例
    } else {
        return new EPollPoller(loop);  //epoll
    }
}