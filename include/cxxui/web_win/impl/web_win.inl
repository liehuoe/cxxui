#include <cxxui/win.hpp>
#include <cxxui/web_win/req_ctx.hpp>
#include "./web_ctrl.inl"

namespace cxxui::detail {

class DefaultWebWindow;

template <typename Derived>
class WebWindowBase : public Window<Derived> {
    using Base = Window<Derived>;

protected:
    std::optional<WindowError> WaitWebCreated() const noexcept {
        if (!this->hwnd_) {
            return WindowError(ERROR_INVALID_HANDLE, "Window is not created!");
        }
        MSG msg;
        while (!this->ctrl_) {
            if (!PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                continue;
            }
            if (msg.message == WM_QUIT) {
                PostQuitMessage(static_cast<int>(msg.wParam));
                return WindowError(ERROR_NOT_READY, "WM_QUIT received!");
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message != UM_WEB_CREATED || msg.hwnd != this->hwnd_) {
                continue;
            }
            if (FAILED(msg.wParam)) {
                return WindowError(static_cast<long>(msg.wParam), "CreateWebView failed!");
            }
        }
        return std::nullopt;
    }
    void SetHtml(std::string_view html) {
        HRESULT hr = GetWebView()->NavigateToString(U82W(html).c_str());
        if (FAILED(hr)) {
            throw WindowError(hr, "NavigateToString failed!");
        }
    }
    void SetUrl(std::string_view url) {
        HRESULT hr = GetWebView()->Navigate(U82W(url).c_str());
        if (FAILED(hr)) {
            throw WindowError(hr, "Navigate failed!");
        }
    }
    void SetBackground(const Color& color) {
        ComPtr<ICoreWebView2Controller2> ctrl2;
        HRESULT hr = MSWebView2As(ctrl_.GetCtrl(), ctrl2);
        if (FAILED(hr)) {
            throw WindowError(hr, "As ICoreWebView2Controller2 failed!");
        }
        hr = ctrl2->put_DefaultBackgroundColor({color.alpha, color.red, color.green, color.blue});
        if (FAILED(hr)) {
            throw WindowError(hr, "put_DefaultBackgroundColor failed!");
        }
    }
    void OnJsMsg(std::function<std::string(std::string)> handler) {
        HRESULT hr = GetWebView()->add_WebMessageReceived(
            MSWebView2Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [handler = std::move(handler)](
                    ICoreWebView2* sender,
                    ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                    LPWSTR msg;
                    HRESULT hr_msg = args->get_WebMessageAsJson(&msg);
                    if (FAILED(hr_msg)) {
                        return hr_msg;
                    }
                    std::string req = W2U8(msg);
                    CoTaskMemFree(msg);
                    std::string resp = handler(std::move(req));
                    sender->PostWebMessageAsJson(U82W(resp).c_str());
                    return S_OK;
                })
                .Get(),
            nullptr);
        if (FAILED(hr)) {
            throw WindowError(hr, "add_WebMessageReceived failed!");
        }
    }
    void SendJsMsg(std::string_view msg) {
        HRESULT hr = GetWebView()->PostWebMessageAsJson(U82W(msg).c_str());
        if (FAILED(hr)) {
            throw WindowError(hr, "PostWebMessageAsJson failed!");
        }
    }
    void RunJs(std::string_view js_code, bool on_created) {
        if (on_created) {
            GetWebView()->AddScriptToExecuteOnDocumentCreated(U82W(js_code).c_str(), nullptr);
        } else {
            GetWebView()->ExecuteScript(U82W(js_code).c_str(), nullptr);
        }
    }
    /**
     * webview 官方不支持设置焦点到 webview 窗口
     * 只能自己找到相应的子窗口进行操作
     */
    void Focus() const {
        HWND found_hwnd = 0;
        EnumChildWindows(
            this->hwnd_,
            [](HWND child_hwnd, LPARAM lp) -> BOOL {
                wchar_t name[256] = {0};
                GetClassNameW(child_hwnd, name, ARRAYSIZE(name));
                if (wcscmp(name, L"Chrome_WidgetWin_0") == 0) {
                    *reinterpret_cast<HWND*>(lp) = child_hwnd;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&found_hwnd));
        if (!found_hwnd) {
            throw WindowError(E_FAIL, "Get webview hwnd failed!");
        }
        SetFocus(found_hwnd);
    }
    void OnWebRequest(std::function<void(WebRequest& ctx)> handler, std::string_view filter) {
        ComPtr<ICoreWebView2> webview = GetWebView();

        ComPtr<ICoreWebView2_22> webview22;
        HRESULT hr = MSWebView2As(webview, webview22);
        if (SUCCEEDED(hr)) {
            hr = webview22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                detail::U82W(filter).c_str(),
                COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
        } else {
            // 退化到旧版本
            hr = webview->AddWebResourceRequestedFilter(detail::U82W(filter).c_str(),
                                                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        }
        if (FAILED(hr)) {
            throw WindowError(hr, "AddWebResourceRequestedFilter failed!");
        }
        hr = webview->add_WebResourceRequested(
            MSWebView2Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [handler = std::move(handler)](
                    ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                    WebRequest ctx{args, WebCtrl::GetEnv()};
                    handler(ctx);
                    return S_OK;
                })
                .Get(),
            nullptr);
        if (FAILED(hr)) {
            throw WindowError(hr, "add_WebResourceRequested failed!");
        }
    }

protected:
    WebCtrl ctrl_;
    ComPtr<ICoreWebView2> GetWebView() const {
        ComPtr<ICoreWebView2> webview;
        HRESULT hr = ctrl_->get_CoreWebView2(&webview);
        if (FAILED(hr)) {
            throw WindowError(hr, "get_CoreWebView2 failed!");
        }
        return webview;
    }
    void InitSetting() {
        ComPtr<ICoreWebView2> webview;
        if (FAILED(ctrl_->get_CoreWebView2(&webview))) {
            return;
        }
        // 统一web端收发消息接口
        webview->AddScriptToExecuteOnDocumentCreated(
            LR"JS(
window.SendCppMsg = window.chrome.webview.postMessage;
window.SetCppMsgHandler = function (handler) {
    window.chrome.webview.addEventListener('message', (e) => handler(e.data));
};
)JS",
            nullptr);

        ComPtr<ICoreWebView2Settings> settings;
        if (FAILED(webview->get_Settings(&settings))) {
            return;
        }
        ComPtr<ICoreWebView2Settings9> settings9;
        if (FAILED(MSWebView2As(settings, settings9))) {
            return;
        }
        // 让 js 可以自行控制拖拽区域
        settings9->put_IsNonClientRegionSupportEnabled(true);
        // 发布版本禁用浏览器的各种快捷键
#ifndef _DEBUG
        settings9->put_AreBrowserAcceleratorKeysEnabled(false);
#endif
    }

protected:
    CXXUI_WIN_EVENT(Derived)
    void OnSize(SizeEvent& event) {
        if (ctrl_) {
            ctrl_->put_Bounds({0, 0, event.GetWidth(), event.GetHeight()});
        }
        Window<Derived>::OnSize(event);
    }
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CREATE: {
                ctrl_.Create(this->hwnd_);
                break;
            }
            case UM_WEB_CREATED: {
                if (FAILED(wp)) {
                    WindowError err{static_cast<long>(wp), "CreateWebView failed!"};
                    static_cast<Derived*>(this)->OnWebCreated(err);
                    break;
                }
                ctrl_.SetCtrl(reinterpret_cast<ICoreWebView2Controller*>(lp));
                RECT rc;
                GetClientRect(this->hwnd_, &rc);
                ctrl_->put_Bounds(rc);       // 适应父窗口大小
                ctrl_->put_IsVisible(true);  // webview默认可见, 隐藏操作由父窗口控制
                Focus();                     // 创建 webview 后默认获取焦点, 跟其他控件/窗口对齐
                InitSetting();               // 其他的默认设置
                static_cast<Derived*>(this)->OnWebCreated(std::nullopt);
                break;
            }
        }
        return Window<Derived>::OnWin32Msg(msg, wp, lp);
    }
};

}  // namespace cxxui::detail
