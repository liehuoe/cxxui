# CXXUI

[![Generic badge](https://img.shields.io/badge/C++-17+-blue.svg)](https://github.com/liehuoe/cxxui)
[![Generic badge](https://img.shields.io/badge/MSVC-17+-blue.svg)](https://github.com/liehuoe/cxxui)

**CXXUI** 是使用CRTP技术进行封装的现代C++ UI库，目标是轻量、高性能、易用，目前仅支持win32平台。

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

见 [Examples](https://github.com/liehuoe/cxxui/tree/main/examples)

## 最后

- 我是一名普通的个人开发者，商业合作请加微信

![微信](./docs/images/weixin.jpg)

- 如果你觉得这个项目不错，请作者喝杯咖啡

![打赏](./docs/images/coffee.jpg)
