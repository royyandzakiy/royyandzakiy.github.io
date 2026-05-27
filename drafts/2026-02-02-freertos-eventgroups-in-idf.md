freertos eventgroups in idf

a common tool in using rtos is eventgroups. in this particular freertos implementation here we have different tasks waiting to act when they detect specific event flags being active

this is a nice way to develop bcs it helps in seperation of concerns, enabling these tasks to be event driven, rather than working in sequence. this also enables to free the cpu, bcs eventeaitbit here makes them to wait without taking any resources (its not like a sleep or while(1))

https://github.com/royyandzakiy/esp-freertos-eventgroups/blob/master/src%2Fmain.cpp