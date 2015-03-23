wyliodrin-server
================

Wyliodrin Server


Libraries
--------
You need to install the following libraries:
* [libstrophe](http://strophe.im/libstrophe/) (fixed for wyliodrin)
* [libexpat](http://expat.sourceforge.net/)
* libresolv

###Raspbian

    sudo apt-get install libexpat1 libexpat1-dev automake libssl-dev

Download [libstrophe](https://github.com/alexandruradovici/wyliodrin-libstrophe) fixed for wyliodrin and compile it
    
    ./bootstrap.sh
    ./configure
    make
    sudo make install


    


