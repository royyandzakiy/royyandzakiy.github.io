/**
Can we use the latest c++ features on microcontrollers? Yes. 

I've been testing out to use it on Zephyr and ESP-IDF

One particular feature I tried out in this example is the use of std::variant for different States, and ultimately using std::visit to iterate through them. Here I provide implementation of a general get_state_info to read what the current state is using std::is_same type_trait, and return an std::string_view of different messages based on the stored current_state_

Let's say, a more shiny Statemachine haha. A different approach than using bare enums and switch statements (not to say they are anywhere less)

Check it out!
https://github.com/royyandzakiy/zephyr-modern-cpp

 */

/**
 * 1
 * 
 */
// --- State Variant Definition ---
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

// monostate enables the StateVariant type to be initialized without specifying the exact Variant type
using StateVariant = std::variant<std::monostate, IdleState, MonitoringState, AlertState, CalibratingState>;

/**
 * 2
 */
auto get_state_info() const -> std::string_view {
    // clang-format off
    return std::visit(Overloaded{
        // this way, compiler enforces all Variant types to be covered, or else compilation error
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
    // clang-format on
}