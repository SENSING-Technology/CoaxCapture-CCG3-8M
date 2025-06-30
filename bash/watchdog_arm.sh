#!/bin/bash
sudo ../tools/serdes_cfg /dev/xdma0_user 100 "echo 1 > ./sys/devices/platform/gpio-pwm/watchdog_status"
