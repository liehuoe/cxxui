#pragma once
#include <exception>

namespace cxxui {

/**
 * @brief 窗口的异常定义
 *
 */
class WindowError : public std::exception {
public:
    WindowError(long code, const char* msg)
        : code_(code),
          std::exception(msg) {}
    long GetErrorCode() { return code_; }

private:
    long code_;
};

}  // namespace cxxui
