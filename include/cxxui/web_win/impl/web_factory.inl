#include <functional>
#include <filesystem>
#include <wrl.h>
#include <WebView2.h>
#include <cxxui/core/detail/string_coder.hpp>

/** 定义 webview2 runtime 的目录，以制作便携版。
 * 如果目录不存在，则退化为查找系统安装的 webview2 runtime
 */
#ifndef CXXUI_WEBVIEW2_DIR
    #define CXXUI_WEBVIEW2_DIR "./webview2"
#endif

namespace cxxui::detail {

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
            ComPtr<ICoreWebView2Environment15> env15;
            ComPtr<ICoreWebView2ControllerOptions> opts;
            ComPtr<ICoreWebView2ControllerOptions3> opts3;
            if (SUCCEEDED(env_.As<ICoreWebView2Environment15>(&env15)) &&
                SUCCEEDED(env15->CreateCoreWebView2ControllerOptions(&opts)) &&
                SUCCEEDED(opts.As<ICoreWebView2ControllerOptions3>(&opts3))) {
                // 设置默认透明背景，解决可能的背景闪烁问题
                opts3->put_DefaultBackgroundColor({0, 0, 0, 0});
                return env15->CreateCoreWebView2ControllerWithOptions(
                    hwnd,
                    opts.Get(),
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd, callback = std::move(callback)](
                            HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (IsWindow(hwnd)) {
                                callback(result, ctrl);
                            }
                            return S_OK;
                        })
                        .Get());
            } else {
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
        }
        // 设置 webview2 缓存路径
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        auto udf = std::filesystem::path(exe_path).parent_path() / L".cache";
        // 异步创建 env，创建期间缓存创建任务到queue
        // env 创建完成后在 OnEnvCreated 处理 queue
        queue_ = std::make_unique<std::vector<QueueData>>();
        queue_->emplace_back(QueueData{hwnd, callback});
        std::filesystem::path webview2_dir = GetWebView2Dir();
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            std::filesystem::exists(webview2_dir) ? webview2_dir.c_str() : nullptr,
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
    std::filesystem::path GetWebView2Dir() {
        std::filesystem::path dir;
        if constexpr (CXXUI_WEBVIEW2_DIR[0] == '.') {
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH - 1);
            dir = std::filesystem::path(exe_path).parent_path() / detail::U82W(CXXUI_WEBVIEW2_DIR);
        } else {
            dir = detail::U82W(CXXUI_WEBVIEW2_DIR);
        }
        return dir;
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
}  // namespace cxxui::detail