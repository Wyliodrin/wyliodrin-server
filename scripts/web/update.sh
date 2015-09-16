#!/bin/sh

###################################################################################################
# Update libwyliodrin and wyliodrin-server
###################################################################################################



###################################################################################################
# Script variables
###################################################################################################

SANDBOX_PATH=/sandbox
WVERSION=v2.3
LWVERSION=v1.16



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

# Test whether the board has the new C server or not
if [ "$wyliodrin_server" = "" ]; then
  echo ""
  echo "***************************************"
  echo "*** Please upgrade to our new image ***"
  echo "***************************************"
  echo ""
  exit 1
fi



###################################################################################################
# Actual update
###################################################################################################

# Create sandbox
mkdir -p $SANDBOX_PATH

if [ "$wyliodrin_board" = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

  # Update wiringPi
  cd $SANDBOX_PATH
  rm -rf wiringPi
  git clone https://github.com/Wyliodrin/wiringPi.git
  cd wiringPi
  sed 's/sudo//g' build > build2
  chmod +x build2
  ./build2

  # Install pybass
  cd $SANDBOX_PATH
  git clone https://github.com/Wyliodrin/pybass.git
  cd pybass
  python setup.py install

  # Copy bashrc
  cp /home/pi/.bashrc /wyliodrin

elif [ "$wyliodrin_board" = "beagleboneblack" ]; then
  CMAKE_PARAMS="-DBEAGLEBONEBLACK=ON -DNODE_ROOT_DIR=/usr/include"

elif [ "$wyliodrin_board" = "arduinogalileo" ]; then
  CMAKE_PARAMS="-DGALILEO=ON"

elif [ "$wyliodrin_board" = "edison" ]; then
  CMAKE_PARAMS="-DEDISON=ON"

elif [ "$wyliodrin_board" = "redpitaya" ]; then
  CMAKE_PARAMS="-DREDPITAYA=ON"

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
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install

# Clean
rm -rf $SANDBOX_PATH
