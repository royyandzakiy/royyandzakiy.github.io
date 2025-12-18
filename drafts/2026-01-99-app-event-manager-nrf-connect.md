appevtngr in zephyr (jan)

another design of event driven architecture is the aem in zeohyr. this subsystem/frameworks enforces its users to strcture their code in moduels. each modules conencted to the event driven backend will need to explicitly sub ir listen to another module to get notified of any evts happening

to me this arch style really helps when i need to isolate a particular module to debug or test easily. i could simply rempve it from the cmakelist, and logically, the tight coupling design will not affect other modules

https://docs.nordicsemi.com/bundle/ncs-3.0.2/page/nrf/libraries/others/app_event_manager.html#