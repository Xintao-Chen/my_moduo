#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块
class Poller {
    public:
        using ChannelList = std::vector<Channel *>;

        Poller(EventLoop *loop);        //所有的子类在初始化时都要调用父类的构造函数
        virtual ~Poller() = default;    //vitrual 虚函数，让父类指针调用子类的函数实现

        // 给所有IO复用保留统一的接口
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;

        // 判断参数channel是否在当前的Poller当中
        bool hasChannel(Channel *channel) const;

        // EventLoop可以通过该接口获取默认的IO复用的具体实现
        static Poller *newDefaultPoller(EventLoop *loop);

    protected:
        // map的key:sockfd value:sockfd所属的channel通道类型
        using ChannelMap = std::unordered_map<int, Channel *>;
        ChannelMap channels_;

    private:
        EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};