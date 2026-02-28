#include <cxxui/win.hpp>

class EventWindow : public cxxui::Window<EventWindow> {
public:
    using Window::Window;
    /**
     * 1. 处理win32消息, 手动处理win32消息会失去跨平台性
     */
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM) {
        switch (msg) {
            // 设置背景颜色为红色
            case WM_ERASEBKGND: {
                HDC hdc = (HDC)wp;
                RECT rc;
                GetClientRect(hwnd_, &rc);
                HBRUSH brush = CreateSolidBrush(RGB(255, 150, 150));
                FillRect(hdc, &rc, brush);
                DeleteObject(brush);
                return true;
            }
            default:
                break;
        }
        return std::nullopt;
    }
    /**
     * 2. 窗口创建完成的事件
     */
    void OnCreated() {
        fprintf(stderr, "CreatedEvent\n");
        cxxui::Window<EventWindow>::OnCreated();
    }
    /**
     * 3. 窗口大小发生变化的事件
     */
    void OnSize(const cxxui::SizeEvent& event) {
        fprintf(stderr, "SizeEvent: [%d, %d]\n", event.GetWidth(), event.GetHeight());
        cxxui::Window<EventWindow>::OnSize(event);
    }
    /**
     * 4. 窗口激活或失去激活触发的事件
     */
    void OnActivate(const cxxui::ActivateEvent& event) {
        fprintf(stderr, "ActivateEvent: %d\n", event.IsActive());
        cxxui::Window<EventWindow>::OnActivate(event);
    }
};
;

int main() {
    /**
     * 1. DPI不为100%时按DPI比例缩放窗口
     * 2. 窗口在鼠标所在屏幕居中显示
     */
    cxxui::Window<> main_win{cxxui::WindowOptions()
                                 .SetTitle("窗口显示在鼠标所在屏幕的居中位置")
                                 .SetWidth(400)
                                 .SetHeight(400)};
    main_win.Show();
    /**
     * 1. Win32Window 继承 cxxui::Window 处理事件
     */
    EventWindow event_win{cxxui::WindowOptions()
                              .SetTitle("处理事件的窗口")
                              .SetWidth(400)
                              .SetHeight(400)
                              .SetScale(false)  // 固定窗口大小为用户设置的值, 不缩放
                              .SetX(100)
                              .SetY(100)};
    event_win.Show();
    try {
        event_win.SetTitleColor({255, 150, 150});
    } catch (const std::exception& e) {
        fprintf(stderr, "SetTitleColor error: %s\n", e.what());
    }
    /**
     * 1. 焦点在主窗口
     */
    main_win.Focus();
    /**
     * 1. 调用Run()的窗口为主窗口, 主窗口关闭整个进程关闭
     */
    return main_win.Run();
}