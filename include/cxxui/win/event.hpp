#pragma once
#include "impl/event.inl"

namespace cxxui {

/**
 * @brief 窗口大小发生变化的事件
 */
class SizeEvent : public detail::SizeEventBase {
public:
    /**
     * @brief 获取窗口新大小的宽度
     * @details 子类回调函数定义：void OnSize(const cxxui::SizeEvent&);
     */
    int GetWidth() const { return SizeEventBase::GetWidth(); }
    /**
     * @brief 获取窗口新大小的高度
     */
    int GetHeight() const { return SizeEventBase::GetHeight(); }
};

}  // namespace cxxui
