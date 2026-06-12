#pragma once

#include <string_view>
#include <windows.h>
#include "string_coder.hpp"

namespace cxxui::detail {

class Library {
public:
    Library() = default;
    Library(std::string_view dll_name) { Load(dll_name); }
    Library(std::wstring_view dll_name) { Load(dll_name); }
    ~Library() { Unload(); }
    bool Load(std::string_view dll_name) { return Load(U82W(dll_name)); }
    bool Load(std::wstring_view dll_name) {
        Unload();
        handle_ = LoadLibraryW(dll_name.data());
        return handle_ != nullptr;
    }
    void Unload() {
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
    HMODULE handle_ = nullptr;
};

}  // namespace cxxui::detail