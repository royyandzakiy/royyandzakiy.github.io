/**
Can we use the latest c++ features on microcontrollers? I've been testing out to use it on Zephyr and ESP-IDF

Here is the continuation of the last post.

This time I try using Concepts to iterate through a bunch of "Sensors". Here I use the generic function process_sensors that utilizes C++ Template Metaprogramming and uses the Ellipsis "..." operator to process through an arbritary amount of sensors (in this case, 3). The result is an expressive and elegant code (feels like I'm coding in javascript with the Spread Operator haha). Here the std::span stores the raw reading of each sensors, invoked each time this process_sensors gets called.

Is it the most efficient for when we need to optimize for firmware size, nope. Is it fun? Yesh

Check it out!
https://github.com/royyandzakiy/zephyr-modern-cpp
 * 
 */

 /**
  * 1
  * 
  */
// --- Concepts & Constraints ---
template <typename T>
concept SensorType = requires(T t) {
	{ t.read() } -> std::convertible_to<int32_t>;
	{ t.get_id() } -> std::convertible_to<int>;
};

// --- Sensor Concepts Implementation ---
class TemperatureSensor {
  public:
	auto read() const -> int32_t {
		return 2350 + (sys_rand32_get() % 100); // 23.50 – 24.49
	}
	auto get_id() const -> int {
		return 1;
	}
};

class HumiditySensor {
  public:
	auto read() const -> int32_t {
		return 4500 + (sys_rand32_get() % 200); // 45.00 – 46.99
	}
	auto get_id() const -> int {
		return 2;
	}
};

class PressureSensor {
  public:
	auto read() const -> int32_t {
		return 101325 + (sys_rand32_get() % 500); // 1013.25+
	}
	auto get_id() const -> int {
		return 3;
	}
};

/**
 * 2
 */
template <SensorType... Sensors> 
auto process_sensors(Sensors &&...sensors) -> void {
    const std::array measurements{sensors.read()...};

    int sensorIdx = 0;
    (
        [&]<typename T>(T &sensor) {
            const char *label = "Unknown";
            if constexpr (std::is_same_v<T, TemperatureSensor>)
                label = "Temp";
            if constexpr (std::is_same_v<T, HumiditySensor>)
                label = "Humid";
            if constexpr (std::is_same_v<T, PressureSensor>)
                label = "Press";

            LOG_INF("Sensor: %s | Value: %d", label, measurements[sensorIdx++]);
        }(sensors),
    ...);
}

auto main() -> int {
    StateMachine sm_;
    TemperatureSensor t_;
    HumiditySensor h_;
    PressureSensor p_;

    sm_.process_sensors(t_, h_, p_ /*, othersensors_, othersensors2_, etc */);
    
    return 0;
}