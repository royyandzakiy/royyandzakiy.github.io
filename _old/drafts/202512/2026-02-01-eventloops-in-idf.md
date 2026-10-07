evtloop in esp idf

this framework is idfs take in rtos. the way it works is like a combination a task, with a queue built in, with an event mgr tightly coupled. we need to formally register any handlers that will care about which events. i persoanlly this styl of api to be quite verbose and tedious, but makes it clear if you have any event bits that are unused

for those developong with esp and would want the very best prrformance, i strongly suggest to use this native api. its also used everwhere in their idf sdk, so to debug well, you should best be comfortable and even proficient eith this framework

https://github.com/royyandzakiy/esp-rtos-event-loop/blob/master/src%2Fmain.cpp