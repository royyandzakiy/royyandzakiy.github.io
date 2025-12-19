---
date:   2025-12-26 13:03:03 +0700
title:  Zephyr Modern C++: Sensors Concept
layout: post
categories: post
author: "Royyan"
tags: cpp
comments: true
---
Can we use the latest c++ features on microcontrollers? I've been testing out to use it on Zephyr and ESP-IDF

Here is the continuation of the last post.

This time I try using Concepts to iterate through a bunch of "Sensors". Here I use the generic function process_sensors that utilizes C++ Template Metaprogramming and uses the Ellipsis "..." operator to process through an arbritary amount of sensors (in this case, 3). The result is an expressive and elegant code (feels like I'm coding in javascript with the Spread Operator haha). 

The measurements array stores results of read() of each sensors, invoked each time this process_sensors gets called. Fun fact, here the std::array automatically deduces it's own type and size because of Class Template Argument Deduction (CTAD) enforced by the SensorType concept (no need to define std::array<type, size>).

Is it the most efficient in the case when we need to optimize for firmware size, probably not. Is it fun? Yesh

Check it out!

[github.com/royyandzakiy/zephyr-modern-cpp](https://github.com/royyandzakiy/zephyr-modern-cpp)

```cpp
// code will be added here...
```