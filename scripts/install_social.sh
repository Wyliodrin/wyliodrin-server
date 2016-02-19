#!/bin/bash



###################################################################################################
# Sanity checks
###################################################################################################

# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  echo 'This script must be run as root'
  exit 1
fi



###################################################################################################
# Instalation folder
###################################################################################################

INSTALL_SOCIAL_PATH=/tmp/install_social

rm -rf $INSTALL_SOCIAL_PATH
mkdir $INSTALL_SOCIAL_PATH



###################################################################################################
# Installation of python libraries
###################################################################################################

cd $INSTALL_SOCIAL_PATH
echo "Checking python setuptools"
if ! echo "import setuptools" | python; then
	echo "Installing python setuptools"
	curl -L https://bootstrap.pypa.io/ez_setup.py | python
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking python facebook"
if ! echo "import facebook" | python; then
	echo "Installing python facebook"
	curl -L https://github.com/pythonforfacebook/facebook-sdk/archive/master.zip > facebook.zip
	unzip facebook.zip
	cd facebook-sdk-master
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -f facebook.zip
	rm -rf facebook-sdk-master
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking python tweepy"
if ! echo "import tweepy" | python; then
	echo "Installing python tweepy"
	curl -L https://github.com/tweepy/tweepy/archive/v2.3.0.zip > tweepy.zip
	unzip tweepy.zip
	cd tweepy-2.3.0
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -f tweepy.zip
	rm -rf tweepy-2.3.0
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking python twilio"
if ! echo "import twilio" | python; then
	echo "Installing python twilio"
	curl -L https://github.com/twilio/twilio-python/archive/3.6.5.zip > twilio-python.zip
	unzip twilio-python.zip
	cd twilio-python-3.6.5
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -f twilio-python.zip
	rm -rf twilio-python-3.6.5
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking python flask"
if ! echo "import flask" | python; then
	echo "Installing python flask"
	curl -L http://pypi.python.org/packages/source/F/Flask/Flask-0.10.1.tar.gz > flask.tar.gz
	tar zxf flask.tar.gz
	cd Flask-0.10.1
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -f flask.tar.gz
	rm -rf Flask-0.10.1
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking pybass"
if ! echo "import pybass" | python; then
	echo "Installing pybass"
	curl -L https://github.com/Wyliodrin/pybass/archive/master.zip > pybass.zip
	unzip pybass.zip
	cd pybass-master
	chmod a+x install.sh
	./install.sh

	cd $INSTALL_SOCIAL_PATH
	rm -f pybass.zip
	rm -rf pybass-master
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking pyfirmata"
if ! echo "import pyfirmata" | python; then
	echo "Installing pyfirmata"
	git clone https://github.com/tino/pyFirmata.git
	cd pyFirmata
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -rf pyFirmata
fi

cd $INSTALL_SOCIAL_PATH
echo "Checking python requests"
if ! echo "import requests" | python; then
	echo "Installing requests"
	git clone git://github.com/kennethreitz/requests.git
	cd requests
	python setup.py install

	cd $INSTALL_SOCIAL_PATH
	rm -rf requests
fi



###################################################################################################
# Installation of javascript libraries
###################################################################################################

export NODE_PATH=`npm root -g`

cd $INSTALL_SOCIAL_PATH

echo "Checking javascript facebook"
if ! echo "require ('fb')" | node; then
	echo "Installing javascript facebook"
	npm install -g fb
fi

echo "Checking javascript twitter"
if ! echo "require ('twitter-ng')" | node; then
	echo "Installing javascript twitter"
	npm install -g twitter-ng
fi

echo "Checking javascript twilio"
if ! echo "require ('twilio')" | node; then
	echo "Installing javascript twilio"
	npm install -g twilio
fi

echo "Checking javascript nodemailer"
if ! echo "require ('nodemailer')" | node; then
	echo "Installing javascript nodemailer"
	npm install -g nodemailer
fi

echo "Checking javascript express"
if ! echo "require ('express')" | node; then
	echo "Installing javascript express"
	npm install -g express
	npm install -g morgan
	npm install -g body-parser
	npm install -g jinja
fi



###################################################################################################
# Clean
###################################################################################################

rm -rf $INSTALL_SOCIAL_PATH
