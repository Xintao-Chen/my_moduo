#pragma once 

class noncopyable {
    public: //外部可以
        noncopyable(const noncopyable &) = delete;  //没有复制构造函数
        noncopyable& operator=(const noncopyable &) = delete; //禁止赋值操作
    protected: //子类可用，外部不行
        noncopyable() = default;
        ~noncopyable() = default;
    private: //同类可用，子类不行，外部不行
};