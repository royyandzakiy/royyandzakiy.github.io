- change base to completely use cpp23 where possible
- change the slug https://royyandzakiy.com/2025-12-29/latest-clang-compile/
    - to be: /blog/yyyy-mm/blogpost
- remove the logo for tags, just use icon

- mainly use clang-cl & clang
- add complete cmake project structure
    - add clangd, clang tidy
    - add conan: use fmt, use gtest
- remove any rtti / heap related cpp23, make it static through and through