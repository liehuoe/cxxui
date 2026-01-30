#pragma once
#include "web_win/req_ctx.hpp"
#include "impl/web_win.inl"

namespace cxxui {

template <typename Derived = detail::DefaultWebWindow>
class WebWindow : public detail::WebWindowBase<Derived> {
    using Base = detail::WebWindowBase<Derived>;

public:
    WebWindow() = default;
    WebWindow(WindowOptions opts) { Create(std::move(opts)); }
    /**
     * @brief 创建窗口
     *
     * @param opts 创建窗口的选项
     */
    void Create(WindowOptions opts) {
        Base::Create(opts);
        try {
            this->Init();
        } catch (...) {
            Base::Close();
            throw;
        }
    }
    /**
     * @brief 等待webview创建完成
     */
    void WaitWebCreated() const { Base::WaitWebCreated(); }
    /**
     * @brief 设置html内容
     *
     * @param html html内容
     */
    void SetHtml(std::string_view html) { Base::SetHtml(html); }
    /**
     * @brief 设置要访问的url
     *
     * @param url 要访问的url
     */
    void SetUrl(std::string_view url) { Base::SetUrl(url); }
    /**
     * @brief 设置窗口背景颜色
     *
     * @param color 颜色
     */
    void SetBackground(const Color& color) { Base::SetBackground(color); }
    /**
     * @brief 设置接收javascript消息的处理函数
     *
     * @param handler 接收js发送的字符串消息，返回处理后的字符串消息给js
     *                js 通过 window.SendCppMsg(data: any) 发送消息到 C++
     *                js 通过 window.OnCppMsg(handler: (data: any) => void) 接收 C++ 的消息
     */
    using JsMsgHandler = std::function<std::string(std::string)>;
    void SetJsMsgHandler(JsMsgHandler handler) { Base::SetJsMsgHandler(std::move(handler)); }
    /**
     * @brief 发送消息给 javascript
     */
    void SendJsMsg(std::string_view msg) { Base::SendJsMsg(msg); }
    /**
     * @brief 运行javascript代码
     *
     * @param js_code javascript代码
     * @param on_created 是否在webview创建完成后运行, false则立即运行
     */
    void RunJs(std::string_view js_code, bool on_created = false) {
        Base::RunJs(js_code, on_created);
    }
    /**
     * @brief 使窗口获得焦点
     */
    void Focus() const { Base::Focus(); }
    /**
     * @brief 设置webview网页请求的处理函数
     *
     * @param handler 处理函数
     * @param filter 需要拦截的匹配URL
     * filter字符串格式参考：https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_22?view=webview2-1.0.3719.77&viewFallbackFrom=webview2-1.0.2484.35&spm=5176.28103460.0.0.7ce17551NJ6vBi#addwebresourcerequestedfilterwithrequestsourcekinds
     */
    using RequestHandler = std::function<void(RequestContext&)>;
    void SetRequestHandler(RequestHandler handler, std::string_view filter = "*") {
        Base::SetRequestHandler(handler, filter);
    }

protected:
    friend class Base;
    /**
     * @brief webview完成创建的事件
     */
    void OnWebCreated(std::optional<WindowError>) {}
};

namespace detail {
class DefaultWebWindow : public WebWindow<DefaultWebWindow> {};
}  // namespace detail

}  // namespace cxxui
