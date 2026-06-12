#include <functional>
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
            if (SUCCEEDED(MSWebView2As(env_, env15)) &&
                SUCCEEDED(env15->CreateCoreWebView2ControllerOptions(&opts)) &&
                SUCCEEDED(MSWebView2As(opts, opts3))) {
                // 设置默认透明背景，解决可能的背景闪烁问题
                opts3->put_DefaultBackgroundColor({0, 0, 0, 0});
                return env15->CreateCoreWebView2ControllerWithOptions(
                    hwnd,
                    opts.Get(),
                    MSWebView2Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [callback = std::move(callback)](HRESULT result,
                                                         ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (SUCCEEDED(result)) {
                                HWND parent = nullptr;
                                ctrl->get_ParentWindow(&parent);
                                if (!IsWindow(parent)) {
                                    return S_OK;
                                }
                            }
                            callback(result, ctrl);
                            return S_OK;
                        })
                        .Get());
            } else {
                return env_->CreateCoreWebView2Controller(
                    hwnd,
                    MSWebView2Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [callback = std::move(callback)](HRESULT result,
                                                         ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (SUCCEEDED(result)) {
                                HWND parent = nullptr;
                                ctrl->get_ParentWindow(&parent);
                                if (!IsWindow(parent)) {
                                    return S_OK;
                                }
                            }
                            callback(result, ctrl);
                            return S_OK;
                        })
                        .Get());
            }
        }
        // 异步创建 env，创建期间缓存创建任务到queue
        // env 创建完成后在 OnEnvCreated 处理 queue
        queue_ = std::make_unique<std::vector<QueueData>>();
        queue_->emplace_back(QueueData{hwnd, callback});
        HRESULT hr = MSWebView2{}.CreateEnvironment(
            CXXUI_WEBVIEW2_DIR,
            CXXUI_WEBVIEW2_USER_DIR,
            MSWebView2Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                &OnEnvCreated)
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
}  // namespace cxxui::detail