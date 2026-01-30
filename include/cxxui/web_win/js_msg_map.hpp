#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace cxxui {

using json = nlohmann::json;

/**
 * @brief 响应码
 */
enum class JsMsgError {
    /**
     * @brief 处理成功
     */
    SUCCESS,
    /**
     * @brief 无效请求，url不正确
     */
    INVALID_REQ,
    /**
     * @brief 找不到响应函数，没有绑定相关url的响应函数
     */
    NO_METHOD,
    /**
     * @brief 响应函数执行失败
     */
    EXEC_ERROR,
};

template <typename Derived>
class JsMsgHandler {
protected:
    /**
     * @brief 处理请求数据的处理函数
     *
     * @param msg js 传入的 json 字符串，示例：
    {
        "url": "/user/login",
        "data": {
            "username": "admin",
            "password": "123456"
        }
    }
     * @return std::string 返回给 js 的响应 json 字符串，示例：
    {
         "code": 3,
         "error": "用户名不存在",
         "data": ...
    }
     */
    std::string Handle(std::string msg) const {
        json ctx;
        std::string url;
        try {
            ctx = json::parse(std::move(msg));
            url = ctx.at("url").get<std::string>();
        } catch (const std::exception& e) {
            ctx["code"] = static_cast<int>(JsMsgError::INVALID_REQ);
            ctx["error"] = e.what();
            return ctx.dump();
        }
        std::function<json(json&)> func;
        try {
            func = static_cast<const Derived*>(this)->FindHandler(url);
        } catch (const std::exception& e) {
            ctx["code"] = static_cast<int>(JsMsgError::NO_METHOD);
            ctx["error"] = e.what();
            return ctx.dump();
        }
        try {
            auto data = func(ctx["data"]);
            ctx["data"] = data;
            ctx["code"] = static_cast<int>(JsMsgError::SUCCESS);
        } catch (const std::exception& e) {
            ctx["code"] = static_cast<int>(JsMsgError::EXEC_ERROR);
            ctx["error"] = e.what();
            return ctx.dump();
        }
        return ctx.dump();
    }
};

template <typename Derived = detail::DefaultJsMsgMap>
class JsMsgMap : public JsMsgHandler<Derived> {
    friend class JsMsgHandler<Derived>;

public:
    /**
     * @brief 获取全局单例实例
     *
     * @return JsMsgMap&
     */
    static JsMsgMap& GetSingleton() {
        static JsMsgMap instance;
        return instance;
    }
    /**
     * @brief 绑定请求的url及其响应函数
     *
     * @param url 需要绑定的 url
     * @param func 响应函数，传入请求json数据，返回响应json数据
     */
    void bind(std::string url, std::function<json(json&)> func) {
        handlers_[std::move(url)] = std::move(func);
    }
    /**
     * @brief 获取js请求的处理函数，用于设置SetJsMsgHandler
     *
     * @return std::function<std::string(std::string)>
     */
    std::function<std::string(std::string)> GetHandler() {
        return [this](std::string msg) { return this->Handle(std::move(msg)); };
    }

protected:
    std::map<std::string, std::function<json(json&)>> handlers_;
    std::function<json(json&)> FindHandler(const std::string& url) const {
        auto it = handlers_.find(url);
        if (it == handlers_.end()) {
            throw std::runtime_error("Method not found!");
        } else {
            return it->second;
        }
    }
};

namespace detail {
class DefaultJsMsgMap : public JsMsgMap<DefaultJsMsgMap> {};
}  // namespace detail

}  // namespace cxxui