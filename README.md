# my_moduo

本项目是对 Muduo 网络库核心模块的简化复现，包含以下部分：

- `EPollPoller`：封装 epoll，用于监听 fd 上的 IO 事件。
- `Channel`：对每个 fd 的封装，保存感兴趣的事件类型和回调函数。
- `EventLoop`：事件循环控制器，负责调用 `Poller` 获取事件并分发给 `Channel`。

---

## 🌐 整体流程概览

### 🔁 主事件循环（EventLoop::loop）工作流程：

```text
        主线程或子线程创建 EventLoop
                     │
                     ▼
        ┌──── 事件循环开始: EventLoop::loop() ─────┐
        │                                          │
        │ ① 调用 poller_->poll() 等待事件          │
        │    └── 内部调用 epoll_wait()（可阻塞）   │
        │    └── 返回活跃 fd 的列表（epoll_event） │
        │                                          │
        │ ② EPollPoller 根据 fd 找回对应的 Channel │
        │    └── 使用 channels_ 哈希表查找        │
        │    └── 设置 Channel::revents_            │
        │    └── 加入 activeChannels 列表         │
        │                                          │
        │ ③ EventLoop 遍历 activeChannels         │
        │    └── 调用 channel->handleEvent()      │
        │    └── 执行注册的回调函数（读/写/关闭）   │
        │                                          │
        │ ④ 执行 pendingFunctors_ 中的任务          │
        └──────────────────────────────────────────┘


BUFFER:: 在读写fd时存储信息， 不够了在扩大，  extrabuf来临时存放数据