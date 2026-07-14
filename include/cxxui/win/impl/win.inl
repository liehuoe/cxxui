#include <typeindex>
#include <optional>
#include <array>

#include <windows.h>
#include <dwmapi.h>
#ifdef _MSC_VER
    #pragma comment(lib, "user32.lib")  // CreateWindow
    #pragma comment(lib, "dwmapi.lib")  // DwmSetWindowAttribute
#endif
/** 发布版本不显示控制台窗口 */
#if defined(_MSC_VER) && !defined(_DEBUG)
    #pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

#include <cxxui/win/error.hpp>
#include <cxxui/core/color.hpp>
#include <cxxui/win/event.hpp>
#include <cxxui/win/options.hpp>
#include <cxxui/core/detail/string_coder.hpp>
#include "detail/user32.hpp"

/** 定义窗口类名称, 用户可以定义该宏定义以覆盖默认值 */
#ifndef CXXUI_WIN32_CLASS_NAME
    #define CXXUI_WIN32_CLASS_NAME L"cxxui_window"
#endif

namespace cxxui::detail {

class DefaultWindow;

void Exit(int exit_code = 0) noexcept { PostQuitMessage(exit_code); }
bool IsDarkMode() noexcept {
    bool result = false;
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0,
                      KEY_READ,
                      &hkey) != ERROR_SUCCESS) {
        return result;
    }
    DWORD type, val, size = sizeof(DWORD);
    if (RegQueryValueExW(
            hkey, L"AppsUseLightTheme", NULL, &type, reinterpret_cast<LPBYTE>(&val), &size) ==
        ERROR_SUCCESS) {
        result = (val == 0);  // 0 = Dark, 1 = Light
    }
    RegCloseKey(hkey);
    return result;
}

class WinFactory {
public:
    /** 获取单例实例 */
    static WinFactory& GetInstance() {
        static WinFactory instance;
        return instance;
    }
    /** 注册窗口类 */
    void RegClass(LPCWSTR class_name, WNDPROC wnd_proc) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;                             // 窗口水平、垂直重绘
        wc.lpfnWndProc = wnd_proc;                                      // 指定窗口过程函数
        wc.hInstance = GetModuleHandle(nullptr);                        // 应用程序实例句柄
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);                    // 使用系统默认的箭头光标
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);  // 默认背景颜色
        wc.lpszClassName = class_name;                                  // 窗口类名
        RegisterClassExW(&wc);
    }
    /** 设置主函数窗口句柄 */
    void SetMainWindow(HWND hwnd) { main_hwnd_ = hwnd; }
    HWND GetMainWindow() const { return main_hwnd_; }

private:
    WinFactory() {
        // 设置DPI感知
        User32{}.SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

private:
    HWND main_hwnd_ = nullptr;
};

template <typename Derived>
class WindowBase {
public:
    ~WindowBase() {
        if (hwnd_) {
            DestroyWindow(hwnd_);
        }
    }
    /**
     * @brief WIN32平台获取该窗口的窗口类名称
     *
     * @return std::array<wchar_t, N>
     */
    static constexpr auto GetWin32ClassName() noexcept {
        constexpr auto prefix_size = std::char_traits<wchar_t>::length(CXXUI_WIN32_CLASS_NAME);
        std::array<wchar_t, prefix_size + 16 + 1> hex{};
        hex[hex.size() - 1] = L'\0';
        auto hash = std::type_index(typeid(WindowBase<Derived>)).hash_code();
        for (int i = static_cast<int>(hex.size() - 2); i >= 0; --i) {
            int nibble = static_cast<int>(hash & 0xF);
            hex[i] = static_cast<wchar_t>(nibble < 10 ? L'0' + nibble : L'A' + nibble - 10);
            hash >>= 4;
        }
        for (std::size_t i = 0; i < prefix_size; ++i) {
            hex[i] = CXXUI_WIN32_CLASS_NAME[i];
        }
        return hex;
    }

protected:
    int Run() noexcept {
        detail::WinFactory::GetInstance().SetMainWindow(hwnd_);  // 设置主窗口
        // 消息循环
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        detail::WinFactory::GetInstance().SetMainWindow(nullptr);  // 置空，防止重复发送WM_QUIT
        return static_cast<int>(msg.wParam);
    }
    void Exit(int exit_code) noexcept { PostQuitMessage(exit_code); }
    void Create(WindowOptionsBase& opts) {
        if (hwnd_) {
            throw WindowError(ERROR_ALREADY_EXISTS, "Window already exists!");
        }
        auto class_name = GetWin32ClassName();
        detail::WinFactory::GetInstance().RegClass(class_name.data(), OnWndProc);
        opts.ScaleRect();
        CreateWindowExW(opts.ex_style_,
                        class_name.data(),                  // 窗口类名
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
    void Show(bool show) {
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
    Handle GetHandle() const noexcept { return hwnd_; }

protected:
    /**
     * @brief WIN32的窗口句柄
     */
    HWND hwnd_ = 0;
    /**
     * @brief 子类接收win32消息的事件
     */
    std::optional<LRESULT> OnWin32Msg(UINT, WPARAM, LPARAM) { return std::nullopt; }

private:
    static LRESULT CALLBACK OnWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        WindowBase<Derived>* self;
        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lp);
            self = reinterpret_cast<WindowBase<Derived>*>(pcs->lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<WindowBase<Derived>*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!self) {
                return DefWindowProcW(hwnd, msg, wp, lp);
            }
        }

        switch (msg) {
            case WM_CREATE: {
                static_cast<Derived*>(self)->OnCreated();
                break;
            }
            case WM_CLOSE: {
                ClosingEvent event;
                static_cast<Derived*>(self)->OnClosing(event);
                if (!event.close_) {
                    return 0;
                }
                break;
            }
            case WM_DESTROY: {
                // 如果在 OnClosed 中触发析构, 会导致 DestroyWindow 再触发一次 WM_DESTROY
                // 设置 GWLP_USERDATA 为空解决问题
                SetWindowLongPtr(self->hwnd_, GWLP_USERDATA, 0);
                static_cast<Derived*>(self)->OnClosed();
                // 如果是主窗口, 则退出进程
                if (self->hwnd_ == WinFactory::GetInstance().GetMainWindow()) {
                    WinFactory::GetInstance().SetMainWindow(nullptr);
                    PostQuitMessage(0);
                }
                break;
            }
            case WM_SIZE: {
                SizeEvent event;
                event.lp_ = lp;
                static_cast<Derived*>(self)->OnSize(event);
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
            case WM_ACTIVATE: {
                ActivateEvent event;
                event.wp_ = wp;
                static_cast<Derived*>(self)->OnActivate(event);
                break;
            }
            case WM_SETTINGCHANGE: {
                if (!lp) {
                    break;
                }
                SettingEvent event;
                event.name_ = reinterpret_cast<LPWSTR>(lp);
                static_cast<Derived*>(self)->OnSetting(event);
                break;
            }
            default:
                break;
        }
        auto res = static_cast<Derived*>(self)->OnWin32Msg(msg, wp, lp);
        return res ? res.value() : DefWindowProcW(hwnd, msg, wp, lp);
    }
};

}  // namespace cxxui::detail
