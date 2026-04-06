---
layout: post
title: "Example Content"
author: "Chester"
tags: cppExample Main
excerpt_separator: <!--more-->
sticky: true
hidden: true
comments: true
---
So far, there is a 540 bytes difference when using std variant/visit, compared to raw enum/switch

First build (std::variant/visit):
- FLASH: 73,112 bytes (6.97% of 1MB)
- RAM: 53,928 bytes (11.76% of 448KB)

Second build (enum/switch) (code depicted in image below):
- FLASH: 72,572 bytes (6.92% of 1MB)
- RAM: 53,928 bytes (11.76% of 448KB)

I also, tried comparing when adding the C++ library to the compilation, and removing it. There is a 4256 byte addition

Third build (converting all C++ to pure C & but still compiling C++ library in kconfig):
- FLASH: 71,588 bytes (6.83% of 1MB)
- RAM: 53,952 bytes (11.76% of 448KB)

Fourth build (converting all C++ to pure C & removing C++ library in kconfig):
- FLASH: 67,332 bytes (6.42% of 1MB)
- RAM: 53,840 bytes (11.74% of 448KB)

state machine using std::variant, std::visit

![alt text](assets/images/zephyr-cpp-vs-c-size_variant.png)

state machine using enum & switch

![alt text](assets/images/zephyr-cpp-vs-c-size_switch.png)

removing most all cpp code, change into c

![alt text](assets/images/zephyr-cpp-vs-c-size_c_cpp.png)

removing cpp lib in prj.conf

![alt text](assets/images/zephyr-cpp-vs-c-size_pure_c.png)

code using enum & switch

![alt text](assets/images/zephyr-cpp-vs-c-size_recode_pure_c.png)