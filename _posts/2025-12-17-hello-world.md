---
layout: post
title:  "Hello World!"
date:   2025-12-17 13:03:03 +0700
categories: post
---
Here is a simple `Hello, World!` code implementation using C++23's `std::println`

```cpp
#include <print> 
#include <string>

void say_hello(std::string name) {
   std::println("Hello, {}", name); 
}

int main() { 
  say_hello("World!"); 
  
  return 0; 
}
```