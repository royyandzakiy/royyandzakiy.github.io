---
layout: post
title:  ESP-IDF Test Frameworks
date:   2025-10-05 13:03:03 +0700
categories: post
author: "Royyan"
tags: embedded
comments: true
---
I've lately been keen in learning test systems in the embedded, especially for esp-idf. I've been wanting to really understand how far I can take the automated testing and CI processes using built in tools of the espressif ecosystem, or common practices. This talk by Zim Kalinowski really fascinates me, knowing that within espressif, they have been implementing these full blown processes for the longest time. Really excited to ramp up my gear and try implementing these tools myself.

Some of the things he will talk about is as follows:
- Test framework Introduction
- Using PyTest with Unity
- Setting Up CI in GitHub
- Using CMock and several already mocked IDF components
- Using QEMU
- Using Self-hosted test runner for on target tests (hardware based)

[EDC22 Day 2 Talk 9: Leveraging ESP IDF Test Framework in Third-Party Projects](https://www.youtube.com/watch?v=HQcTWjOv8fM)

<div class="video-container">
  <iframe src="https://www.youtube.com/embed/HQcTWjOv8fM" 
          frameborder="0" 
          allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" 
          allowfullscreen>
  </iframe>
</div>