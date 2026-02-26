#include <limits>
#include <string_view>

#include "detail/shcore.hpp"

namespace cxxui::detail {

class WindowOptionsBase {
    template <typename Derived>
    friend class WindowBase;

protected:
    void SetTitle(std::string_view title) { title_ = title; }
    std::string_view GetTitle() const { return title_; }
    void SetWidth(int width) { width_ = width; }
    int GetWidth() const { return width_; }
    void SetHeight(int height) { height_ = height; }
    int GetHeight() const { return height_; }
    void SetX(int x) { x_ = x; }
    int GetX() const { return x_; }
    void SetY(int y) { y_ = y; }
    int GetY() const { return y_; }
    void SetScale(bool scale) { scale_ = scale; }
    bool GetScale() const { return scale_; }

protected:
    /** 窗口标题 */
    std::string_view title_ = "";
    /** 窗口宽度 */
    int width_ = 640;
    /** 窗口高度 */
    int height_ = 480;
    /** 窗口x坐标, 默认显示在鼠标所在屏幕的居中位置 */
    int x_ = (std::numeric_limits<int>::min)();
    /** 窗口x坐标, s默认显示在鼠标所在屏幕的居中位置 */
    int y_ = (std::numeric_limits<int>::min)();
    /** 是否根据显示器的DPI缩放窗口到合适的比例 */
    bool scale_ = true;
    DWORD style_ = WS_OVERLAPPEDWINDOW;
    DWORD ex_style_ = 0;
    HWND parent_ = nullptr;
    /** 获取坐标点所在屏幕的相关信息 */
    void GetPointInfo(POINT pt, RECT& screen, float& dpi) {
        // 获取鼠标所在屏幕
        if (pt.x > 0) {
            --pt.x;
        }
        if (pt.y > 0) {
            --pt.y;
        }
        HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        // 获取屏幕DPI
        dpi = Shcore{}.GetDpiForMonitor(monitor) / 96.0f;
        // 获取屏幕区域
        MONITORINFO mi{};
        mi.cbSize = sizeof(MONITORINFO);
        if (!GetMonitorInfoW(monitor, &mi)) {
            screen = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        } else {
            screen = mi.rcMonitor;
        }
    }
    /** 处理DPI导致的窗口大小变化 */
    void ScaleRect() {
        if (width_ == CW_USEDEFAULT) {
            width_ = 640;
        }
        if (height_ == CW_USEDEFAULT) {
            height_ = 480;
        }
        int center_count = 0;  // 0 - 不居中, 1 - x居中, 2 - y居中, 3 - 全居中
        if (x_ == CW_USEDEFAULT) {
            center_count += 1;
        }
        if (y_ == CW_USEDEFAULT) {
            center_count += 2;
        }
        if (!scale_ && center_count == 0) {
            return;
        }
        // 获取鼠标所在屏幕
        POINT pt;
        GetCursorPos(&pt);
        RECT cur_screen;
        float dpi;
        GetPointInfo(pt, cur_screen, dpi);
        // 显示在屏幕中央
        if (center_count == 3) {
            if (scale_ && dpi != 1.0f) {
                width_ = static_cast<int>(width_ * dpi);
                height_ = static_cast<int>(height_ * dpi);
            }
            x_ = (cur_screen.left + cur_screen.right - width_) / 2;
            y_ = (cur_screen.top + cur_screen.bottom - height_) / 2;
            return;
        }
        // 计算窗口中心位置所在屏幕信息
        if (center_count == 1) {
            x_ = (cur_screen.left + cur_screen.right - width_) / 2;
        } else if (center_count == 2) {
            y_ = (cur_screen.top + cur_screen.bottom - height_) / 2;
        }
        POINT center{x_ + width_ / 2, y_ + height_ / 2};
        RECT screen;
        GetPointInfo(center, screen, dpi);
        if (screen.left != cur_screen.left || screen.top != cur_screen.top) {
            // 窗口不在鼠标所在屏幕, 重新计算中点
            if (center_count == 1) {
                x_ = (screen.left + screen.right - width_) / 2;
            } else if (center_count == 2) {
                y_ = (screen.top + screen.bottom - height_) / 2;
            }
        }
        if (!scale_ || dpi == 1.0f) {
            return;
        }
        int new_width = static_cast<int>(width_ * dpi);
        int new_height = static_cast<int>(height_ * dpi);
        if (center_count == 1) {
            x_ = (screen.left + screen.right - new_width) / 2;
        } else if (center_count == 2) {
            y_ = (screen.top + screen.bottom - new_height) / 2;
        }
        // 检查改变后的中点
        center = {x_ + new_width / 2, y_ + new_height / 2};
        float dpi2;
        GetPointInfo(center, screen, dpi2);
        if (dpi2 == dpi) {
            width_ = new_width;
            height_ = new_height;
            return;
        }
        width_ = static_cast<int>(width_ * dpi2);
        height_ = static_cast<int>(height_ * dpi2);
        if (dpi2 < dpi) {
            x_ = center.x - width_ / 2;
            y_ = center.y - height_ / 2;
        }
    }
};

}  // namespace cxxui::detail