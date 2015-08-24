WTalk
=====

WTalk is a light memory-safe C client that allows boards to communicate with clients.

Dependencies
------------
* [libjansson v2.7](http://www.digip.org/jansson/)
* [libstrophe](http://strophe.im/libstrophe/)
* [libhiredis](https://github.com/redis/hiredis)
* [libevent] (https://github.com/libevent/libevent)

Building
--------

### Raspberry Pi

 1. Flash an SD card with [minibian](https://minibianpi.wordpress.com/)
 2. Download [install_raspberrypi.sh](https://raw.githubusercontent.com/Wyliodrin/wyliodrin-server/master/scripts/install_raspberrypi.sh) on your Raspberry Pi and run it
 3. Add an Raspberry Pi device on [wyliodrin](https://wyliodrin.com/)
 4. Download the wyliodrin.json file and put in /boot
 5. reboot
