#pragma once
#include "impl/event.inl"

namespace cxxui {

/**
 * @brief 窗口大小发生变化的事件
 * @details 子类回调函数定义：void OnSize(const cxxui::SizeEvent&);
 */
class SizeEvent : public detail::SizeEventBase {
public:
    /**
     * @brief 获取窗口新大小的宽度
     */
    int GetWidth() const { return SizeEventBase::GetWidth(); }
    /**
     * @brief 获取窗口新大小的高度
     */
    int GetHeight() const { return SizeEventBase::GetHeight(); }
};

/**
 * @brief 窗口激活或失去激活触发的事件
 * @details 子类回调函数定义：void OnActivate(const cxxui::ActivateEvent&);
 */
class ActivateEvent : public detail::ActivateEventBase {
public:
    /**
     * @brief 获取窗口当前的激活状态
     */
    bool IsActive() const { return ActivateEventBase::IsActive(); }
};

/**
 * @brief 修改系统设置触发的事件
 * @details 子类回调函数定义：void OnSetting(const cxxui::SettingEvent&);
 */
class SettingEvent : public detail::SettingEventBase {
public:
    /**
     * @brief 是否颜色主题发生变化
     */
    bool IsColorThemeChanged() const { return SettingEventBase::IsColorThemeChanged(); }
};

}  // namespace cxxui
