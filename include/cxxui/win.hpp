#pragma once
#include <string_view>

#include <cxxui/core/color.hpp>
#include "win/error.hpp"
#include "win/options.hpp"
#include "win/event.hpp"
#include "win/impl/win.inl"

namespace cxxui {

template <typename Derived = detail::DefaultWindow>
class Window : public detail::WindowBase<Derived> {
    using Base = detail::WindowBase<Derived>;

public:
    Window() = default;
    Window(WindowOptions opts) { Create(std::move(opts)); }
    /**
     * @brief 开始运行消息循环，调用Run函数的窗口会定义为主窗口，主窗口退出则整个进程退出
     *
     * @return int 消息循环结束的退出码
     */
    int Run() noexcept { return Base::Run(); }
    /**
     * @brief 创建窗口
     *
     * @param opts 创建窗口的选项
     */
    void Create(WindowOptions opts) { Base::Create(opts); }
    /**
     * @brief 显示/隐藏窗口
     *
     * @param is_show 是否显示, 默认显示
     */
    void Show(bool is_show = true) const noexcept { Base::Show(is_show); }
    /**
     * @brief 关闭窗口
     */
    void Close() const noexcept { Base::Close(); }
    /**
     * @brief 使窗口获得焦点
     */
    void Focus() const { Base::Focus(); }
    /**
     * @brief 设置窗口标题
     *
     * @param title
     */
    void SetTitle(std::string_view title) { Base::SetTitle(title); }
    /**
     * @brief 设置标题栏颜色
     *
     * @param color
     */
    void SetTitleColor(const Color& color) { Base::SetTitleColor(color); }
    /**
     * @brief 设置标题栏图标
     *
     * @param icon_id 图标资源ID
     */
    void SetIcon(std::uint32_t icon_id) { Base::SetIcon(icon_id); }

protected:
    friend class detail::WindowBase<Derived>;
    /**
     * @brief 窗口创建完成的事件
     */
    void OnCreated() {}
    /**
     * @brief 窗口大小发生变化的事件
     */
    void OnSize(const SizeEvent&) {}
    /**
     * @brief 窗口激活或失去激活触发的事件
     */
    void OnActivate(const ActivateEvent&) {}
};

/**
 * @brief 退出整个程序
 *
 * @param exit_code 退出码, 程序执行的返回值， 默认为 0
 */
inline void Exit(int exit_code = 0) noexcept { detail::Exit(exit_code); }

namespace detail {
class DefaultWindow : public Window<DefaultWindow> {};
}  // namespace detail

}  // namespace cxxui
