#!/bin/bash

###################################################################################################
# Edison install script
###################################################################################################


###################################################################################################
# Script variables
###################################################################################################

SANDBOX_PATH=/sandbox
WVERSION=v3.0
LWVERSION=v1.16

###################################################################################################
# Actual installation
###################################################################################################

# Create sandbox directory
mkdir -p $SANDBOX_PATH

# Create storage
mkdir -p /media/storage
grep /dev/mmcblk0p9 /etc/fstab || echo "/dev/mmcblk0p9      /media/storage         auto       ro,offset=8192   0 0" >> /etc/fstab

# Upgrade
echo "src inteli586 http://iotdk.intel.com/repos/1.1/iotdk/i586" > /etc/opkg/intel.conf
echo "src intel-iotdk http://iotdk.intel.com/repos/1.1/intelgalactic" > /etc/opkg/intel-iotdk.conf
opkg update
opkg upgrade

# Install some stuff
opkg install bash
opkg install redis libhiredis-dev
opkg install libfuse2
opkg install libfuse-dev
opkg install git
opkg install cmake
opkg install libexpat-dev
opkg install libjansson-dev
opkg install icu-dev
opkg install bluez5-dev

# Install nodejs
if [ ! -e /usr/lib/node ];
then
	ln -s /usr/lib/node_modules /usr/lib/node
fi
#mkdir -p /home/node
#cp -R /usr/lib/node_modules/* /home/node
#rm -rf /usr/node
#rm -rf /usr/lib/node_modules
#ln -s /home/node /usr/lib/node
#ln -s /home/node /usr/lib/node_modules

# Install redis
cd $SANDBOX_PATH
echo "Checking for python setuptools"
if ! echo "import setuptools" | python; then
echo "Installing python setuptools"
curl -L https://bootstrap.pypa.io/ez_setup.py | python
fi
git clone https://github.com/andymccurdy/redis-py.git /tmp/redis-py
cd /tmp/redis-py
python setup.py install

# Install libevent
cd $SANDBOX_PATH
git clone https://github.com/libevent/libevent.git
cd libevent/
./autogen.sh
./configure --prefix=/usr
make
make install
cd $SANDBOX_PATH
rm -rf libevent

# Install libwyliodrin
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/libwyliodrin.git
cd libwyliodrin
git checkout $LWVERSION
mkdir build
cd build
cmake -DEDISON=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install
cd $SANDBOX_PATH/wylio && make install
cd $SANDBOX_PATH
rm -rf libwyliodrin

mkdir /home/wyliodrin
ln -s /home/wyliodrin /wyliodrin

# Redis service
echo "
[Unit]
Description=Redis Server
#After=default.target
[Service]
Type=simple
ExecStart=/usr/bin/redis-server
ExecStop=/bin/kill -15 $MAINPID
PIDFile=/var/run/redis.pid
Restart=always
[Install]
WantedBy=multi-user.target
" > /lib/systemd/system/redis.service

# Install libstrophe
cd $SANDBOX_PATH
git clone https://github.com/strophe/libstrophe.git
cd libstrophe
./bootstrap.sh
./configure --prefix=/usr
make install
cd $SANDBOX_PATH
rm -rf libstrophe

# Install wyliodrin-server
cd $SANDBOX_PATH
git clone https://github.com/Wyliodrin/wyliodrin-server.git
cd wyliodrin-server
git checkout $WVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make install
cd $SANDBOX_PATH/hypervisor
mkdir build
cd build
cmake cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make install
cd $SANDBOX_PATH
rm -rf wyliodrin-server

mkdir -p /etc/wyliodrin
echo -n edison > /etc/wyliodrin/boardtype

printf '
  "config_file": "/boot/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build",
  "board": "edison",
  "run": "make -f Makefile.edison run",
  "shell": "bash",
  "stop": "kill -9"
}\n' > /etc/wyliodrin/settings_edison.json

mkdir -p /wyliodrin
mkdir -p /wyliodrin/mnt
mkdir -p /wyliodrin/build

echo "
[Unit]
Description=Wyliodrin server
After=wyliodrin-hypervisor.service
ConditionFileNotEmpty=/media/storage/wyliodrin.json
[Service]
Type=simple
Environment=\"libwyliodrin_version=v1.16\"
ExecStart=/usr/bin/wyliodrind
Restart=always
ExecStop=/bin/kill -15 $MAINPID
WorkingDirectory=/home/wyliodrin
PIDFile=/var/run/wyliodrin-server.pid
[Install]
WantedBy=multi-user.target
" > /lib/systemd/system/wyliodrin-server.service

echo "
[Unit]
Description=Wyliodrin hypervisor
After=redis.service
ConditionFileNotEmpty=/media/storage/wyliodrin.json
[Service]
Type=simple
ExecStart=/usr/bin/wyliodrin_hypervisor
Restart=always
ExecStop=/bin/kill -15 $MAINPID
WorkingDirectory=/home/wyliodrin
PIDFile=/var/run/wyliodrin-hypervisor.pid
[Install]
WantedBy=multi-user.target
" > /lib/systemd/system/wyliodrin-hypervisor.service

# Enable services
systemctl enable redis
systemctl enable wyliodrin-server
systemctl enable wyliodrin-hypervisor

# Run some more scripts
export wyliodrin_board=edison
install_social
update_streams

# Clean
rm -rf $SANDBOX_PATH

reboot
