#include <cxxui/web_win.hpp>
#include <cxxui/web_win/js_msg_map.hpp>

constexpr std::string_view HTML = R"html(
<html>
    <head>
        <link rel="icon" href="data:," />
        <style>
            * { background: white; }
        </style>
        <script>
            function onClick() {
                // 点击按钮发送当前值给C++
                const count = parseInt(document.getElementById("msg").innerHTML);
                window.SendCppMsg({ url: "/count", data: { count } });
            }
            // 接收C++发送的消息并显示
            window.SetCppMsgHandler((res) => {
                if (res.code === 0) {
                    document.getElementById("msg").innerHTML = res.data.count.toString();
                } else {
                    alert(res.error);
                }
            });
        </script>
    </head>
    <body>
        <h1 id="msg">0</h1>
        <button onclick="onClick()">Click me</button>
    </body>
<html>
)html";

class EventWebWindow : public cxxui::WebWindow<EventWebWindow> {
public:
    using WebWindow::WebWindow;
    /**
     * 1. webview创建完成
     */
    void OnWebCreated(std::optional<cxxui::WindowError> err) {
        if (err) {
            fprintf(stderr, "WebCreatedEvent error: %s\n", err->what());
        } else {
            fprintf(stderr, "WebCreatedEvent success\n");
        }
        cxxui::WebWindow<EventWebWindow>::OnWebCreated(err);
    }
};

int main() {
    EventWebWindow web_win{
        cxxui::WindowOptions().SetTitle("WebWindow").SetWidth(400).SetHeight(400)};
    /**
     * 1. 进行WebWindow相关的操作前需要等待webview2创建完成
     */
    web_win.WaitWebCreated();
    /**
     * 2. 通过拦截请求返回响应的html字符串
     */
    web_win.SetRequestHandler([](cxxui::RequestContext& ctx) {
        fprintf(stderr, "url: %s\n", ctx.GetUrl().c_str());
        ctx.SetHeaders(std::string{ctx.GetContentType("html")});
        ctx.SetResponse(HTML.data(), HTML.size());
    });
    /**
     * 3. 响应js请求并返回响应
     */
    cxxui::JsMsgMap<> req_map;
    req_map.bind("/count", [](cxxui::json& arg) {
        // 获取 { "count": number }
        int count = arg.at("count").get<int>() + 1;
        // 在当前值的基础上 + 1 并返回
        return cxxui::json{{"count", count}};
    });
    web_win.SetJsMsgHandler(req_map.GetHandler());

    web_win.SetUrl("http://localhost");
    web_win.Show();
    return web_win.Run();
}