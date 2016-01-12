#!/bin/bash



###################################################################################################
# Raspberry Pi install script
#
# !!! BEFORE RUNNING THIS SCRIPT !!!
# Select 1 Expand Filesystem.
# Select 8 Advanced Options and then  A6 SPI - Enable/Disable automatic loading.
# Select 8 Advanced Options and then  A7 I2C - Enable/Disable automatic loading.
# Add "dtparam=i2c1=on" in /boot/config.txt.
# Add "i2c-dev" and "i2c-bcm2708" in /etc/modules.
# Follow [1] for more details on how to enable I2C on your raspberry pi.
#
# [1] https://www.abelectronics.co.uk/i2c-raspbian-wheezy/info.aspx
#
# Tested on 2015-11-21-raspbian-jessie-lite.
#
# Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
# Date last modified: December 2015
###################################################################################################



###################################################################################################
# Sanity checks
###################################################################################################

# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  printf 'ERROR: This script must be run as root\n' 1>&2
  exit 1
fi

# Check whether raspi-config is installed or not
dpkg -s raspi-config > /dev/null 2>&1
if [ $? -ne 0 ]; then
  printf 'ERROR: raspi-config not installed\n' 1>&2
  exit 1
fi

# Check minimum space required
MIN_SIZE=$((700 * 1024))
df_result=($(df / | tail -n 1))
if [ ${df_result[3]} -lt $MIN_SIZE ]; then
  printf 'ERROR: At least 700MB of space required\n' 1>&2
  exit 1
fi



###################################################################################################
# Script variables
###################################################################################################

SANDBOX_PATH=/sandbox
WVERSION=v3.8
LWVERSION=v2.0



###################################################################################################
# Actual installation
###################################################################################################

# Install some stuff
apt-get update
apt-get install -y git gcc g++ gcc-4.7 g++-4.7 make pkg-config libexpat1-dev libssl-dev           \
  libhiredis-dev dh-autoreconf libfuse-dev libcurl4-gnutls-dev libevent-dev redis-server          \
  supervisor vim python-dev libi2c-dev python-pip libjansson-dev cmake mc mplayer arduino minicom \
  picocom bluez fuse libusb-dev libbluetooth-dev bluetooth joystick wpasupplicant                 \
  python-smbus curl libicu-dev mpg123 firmware-ralink firmware-realtek wireless-tools
apt-get clean

# Use gcc and g++ 4.7
update-alternatives --remove-all gcc
update-alternatives --remove-all g++
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.6 1             \
  --slave /usr/bin/g++ g++ /usr/bin/g++-4.6
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 2             \
  --slave /usr/bin/g++ g++ /usr/bin/g++-4.7
update-alternatives --auto gcc

# Install some more stuff
pip install msgpack-python
pip install redis
pip install ino
pip install pyfirmata

# Create sandbox directory
mkdir -p $SANDBOX_PATH

# Install pybass
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/pybass.git
cd pybass
python setup.py install
cp lib/hardfp/libbass* /usr/local/lib
ldconfig
cd $SANDBOX_PATH
rm -rf pybass

# Install BrickPi
cd $SANDBOX_PATH
git clone https://github.com/DexterInd/BrickPi_Python.git
cd BrickPi_Python
python setup.py install
cd $SANDBOX_PATH
rm -rf BrickPi_Python

# Install libstrophe
cd $SANDBOX_PATH
git clone https://github.com/strophe/libstrophe.git
cd libstrophe
./bootstrap.sh
./configure --prefix=/usr
make
make install
cd $SANDBOX_PATH
rm -rf libstrophe

# Install node
cd $SANDBOX_PATH
wget https://gist.githubusercontent.com/raw/3245130/v0.10.24/node-v0.10.24-linux-arm-armv6j-vfp-hard.tar.gz
tar -xzf node-v0.10.24-linux-arm-armv6j-vfp-hard.tar.gz
rm -f node-v0.10.24-linux-arm-armv6j-vfp-hard.tar.gz
cd node-v0.10.24-linux-arm-armv6j-vfp-hard
cp -R * /usr
cd $SANDBOX_PATH
rm -rf node-v0.10.24-linux-arm-armv6j-vfp-hard

# Install wiringPi
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/wiringPi.git
cd wiringPi
./build
cd $SANDBOX_PATH
rm -rf wiringPi

# Install pcre
cd $SANDBOX_PATH
wget ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-8.38.tar.gz
tar -xzf pcre-8.38.tar.gz
rm -f pcre-8.38.tar.gz
cd pcre-8.38
./configure --prefix=/usr
make
make install
cd $SANDBOX_PATH
rm -rf pcre-8.38

# Install swig 3+
cd $SANDBOX_PATH
wget http://prdownloads.sourceforge.net/swig/swig-3.0.5.tar.gz
tar -xzf swig-3.0.5.tar.gz
rm -f swig-3.0.5.tar.gz
cd swig-3.0.5
./configure --prefix=/usr
make
make install
cd $SANDBOX_PATH
rm -rf swig-3.0.5

# Install libwyliodrin
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/libwyliodrin.git
cd libwyliodrin
git checkout $LWVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DRASPBERRYPI=ON ..
make
make install
cd $SANDBOX_PATH
cd libwyliodrin/wylio
make
make install
cd $SANDBOX_PATH
rm -rf libwyliodrin

# Run libwyliodrin scripts
install_social
update_streams

# Link wyliodrin module used in node-red
ln -s /usr/lib/node_modules /usr/lib/node

# Install wyliodrin-server
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/wyliodrin-server.git
cd wyliodrin-server
git checkout $WVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DRASPBERRYPI=ON ..
make
make install
cd $SANDBOX_PATH
cd wyliodrin-server/hypervisor
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install
cd $SANDBOX_PATH
rm -rf wyliodrin-server

# Set boardtype to raspberry
mkdir -p /etc/wyliodrin
echo -n raspberrypi > /etc/wyliodrin/boardtype

# Create settings_raspberry.json
printf '{
  "config_file":  "/boot/wyliodrin.json",
  "home":         "/wyliodrin",
  "mount_file":   "/wyliodrin/projects/mnt",
  "build_file":   "/wyliodrin/projects/build",
  "shell":        "bash",
  "run":          "sudo -E make -f Makefile.raspberrypi run",
  "stop":         "sudo kill -9",
  "poweroff":     "sudo poweroff",
  "logout":       "/etc/wyliodrin/logs.out",
  "logerr":       "/etc/wyliodrin/logs.err",
  "hlogout":      "/etc/wyliodrin/hlogs.out",
  "hlogerr":      "/etc/wyliodrin/hlogs.err"
}\n' > /etc/wyliodrin/settings_raspberrypi.json

# Create home
mkdir /wyliodrin

# Create mount and build directories
mkdir -p /wyliodrin/projects/mnt
mkdir -p /wyliodrin/projects/build

# Startup script
printf '
[supervisord]
[program:wyliodrind]
command=/usr/bin/wyliodrind
user=pi
autostart=true
autorestart=true
environment=HOME="/wyliodrin"
priority=20

[supervisord]
[program:wyliodrin_hypervisor]
command="/usr/bin/wyliodrin_hypervisor"
user=pi
autostart=true
autorestart=true
environment=HOME="/wyliodrin"
priority=10
' >> /etc/supervisor/supervisord.conf

# Wifi
cp /etc/network/interfaces /etc/network/interfaces.orig
printf 'auto lo
iface lo inet loopback
iface eth0 inet dhcp
allow-hotplug wlan0
auto wlan0
iface wlan0 inet manual
wpa-roam /etc/wyliodrin/wireless.conf
iface default inet dhcp
' > /etc/network/interfaces

# Change owner of directories used by wyliodrin
chown -R pi:pi /wyliodrin
chown -R pi:pi /etc/wyliodrin

# Add pi to the fuse group
usermod -a -G fuse pi

# Copy bashrc
cp /home/pi/.bashrc /wyliodrin

# Clean
apt-get clean
rm -rf $SANDBOX_PATH
rm -rf /home/pi/.npm
rm -rf /home/root/.npm

# Reboot
reboot
