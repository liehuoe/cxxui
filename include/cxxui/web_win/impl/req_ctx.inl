#include <string>
#include <unordered_map>

#include <windows.h>
#include <wrl.h>
#include <WebView2.h>
#include <shlwapi.h>
#ifdef _MSC_VER
    #pragma comment(lib, "shlwapi.lib")  // SHCreateMemStream
#endif

#include <cxxui/win/error.hpp>
#include <cxxui/core/detail/string_coder.hpp>

namespace cxxui::detail {

using namespace Microsoft::WRL;

class RequestContextBase {
    template <typename T>
    friend class WebWindowBase;

protected:
    RequestContextBase(ICoreWebView2WebResourceRequestedEventArgs* args,
                       ICoreWebView2Environment* env)
        : args_(args),
          env_(env) {
        HRESULT hr = args->get_Request(&req_);
        if (FAILED(hr)) {
            throw WindowError(hr, "get_Request failed!");
        }
    }
    std::string GetUrl() const {
        std::string output;
        LPWSTR url_str;
        if (FAILED(req_->get_Uri(&url_str))) {
            return output;
        }
        output = W2U8(url_str);
        CoTaskMemFree(url_str);
        return output;
    }
    void SetHeaders(std::string headers) { headers_ = std::move(headers); }
    std::string_view GetContentType(std::string_view ext_name, std::string_view default_type) {
        static const std::unordered_map<std::string_view, std::string_view> mime_map = {
            {"html", "Content-Type: text/html; charset=utf-8\r\n"},
            {"htm", "Content-Type: text/html; charset=utf-8\r\n"},
            {"css", "Content-Type: text/css; charset=utf-8\r\n"},
            {"js", "Content-Type: application/javascript; charset=utf-8\r\n"},
            {"json", "Content-Type: application/json; charset=utf-8\r\n"},
            {"png", "Content-Type: image/png\r\n"},
            {"jpg", "Content-Type: image/jpeg\r\n"},
            {"jpeg", "Content-Type: image/jpeg\r\n"},
            {"ico", "Content-Type: image/x-icon\r\n"},
        };
        auto it = mime_map.find(ext_name);
        return it != mime_map.end() ? it->second : default_type;
    }
    void SetResponse(int status_code) { SetResponse(status_code, nullptr); }
    void SetResponse(const void* data, std::size_t size, int status_code) {
        SetResponse(status_code,
                    SHCreateMemStream(static_cast<const BYTE*>(data), static_cast<UINT>(size)));
    }
    void SetResponse(std::string_view file_path) {
        IStream* stream = nullptr;
        HRESULT hr = SHCreateStreamOnFileEx(U82W(file_path).data(),
                                            STGM_READ | STGM_SHARE_DENY_WRITE,
                                            FILE_ATTRIBUTE_NORMAL,
                                            FALSE,
                                            nullptr,
                                            &stream);
        if (FAILED(hr)) {
            SetResponse(404);
            throw WindowError(hr, "SHCreateStreamOnFileEx failed!");
        } else {
            SetResponse(200, stream);
        }
    }

private:
    ICoreWebView2WebResourceRequestedEventArgs* args_;
    ICoreWebView2Environment* env_;
    ComPtr<ICoreWebView2WebResourceRequest> req_;
    std::string headers_;
    void SetResponse(int status_code, IStream* stream) {
        ComPtr<ICoreWebView2WebResourceResponse> response;
        env_->CreateWebResourceResponse(
            stream, status_code, GetReasonPhrase(status_code), U82W(headers_).data(), &response);
        args_->put_Response(response.Get());
    }
    static LPCWSTR GetReasonPhrase(int status_code) {
        switch (status_code) {
            case 200:
                return L"OK";
            case 201:
                return L"Created";
            case 204:
                return L"No Content";
            case 301:
                return L"Moved Permanently";
            case 302:
                return L"Found";
            case 304:
                return L"Not Modified";
            case 400:
                return L"Bad Request";
            case 401:
                return L"Unauthorized";
            case 403:
                return L"Forbidden";
            case 404:
                return L"Not Found";
            case 405:
                return L"Method Not Allowed";
            case 500:
                return L"Internal Server Error";
            case 502:
                return L"Bad Gateway";
            case 503:
                return L"Service Unavailable";
            default:
                return L"Unknown";
        }
    }
};

}  // namespace cxxui::detail
