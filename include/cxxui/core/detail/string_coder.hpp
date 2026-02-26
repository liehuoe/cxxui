#pragma once

#include <string>
#include <windows.h>

namespace cxxui::detail {

/** UTF-8 转 Unicode */
inline std::wstring U82W(std::string_view input) {
    std::wstring output;
    if (input.empty()) {
        return output;
    }
    UINT cp = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    auto input_c = input.data();
    auto input_len = static_cast<int>(input.size());
    auto len = MultiByteToWideChar(cp, flags, input_c, input_len, nullptr, 0);
    if (len <= 0) {
        return output;
    }
    output.resize(len);
    MultiByteToWideChar(cp, flags, input_c, input_len, &output[0], len);
    return output;
}

/** Unicode 转 UTF-8 */
inline std::string W2U8(std::wstring_view input) {
    std::string output;
    if (input.empty()) {
        return output;
    }
    UINT cp = CP_UTF8;
    auto input_c = input.data();
    auto input_len = static_cast<int>(input.size());
    auto len = WideCharToMultiByte(cp, 0, input_c, input_len, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return output;
    }
    output.resize(len);
    WideCharToMultiByte(cp, 0, input_c, input_len, &output[0], len, nullptr, nullptr);
    return output;
}

}  // namespace cxxui::detail
