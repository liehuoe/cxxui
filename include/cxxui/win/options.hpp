#pragma once
#include "impl/options.inl"

namespace cxxui {

/**
 * @brief 创建窗口的选项
 *
 */
class WindowOptions : public detail::WindowOptionsBase {
public:
    /**
     * @brief 设置窗口标题
     *
     * @param title 字符串引用, 不复制内存
     * @return WindowOptions&
     */
    WindowOptions& SetTitle(std::string_view title) {
        WindowOptionsBase::SetTitle(title);
        return *this;
    }
    std::string_view GetTitle() const { return WindowOptionsBase::GetTitle(); }
    /**
     * @brief 设置窗口宽度
     *
     * @param width 默认 640px
     * @return WindowOptions&
     */
    WindowOptions& SetWidth(int width) {
        WindowOptionsBase::SetWidth(width);
        return *this;
    }
    int GetWidth() const { return WindowOptionsBase::GetWidth(); }
    /**
     * @brief 设置窗口高度
     *
     * @param height 默认 480px
     * @return WindowOptions&
     */
    WindowOptions& SetHeight(int height) {
        WindowOptionsBase::SetHeight(height);
        return *this;
    }
    int GetHeight() const { return WindowOptionsBase::GetHeight(); }
    /**
     * @brief 设置窗口x坐标
     *
     * @param x 默认显示在鼠标所在屏幕的居中位置
     * @return WindowOptions&
     */
    WindowOptions& SetX(int x) {
        WindowOptionsBase::SetX(x);
        return *this;
    }
    int GetX() const { return WindowOptionsBase::GetX(); }
    /**
     * @brief 设置窗口y坐标
     *
     * @param y 默认显示在鼠标所在屏幕的居中位置
     * @return WindowOptions&
     */
    WindowOptions& SetY(int y) {
        WindowOptionsBase::SetY(y);
        return *this;
    }
    int GetY() const { return WindowOptionsBase::GetY(); }
    /**
     * @brief 是否根据显示器的DPI缩放窗口到合适的比例
     *
     * @param scale 默认自适应DPI
     * @return WindowOptions&
     */
    WindowOptions& SetScale(bool scale) {
        WindowOptionsBase::SetScale(scale);
        return *this;
    }
    bool GetScale() const { return WindowOptionsBase::GetScale(); }
};

}  // namespace cxxui