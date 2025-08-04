#include <errno.h>     //linux 错误码常量
#include <unistd.h>    //linux 常用的系统调用接口
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1;    // 某个channel还没添加至Poller          // channel的成员index_初始化为-1
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop) 
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))   //EPOLL_CLOEXEC在启动别的程序时这个epollfd_自动关闭
    , events_(kInitEventListSize) 
{
    if (epollfd_ < 0 ) {
        LOG_FATAL("epoll_create error:%d \n", errno);   //errno 记录了错误
    }        
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);    //释放资源
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION_, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now()); //一个由当前事件创立的事件戳对象

    if (numEvents > 0) {
        LOG_DEBUG("%d events happend\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s No events happend in time\n", __FUNCTION__);
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error:%d!", errno);
        }
    }
    return now;
}

// updateChannel会检测这个channel没有被注册过，然后会调用update真正去epoll_ctl注册
void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    LOG_DEBUG("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;    //Poller的一个map, 记录关注的channel
        } else { 
            //index == kDeleted 这个channel已经删除了 ==  说明map里有这个记录的就什么也不做 
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        //channel 已经在poller里注测了
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            //这个channel没有关心的event, 移出epoll监听的队列
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_DEBUG("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    int fd = channel->fd();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// update 将channel 和其关心的事件注册到epoll
void EPollPoller::update(int op, Channel *channel) {
    epoll_event ev{};  //将值全部初始化为0， 等于 ::memset(&ev, 0, sizeof(ev));
    int fd = channel->fd();
    ev.events = channel->events();
    ev.data.fd = fd;
    ev.data.ptr = channel;

    if (::epoll_ctl(epollfd_, op, fd, &ev) < 0) {
        if (op == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}


//activeChannels 是一个临时列表
// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);  //是在poll的epoll_wait里对events_进行的修改,前n是监听到有动静的epoll_event
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
    }
}


