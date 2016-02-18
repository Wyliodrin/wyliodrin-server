#!/bin/bash



###################################################################################################
# Update libwyliodrin and wyliodrin-server
#
# Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
# Date last modified: January 2016
###################################################################################################



### Sanity checks #################################################################################

# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  printf 'ERROR: This script must be run as root\n' 1>&2
  exit 1
fi

###################################################################################################



### Script variables ##############################################################################

SANDBOX_PATH=/sandbox
HOME=/wyliodrin
WVERSION=v3.18
LWVERSION=v2.3
BOARD=$(cat /etc/wyliodrin/boardtype)

###################################################################################################



### Functions #####################################################################################

update_libwyliodrin() {
  if [ "$#" -ne 1 ]; then
    echo "usage: update_libwyliodrin -DBOARD=ON"
    return -1
  fi

  echo "Updating libwyliodrin"                                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf libwyliodrin                                                                            &&
  git clone https://github.com/Wyliodrin/libwyliodrin.git                                        &&
  cd libwyliodrin                                                                                &&
  git checkout $LWVERSION                                                                        &&
  mkdir build                                                                                    &&
  cd build                                                                                       &&
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $1 ..                                                   &&
  make                                                                                           &&
  make install                                                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  cd libwyliodrin/wylio                                                                          &&
  make                                                                                           &&
  make install                                                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf libwyliodrin                                                                            &&
  echo "libwyliodrin update success"                                                             &&
  return 0

  echo "libwyliodrin update fail" > /dev/stderr
  return -1
}

update_wyliodrin_server () {
  if [ "$#" -ne 1 ]
  then
    echo "usage: update_wyliodrin_server -DBOARD=ON"
    return -1
  fi

  echo "Updating wyliodrin-server"                                                               &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf wyliodrin_server                                                                        &&
  git clone https://github.com/Wyliodrin/wyliodrin-server.git                                    &&
  cd wyliodrin-server                                                                            &&
  git checkout $WVERSION                                                                         &&
  mkdir build                                                                                    &&
  cd build                                                                                       &&
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $1 ..                                                   &&
  make                                                                                           &&
  make install                                                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  cd wyliodrin-server/hypervisor                                                                 &&
  mkdir build                                                                                    &&
  cd build                                                                                       &&
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..                                                      &&
  make                                                                                           &&
  make install                                                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf wyliodrin-server                                                                        &&
  echo "$WVERSION" > /etc/wyliodrin/version                                                      &&
  echo "wyliodrin-server update success"                                                         &&
  return 0

  echo "wyliodrin-server update fail" > /dev/stderr
  return -1
}

update_wyliodrin_shell() {
  echo "Updating wyliodrin-shell"                                                                &&
  cd $SANDBOX_PATH                                                                               &&
  git clone https://github.com/Wyliodrin/wyliodrin-shell.git                                     &&
  cd wyliodrin-shell                                                                             &&
  npm update -g npm                                                                              &&
  npm install                                                                                    &&
  npm install grunt-cli                                                                          &&
  ./node_modules/grunt-cli/bin/grunt build                                                       &&
  rm -rf gruntfile.js package.json public/ server/                                               &&
  mv tmp/* .                                                                                     &&
  rm -rf tmp/                                                                                    &&
  mkdir -p /usr/wyliodrin/wyliodrin-shell                                                        &&
  cp -rf * /usr/wyliodrin/wyliodrin-shell                                                        &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf wyliodrin-shell                                                                         &&
  echo "wyliodrin-shell update success"                                                          &&
  return 0

  echo "wyliodrin-shell update fail" > /dev/stderr
  return -1
}

update_wyliodrin_app_server() {
  echo "Updating wyliodrin-app-server"                                                           &&
  cd $SANDBOX_PATH                                                                               &&
  git clone https://github.com/Wyliodrin/wyliodrin-app-server.git                                &&
  cd wyliodrin-app-server                                                                        &&
  npm install                                                                                    &&
  mkdir -p /usr/wyliodrin/wyliodrin-app-server                                                   &&
  cp -rf * /usr/wyliodrin/wyliodrin-app-server                                                   &&
  cd $SANDBOX_PATH                                                                               &&
  rm -rf wyliodrin-app-server                                                                    &&
  echo "wyliodrin-app-server update success"                                                     &&
  return 0

  echo "wyliodrin-app-server update fail" > /dev/stderr
  return -1
}

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

  printf '[Unit]
Description=Wyliodrin Shell
After=wyliodrin-hypervisor
ConditionFileNotEmpty=/media/card/wyliodrin.json

[Service]
Type=simple
Environment="PORT=9000"
WorkingDirectory=/usr/wyliodrin/wyliodrin-shell
ExecStart=/usr/bin/node main.js
TimeoutStartSec=0
Restart=always

[Install]
WantedBy=multi-user.target
' > /lib/systemd/system/wyliodrin-shell.service

  update_wyliodrin_shell

  systemctl enable wyliodrin-server.service
  systemctl enable wyliodrin-hypervisor.service
  systemctl enable wyliodrin-shell.service

elif [ $BOARD = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

  # Install wyliodrin-app-server
  if ! [ -a /usr/wyliodrin-app-server/startup.sh ]; then
    update_wyliodrin_app_server
  else
    echo "wyliodrin-app-server is up to date"
  fi

elif [ $BOARD = "udooneo" ]; then
  CMAKE_PARAMS="-DUDOONEO=ON"

elif [ $BOARD = "edison" ]; then
  CMAKE_PARAMS="-DEDISON=ON"

elif [ $BOARD = "beaglebone" ]; then
  CMAKE_PARAMS="-DBEAGLEBONE=ON"

elif [ $BOARD = "REDPITAYA" ]; then
  CMAKE_PARAMS="-DREDPITAYA=ON"

else
  echo "ERROR: unknown board: " $BOARD > /dev/stderr
  exit 1
fi

# Update
if ! [ -a /usr/bin/wylio ] || [[ "$(/usr/bin/wylio -v)" < "$LWVERSION" ]]; then
  update_libwyliodrin $CMAKE_PARAMS
else
  echo "libwyliodrin is up to date"
fi

if ! [ -a /etc/wyliodrin/version ] || [[ "$(cat /etc/wyliodrin/version)" < "$WVERSION" ]]; then
  update_wyliodrin_server $CMAKE_PARAMS
else
  echo "wyliodrin-server is up to date"
fi

# Clean
rm -rf $SANDBOX_PATH

echo "Changes will be applied on next reboot"

###################################################################################################
