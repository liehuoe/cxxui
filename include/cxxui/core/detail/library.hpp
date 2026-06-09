#pragma once

#include <string_view>
#include <windows.h>

namespace cxxui::detail {

class Library {
public:
    Library(std::string_view dll_name) { handle_ = LoadLibraryA(dll_name.data()); }
    ~Library() {
        if (handle_) {
            FreeLibrary(handle_);
        }
    }
    bool IsValid() const noexcept { return handle_ != nullptr; }
    template <typename T>
    T Get(std::string_view func_name) noexcept {
        if (!IsValid()) {
            return nullptr;
        }
#ifndef _MSC_VER
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        return reinterpret_cast<T>(GetProcAddress(handle_, func_name.data()));
#ifndef _MSC_VER
    #pragma GCC diagnostic pop
#endif
    }

private:
    HMODULE handle_;
};

}  // namespace cxxui::detail