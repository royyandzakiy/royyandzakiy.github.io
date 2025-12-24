---
date:   2025-12-26 13:03:03 +0700
title:  Zephyr Modern C++ - Sensors Concept
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

class StateMachine {
    // ...
    template <SensorType... Sensors> auto process_sensors(Sensors &&...sensors) -> void {
		const std::array measurements{sensors.read()...};
		int temperatureSensorIdx = 0;

		// all sensor logic
		{
			int sensorIdx = 0;
			(
				[&]<typename T>(T &sensor) {
					const char *label = "Unknown";
					if constexpr (std::is_same_v<T, TemperatureSensor>) {
						label = "Temp";
						temperatureSensorIdx = sensorIdx;
					}
					if constexpr (std::is_same_v<T, HumiditySensor>)
						label = "Humid";
					if constexpr (std::is_same_v<T, PressureSensor>)
						label = "Press";

					LOG_INF("Sensor: %s | Value: %d", label, measurements[sensorIdx++]);
				}(sensors),
				...);
		}

		// TemperatureSensor value will cause system State change
		{
			const int32_t latest_temp = measurements[0];
			const size_t stored_count = std::min(buffer_index_, temp_data_circbuf_.size());

			// Update circular buffer
			temp_data_circbuf_[buffer_index_ % temp_data_circbuf_.size()] = latest_temp;
			buffer_index_++;

			std::visit(
				[this, latest_temp, stored_count]<typename Traw>(Traw &state) {
					using T = std::decay_t<Traw>; // cleanup any const/refs

					// this way, enforcement to ensure all Variant types are covered by the compiler becomes optional
					// (else, via static_assert). if I ommit the else, we can have a non-exhaustive "state machine"
					if constexpr (std::is_same_v<T, IdleState>) {
						if (latest_temp > TEMP_THRESHOLD_START_MONITORING) {
							transition_to(MonitoringState{latest_temp, 1});
						}
					} else if constexpr (std::is_same_v<T, MonitoringState>) {
						state.sample_count++;

						auto sum =
							std::accumulate(temp_data_circbuf_.begin(), temp_data_circbuf_.begin() + stored_count, 0LL);

						state.average_temp_value = static_cast<int32_t>(sum / stored_count);

						if (state.average_temp_value > TEMP_THRESHOLD_ALERT) {
							transition_to(AlertState{"Temperature High"sv, TEMP_THRESHOLD_ALERT});
						}
					} else if constexpr (std::is_same_v<T, AlertState>) {
						if (latest_temp < TEMP_THRESHOLD_RECOVERY) {
							transition_to(CalibratingState{CALIBRATION_REFERENCE_TEMP, 1});
						}
					} else if constexpr (std::is_same_v<T, CalibratingState>) {
						if (++state.calibration_step > MAX_CALIBRATION_STEPS) {
							transition_to(IdleState{});
						}
					} else if constexpr (std::is_same_v<T, std::monostate>) {
						transition_to(IdleState{});
					} else {
						// C++23: If a new state is added to StateVariant but not handled yet, force compile error
						static_assert(uncovered_type_exists<T>,
									  "Non-exhaustive visitor: You forgot to handle a state!");
					}
				},
				current_state_);
		}
	}
};

auto main() -> int {
    StateMachine sm_;
	TemperatureSensor t_;
	HumiditySensor h_;
	PressureSensor p_;

    sm_.process_sensors(t_, h_, p_);

    return 0;
}
```