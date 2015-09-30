#!/bin/bash



### Script variables ##############################################################################

SANDBOX_PATH=/wyliodrin/sandbox
HOME=/wyliodrin
WVERSION=v2.11
LWVERSION=v1.16



### Actual update #################################################################################

# Create sandbox
mkdir -p $SANDBOX_PATH

# Create home
mkdir -p $HOME

if [ "$wyliodrin_board" = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

elif [ "$wyliodrin_board" = "beagleboneblack" ]; then
  CMAKE_PARAMS="-DBEAGLEBONEBLACK=ON -DNODE_ROOT_DIR=/usr/include"

  printf "{\n\
    \"config_file\": \"/boot/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"beagleboneblack\",\n\
    \"run\": \"make -f Makefile.beagleboneblack run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_beagleboneblack.json

elif [ "$wyliodrin_board" = "arduinogalileo" ]; then
  CMAKE_PARAMS="-DGALILEO=ON"

  printf "{\n\
    \"config_file\": \"/media/card/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"arduinogalileo\",\n\
    \"run\": \"make -f Makefile.arduinogalileo run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_arduinogalileo.json

elif [ "$wyliodrin_board" = "edison" ]; then
  CMAKE_PARAMS="-DEDISON=ON"

  printf "{\n\
    \"config_file\": \"/media/storage/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"edison\",\n\
    \"run\": \"sudo -E make -f Makefile.edison run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_edison.json

elif [ "$wyliodrin_board" = "redpitaya" ]; then
  CMAKE_PARAMS="-DREDPITAYA=ON"

  printf "{\n\
    \"config_file\": \"/boot/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"redpitaya\",\n\
    \"run\": \"make -f Makefile.redpitaya run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_redpitaya.json

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

# Reboot
reboot
