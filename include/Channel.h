#pragma once  //防止头文件被重复包含

#include <functional>
#include <memory>

#include "noncopyable.h"

// 前向声明EventLoop类, 如果没有创建对象，访问成员， 就不需要完整定义，只需声明告诉编译器这个类存在。
class EventLoop;  
class Timestamp;

class Channel : noncopyable{
    public:
        using EventCallback = std::function<void()>; 
        using ReadEventCallback = std::function<void(Timestamp)>;

        Channel(EventLoop *loop, int fd);
        ~Channel();

        // fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用
        void handleEvent(Timestamp receiveTime);

        // 写在类里，说不定可以内联，编译器可能直接调用的时候直接嵌入代码而不是调用函数，更快
        // Note: std::move 是一个static_cast<T&&> 让这个东西变成能被搬走资源
        void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
        void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); } 
        void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
        void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

        // 设置fd相应的事件状态 相当于epoll_ctl add delete
        void enableReading() { events_ |= kReadEvent; update(); }
        void disableReading() { events_ &= ~kReadEvent; update(); }
        void enableWriting() { events_ |= kWriteEvent; update(); }
        void disableWriting() { events_ &= ~kWriteEvent; update(); }
        void disableAll() { events_ = kNoneEvent; update(); }

        // 返回fd当前的事件状态
        bool isNoneEvent() const { return events_ == kNoneEvent; }
        bool isWriting() const { return events_ & kWriteEvent; }
        bool isReading() const { return events_ & kReadEvent; }

        // 防止当channel被手动remove掉 channel还在执行回调操作
        void tie(const std::shared_ptr<void> &);

        int fd() const { return fd_; }
        int events() const { return events_; }

        // 外部将实际发生的时间封装进channel里
        int set_revents(int revt) {revents_ = revt; }

        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        // one loop per thread
        EventLoop *ownerLoop() { return loop_; }
        void remove();

    private:

        void update();      //通知pollor (epoll的封装) 这个channel发生了什么变化
        void handleEventWithGuard(Timestamp receiveTime); 

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        const int fd_;      // fd，Poller监听的对象
        int events_;        // 注册fd感兴趣的事件
        int revents_;       // Poller返回的具体发生的事件
        EventLoop* loop_;    // 事件循环

        int index_; // 不明白干什么的

        std::weak_ptr<void> tie_;  // TcpConnection 的指针，检查这个channel是否还活着（连接关闭没有）
        bool tied_;     // 有tie_, 就说明还连接着。


        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
};
