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

template <typename T>
constexpr uint64_t GetHash() noexcept {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
#else
    #error "Unsupported compiler"
#endif
    uint64_t hash = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;
    for (unsigned char c : sig) {
        hash ^= static_cast<uint64_t>(c);
        hash *= prime;
    }
    return hash;
}

template <typename Derived>
class WindowBase {
public:
    ~WindowBase() {
        if (hwnd_) {
            HWND hwnd = hwnd_;
            hwnd_ = nullptr;
            DestroyWindow(hwnd);
        }
    }
    /**
     * @brief WIN32平台获取该窗口的窗口类名称, 开发者可以覆盖该函数以自定义窗口类名称
     *
     * @return std::wstring
     */
    static std::wstring GetWin32ClassName() noexcept {
        constexpr auto kHash = GetHash<Derived>();
        constexpr auto kSize = sizeof(kHash) * 2;

        std::array<wchar_t, kSize + 1> hex;
        auto hash = kHash;
        for (std::size_t i = 0; i < kSize; ++i) {
            int h = static_cast<int>(hash & 0xF);
            hex[kSize - i - 1] = static_cast<wchar_t>(h < 10 ? L'0' + h : L'A' + h - 10);
            hash >>= 4;
        }
        hex[hex.size() - 1] = L'\0';

        return std::wstring{CXXUI_WIN32_CLASS_NAME} + hex.data();
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
    template <typename T>
    void Create(WindowOptionsBase<T>& opts) {
        if (hwnd_) {
            throw WindowError(ERROR_ALREADY_EXISTS, "Window already exists!");
        }
        auto class_name = Derived::GetWin32ClassName();
        detail::WinFactory::GetInstance().RegClass(class_name.data(), OnWndProc);
        opts.ScaleRect();
        CreateWindowExW(opts.ex_style_,
                        class_name.data(),                 // 窗口类名
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
    void Close() const noexcept { SendMessage(hwnd_, WM_CLOSE, 0, 0); }
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
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                if (self->hwnd_) {
                    // self->hwnd_ 如果为空，代表子类正在析构，则不触发 OnClosed
                    static_cast<Derived*>(self)->OnClosed();
                    self->hwnd_ = nullptr;
                }
                // 如果是主窗口, 则退出进程
                if (hwnd == WinFactory::GetInstance().GetMainWindow()) {
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
