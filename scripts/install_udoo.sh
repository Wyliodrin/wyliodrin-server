#!/bin/bash



###################################################################################################
# UDOO Neo install script
#
# ssh udooer@192.168.7.2 when powered via usb
# password: udooer
#
# sudo visudo -> udooer ALL=(ALL) NOPASSWD: ALL
#
# Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
# Date last modified: January 2016
###################################################################################################



###################################################################################################
# Script variables
###################################################################################################

SANDBOX_PATH=/sandbox
WVERSION=v3.15
LWVERSION=v2.1



###################################################################################################
# Sanity checks
###################################################################################################

# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  printf 'ERROR: This script must be run as root\n' 1>&2
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
# Actual installation
###################################################################################################

# Install some stuff
apt-get update
apt-get install -y cmake libexpat1-dev libssl-dev libhiredis-dev libfuse-dev libcurl4-gnutls-dev \
  libevent-dev libjansson-dev libtool redis-server supervisor python-dev xvfb libbluetooth-dev   \
  libusb-dev libudev-dev libusb-1.0-0-dev libi2c-dev
apt-get clean

pip install redis

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

# Link node
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
echo "$WVERSION" > /etc/wyliodrin/version

# Install wyliodrin-shell
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/wyliodrin-shell.git
cd wyliodrin-shell
npm install
npm install grunt-cli
./node_modules/grunt-cli/bin/grunt build
rm -rf gruntfile.js package.json public/ server/
mv tmp/* .
rm -rf tmp/
mkdir -p /usr/wyliodrin/wyliodrin-shell
cp -rf * /usr/wyliodrin/wyliodrin-shell
cd $SANDBOX_PATH
rm -rf wyliodrin-shell

mkdir -p /etc/wyliodrin
echo -n raspberrypi > /etc/wyliodrin/boardtype

mkdir -p /wyliodrin/projects/mnt
mkdir -p /wyliodrin/projects/build

printf '{
  "config_file":  "/boot/wyliodrin.json",
  "home":         "/wyliodrin",
  "mount_file":   "/wyliodrin/projects/mnt",
  "build_file":   "/wyliodrin/projects/build",
  "shell":        "bash",
  "run":          "sudo -E make -f Makefile.udoo run",
  "stop":         "sudo kill -9",
  "poweroff":     "sudo poweroff",
  "logout":       "/etc/wyliodrin/logs.out",
  "logerr":       "/etc/wyliodrin/logs.err",
  "hlogout":      "/etc/wyliodrin/hlogs.out",
  "hlogerr":      "/etc/wyliodrin/hlogs.err"
}\n' > /etc/wyliodrin/settings_udoo.json

# Startup script
printf '
[supervisord]
[program:wyliodrind]
command=/usr/bin/wyliodrind
user=udooer
autostart=true
autorestart=true
environment=HOME="/wyliodrin"
priority=20

[supervisord]
[program:wyliodrin_hypervisor]
command="/usr/bin/wyliodrin_hypervisor"
user=udooer
autostart=true
autorestart=true
environment=HOME="/wyliodrin"
priority=10

[supervisord]
[program:wyliodrin-shell]
directory=/usr/wyliodrin/wyliodrin-shell
command=/usr/bin/node main.js
user=udooer
autostart=true
autorestart=true
environment=PORT="9000"
priority=30
' >> /etc/supervisor/supervisord.conf

# Change owner of directories used by wyliodrin
chown -R udooer:udooer /wyliodrin
chown -R udooer:udooer /etc/wyliodrin

# Add pi to the fuse group
usermod -a -G fuse udooer
