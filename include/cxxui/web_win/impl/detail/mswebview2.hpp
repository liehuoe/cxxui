#pragma once

#include <atomic>
#include <filesystem>
#include <wrl.h>
#include <WebView2.h>
#include <cxxui/core/detail/library.hpp>
#include <cxxui/core/detail/reg_key.hpp>

namespace cxxui::detail {

template <typename Interface, typename HandlerType>
class MSWebView2Handler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
                          public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
                          public ICoreWebView2WebMessageReceivedEventHandler,
                          public ICoreWebView2WebResourceRequestedEventHandler {
    HandlerType handler_;
    std::atomic<ULONG> ref_count_{1};

public:
    MSWebView2Handler(HandlerType&& handler)
        : handler_(std::move(handler)) {}
    virtual ~MSWebView2Handler() = default;

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override {
        if constexpr (std::is_same_v<Interface,
                                     ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>) {
            return handler_(result, env);
        } else {
            return E_NOTIMPL;
        }
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* ctrl) override {
        if constexpr (std::is_same_v<Interface,
                                     ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>) {
            return handler_(result, ctrl);
        } else {
            return E_NOTIMPL;
        }
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender,
                                     ICoreWebView2WebMessageReceivedEventArgs* args) override {
        if constexpr (std::is_same_v<Interface, ICoreWebView2WebMessageReceivedEventHandler>) {
            return handler_(sender, args);
        } else {
            return E_NOTIMPL;
        }
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender,
                                     ICoreWebView2WebResourceRequestedEventArgs* args) override {
        if constexpr (std::is_same_v<Interface, ICoreWebView2WebResourceRequestedEventHandler>) {
            return handler_(sender, args);
        } else {
            return E_NOTIMPL;
        }
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if constexpr (std::is_same_v<Interface,
                                     ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>) {
            if (riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) {
                *ppv = static_cast<Interface*>(this);
            }
        } else if constexpr (std::is_same_v<
                                 Interface,
                                 ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>) {
            if (riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) {
                *ppv = static_cast<Interface*>(this);
            }
        } else if constexpr (std::is_same_v<Interface,
                                            ICoreWebView2WebMessageReceivedEventHandler>) {
            if (riid == IID_ICoreWebView2WebMessageReceivedEventHandler) {
                *ppv = static_cast<Interface*>(this);
            }
        } else if constexpr (std::is_same_v<Interface,
                                            ICoreWebView2WebResourceRequestedEventHandler>) {
            if (riid == IID_ICoreWebView2WebResourceRequestedEventHandler) {
                *ppv = static_cast<Interface*>(this);
            }
        }

        if (*ppv) {
            AddRef();
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_count_; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --ref_count_;
        if (count == 0) {
            delete this;
        }
        return count;
    }
};

template <typename Interface, typename HandlerType>
Microsoft::WRL::ComPtr<MSWebView2Handler<Interface, HandlerType>> MSWebView2Callback(
    HandlerType&& handler) {
    Microsoft::WRL::ComPtr<MSWebView2Handler<Interface, HandlerType>> ptr;
    ptr.Attach(new MSWebView2Handler<Interface, HandlerType>{std::forward<HandlerType>(handler)});
    return ptr;
}

template <typename T, typename U>
HRESULT MSWebView2As(Microsoft::WRL::ComPtr<T>& src, Microsoft::WRL::ComPtr<U>& dst) {
    IID riid = IID_IUnknown;
    if constexpr (std::is_same_v<U, ICoreWebView2Environment15>) {
        riid = IID_ICoreWebView2Environment15;
    } else if constexpr (std::is_same_v<U, ICoreWebView2ControllerOptions3>) {
        riid = IID_ICoreWebView2ControllerOptions3;
    } else if constexpr (std::is_same_v<U, ICoreWebView2Settings9>) {
        riid = IID_ICoreWebView2Settings9;
    } else if constexpr (std::is_same_v<U, ICoreWebView2Controller2>) {
        riid = IID_ICoreWebView2Controller2;
    } else if constexpr (std::is_same_v<U, ICoreWebView2_22>) {
        riid = IID_ICoreWebView2_22;
    } else {
        static_assert(std::is_same_v<T, U>,
                      "Error: MSWebView2As is not implemented for some types!");
    }
    return src.CopyTo(riid, reinterpret_cast<void**>(dst.GetAddressOf()));
}

class MSWebView2 {
public:
    /** 参考 webview 的实现（https://github.com/webview/webview） */
    HRESULT CreateEnvironment(std::string_view app_dir,
                              std::string_view user_dir,
                              ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* handler) {
        Library lib;
        RuntimeType type = RuntimeType::Installed;
        auto app_path = ParseDir(app_dir);
        if (std::filesystem::exists(app_path) && lib.Load(GetDllPath(app_path).c_str())) {
            type = RuntimeType::Embedded;
        } else {
            RegKey reg;
            constexpr std::wstring_view sub_key =
                L"SOFTWARE\\Microsoft\\EdgeUpdate\\ClientState"
                L"\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
            if (!reg.Open(HKEY_LOCAL_MACHINE, sub_key, KEY_READ | KEY_WOW64_32KEY)) {
                reg.Open(HKEY_CURRENT_USER, sub_key, KEY_READ | KEY_WOW64_32KEY);
            }
            app_path = reg.ReadString(L"EBWebView");
            lib.Load(GetDllPath(app_path).c_str());
        }
        auto func = lib.Get<CreateWebViewEnvironmentWithOptionsInternalPtr>(
            "CreateWebViewEnvironmentWithOptionsInternal");
        if (!func) {
            MessageBoxW(nullptr,
                        L"No available WebView2 Runtime found!",
                        L"WebView2 Runtime",
                        MB_OK | MB_ICONINFORMATION);
            return E_NOTIMPL;
        }
        return func(true, type, ParseDir(user_dir).c_str(), nullptr, handler);
    }

private:
    enum class RuntimeType { Installed = 0, Embedded = 1 };
    using CreateWebViewEnvironmentWithOptionsInternalPtr =
        HRESULT(STDMETHODCALLTYPE*)(bool,
                                    RuntimeType,
                                    PCWSTR,
                                    IUnknown*,
                                    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
    std::filesystem::path GetDllPath(const std::filesystem::path& dir) {
        return dir /
               L"EBWebView/"
#if defined(_M_X64) || defined(__x86_64__)
               L"x64/"
#elif defined(_M_IX86) || defined(__i386__)
               L"x86/"
#elif defined(_M_ARM64) || defined(__aarch64__)
               L"arm64/"
#else
    #error WebView2 integration for this platform is not yet supported.
#endif
               L"EmbeddedBrowserWebView.dll";
    }
    /** 相对路径解析到程序目录里，而不是工作目录 */
    std::filesystem::path ParseDir(std::string_view src) {
        std::filesystem::path dst;
        if (src.empty()) {
            return dst;
        }
        if (src[0] == '.') {
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH - 1);
            dst = std::filesystem::path(exe_path).parent_path();
        }
        return dst / detail::U82W(src);
    }
};

}  // namespace cxxui::detail
