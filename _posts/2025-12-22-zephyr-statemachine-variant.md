---
date:   2025-12-22 13:03:03 +0700
title:  Zephyr Modern C++ - Statemachines using std::variant/std::visit
layout: post
categories: post
author: "Royyan"
tags: cpp
comments: true
---
Can we use the latest C++ features on microcontrollers? Yes. 

I've been testing out to use it on Zephyr and ESP-IDF

One particular feature I tried out in this example is the use of std::variant for different States, and ultimately using std::visit to iterate through them. Here I provide implementation of a general get_state_info to read what the current state is using the Overloaded Lambda idiom (to enforce availability of all StateVariants), it will then return a std::string_view of different messages based on the current_state_

Let's say, a more shiny Statemachine haha. A different approach than using bare enums and switch statements (not simply saying they are less)

Check it out here!

[github.com/royyandzakiy/zephyr-modern-cpp](https://github.com/royyandzakiy/zephyr-modern-cpp)

```cpp
enum class StateId {
	IDLE,
	MONITORING,
	ALERT,
	CALIBRATING
};
struct IdleState {};
struct MonitoringState {
	int32_t average_temp_value;
	int sample_count;
};

struct AlertState {
	std::string_view message;
	int32_t threshold_temp_value;
};

struct CalibratingState {
	int32_t reference_temp_value;
	int calibration_step;
};

using StateVariant = std::variant<std::monostate, IdleState, MonitoringState, AlertState, CalibratingState>;

class StateMachine {
    // ...
    auto get_state_info() const -> std::string_view {
		return std::visit(Overloaded{
			[this](const IdleState&) -> std::string_view {
				return "Status: Idle - Awaiting for sensor trigger"sv;
			},
			[this](const MonitoringState& s) -> std::string_view {
				// psst: not using std::format because the zephyr SDK does not have #include <format>
				int len = snprintf(format_buffer_.data(), format_buffer_.size(), 
									"Status: Monitoring [Avg: %d.%02d°C | Samples: %d]",
									ipart(s.average_temp_value), fpart(s.average_temp_value), s.sample_count);
				return { format_buffer_.data(), static_cast<size_t>(len) };
			},
			[this](const AlertState& s) -> std::string_view {
				int len = snprintf(format_buffer_.data(), format_buffer_.size(), 
								"Status: ALERT [%.*s | Threshold: %d.%02d°C]", 
								static_cast<int>(s.message.length()), s.message.data(),
								ipart(s.threshold_temp_value), fpart(s.threshold_temp_value));
				return {format_buffer_.data(), static_cast<size_t>(len)};
			},
			[this](const CalibratingState& s) -> std::string_view {
				int len = snprintf(format_buffer_.data(), format_buffer_.size(), 
								"Status: Calibrating [Ref: %d.%02d°C | Step: %d/5]", 
								ipart(s.reference_temp_value), fpart(s.reference_temp_value), s.calibration_step);
				return {format_buffer_.data(), static_cast<size_t>(len)};
			},
			[this](const auto&) -> std::string_view {
				// default fallback
				return "Status: Unknown State! [Error]"sv;
			}
		}, current_state_);
	}
    // ...
};

auto main() -> int {
    StateMachine sm_;
	TemperatureSensor t_;
	HumiditySensor h_;
	PressureSensor p_;
    
    sm_.process_sensors(t_, h_, p_);

    // ...

    auto info = sm_.get_state_info();

    return 0;
}
```