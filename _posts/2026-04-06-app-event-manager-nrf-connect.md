---
date:   2026-04-06 13:03:03 +0700
title:  Zephyr App Event Manager (app_event_manager)
layout: post
categories: post
author: "Royyan"
tags: zephyr embedded
comments: true
---

Another design of event-driven architecture is the **AEM (app_event_manager)** in Zephyr. This subsystem/framework enforces its users to structure their code in modules. Each module connected to the event-driven backend will need to explicitly subscribe/listen to another module to get notified of any events happening.

To me, this architecture style really helps when I need to isolate a particular module to debug or test easily. I could simply remove it from the CMakeLists.txt, and logically, the tight coupling design will not affect other modules.

[https://docs.nordicsemi.com/bundle/ncs-3.0.2/page/nrf/libraries/others/app_event_manager.html](https://docs.nordicsemi.com/bundle/ncs-3.0.2/page/nrf/libraries/others/app_event_manager.html)

Here's a small snippet of how it looks in practice:

```cpp
/* --- Event Definition --- */
APP_EVENT_TYPE_DEFINE(sensor_data_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			APP_EVENT_TYPE_FLAG_INIT_LOG_ENABLE
		  ));

/* --- Module A: Publisher --- */
static void publish_sensor_data(int32_t value) {
	struct sensor_data_event *evt = new_sensor_data_event();
	evt->value = value;
	APP_EVENT_SUBMIT(evt);
}

/* --- Module B: Subscriber --- */
static bool app_event_handler(const app_event_header *aeh) {
	if (is_sensor_data_event(aeh)) {
		struct sensor_data_event *evt = cast_sensor_data_event(aeh);
		LOG_INF("Received sensor data: %d", evt->value);
		return true;
	}
	return false;
}

APP_EVENT_LISTENER_DEFINE(sensor_event_listener, app_event_handler);
APP_EVENT_SUBSCRIBE(sensor_event_listener, sensor_data_event);
```

With this approach, if I want to test Module B in isolation, I can simply remove Module A from the build, and the app still runs, it simply stops sending events. Really nice for unit testing and incremental development.