#include <optional>

#include <dwmapi.h>
#ifdef _MSC_VER
    #pragma comment(lib, "user32.lib")  // CreateWindow
    #pragma comment(lib, "dwmapi.lib")  // DwmSetWindowAttribute
#endif
/** 发布版本不显示控制台窗口 */
#if defined(_MSC_VER) && !defined(_DEBUG)
    #pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

#include <cxxui/core/detail/string_coder.hpp>
#include "detail/user32.hpp"

/** 定义窗口类名称, 用户可以定义该宏定义以覆盖默认值 */
#ifndef CXXUI_WIN32_CLASS_NAME
    #define CXXUI_WIN32_CLASS_NAME L"cxxui_window"
#endif

namespace cxxui::detail {

class DefaultWindow;

void Exit(int exit_code = 0) noexcept { PostQuitMessage(exit_code); }

class WndProcBase {
    friend class WinFactory;

public:
    virtual ~WndProcBase() = default;

protected:
    HWND hwnd_ = 0;
    virtual std::optional<LRESULT> OnWndProc(UINT msg, WPARAM wp, LPARAM lp) = 0;
};

class WinFactory {
public:
    /**
     * @brief 窗口初始化函数, 整个进程运行过程只初始化一次
     *
     * @return bool 初始化成功返回true, 失败返回false
     */
    static bool Init() {
        if (main_hwnd_ != 0) {
            return true;
        } else {
            main_hwnd_ = reinterpret_cast<HWND>(-1);
        }
        // 设置DPI感知
        User32{}.SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        // 注册窗口类
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;                             // 窗口水平、垂直重绘
        wc.lpfnWndProc = WndProc;                                       // 指定窗口过程函数
        wc.hInstance = GetModuleHandle(nullptr);                        // 应用程序实例句柄
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);                    // 使用系统默认的箭头光标
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);  // 默认背景颜色
        wc.lpszClassName = CXXUI_WIN32_CLASS_NAME;                      // 窗口类名
        return RegisterClassExW(&wc);
    }
    static void SetMainWindow(HWND hwnd) { main_hwnd_ = hwnd; }

private:
    inline static HWND main_hwnd_ = 0;
    /** 消息处理函数 */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        WndProcBase* win;
        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lp);
            win = reinterpret_cast<WndProcBase*>(pcs->lpCreateParams);
            win->hwnd_ = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
        } else {
            win = reinterpret_cast<WndProcBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!win) {
                return DefWindowProcW(hwnd, msg, wp, lp);
            }
        }
        std::optional<LRESULT> result = win->OnWndProc(msg, wp, lp);
        switch (msg) {
            case WM_DESTROY: {
                // 如果是主窗口, 则退出进程
                if (win->hwnd_ == main_hwnd_) {
                    PostQuitMessage(0);
                }
                win->hwnd_ = 0;
                break;
            }
            case WM_DPICHANGED: {
                RECT* rc = reinterpret_cast<RECT*>(lp);
                SetWindowPos(hwnd,
                             nullptr,
                             rc->left,
                             rc->top,
                             rc->right - rc->left,
                             rc->bottom - rc->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                break;
            }
        }
        return result ? result.value() : DefWindowProcW(hwnd, msg, wp, lp);
    }
};

template <typename Derived>
class WindowBase : public WndProcBase {
public:
    virtual ~WindowBase() {
        if (hwnd_) {
            DestroyWindow(hwnd_);
        }
    }

protected:
    int Run() noexcept {
        // 设置主窗口
        detail::WinFactory::SetMainWindow(hwnd_);
        // 消息循环
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return static_cast<int>(msg.wParam);
    }
    void Exit(int exit_code) noexcept { PostQuitMessage(exit_code); }
    void Create(WindowOptionsBase& opts) {
        if (hwnd_) {
            throw WindowError(ERROR_ALREADY_EXISTS, "Window already exists!");
        }
        detail::WinFactory::Init();
        opts.ScaleRect();
        CreateWindowExW(opts.ex_style_,
                        CXXUI_WIN32_CLASS_NAME,             // 窗口类名
                        detail::U82W(opts.title_).c_str(),  // 窗口标题
                        opts.style_,                        // 窗口样式
                        opts.x_,                            // 窗口 x 坐标
                        opts.y_,                            // 窗口 y 坐标
                        opts.width_,                        // 窗口宽度
                        opts.height_,                       // 窗口高度
                        opts.parent_,                       // 父窗口句柄 (nullptr 表示没有父窗口)
                        nullptr,                            // 菜单句柄 (nullptr 表示没有菜单)
                        GetModuleHandle(nullptr),           // 窗口实例句柄
                        this                                // 传递给 WM_CREATE 的参数
        );
        if (!hwnd_) {
            throw WindowError(GetLastError(), "CreateWindowEx failed!");
        }
    }
    void Show(bool show) const {
        if (!hwnd_) {
            throw WindowError(ERROR_INVALID_HANDLE, "Window is not created!");
        }
        if (show) {
            ShowWindow(hwnd_, SW_SHOW);
            UpdateWindow(hwnd_);
        } else {
            ShowWindow(hwnd_, SW_HIDE);
        }
    }
    void Close() const noexcept { DestroyWindow(hwnd_); }
    void Focus() const { SetFocus(hwnd_); }
    void SetTitle(std::string_view title) { SetWindowTextW(hwnd_, detail::U82W(title).c_str()); }
    void SetTitleColor(const Color& color) {
        COLORREF rgb = RGB(color.red, color.green, color.blue);
        HRESULT hr = DwmSetWindowAttribute(hwnd_, DWMWA_CAPTION_COLOR, &rgb, sizeof(rgb));
        if (FAILED(hr)) {
            throw WindowError(hr, "DwmSetWindowAttribute failed!");
        }
    }
    void SetIcon(std::uint32_t icon_id) {
        HICON icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(icon_id));
        LPARAM lp = reinterpret_cast<LPARAM>(icon);
        // 设置大图标（标题栏）
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, lp);
        // 设置小图标（任务栏、Alt+Tab）
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, lp);
    }

protected:
    /**
     * @brief 子类接收win32消息的事件
     */
    std::optional<LRESULT> OnWin32Msg(UINT, WPARAM, LPARAM) { return std::nullopt; }

protected:
    std::optional<LRESULT> OnWndProc(UINT msg, WPARAM wp, LPARAM lp) override final {
        switch (msg) {
            case WM_CREATE: {
                static_cast<Derived*>(this)->OnCreated();
                break;
            }
            case WM_SIZE: {
                SizeEvent event;
                event.lp_ = lp;
                static_cast<Derived*>(this)->OnSize(event);
                break;
            }
            case WM_ACTIVATE: {
                ActivateEvent event;
                event.wp_ = wp;
                static_cast<Derived*>(this)->OnActivate(event);
                break;
            }
            default:
                break;
        }
        return static_cast<Derived*>(this)->OnWin32Msg(msg, wp, lp);
    }
};

}  // namespace cxxui::detail
