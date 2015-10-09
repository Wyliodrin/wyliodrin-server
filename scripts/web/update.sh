#!/bin/bash



### Script variables ##############################################################################

SANDBOX_PATH=/wyliodrin/sandbox
HOME=/wyliodrin
WVERSION=v2.20
LWVERSION=v1.16



### Actual update #################################################################################

# Create sandbox
mkdir -p $SANDBOX_PATH

# Create home
mkdir -p $HOME

if [ "$wyliodrin_board" = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

  # Copy bashrc
  cp /home/pi/.bashrc /wyliodrin

  # Settings
printf '{
  "config_file": "/boot/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build\",
  "board": "raspberrypi",
  "run": "sudo -E make -f Makefile.raspberrypi run",
  "shell": "bash"
}\n' > /etc/wyliodrin/settings_raspberrypi.json

elif [ "$wyliodrin_board" = "beagleboneblack" ]; then
  CMAKE_PARAMS="-DBEAGLEBONEBLACK=ON -DNODE_ROOT_DIR=/usr/include"

printf '{
  "config_file": "/boot/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build\",
  "board": "beagleboneblack",
  "run": "make -f Makefile.beagleboneblack run",
  "shell": "bash"
}\n' > /etc/wyliodrin/settings_beagleboneblack.json

elif [ "$wyliodrin_board" = "arduinogalileo" ]; then
  CMAKE_PARAMS="-DGALILEO=ON"

printf '{
  "config_file": "/media/card/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build\",
  "board": "arduinogalileo",
  "run": "make -f Makefile.arduinogalileo run",
  "shell": "bash"
}\n' > /etc/wyliodrin/settings_arduinogalileo.json

elif [ "$wyliodrin_board" = "edison" ]; then
  CMAKE_PARAMS="-DEDISON=ON"

printf '{
  "config_file": "/media/storage/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build\",
  "board": "edison",
  "run": "make -f Makefile.edison run",
  "shell": "bash"
}\n' > /etc/wyliodrin/settings_edison.json

elif [ "$wyliodrin_board" = "redpitaya" ]; then
  CMAKE_PARAMS="-DREDPITAYA=ON"

printf '{
  "config_file": "/boot/wyliodrin.json",
  "home": "/wyliodrin",
  "mount_file": "/wyliodrin/projects/mnt",
  "build_file": "/wyliodrin/projects/build\",
  "board": "redpitaya",
  "run": "make -f Makefile.redpitaya run",
  "shell": "bash"
}\n' > /etc/wyliodrin/settings_redpitaya.json

elif [ "$wyliodrin_board" = "" ]; then
  echo "ERROR: there is no environment variable named wyliodrin_board" \
    >/dev/stderr
  exit 1

else
  echo "ERROR: unknown board: $wyliodrin_board" > /dev/stderr
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

# Clean
rm -rf $SANDBOX_PATH
