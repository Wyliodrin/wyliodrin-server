#!/bin/bash



### Script variables ##############################################################################

SANDBOX_PATH=/wyliodrin/sandbox
HOME=/wyliodrin
WVERSION=v2.9
LWVERSION=v1.16



### Sanity checks #################################################################################

# Test whether the board has the new C server or not
if [ "$wyliodrin_server" = "" ]; then
  echo ""
  echo "***************************************"
  echo "*** Please upgrade to our new image ***"
  echo "***************************************"
  echo ""
  exit 1
fi



### Actual update #################################################################################

# Create sandbox
mkdir -p $SANDBOX_PATH

# Create home
mkdir -p $HOME

if [ "$wyliodrin_board" = "raspberrypi" ]; then
  CMAKE_PARAMS="-DRASPBERRYPI=ON"

  # Add pi to the fuse group
  usermod -a -G fuse pi

  # Prepare stuff for user pi
  rm -rf /wyliodrin/build/*
  chmod 777 /wyliodrin
  chmod 777 /wyliodrin/mnt
  chmod 777 /wyliodrin/build

  # Update wiringPi
  cd $SANDBOX_PATH
  rm -rf wiringPi
  git clone https://github.com/Wyliodrin/wiringPi.git
  cd wiringPi
  ./build

  # Install pybass
  cd $SANDBOX_PATH
  git clone https://github.com/Wyliodrin/pybass.git
  cd pybass
  python setup.py install

  # Copy bashrc
  cp /home/pi/.bashrc /wyliodrin

  printf "{\n\
    \"config_file\": \"/boot/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"raspberrypi\",\n\
    \"run\": \"sudo -E make -f Makefile.raspberrypi run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_raspberrypi.json

printf '; supervisor config file

[unix_http_server]
file=/var/run//supervisor.sock   ; (the path to the socket file)
chmod=0700                       ; sockef file mode (default 0700)

[supervisord]
logfile=/var/log/supervisor/supervisord.log ; (main log file;default $CWD/supervisord.log)
pidfile=/var/run/supervisord.pid ; (supervisord pidfile;default supervisord.pid)
childlogdir=/var/log/supervisor            ; ('AUTO' child log dir, default $TEMP)

; the below section must remain in the config file for RPC
; (supervisorctl/web interface) to work, additional interfaces may be
; added by defining them in separate rpcinterface: sections
[rpcinterface:supervisor]
supervisor.rpcinterface_factory = supervisor.rpcinterface:make_main_rpcinterface

[supervisorctl]
serverurl=unix:///var/run//supervisor.sock ; use a unix:// URL  for a unix socket

; The [include] section can just contain the "files" setting.  This
; setting can list multiple files (separated by whitespace or
; newlines).  It can also contain wildcards.  The filenames are
; interpreted as relative to this file.  Included files *cannot*
; include files themselves.

[include]
files = /etc/supervisor/conf.d/*.conf
[supervisord]
[program:wtalk]
command=/usr/bin/wyliodrind
user=pi
autostart=true
autorestart=true
' > /etc/supervisor/supervisord.conf

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
    \"config_file\": \"/boot/wyliodrin.json\",\n\
    \"mountFile\": \"/wyliodrin/mnt\",\n\
    \"buildFile\": \"/wyliodrin/build\",\n\
    \"board\": \"arduinogalileo\",\n\
    \"run\": \"make -f Makefile.arduinogalileo run\",\n\
    \"shell_cmd\": \"bash\"\n\
  }\n" > /etc/wyliodrin/settings_arduinogalileo.json

elif [ "$wyliodrin_board" = "edison" ]; then
  CMAKE_PARAMS="-DEDISON=ON"

  printf "{\n\
    \"config_file\": \"/boot/wyliodrin.json\",\n\
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
