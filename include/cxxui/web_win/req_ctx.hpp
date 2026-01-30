#pragma once
#include "impl/req_ctx.inl"

namespace cxxui {

class RequestContext : public detail::RequestContextBase {
    using Base = detail::RequestContextBase;
    using Base::RequestContextBase;

public:
    /**
     * @brief 获取请求的URL
     *
     * @return std::string url字符串
     */
    std::string GetUrl() const { return Base::GetUrl(); };
    /**
     * @brief 设置 headers 字符串
     *
     * @param headers
     */
    void SetHeaders(std::string headers) { Base::SetHeaders(std::move(headers)); }
    /**
     * @brief 根据扩展名获取 headers 中的 ContentType 字符串
     *
     * @param ext_name 扩展名，比如 jpg，ico
     * @param default_type 遇到未知扩展名时返回的默认字符串
     * @return std::string_view
     */
    std::string_view GetContentType(
        std::string_view ext_name,
        std::string_view default_type = "Content-Type:application/octet-stream\r\n") {
        return Base::GetContentType(ext_name, default_type);
    }
    /**
     * @brief 设置响应码，比如404
     *
     * @param status_code
     */
    void SetResponse(int status_code) { Base::SetResponse(status_code); }
    /**
     * @brief 设置响应内容
     *
     * @param data 响应内容数据指针
     * @param size 响应内容数据大小
     * @param status_code 响应码，默认200
     */
    void SetResponse(const void* data, std::size_t size, int status_code = 200) {
        Base::SetResponse(data, size, status_code);
    }
    /**
     * @brief 设置响应文件
     *
     * @param file_path 响应文件路径，程序将读取路径的文件内容并响应
     * @param status_code 响应码，默认200
     */
    void SetResponse(std::string_view file_path) {
        Base::SetResponse(file_path);
    }
};

}  // namespace cxxui