#include <memory>
#include <functional>
#include <array>
#include <filesystem>

#include <wrl.h>
#include <WebView2.h>

#include <cxxui/win.hpp>
#include "detail/wm_msg.h"

namespace cxxui::detail {

class DefaultWebWindow;

using namespace Microsoft::WRL;

/** 全局管理env, 创建webview */
class WebFactory {
    using WebCallback = std::function<void(HRESULT, ComPtr<ICoreWebView2Controller>)>;

    struct QueueData {
        HWND hwnd;
        WebCallback callback;
    };

public:
    ~WebFactory() { CoUninitialize(); }
    /** 获取单例实例 */
    static WebFactory& GetInstance() {
        static WebFactory instance;
        return instance;
    }
    /** 获取env */
    ComPtr<ICoreWebView2Environment> GetEnv() { return env_; }
    /** 创建webview */
    HRESULT CreateWebView(HWND hwnd, WebCallback callback) {
        if (env_) {
            return env_->CreateCoreWebView2Controller(
                hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd, callback = std::move(callback)](
                        HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT {
                        if (IsWindow(hwnd)) {
                            callback(result, ctrl);
                        }
                        return S_OK;
                    })
                    .Get());
        }
        // 设置 webview2 缓存路径
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        auto udf = std::filesystem::path(exe_path).parent_path() / L".cache";
        // 异步创建 env，创建期间缓存创建任务到queue
        // env 创建完成后在 OnEnvCreated 处理 queue
        queue_ = std::make_unique<std::vector<QueueData>>();
        queue_->emplace_back(QueueData{hwnd, callback});
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            udf.c_str(),
            nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(OnEnvCreated)
                .Get());
        if (FAILED(hr)) {
            queue_.reset();
            return hr;
        }
        return S_OK;
    }

private:
    WebFactory(WebFactory const&) = delete;
    WebFactory& operator=(WebFactory const&) = delete;
    WebFactory() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); };

private:
    ComPtr<ICoreWebView2Environment> env_;
    std::unique_ptr<std::vector<QueueData>> queue_;
    static HRESULT OnEnvCreated(HRESULT result, ICoreWebView2Environment* env) {
        WebFactory& factory = GetInstance();
        auto queue = std::move(factory.queue_);
        if (FAILED(result)) {
            for (auto& data : *queue) {
                data.callback(result, nullptr);
            }
        } else {
            factory.env_ = env;
            for (auto& data : *queue) {
                HRESULT hr = factory.CreateWebView(data.hwnd, data.callback);
                if (FAILED(hr)) {
                    data.callback(hr, nullptr);
                }
            }
        }
        return S_OK;
    }
};

template <typename Derived>
class WebWindowBase : public Window<Derived> {
    friend class detail::WindowBase<Derived>;

protected:
    void WaitWebCreated() const {
        if (this->ctrl_) {
            return;
        } else if (!this->hwnd_) {
            throw WindowError(ERROR_INVALID_HANDLE, "Window is not created!");
        }
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message != UM_WEB_CREATED || msg.hwnd != this->hwnd_) {
                continue;
            }
            if (FAILED(msg.wParam)) {
                throw WindowError(static_cast<long>(msg.wParam), "CreateWebView failed!");
            }
            return;
        }
        throw WindowError(0, "GetMessage failed!");
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
        HRESULT hr = ctrl_.As(&ctrl2);
        if (FAILED(hr)) {
            throw WindowError(hr, "As ICoreWebView2Controller2 failed!");
        }
        hr = ctrl2->put_DefaultBackgroundColor({color.alpha, color.red, color.green, color.blue});
        if (FAILED(hr)) {
            throw WindowError(hr, "put_DefaultBackgroundColor failed!");
        }
    }
    void SetJsMsgHandler(std::function<std::string(std::string)> handler) {
        GetWebView()->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [handler = std::move(handler)](
                    ICoreWebView2* sender,
                    ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                    LPWSTR msg;
                    HRESULT hr = args->get_WebMessageAsJson(&msg);
                    if (FAILED(hr)) {
                        return hr;
                    }
                    std::string req = W2U8(msg);
                    CoTaskMemFree(msg);
                    std::string resp = handler(std::move(req));
                    sender->PostWebMessageAsJson(U82W(resp).c_str());
                    return S_OK;
                })
                .Get(),
            nullptr);
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
        HWND hwnd = 0;
        EnumChildWindows(
            this->hwnd_,
            [](HWND hwnd, LPARAM lp) -> BOOL {
                wchar_t name[256] = {0};
                GetClassNameW(hwnd, name, ARRAYSIZE(name));
                if (wcscmp(name, L"Chrome_WidgetWin_0") == 0) {
                    *reinterpret_cast<HWND*>(lp) = hwnd;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&hwnd));
        if (!hwnd) {
            throw WindowError(E_FAIL, "Get webview hwnd failed!");
        }
        SetFocus(hwnd);
    }
    void SetRequestHandler(std::function<void(RequestContext& ctx)> handler,
                           std::string_view filter) {
        ComPtr<ICoreWebView2_22> webview22;
        HRESULT hr = GetWebView().As<ICoreWebView2_22>(&webview22);
        if (FAILED(hr)) {
            throw WindowError(hr, "get_CoreWebView2_22 failed!");
        }
        webview22->AddWebResourceRequestedFilterWithRequestSourceKinds(
            detail::U82W(filter).c_str(),
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
        webview22->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this, handler = std::move(handler)](
                    ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                    RequestContext ctx{args, WebFactory::GetInstance().GetEnv().Get()};
                    handler(ctx);
                    return S_OK;
                })
                .Get(),
            nullptr);
    }

protected:
    ComPtr<ICoreWebView2Controller> ctrl_;
    ComPtr<ICoreWebView2> GetWebView() const {
        ComPtr<ICoreWebView2> webview;
        HRESULT hr = ctrl_->get_CoreWebView2(&webview);
        if (FAILED(hr)) {
            throw WindowError(hr, "get_CoreWebView2 failed!");
        }
        return webview;
    }
    void Init() {
        HRESULT hr = detail::WebFactory::GetInstance().CreateWebView(
            this->hwnd_, [this](HRESULT result, ComPtr<ICoreWebView2Controller> ctrl) {
                if (FAILED(result)) {
                    PostMessageW(this->hwnd_, UM_WEB_CREATED, result, 0);
                    return;
                }
                ctrl_ = ctrl;

                RECT rc;
                GetClientRect(this->hwnd_, &rc);
                ctrl_->put_Bounds(rc);       // 适应父窗口大小
                ctrl_->put_IsVisible(true);  // webview默认可见, 隐藏操作由父窗口控制
                Focus();                     // 创建 webview 后默认获取焦点, 跟其他控件/窗口对齐
                InitSetting();               // 其他的默认设置
                PostMessageW(this->hwnd_, UM_WEB_CREATED, S_OK, 0);  // 发送创建完成的消息
            });
        if (FAILED(hr)) {
            throw WindowError(hr, "CreateWebView failed!");
        }
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
        if (FAILED(settings.As<ICoreWebView2Settings9>(&settings9))) {
            return;
        }
        // 让 js 可以自行控制拖拽区域
        settings9->put_IsNonClientRegionSupportEnabled(true);
        // 发布版本禁用浏览器的各种快捷键
#ifndef _DEBUG
        settings9->put_AreBrowserAcceleratorKeysEnabled(false);
#endif
    }
    void OnSize(const SizeEvent& event) {
        if (ctrl_) {
            ctrl_->put_Bounds({0, 0, event.GetWidth(), event.GetHeight()});
        }
        Window<Derived>::OnSize(event);
    }
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case UM_WEB_CREATED:
                if (FAILED(wp)) {
                    WindowError err{static_cast<long>(wp), "CreateWebView failed!"};
                    static_cast<Derived*>(this)->OnWebCreated(err);
                } else {
                    static_cast<Derived*>(this)->OnWebCreated(std::nullopt);
                }
        }
        return Window<Derived>::OnWin32Msg(msg, wp, lp);
    }
};

}  // namespace cxxui::detail
