---
date:   2026-04-06 13:03:03 +0700
title:  Lefticus - C++ Project Template
layout: post
categories: post
author: "Royyan"
tags: cpp
comments: true
---

I have been slowly adding more stuff for my complete [@royyandzakiy/cpp-project-template](https://github.com/royyandzakiy/cpp-project-template). Lately I've been focusing on adding things to do in the CI, and I was re-watching Jason's video on HIS version of C++ project template.

<div class="video-container">
  <iframe src="https://youtu.be/YbgH7yat-Jo?si=aQe3pZWKQuQBzMV3" 
          frameborder="0" 
          allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" 
          allowfullscreen>
  </iframe>
</div>

Based on the video, here is a list of all the tools, libraries, and features that are used, added to, or used along with the cpp_starter_project:

Build & Configuration Tools:

- CMake
- CTest
- Conan (package manager)
- CCache

Code Quality & Formatting Tools:

- clang-format
- cmake-format
- clang-tidy
- CPPCheck

Compilers & Sanitizers:

- GCC / Clang / MSVC support
- Address Sanitizer
- Undefined Behavior Sanitizer
- Thread Sanitizer
- Fuzzing Sanitizer (libFuzzer from Clang/LLVM)

Libraries (managed via Conan):

- Catch2 (testing framework)
- docopt (command-line option parsing)
- fmt (formatting library, now partly in C++20)
- spdlog (fast logging)
- imgui-sfml (for IMGUI example)
- SFML (pulled in by imgui-sfml)
- SDL (for SDL example)

Documentation:

- Doxygen

Example GUI Libraries Shown:

- Qt
- FLTK
- GTKMM
- IMGUI
- NANA
- SDL

Editors/IDEs Mentioned:

- Neovim (with Neoformat plugin)

Other Features:

- Inter-procedural optimization (IPO) / Link Time Optimization (LTO)
- Precompiled headers (PCH)
- compile_commands.json generation (for clang tooling)
- Warnings as errors (-Werror / /WX)