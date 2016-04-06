# Test whether the script is run by root or not
if [ ! "$(whoami)" = "root" ]; then
  echo "ERROR: This script must be run as root" 1>&2
  exit 1
fi

apt-get update &&
(cat /etc/apt/sources.list | grep http://wyliodrin.com/public/debian/udooneo) || echo "deb http://wyliodrin.com/public/debian/udooneo trusty main" >> /etc/apt/sources.list &&
apt-get update &&
apt-get install -y --force-yes libstrophe libstrophe-dev &&
apt-get install -y --force-yes libwyliodrin1 libwyliodrin-dev &&
apt-get install -y --force-yes wyliodrin-server &&
apt-get install -y --force-yes wyliodrin-shell &&
apt-get install -y --force-yes wyliodrin-app-server &&
apt-get install -y --force-yes node-red &&
apt-get install -y --force-yes wyliodrin-social
