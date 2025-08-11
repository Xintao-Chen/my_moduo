#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stddef.h>

class Buffer
{
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kinitalSize = 1024;

        explicit Buffer(size_t initalSize = kinitalSize)
            : buffer_(kCheapPrepend + initalSize)
            , readerIndex_(kCheapPrepend)
            , writerIndex_(kCheapPrepend)
            {}

        size_t readableBytes() const { return writerIndex_ - readerIndex_; }
        size_t writableBytes() const { return buffer_.size() - writerIndex_; }
        size_t prependableBytes() const { return readerIndex_; }

        // 返回buffer中可读数据的起始地址
        const char* peek() const { return begin() + readerIndex_; } 

        // 标记已取回数据
        void retrieve(size_t len) {
            if (len < readableBytes()) {
                readerIndex_ += len;
            } else {
                retrieveAll();
            }
        } 

        void retrieveAll() {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }

        // 把onMessage函数上报的Buffer数据 转成string类型的数据返回
        std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
        std::string retrieveAsString(size_t len)
        {
            std::string result(peek(), len);   //从const char* 地址开始，len长度构造string
            retrieve(len); // 上面一句把缓冲区中可读的数据已经读取出来 这里肯定要对缓冲区进行复位操作
            return result;
        }

        // buffer_.size - writerIndex_  当打算写入的时候先检查是否有足够的空间可以写
        void ensureWritableBytes(size_t len) {
            if (len > writableBytes()) {
                makeSpace(len); //扩容
            }
        }

        // 把[data, data+len]内存上的数据添加到writable缓冲区当中, 然后变成可读
        void append(const char* data, size_t len) {
            ensureWritableBytes(len);
            std::copy(data, data+len, beginWrite());
            writerIndex_ += len;
        }

        char* benginWrite() { return begin() + writerIndex_; }
        const char* beginWrite() const { return begin() + writerIndex_; }

        ssize_t readFd(int fd, int *saveErrno);
        ssize_t writeFd(int fd, int *saveErrno);
    
    private:
        char* begin() {return buffer_.data() ; }    
        const char* begin() const { return buffer_.data(); }   

        void makeSpace(size_t len) {
            /**
             * | kCheapPrepend |xxx| reader | writer |                     // xxx标示reader中已读的部分
             * | kCheapPrepend | reader ｜          len          |
             **/
            if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
                buffer_.resize(writerIndex_ + len);
            } else {   //把reader搬到从xxx开始，让xxxwriter连续
                size_t readable = readableBytes();
                std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;
            }
        }

        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;
};