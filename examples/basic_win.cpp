#include <cxxui/win.hpp>

class EventWindow : public cxxui::Window<EventWindow> {
public:
    using Window::Window;

private:
    /**
     * 友元声明，让基类能调用private或protected中的事件回调函数
     */
    CXXUI_WIN_EVENT(EventWindow)
    /**
     * 处理win32消息, 手动处理win32消息会失去跨平台性
     */
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            // 设置背景颜色为红色
            case WM_ERASEBKGND: {
                HDC hdc = reinterpret_cast<HDC>(wp);
                RECT rc;
                GetClientRect(hwnd_, &rc);
                static HBRUSH brush = CreateSolidBrush(RGB(255, 150, 150));
                FillRect(hdc, &rc, brush);
                return TRUE;
            }
            default:
                break;
        }
        return cxxui::Window<EventWindow>::OnWin32Msg(msg, wp, lp);
    }
    /**
     * 窗口创建完成的事件
     */
    void OnCreated() {
        fprintf(stderr, "CreatedEvent\n");
        cxxui::Window<EventWindow>::OnCreated();
    }
    /**
     * 用户关闭窗口时触发的事件
     */
    void OnClosing(cxxui::ClosingEvent& event) {
        static int count = 0;
        ++count;
        fprintf(stderr, "ClosingEvent, count: %d\n", count);
        if (count < 2) {
            event.PreventClose();
        }
        cxxui::Window<EventWindow>::OnClosing(event);
    }
    /**
     * 窗口被关闭时触发的事件
     */
    void OnClosed() {
        fprintf(stderr, "ClosedEvent\n");
        cxxui::Window<EventWindow>::OnClosed();
    }
    /**
     * 窗口大小发生变化的事件
     */
    void OnSize(cxxui::SizeEvent& event) {
        fprintf(stderr, "SizeEvent: [%d, %d]\n", event.GetWidth(), event.GetHeight());
        cxxui::Window<EventWindow>::OnSize(event);
    }
    /**
     * 窗口激活或失去激活触发的事件
     */
    void OnActivate(cxxui::ActivateEvent& event) {
        fprintf(stderr, "ActivateEvent: %d\n", event.IsActive());
        cxxui::Window<EventWindow>::OnActivate(event);
    }
    /**
     * 修改系统设置触发的事件
     */
    void OnSetting(cxxui::SettingEvent& event) {
        const char* fmt_str = R"(SettingEvent:
    ColorThemeChanged: %d
)";
        fprintf(stderr, fmt_str, event.IsColorThemeChanged());
        cxxui::Window<EventWindow>::OnSetting(event);
    }
};

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            fprintf(stderr, "MyWndProc: WM_CREATE\n");
            break;
        default:
            break;
    }
    // 调用WinFactory的WndProc, 保证 GUI 库的正常运行
    return cxxui::detail::WinFactory::WndProc(hwnd, msg, wp, lp);
}

int main() {
    /**
     * 对于 Win32, 用户可以自定义窗口过程函数
     */
    cxxui::detail::WinFactory::Init(MyWndProc);
    /**
     * 判断系统是否正在使用深色模式
     */
    fprintf(stderr, "IsDarkMode: %d\n", cxxui::IsDarkMode());
    /**
     * DPI不为100%时按DPI比例缩放窗口
     * 窗口在鼠标所在屏幕居中显示
     */
    cxxui::Window<> main_win{cxxui::WindowOptions{}
                                 .SetTitle("窗口显示在鼠标所在屏幕的居中位置")
                                 .SetWidth(400)
                                 .SetHeight(400)};
    try {
        main_win.SetTitleColor({255, 150, 150});
    } catch (const std::exception& e) {
        fprintf(stderr, "SetTitleColor error: %s\n", e.what());
    }
    main_win.Show();
    /**
     * Win32Window 继承 cxxui::Window 处理事件
     */
    EventWindow event_win{cxxui::WindowOptions{}
                              .SetTitle("处理事件的窗口")
                              .SetWidth(400)
                              .SetHeight(400)
                              .SetScale(false)  // 固定窗口大小为用户设置的值, 不缩放
                              .SetX(100)
                              .SetY(100)};
    event_win.Show();
    /**
     * 焦点在主窗口
     */
    main_win.Focus();
    /**
     * 调用Run()的窗口为主窗口, 主窗口关闭整个进程关闭
     */
    return main_win.Run();
}