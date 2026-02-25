#pragma once
#include <stdexcept>

namespace cxxui {

/**
 * @brief 窗口的异常定义
 *
 */
class WindowError : public std::runtime_error {
public:
    WindowError(long code, const char* msg)
        : std::runtime_error(msg),
          code_(code) {}
    long GetErrorCode() { return code_; }

private:
    long code_;
};

}  // namespace cxxui
