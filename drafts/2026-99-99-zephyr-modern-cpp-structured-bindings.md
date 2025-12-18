/**
Can we use the latest c++ features on microcontrollers? I've been testing out to use it on Zephyr and ESP-IDF

...

Check it out!
https://github.com/royyandzakiy/zephyr-modern-cpp
 * 
 */

auto get_history_range() const -> std::tuple<int32_t, int32_t> {
    if (buffer_index_ == 0) {
        return {0, 0};
    }

    const size_t history_count = std::min(buffer_index_, temp_data_circbuf_.size());

    int32_t min_temp = temp_data_circbuf_[0];
    int32_t max_temp = temp_data_circbuf_[0];

    for (size_t i = 1; i < history_count; ++i) {
        min_temp = std::min(min_temp, temp_data_circbuf_[i]);
        max_temp = std::max(max_temp, temp_data_circbuf_[i]);
    }

    return {min_temp, max_temp};
}

auto [min_temp, max_temp] = sm_.get_history_range();