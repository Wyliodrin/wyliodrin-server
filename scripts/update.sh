#!/bin/sh

# Update libwyliodrin and wyliodrin-server script

SANDBOX_PATH=/tmp/sandbox
LIBWYLIODRIN_PATH="https://github.com/Wyliodrin/libwyliodrin.git"
WYLIODRIN_SERVER_PATH="https://github.com/Wyliodrin/wyliodrin-server.git"

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

update_raspberrypi () {
  # Update wiringPi
  cd SANDBOX_PATH
  rm -rf wiringPi
  git clone https://github.com/Wyliodrin/wiringPi.git
  cd wiringPi
  sed 's/sudo//g' build > build2
  chmod +x build2
  ./build2
}

update_beagleboneblack () {
}

update_arduinogalileo () {
  opkg update
  opkg upgrade
}

update_edison () {
  if ! echo "import setuptools" | python; then
    curl -L https://bootstrap.pypa.io/ez_setup.py | python
  fi

  cd SANDBOX_PATH
  rm -rf redis-py
  git clone https://github.com/andymccurdy/redis-py.git
  cd redis-py
  python setup.py install

  opkg update
  opkg upgrade
}

update_redpitaya () {
}

# Create sandbox
mkdir -p SANDBOX_PATH

if [ "$wyliodrin_board" = "raspberrypi" ]; then
  update_raspberrypi
  CMAKE_PARAMS="-DRASPBERRYPI=ON"
elif [ "$wyliodrin_board" = "beagleboneblack" ]; then
  update_beagleboneblack
  CMAKE_PARAMS="-DBEAGLEBONEBLACK=ON -DNODE_ROOT_DIR=/usr/include"
elif [ "$wyliodrin_board" = "arduinogalileo" ]; then
  update_arduinogalileo
  CMAKE_PARAMS="-DGALILEO=ON"
elif [ "$wyliodrin_board" = "edison" ]; then
  update_edison
  CMAKE_PARAMS="-DEDISON=ON"
elif [ "$wyliodrin_board" = "redpitaya" ]; then
  update_redpitaya
  CMAKE_PARAMS="-DREDPITAYA=ON"
elif [ "$wyliodrin_board" = "" ]; then
  echo "ERROR: there is no environment variable named wyliodrin_board" \
    >/dev/stderr
  exit 2
else
  echo "ERROR: unknown board: $wyliodrin_board" > /dev/stderr
  exit 3
fi

# Update libwyliodrin
cd SANDBOX_PATH
rm -rf libwyliodrin
git clone LIBWYLIODRIN_PATH
cd libwyliodrin
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $CMAKE_PARAMS ..
make
make install

# Update wyliodrin-server
cd SANDBOX_PATH
rm -rf wyliodrin_server
git clone WYLIODRIN_SERVER_PATH
cd wyliodrin-server
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
make install
