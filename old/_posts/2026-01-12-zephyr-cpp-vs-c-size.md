---
date:   2026-01-12 13:03:03 +0700
title: "Zephyr Modern C++ - State Machine Size Comparison"
layout: post
categories: post
author: "Royyan"
tags: zephyr cpp state-machine
comments: true
---

Can we use the latest C++ features on microcontrollers? **Yes.**

I've been testing out to use it on Zephyr and ESP-IDF.

One particular feature I tried out in this example is the use of `std::variant` for different States, and ultimately using `std::visit` to iterate through them. Here I provide implementation of a general `get_state_info` to read what the current state is using the Overloaded Lambda idiom (to enforce availability of all StateVariants), it will then return a `std::string_view` of different messages based on the `current_state_`.

Let's say, a more shiny StateMachine haha. A different approach than using bare enums and switch statements (not simply saying they are less).

Check it out!
[https://github.com/royyandzakiy/zephyr-modern-cpp](https://github.com/royyandzakiy/zephyr-modern-cpp)

<!--more-->

So far, there is a **540 bytes difference** when using `std::variant`/`std::visit`, compared to raw enum/switch.

**First build (std::variant/visit):**
- FLASH: 73,112 bytes (6.97% of 1MB)
- RAM: 53,928 bytes (11.76% of 448KB)

**Second build (enum/switch)** (code depicted in image below):
- FLASH: 72,572 bytes (6.92% of 1MB)
- RAM: 53,928 bytes (11.76% of 448KB)

I also tried comparing when adding the C++ library to the compilation, and removing it. There is a **4256 byte addition**.

**Third build** (converting all C++ to pure C & but still compiling C++ library in kconfig):
- FLASH: 71,588 bytes (6.83% of 1MB)
- RAM: 53,952 bytes (11.76% of 448KB)

**Fourth build** (converting all C++ to pure C & removing C++ library in kconfig):
- FLASH: 67,332 bytes (6.42% of 1MB)
- RAM: 53,840 bytes (11.74% of 448KB)

**State machine using std::variant, std::visit**

![alt text](/assets/images/zephyr-cpp-vs-c-size_variant.png)

**State machine using enum & switch**

![alt text](/assets/images/zephyr-cpp-vs-c-size_switch.png)

**Removing most all CPP code, change into C**

![alt text](/assets/images/zephyr-cpp-vs-c-size_c_cpp.png)

**Removing CPP lib in prj.conf**

![alt text](/assets/images/zephyr-cpp-vs-c-size_pure_c.png)

**Code using enum & switch**

![alt text](/assets/images/zephyr-cpp-vs-c-size_recode_pure_c.png)

---

The variant doesn't add too much overhead. I was expecting a lot more bytes eaten up just by using Templates itself 😄

cc Srdjan Stokic