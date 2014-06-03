tq2440
======

timer_led

insmod timer_leds.ko
mknod /dev/ex-leds c 232 0
./leds
