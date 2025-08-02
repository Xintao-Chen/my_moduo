#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;

class EventLoop : noncopyable{
    public:
        void updateChannel(Channel* channel) {};
        void removeChannel(Channel* channel) {};
};