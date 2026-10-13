---
title: Projects
command: tree projects/
---

<pre class="tree"><span class="d">projects/</span>
├── <span class="d">zephyr/</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-modern-cpp">zephyr-modern-cpp</a>                 <span class="c cm"># starter: C++17/20/23 on Zephyr</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-cpp-template">zephyr-cpp-template</a>               <span class="c cm"># nRF Connect SDK C++ app template</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-devcontainer">zephyr-devcontainer</a>               <span class="c cm"># devcontainer + CI images on GHCR</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-testing-workshop">zephyr-testing-workshop</a>           <span class="c cm"># ztest, FFF, GoogleTest, Twister</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-pytest">zephyr-pytest</a>                     <span class="c cm"># pytest + Twister, native_sim, nRF53</span>
│   ├── <a href="https://github.com/royyandzakiy/zephyr-ztest-gpio-emul">zephyr-ztest-gpio-emul</a>            <span class="c cm"># testing logic with GPIO emulation</span>
│   └── <a href="https://github.com/royyandzakiy/zephyr-ai-lab">zephyr-ai-lab</a>                     <span class="c cm"># AI coding agents on Zephyr firmware</span>
├── <span class="d">esp32/</span>
│   ├── <a href="https://github.com/royyandzakiy/esp-freertos-tutorial">esp-freertos-tutorial</a>             <span class="c cm"># FreeRTOS tasks & sync, hands-on</span>
│   ├── <a href="https://github.com/royyandzakiy/idf-qemu-freertos-task">idf-qemu-freertos-task</a>            <span class="c cm"># FreeRTOS on ESP-IDF, runs in QEMU</span>
│   ├── <a href="https://github.com/royyandzakiy/esp-idf-ulp-bmi160-buoy">esp-idf-ulp-bmi160-buoy</a>           <span class="c cm"># ULP RISC-V reads BMI160 in sleep</span>
│   ├── <a href="https://github.com/royyandzakiy/multisensor-station">multisensor-station</a>               <span class="c cm"># sensors, MQTT, OTA, redundancy</span>
│   ├── <a href="https://github.com/royyandzakiy/nanopb-esp32">nanopb-esp32</a>                      <span class="c cm"># protobuf messages with nanopb</span>
│   ├── <a href="https://github.com/royyandzakiy/memfault-bare-minimum">memfault-bare-minimum</a>             <span class="c cm"># Memfault: Wi-Fi, OTA, heartbeat</span>
│   ├── <a href="https://github.com/royyandzakiy/esp-idf-hello-pytest">esp-idf-hello-pytest</a>              <span class="c cm"># ESP32-S3 tests, pytest-embedded</span>
│   ├── <a href="https://github.com/royyandzakiy/unittesting-espidf-pio-gtest">unittesting-espidf-pio-gtest</a>      <span class="c cm"># GoogleTest in PlatformIO / IDF</span>
│   ├── <a href="https://github.com/royyandzakiy/esp32-freertos-examples">esp32-freertos-examples</a>           <span class="c cm"># early FreeRTOS examples</span>
│   └── <a href="https://github.com/royyandzakiy/esp32-arduino-uart">esp32-arduino-uart</a>                <span class="c cm"># ESP32 to Arduino over serial</span>
├── <span class="d">wireless/</span>
│   ├── <a href="https://github.com/royyandzakiy/LoRa-RHMesh">LoRa-RHMesh</a>                       <span class="c cm"># LoRa mesh with RadioHead</span>
│   ├── <a href="https://github.com/royyandzakiy/LoRa-RoutingTable">LoRa-RoutingTable</a>                 <span class="c cm"># LoRa routing table from RSSI</span>
│   ├── <a href="https://github.com/royyandzakiy/esp32-uart-ble-bridge">esp32-uart-ble-bridge</a>             <span class="c cm"># BLE client to UART bridge</span>
│   ├── <a href="https://github.com/royyandzakiy/xiaomi-scale">xiaomi-scale</a>                      <span class="c cm"># read a Xiaomi scale over BLE</span>
│   └── <a href="https://github.com/royyandzakiy/wifiheatmap-scan-step">wifiheatmap-scan-step</a>             <span class="c cm"># servo-swept Wi-Fi RSSI heatmap</span>
├── <span class="d">linux/</span>
│   └── <a href="https://github.com/royyandzakiy/meta-roix">meta-roix</a>                         <span class="c cm"># Yocto layer + distro for RPi 4</span>
├── <span class="d">robotics/</span>
│   ├── <a href="https://github.com/royyandzakiy/flightcontroller">flightcontroller</a>                  <span class="c cm"># drone FC: MPU6050, ELRS, PID</span>
│   ├── <a href="https://github.com/royyandzakiy/fc-simulation">fc-simulation</a>                     <span class="c cm"># C++23 FC + Python sim, lockstep HIL</span>
│   ├── <a href="https://github.com/royyandzakiy/mavlink-waypoint-generator">mavlink-waypoint-generator</a>        <span class="c cm"># MAVLink mission generator</span>
│   ├── <a href="https://github.com/royyandzakiy/dynamics-and-control">dynamics-and-control</a>              <span class="c cm"># P, PI, PID control notebooks</span>
│   └── <a href="https://github.com/royyandzakiy/pybullet_mini_control_projects">pybullet_mini_control_projects</a>    <span class="c cm"># PyBullet cartpole, balancing, IK</span>
├── <span class="d">cpp/</span>
│   ├── <a href="https://github.com/royyandzakiy/statemachine-device-manager">statemachine-device-manager</a>       <span class="c cm"># type-safe FSMs via templates</span>
│   ├── <a href="https://github.com/royyandzakiy/gmock-sfinae-concepts-calculator">gmock-sfinae-concepts-calculator</a>  <span class="c cm"># mocking with SFINAE & concepts</span>
│   ├── <a href="https://github.com/royyandzakiy/cmake-spdlog-logger">cmake-spdlog-logger</a>               <span class="c cm"># structured spdlog wrapper, C++20</span>
│   ├── <a href="https://github.com/royyandzakiy/cpp-tracy-project">cpp-tracy-project</a>                 <span class="c cm"># profiling threads with Tracy</span>
│   ├── <a href="https://github.com/royyandzakiy/fuzzer_parser">fuzzer_parser</a>                     <span class="c cm"># libFuzzer + ASan/UBSan on a parser</span>
│   ├── <a href="https://github.com/royyandzakiy/function-pass-ref">function-pass-ref</a>                 <span class="c cm"># passing functions by reference</span>
│   └── <a href="https://github.com/royyandzakiy/royyandzakiy.github.io">royyandzakiy.github.io</a>            <span class="c cm"># this site and its C++ generator</span>
├── <span class="d">templates/</span>
│   ├── <a href="https://github.com/royyandzakiy/cpp-project-template">cpp-project-template</a>              <span class="c cm"># CMake, vcpkg, tests, CI</span>
│   ├── <a href="https://github.com/royyandzakiy/cpp-project-template-min">cpp-project-template-min</a>         <span class="c cm"># a more minimalistic template</span>
│   ├── <a href="https://github.com/royyandzakiy/windows-cpp-template">windows-cpp-template</a>              <span class="c cm"># C++23 on Windows: vcpkg, WinRT</span>
│   ├── <a href="https://github.com/royyandzakiy/windows-lib-cpp-template">windows-lib-cpp-template</a>          <span class="c cm"># static/dynamic libs on Windows</span>
│   ├── <a href="https://github.com/royyandzakiy/cmake-dll-example">cmake-dll-example</a>                 <span class="c cm"># building and using a Windows DLL</span>
│   └── <a href="https://github.com/royyandzakiy/qml-project-template">qml-project-template</a>              <span class="c cm"># Qt 6 QML app in C++23, tested</span>
└── <span class="d">misc/</span>
    ├── <a href="https://github.com/royyandzakiy/msteams-live-caption-capture">msteams-live-caption-capture</a>      <span class="c cm"># export live Teams captions</span>
    ├── <a href="https://github.com/royyandzakiy/RocketManiaGame">RocketManiaGame</a>                   <span class="c cm"># a small rocket game</span>
    ├── <a href="https://github.com/royyandzakiy/TheSentinelWatcher">TheSentinelWatcher</a>                <span class="c cm"># Android sound recorder & detector</span>
    ├── <a href="https://github.com/royyandzakiy/presensi-km-itb">presensi-km-itb</a>                   <span class="c cm"># meeting attendance web app</span>
    └── <a href="https://github.com/royyandzakiy/ojek-online">ojek-online</a>                       <span class="c cm"># ride-hailing web app, PHP</span>

<span class="c">8 directories, 46 projects</span></pre>

<p class="out c"># Everything public on <a href="https://github.com/royyandzakiy">GitHub</a>. Work history on <a href="https://www.linkedin.com/in/royyandzakiy/">LinkedIn</a>.</p>
