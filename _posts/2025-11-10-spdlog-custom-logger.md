---
title: 'Extending spdlog into a Custom Logger'
layout: post
tags: [cpp]
uuid: 9a5b808b-69e4-5eaf-bf4b-d612a7391771
---
Here's a small attempt to make my C++ projects have better logs. Rather than just using cout or printf, I use the spdlog library, then wrap it with a specific macro convention such as LOG_INFO, LOG_ERR etc. I also add a module based "TAG" and log level control to esily discern who sent what, added log sink to enable option of silent logging to not clutter the console. The reason for this LOG_ style is because my colleagues are used to using this call in their zephyr development, just trying to keep things familiar.

[github.com/royyandzakiy/cmake-spdlog-logger](https://github.com/royyandzakiy/cmake-spdlog-logger)