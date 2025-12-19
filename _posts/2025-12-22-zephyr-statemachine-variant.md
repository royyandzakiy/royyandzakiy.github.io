---
date:   2025-12-22 13:03:03 +0700
title:  Zephyr Modern C++: Statemachines
layout: post
categories: post
author: "Royyan"
tags: cpp
comments: true
---
Can we use the latest C++ features on microcontrollers? Yes. 

I've been testing out to use it on Zephyr and ESP-IDF

One particular feature I tried out in this example is the use of std::variant for different States, and ultimately using std::visit to iterate through them. Here I provide implementation of a general get_state_info to read what the current state is using the Overloaded Lambda idiom (to enforce availability of all StateVariants), it will then return a std::string_view of different messages based on the current_state_

Let's say, a more shiny Statemachine haha. A different approach than using bare enums and switch statements (not simply saying they are less)

Check it out!

[github.com/royyandzakiy/zephyr-modern-cpp](https://github.com/royyandzakiy/zephyr-modern-cpp)

```cpp
// code will be added here...
```