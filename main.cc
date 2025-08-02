#include "Logger.h"

int main() {
    const char* name = "Xintao";
    int age = 22;

    // 普通日志输出
    LOG_INFO("Welcome %s, age = %d", name, age);

    // 错误信息输出
    LOG_ERROR("Failed to open file: %s", "data.txt");

    // 调试信息（需要定义 MUDEBUG 宏才会输出）
    LOG_DEBUG("This is a debug message for %s", name);

    // 致命错误（程序将在这里 exit(-1)，后面的代码不会执行）
    LOG_FATAL("Fatal error: user %s not authorized", name);

    // 永远不会执行到这里
    LOG_INFO("This message should not be shown");

    return 0;
}
