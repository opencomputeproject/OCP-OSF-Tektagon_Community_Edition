#!/bin/bash
# ------------------------------------------------------------------------
# prerequisite:
#   1. EVB: boot strap is set to "UART boot"
#   2. EVB: serial UART5 is working
#   3. prepare uart_zephyr.bin
#   4. download bin-xfer from https://gist.github.com/cstrahan/5796653
#   5. sudo chmod +x /usr/bin/bin-xfer.sh
#   6. modify variables 'DEV' and 'FILE' according to yout local envrionment
#
# usage:
#    run 'sh aspeed_uart_download.sh'
# ------------------------------------------------------------------------
DEV=/dev/ttyS12
FILE=/mnt/d/tmp/uart_zephyr.bin

stty -F $DEV 115200 raw cs8 -ixoff -cstopb -parenb
bin-xfer.sh -i $FILE -o $DEV
