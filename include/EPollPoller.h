#pragma once
#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

/**
 * epoll的使用:
 * 1. epoll_create   返回一个epoll实例 == int的fd
 * 2. epoll_ctl (add, mod, del)    控制epoll
 * 3. epoll_wait     返回有事件发生的fd数量
 **/

class Channel;

// Poller 的public成员在EPollPoller里还是public的
class EPollPoller : public Poller {
    public:
        EPollPoller(EventLoop *loop);
        ~EPollPoller() override;

        //纯虚函数的override声明
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
        virtual void updateChannel(Channel *channel) override;
        virtual void removeChannel(Channel *channel) override;

    private:
        static const int kInitEventListSize = 16;

        // 填写活跃的连接
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
        // 更新channel通道 其实就是调用epoll_ctl
        void update(int operation, Channel *channel);

        using EventList = std::vector<epoll_event>; // C++中可以省略struct 直接写epoll_event即可

        int epollfd_;      // epoll_create创建返回的fd保存在epollfd_中
        EventList events_; // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集
};