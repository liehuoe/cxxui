#include <functional>
#include <cxxui/core/detail/wm_msg.h>
#include "detail/mswebview2.hpp"

/** 定义 webview2 runtime 的目录，以制作便携版。
 * 如果目录不存在，则退化为查找系统安装的 webview2 runtime
 */
#ifndef CXXUI_WEBVIEW2_DIR
    #define CXXUI_WEBVIEW2_DIR "./webview2"
#endif
/** 定义 webview2 用户数据目录 */
#ifndef CXXUI_WEBVIEW2_USER_DIR
    #define CXXUI_WEBVIEW2_USER_DIR "./.cache"
#endif

namespace cxxui::detail {

using namespace Microsoft::WRL;

class WebCtrl {
public:
    using WebCallback = std::function<void(HRESULT, ComPtr<ICoreWebView2Controller>)>;
    WebCtrl() {
        if (ref_++ != 0) {
            return;
        }
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        HRESULT hr = MSWebView2{}.CreateEnvironment(
            CXXUI_WEBVIEW2_DIR,
            CXXUI_WEBVIEW2_USER_DIR,
            MSWebView2Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                &OnEnvCreated)
                .Get());
        if (SUCCEEDED(hr)) {
            queue_ = std::make_unique<std::vector<HWND>>();
        }
    }
    ~WebCtrl() {
        ctrl_ = nullptr;
        if (--ref_ == 0) {
            env_ = nullptr;
            CoUninitialize();
        }
    }
    static ICoreWebView2Environment* GetEnv() { return env_.Get(); }
    void SetCtrl(ICoreWebView2Controller* ctrl) { ctrl_ = ctrl; }
    ComPtr<ICoreWebView2Controller>& GetCtrl() { return ctrl_; }
    ICoreWebView2Controller* operator->() const noexcept { return ctrl_.Get(); }
    explicit operator bool() const noexcept { return ctrl_ != nullptr; }
    /** 创建webview */
    void Create(HWND hwnd) {
        if (env_) {
            CreateWebView(hwnd);
            return;
        }
        if (queue_) {
            queue_->emplace_back(hwnd);
            return;
        }
        HRESULT hr = MSWebView2{}.CreateEnvironment(
            CXXUI_WEBVIEW2_DIR,
            CXXUI_WEBVIEW2_USER_DIR,
            MSWebView2Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                &OnEnvCreated)
                .Get());
        if (SUCCEEDED(hr)) {
            queue_ = std::make_unique<std::vector<HWND>>();
            queue_->emplace_back(hwnd);
        } else {
            PostMessageW(hwnd, UM_WEB_CREATED, hr, 0);
        }
    }

private:
    ComPtr<ICoreWebView2Controller> ctrl_;

private:
    inline static std::atomic<long> ref_{0};
    inline static ComPtr<ICoreWebView2Environment> env_;
    inline static std::unique_ptr<std::vector<HWND>> queue_;
    static HRESULT OnEnvCreated(HRESULT result, ICoreWebView2Environment* env) {
        auto queue = std::move(queue_);
        if (FAILED(result)) {
            if (queue) {
                for (auto hwnd : *queue) {
                    PostMessageW(hwnd, UM_WEB_CREATED, result, 0);
                }
            }
        } else {
            env_ = env;
            if (queue) {
                for (auto hwnd : *queue) {
                    CreateWebView(hwnd);
                }
            }
        }
        return S_OK;
    }
    static void CreateWebView(HWND hwnd) {
        HRESULT hr;
        auto callback =
            MSWebView2Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [hwnd](HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT {
                    if (FAILED(result)) {
                        PostMessageW(hwnd, UM_WEB_CREATED, result, 0);
                        return S_OK;
                    }
                    if (!IsWindow(hwnd)) {
                        PostMessageW(hwnd, UM_WEB_CREATED, static_cast<WPARAM>(E_HANDLE), 0);
                    } else {
                        SendMessageW(hwnd, UM_WEB_CREATED, result, reinterpret_cast<LPARAM>(ctrl));
                    }
                    return S_OK;
                });

        ComPtr<ICoreWebView2Environment15> env15;
        ComPtr<ICoreWebView2ControllerOptions> opts;
        ComPtr<ICoreWebView2ControllerOptions3> opts3;
        if (SUCCEEDED(MSWebView2As(env_, env15)) &&
            SUCCEEDED(env15->CreateCoreWebView2ControllerOptions(&opts)) &&
            SUCCEEDED(MSWebView2As(opts, opts3))) {
            // 设置默认透明背景，解决可能的背景闪烁问题
            opts3->put_DefaultBackgroundColor({0, 0, 0, 0});
            hr = env15->CreateCoreWebView2ControllerWithOptions(hwnd, opts.Get(), callback.Get());
        } else {
            hr = env_->CreateCoreWebView2Controller(hwnd, callback.Get());
        }
        if (FAILED(hr)) {
            PostMessageW(hwnd, UM_WEB_CREATED, hr, 0);
        }
    }
};

}  // namespace cxxui::detail
