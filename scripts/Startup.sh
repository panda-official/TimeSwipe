#!/bin/bash


echo 25 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio25/direction

#echo 8 > /sys/class/gpio/export
#echo out > /sys/class/gpio/gpio8/direction
#echo 1 > /sys/class/gpio/gpio8/value

#reset:

echo 17 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio17/direction
#echo 0 > /sys/class/gpio/gpio17/value
#sleep 0.1
echo 1 > /sys/class/gpio/gpio17/value

#reset complite

#09.08.2019: toggle CS

#sleep 0.1
#echo 0 > /sys/class/gpio/gpio8/value
#sleep 0.3
#echo 1 > /sys/class/gpio/gpio8/value




