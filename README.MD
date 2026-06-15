# CXXUI

[![Generic badge](https://img.shields.io/badge/C++-17+-blue.svg)](https://github.com/liehuoe/cxxui)
[![Generic badge](https://img.shields.io/badge/MSVC-17+-blue.svg)](https://github.com/liehuoe/cxxui)
[![Generic badge](https://img.shields.io/badge/gcc-15+-blue.svg)](https://github.com/liehuoe/cxxui)

**CXXUI** 是使用CRTP技术进行封装的现代C++ UI库，所有代码都以零抽象成本为前提进行封装、实现，目标是轻量、高性能、易用、好用。

- 目前仅支持win32平台，以后考虑支持其他平台。
- 目前实现 webview 作为 UI 嵌入到窗口，与 C++ 作为后端配合开发，`cxxui::WebWindow` 基本满足现代化GUI程序开发需求。
- 后续待加入图形渲染引擎以及 C++ 原生 UI 布局接口。


## 安装

- 使用 CMake 进行编译：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

- 在你的项目中使用 CXXUI

```cmake
target_link_libraries(<your_target> PRIVATE cxxui)
```

## 示例

- [Examples](https://github.com/liehuoe/cxxui/tree/main/examples)
- [WeiXins - 微信多开助手](https://github.com/liehuoe/WeiXins)

## 支持

<table>
    <td align="center">
        <div>我的微信</div>
        <img src="docs/images/weixin.jpg" width="200px" />
    </td>
    <td align="center">
        <div>打赏支持</div>
        <img src="docs/images/coffee.jpg" width="200px" />
    </td>
</table>
