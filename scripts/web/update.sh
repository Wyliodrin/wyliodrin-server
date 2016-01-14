#!/bin/bash



### Script variables ##############################################################################

SANDBOX_PATH=/wyliodrin/sandbox
HOME=/wyliodrin
WVERSION=v3.14
LWVERSION=v2.1
BOARD=$(cat /etc/wyliodrin/boardtype)

###################################################################################################



### Actual update #################################################################################

# Create sandbox
mkdir -p $SANDBOX_PATH

if [ $BOARD = "arduinogalileo" ]; then
  CMAKE_PARAMS="-DGALILEO=ON"

  printf '{
  "config_file":  "/media/card/wyliodrin.json",
  "home":         "/wyliodrin",
  "mount_file":   "/wyliodrin/projects/mnt",
  "build_file":   "/wyliodrin/projects/build",
  "shell":        "bash",
  "run":          "make -f Makefile.arduinogalileo run",
  "stop":         "kill -9",
  "poweroff":     "poweroff",
  "logout":       "/etc/wyliodrin/logs.out",
  "logerr":       "/etc/wyliodrin/logs.err",
  "hlogout":      "/etc/wyliodrin/hlogs.out",
  "hlogerr":      "/etc/wyliodrin/hlogs.err"
}\n' > /etc/wyliodrin/settings_arduinogalileo.json

  printf '[Unit]
Description=Wyliodrin Server
After=wyliodrin-hypervisor
ConditionFileNotEmpty=/media/card/wyliodrin.json

[Service]
Type=simple
Environment="HOME=/wyliodrin"
ExecStart=/usr/bin/wyliodrind
TimeoutStartSec=0
Restart=always

[Install]
WantedBy=multi-user.target
' > /lib/systemd/system/wyliodrin-server.service

  printf '[Unit]
Description=Wyliodrin Hypervisor
After=redis
ConditionFileNotEmpty=/media/card/wyliodrin.json

[Service]
Type=simple
Environment="HOME=/wyliodrin"
ExecStart=/usr/bin/wyliodrin_hypervisor
TimeoutStartSec=0
Restart=always

[Install]
WantedBy=multi-user.target
' > /lib/systemd/system/wyliodrin-hypervisor.service

  systemctl enable wyliodrin-server.service
  systemctl enable wyliodrin-hypervisor.service

elif [ $BOARD = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

else
  echo "ERROR: unknown board: " $BOARD > /dev/stderr
  exit 1
fi

# Update libwyliodrin
cd $SANDBOX_PATH
rm -rf libwyliodrin
git clone https://github.com/Wyliodrin/libwyliodrin.git
cd libwyliodrin
git checkout $LWVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $CMAKE_PARAMS ..
make
make install
cd $SANDBOX_PATH
cd libwyliodrin/wylio
make
make install
cd $SANDBOX_PATH
rm -rf libwyliodrin

# Update wyliodrin-server
cd $SANDBOX_PATH
rm -rf wyliodrin_server
git clone https://github.com/Wyliodrin/wyliodrin-server.git
cd wyliodrin-server
git checkout $WVERSION
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $CMAKE_PARAMS ..
make
make install
cd $SANDBOX_PATH
cd wyliodrin-server/hypervisor
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install

# Clean
rm -rf $SANDBOX_PATH

reboot

###################################################################################################
