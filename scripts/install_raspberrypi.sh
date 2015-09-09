#!/bin/bash

###################################################################################################
# Raspberry Pi install script
#
# !!! BEFORE RUNNING THIS SCRIPT !!!
# apt-get install raspi-config
# raspi-config
# Select 1 Expand Filesystem.
# Select 8 Advanced Options and then  A7 I2C - Enable/Disable automatic loading.
# A prompt will appear asking "Would you like the ARM I2C interface to be
# enabled?". Select Yes, exit the utility and reboot your raspberry pi.
# Add "dtparam=i2c1=on" and "dtparam=i2c_arm=on" in /boot/config.txt.
# Add "i2c-dev" in /etc/modules.
# Follow [1] for more details on how to enable I2C on your raspberry pi.
# Add "/usr/local/bin/supervisord -c /etc/supervisord.conf" in /etc/rc.local.
#
# [1] https://www.abelectronics.co.uk/i2c-raspbian-wheezy/info.aspx
###################################################################################################



###################################################################################################
# Sanity checks
###################################################################################################

# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  echo ""
  echo "***************************************"
  echo "*** This script must be run as root ***"
  echo "***************************************"
  echo ""
  exit 1
fi

# Check whether raspi-config is installed or not
dpkg -s raspi-config > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo ""
  echo "**************************************************************************"
  echo "*** Please follow the steps described above before running this script ***"
  echo "**************************************************************************"
  echo ""
  exit 1
fi

# Check minimum space required
MIN_SIZE=$((3 * 1024 * 1024))
df_result=($(df / | tail -n 1))
if [ ${df_result[1]} -lt $MIN_SIZE ]; then
  echo ""
  echo "**************************************"
  echo "*** At least 3GB of space required ***"
  echo "**************************************"
  echo ""
  exit 1
fi



###################################################################################################
# Script variables
###################################################################################################

SANDBOX_PATH=/sandbox
WVERSION=v2.1
LWVERSION=v1.16



###################################################################################################
# Actual installation
###################################################################################################

# Install some stuff
apt-get update
apt-get install -y git gcc g++ gcc-4.7 g++-4.7 make pkg-config libexpat1-dev  \
  libssl-dev libhiredis-dev dh-autoreconf libfuse-dev libcurl4-gnutls-dev     \
  libevent-dev redis-server supervisor python-dev libi2c-dev python-pip libjansson-dev   \
  cmake mc mplayer arduino minicom picocom bluez-utils bluez-compat           \
  bluez-hcidump libusb-dev libbluetooth-dev bluetooth joystick python-smbus   \
  curl libicu-dev mpg123 firmware-ralink firmware-realtek wireless-tools      \
  wpasupplicant
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

# Install BrickPi
cd $SANDBOX_PATH
git clone https://github.com/DexterInd/BrickPi_Python.git
cd BrickPi_Python
python setup.py install

# Install libstrophe
cd $SANDBOX_PATH
git clone https://github.com/strophe/libstrophe.git
cd libstrophe
./bootstrap.sh
./configure --prefix=/usr
make
make install

# Install node
cd $SANDBOX_PATH
wget https://nodejs.org/dist/v0.10.28/node-v0.10.28.tar.gz
tar -xzf node-v0.10.28.tar.gz
rm node-v0.10.28.tar.gz
cd node-v0.10.28
./configure --prefix=/usr
make
make install
cd ..
rm -rfv node-v0.10.28
# wget http://node-arm.herokuapp.com/node_latest_armhf.deb
# dpkg -i node_latest_armhf.deb
# rm -rf node_latest_armhf.deb

# Install serialport
cd $SANDBOX_PATH
npm install -g serialport

# Install wiringPi
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/wiringPi.git
cd wiringPi
sed 's/sudo//g' build > build2
chmod +x build2
./build2

# Install pcre
cd $SANDBOX_PATH
wget ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-8.36.tar.gz
tar -xzf pcre-8.36.tar.gz
rm pcre-8.36.tar.gz
cd pcre-8.36
./configure --prefix=/usr
make
make install

# Install swig 3+
cd $SANDBOX_PATH
wget http://prdownloads.sourceforge.net/swig/swig-3.0.5.tar.gz
tar -xzf swig-3.0.5.tar.gz
rm swig-3.0.5.tar.gz
cd swig-3.0.5
./configure --prefix=/usr
make
make install

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

# Run libwyliodrin scripts
install_social
update_streams

# Link wyliodrin module used in node-red
ln -s /usr/lib/node_modules/wyliodrin /wyliodrin/node-red/node_modules/wyliodrin

# Install wyliodrin-server
cd $SANDBOX_PATH
git clone https://github.com/alexandruradovici/wyliodrin-server.git
cd wyliodrin-server
git checkout $WVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install

# Set boardtype to raspberry
mkdir /etc/wyliodrin
echo -n raspberrypi > /etc/wyliodrin/boardtype

# Create mount and build directories
mkdir /etc/wyliodrin/mnt
mkdir /etc/wyliodrin/build

# Create settings_raspberry.json
printf "{\n\
  \"config_file\": \"/boot/wyliodrin.json\",\n\
  \"mountFile\": \"/etc/wyliodrin/mnt\",\n\
  \"buildFile\": \"/etc/wyliodrin/build\",\n\
  \"board\": \"raspberrypi\"\n\
}\n" > /etc/wyliodrin/settings_raspberrypi.json

# Create running_projects file
touch /etc/wyliodrin/running_projects

# I2C support
cd $SANDBOX_PATH
apt-get install -y python3-dev
wget http://ftp.de.debian.org/debian/pool/main/i/i2c-tools/i2c-tools_3.1.0.orig.tar.bz2
tar xf i2c-tools_3.1.0.orig.tar.bz2
rm i2c-tools_3.1.0.orig.tar.bz2
cd i2c-tools-3.1.0/py-smbus
mv smbusmodule.c smbusmodule.c.orig
wget https://raw.githubusercontent.com/abelectronicsuk/ABElectronics_Python3_Libraries/master/smbusmodule.c
python3 setup.py build
python3 setup.py install

# Startup script
sh -c 'printf "\
[supervisord]\n\
[program:wtalk]\n\
command=/usr/bin/wyliodrind\n"\
>> /etc/supervisord.conf'

# Wifi
cp /etc/network/interfaces /etc/network/interfaces.orig
printf "# do not edit by hand\n\
auto lo\n\
\n\
iface lo inet loopback\n\
iface eth0 inet dhcp\n\
\n\
allow-hotplug wlan0\n\
iface wlan0 inet manual\n\
\n\
wpa-roam /etc/wyliodrin/wireless.conf\n\
iface default inet dhcp\n" > /etc/network/interfaces.wyliodrin

# Clean
rm -rf $SANDBOX_PATH
