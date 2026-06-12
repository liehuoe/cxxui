#pragma once

#include <windows.h>
#include "string_coder.hpp"

namespace cxxui::detail {

class RegKey {
public:
    RegKey() = default;
    RegKey(HKEY root, std::string_view sub_key, REGSAM sam) { Open(root, sub_key, sam); }
    RegKey(HKEY root, std::wstring_view sub_key, REGSAM sam) { Open(root, sub_key, sam); }
    ~RegKey() { Close(); }
    bool Open(HKEY root, std::wstring_view sub_key, REGSAM sam) {
        Close();
        auto result = RegOpenKeyExW(root, sub_key.data(), 0, sam, &key_);
        return result == ERROR_SUCCESS;
    }
    bool Open(HKEY root, std::string_view sub_key, REGSAM sam) {
        return Open(root, U82W(sub_key), sam);
    }
    bool IsOpen() { return key_ != nullptr; }
    void Close() {
        if (key_) {
            RegCloseKey(key_);
        }
    }
    std::wstring ReadString(std::wstring_view name) {
        std::wstring buf;
        DWORD buf_len = 0;
        auto status = RegQueryValueExW(key_, name.data(), nullptr, nullptr, nullptr, &buf_len);
        if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
            return {};
        }
        buf.resize(buf_len / sizeof(std::wstring::value_type));
        status = RegQueryValueExW(
            key_, name.data(), nullptr, nullptr, reinterpret_cast<LPBYTE>(buf.data()), &buf_len);
        if (status != ERROR_SUCCESS) {
            return {};
        }
        buf.resize(wcslen(buf.data()));
        return buf;
    }

private:
    HKEY key_ = nullptr;
};

}  // namespace cxxui::detail
