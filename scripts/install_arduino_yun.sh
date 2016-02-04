#!/bin/bash



###################################################################################################
# Arduino Yun install script
#
# Memory expand -> https://www.arduino.cc/en/Tutorial/ExpandingYunDiskSpace
# gcc -> http://playground.arduino.cc/Hardware/Yun
#
# Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
# Date last modified: January 2016
###################################################################################################


#

opkg install git
opkg install unzip

wget --no-check-certificates https://cmake.org/files/v3.4/cmake-3.4.1.tar.gz
