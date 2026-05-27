/**
Can we use the latest c++ features on microcontrollers? I've been testing out to use it on Zephyr and ESP-IDF

...

Check it out!
https://github.com/royyandzakiy/zephyr-modern-cpp
 * 
 */

// --- Thread stack definitions ---
K_THREAD_STACK_DEFINE(state_mon_stack1, 4096);

// --- Thread structures ---
struct k_thread state_mon_thread1;

void state_monitor_thread([[maybe_unused]] void *p1, [[maybe_unused]] void *p2, [[maybe_unused]] void *p3) {
	const auto thread_id = reinterpret_cast<intptr_t>(p1);
	StateMachineManager manager;

	while (true) {
		// if with initializer
		if (auto state = manager.get_state_id(); state == StateId::ALERT) {
			LOG_WRN("Thread %ld: CRITICAL ALERT STATE", static_cast<long>(thread_id));
		}

		manager.update();
		zephyr_sleep_for(STATE_UPDATE_INTERVAL);
	}
}

k_thread_create(&state_mon_thread1, state_mon_stack1, K_THREAD_STACK_SIZEOF(state_mon_stack1), state_monitor_thread,
					reinterpret_cast<void *>(1), nullptr, nullptr, K_PRIO_COOP(5), 0, K_NO_WAIT);

/**
// OUTPUT
-- [00:00:45.277,923] <inf> state_machine: State: Status: Monitoring [Avg: 24.05°C | Samples: 47] | Buffer: 47 | Range: [23.52, 24.45]
-- [00:00:45.278,045] <inf> state_machine: State: Status: Monitoring [Avg: 24.15°C | Samples: 47] | Buffer: 47 | Range: [23.93, 24.48]
-- [00:00:45.278,167] <inf> state_machine: State: Status: Monitoring [Avg: 24.05°C | Samples: 47] | Buffer: 47 | Range: [23.78, 24.41]
-- [00:00:46.261,352] <inf> state_machine: State: Status: Monitoring [Avg: 24.04°C | Samples: 24] | Buffer: 24 | Range: [23.74, 24.38]
 */