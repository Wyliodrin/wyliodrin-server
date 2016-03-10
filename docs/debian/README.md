# UDOO DUAL/QUAD



## wyliodrin-server
```
git clone https://github.com/Wyliodrin/wyliodrin-server.git wyliodrin-server-3.21
cd wyliodrin-server-3.21
git checkout v3.21
# Modify "cmake_minimum_required (VERSION 2.8.8)" to "cmake_minimum_required (VERSION 2.8.7)" in CMakeLists.txt and hypervisor/CMakeLists.txt
DEBFULLNAME="Razvan MATEI" EMAIL="matei.rm94@gmail.com" DEBEMAIL="matei.rm94@gmail.com" dh_make -e matei.rm94@gmail.com -c gpl2 --createorig
# When asked the type of package, choose multiple binary
cd debian
sudo rm -rf README.* docs *.ex wyliodrin-server*
# In source/format replace "quilt" with "native"
# In changelog replace "unstable" with "trusty"
# Replace the file named "control" with the file below
sudo apt-get install cmake libhiredis-dev libcurl4-gnutls-dev libfuse-dev libjansson-dev libevent-dev
sudo dpkg -i libstrophe-dev_20151014-1_armhf.deb
sudo dpkg -i libstrophe_20151014-1_armhf.deb
cd ..
DEB_CXXFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" DEB_CFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" dpkg-buildpackage -us -uc -j4
```

### control
```
Source: wyliodrin-server
Section: unknown
Priority: extra
Maintainer: Razvan MATEI <matei.rm94@gmail.com>
Build-Depends: debhelper (>= 8.0.0), cmake, libhiredis-dev, libcurl4-gnutls-dev, libfuse-dev, libjansson-dev, libevent-dev, libstrophe
Standards-Version: 3.9.2
Homepage: https://wyliodrin.com/
Vcs-Git: https://github.com/Wyliodrin/wyliodrin-server.git
Vcs-Browser: https://github.com/Wyliodrin/wyliodrin-server

Package: wyliodrin-server
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}, supervisor
Description: wyliodrin server
```



## wyliodrin-app-server
```
git clone https://github.com/Wyliodrin/wyliodrin-app-server.git wyliodrin-app-server-20150308
cd wyliodrin-app-server-20150308
DEBFULLNAME="Razvan MATEI" EMAIL="matei.rm94@gmail.com" DEBEMAIL="matei.rm94@gmail.com" dh_make -e matei.rm94@gmail.com -c gpl2 --createorig
# When asked the type of package, choose single binary
cd debian
sudo rm -rf README.* docs *.ex wyliodrin-app-server*
# In source/format replace "quilt" with "native"
# In changelog replace "unstable" with "trusty"
# Replace the file named "control" with the file below
sudo apt-get install libpam-dev
cd ..
DEB_CXXFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" DEB_CFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" dpkg-buildpackage -us -uc -j4
```

### control
```
Source: wyliodrin-app-server
Section: unknown
Priority: extra
Maintainer: Razvan MATEI <matei.rm94@gmail.com>
Build-Depends: debhelper (>= 8.0.0), npm, libpam-dev
Standards-Version: 3.9.2
Homepage: https://wyliodrin.com/
Vcs-Git: https://github.com/Wyliodrin/wyliodrin-app-server.git
Vcs-Browser: https://github.com/Wyliodrin/wyliodrin-app-server

Package: wyliodrin-app-server
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: wyliodrin app server
```



## wyliodrin-shell
```
git clone https://github.com/Wyliodrin/wyliodrin-shell.git wyliodrin-shell-20150309
cd wyliodrin-shell-20150309
DEBFULLNAME="Razvan MATEI" EMAIL="matei.rm94@gmail.com" DEBEMAIL="matei.rm94@gmail.com" dh_make -e matei.rm94@gmail.com -c gpl2 --createorig
# When asked the type of package, choose single binary
cd debian
sudo rm -rf README.* docs *.ex wyliodrin-shell*
# In source/format replace "quilt" with "native"
# In changelog replace "unstable" with "trusty"
# Replace the file named "control" with the file below
cd ..
DEB_CXXFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" DEB_CFLAGS_APPEND="-mthumb -O2 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -pipe -fomit-frame-pointer" dpkg-buildpackage -us -uc -j4
```

### control
```
Source: wyliodrin-shell
Section: unknown
Priority: extra
Maintainer: Razvan MATEI <matei.rm94@gmail.com>
Build-Depends: debhelper (>= 8.0.0), npm
Standards-Version: 3.9.2
Homepage: https://wyliodrin.com/
Vcs-Git: https://github.com/Wyliodrin/wyliodrin-shell.git
Vcs-Browser: https://github.com/Wyliodrin/wyliodrin-shell

Package: wyliodrin-shell
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: wyliodrin shell
```


## libwyliodrin
```
git clone https://github.com/Wyliodrin/libwyliodrin.git libwyliodrin-2.3
cd libwyliodrin-2.3
git checkout v3.21
# Modify "cmake_minimum_required (VERSION 2.8.8)" to "cmake_minimum_required (VERSION 2.8.7)" in CMakeLists.txt
DEBFULLNAME="Razvan MATEI" EMAIL="matei.rm94@gmail.com" DEBEMAIL="matei.rm94@gmail.com" dh_make -e matei.rm94@gmail.com -c gpl2 --createorig
# When asked the type of package, choose library
sudo rm -rf README.* docs *.ex libwyliodrin*
# In source/format replace "quilt" with "native"
# In changelog replace "unstable" with "trusty"
# Replace the file named "control" with the file below
```

### UDOONEO
```
sudo apt-get install -y cmake swig redis-server libhiredis-dev libjansson-dev libevent-dev python-dev
# node -v -> v0.12.9
# npm -v -> 2.14.9

# [ 66%] Swig source
# swig error : Unrecognized option -javascript
# swig error : Unrecognized option -node

# Swig 3.0.8
sudo apt-get install libpcre3-dev
((cat README.md | grep libpcreaaa) && echo found) || echo not_found
```

# UDOONEO