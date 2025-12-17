---
layout: post
title:  "Hello World!"
date:   2025-12-17 13:03:03 +0700
categories: post
---
Hello World!

{% highlight cpp %} 
#include <iostream> 
#include <string>

void say_hello(std::string name) {
   std::cout << "Hello, " << name << std::endl; 
}

int main() { 
  say_hello("World!"); 
  
  return 0; 
}
{% endhighlight %}