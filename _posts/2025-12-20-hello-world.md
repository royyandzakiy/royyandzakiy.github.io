---
layout: post
title:  Perfect Forwarding the "Hello World!"
date:   2025-12-17 13:03:03 +0700
categories: post
# comments: true
---
I've been going back and forth understanding lvalue and rvalue. Here is an attempt of mine in explaining it

So here is a cool "Perfect Forwarding" `Hello, World!` code implementation using C++. When `std::string{"World!"}` is passed, this is a prvalue (a "temporary") aka an rvalue expression. `T` is deduced to `std::string`, becoming `say_hello(std::string && name_param)`. In the other case, when instead the variable name is passed, this is a variable aka an lvalue, `T` is deduced to `std::string&` becoming `say_hello(std::string& && name_param)`, the `&&` will get reduced (reference collapsing rules), therefore finally becoming `say_hello(std::string& name_param)`.

`std::forward<T>` restores the value category (lvalue/rvalue) of the original argument expression by conditionally casting the named parameter to the value category encoded in T, hence "Perfect Forwarding" (same type, same cv-qualification, same value category).


```cpp
#include <print> 
#include <string>
#include <utility>

template <typename T>
void say_hello(T&& name_param) {
   std::println("Hello, {}", std::forward<T>(name_param)); 
}

int main() { 
    say_hello(std::string{"World!"}); 
    
    std::string name = "Matt!";
    say_hello(name); 
  
    return 0; 
}
```