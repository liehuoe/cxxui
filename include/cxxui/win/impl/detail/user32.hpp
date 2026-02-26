#pragma once

#include <cxxui/core/detail/library.hpp>

namespace cxxui::detail {
class User32 {
public:
    using SetThreadDpiAwarenessContextPtr =
        DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT dpiContext);
    void SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext) {
        auto func = lib_.Get<SetThreadDpiAwarenessContextPtr>("SetThreadDpiAwarenessContext");
        if (!func) {
            return;
        }
        func(dpiContext);
    }

private:
    Library lib_{"user32.dll"};
};

}  // namespace cxxui::detail
