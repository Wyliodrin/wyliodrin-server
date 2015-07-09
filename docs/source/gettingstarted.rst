***************
Getting Started
***************

.. highlight:: c



Compiling and Installing
========================
The wyliodrin-server source is available at
https://github.com/alexandruradovici/wyliodrin-server.


CMake (various platforms, including Windows)
--------------------------------------------
wyliodrin-server can be built using CMake_. Create a build directory for an
out-of-tree build, change to that directory, and run ``cmake`` to configure
the project.

Example:
::
  cd wyliodrin-server
  mkdir build
  cd build
  cmake ..
  make
  make install

.. note::
  In the example above ``..`` is used as an argument for ``cmake``.
  This is the path to the project root directory.



Configuration Files
===================

In order for wyliodrin-server to establish a connection between the device
it is running onto, and the device's owner, the following files are needed:


`boardtype`
-----------
As the name suggets, this file contains the type of board wyliodrin-json is
running onto. The accepted values are ``arduinogalileo``, ``beagleboneblack``,
``edison`` and ``raspberrypi``.

.. important::
  There must be not any whitespace characters
  after the board's name.

This file is located in ``/etc/wyliodrin``. The path is set in the
``BOARDTYPE_PATH`` directive in `wtalk.c`_.


`settings_<boardtype>.json`
---------------------------
This file contains settings in JSON format such as the path to
``wyliodrin.json``, the path to the mount directory, the path to the build
directory, etc.

Here ``<boardtype>`` is replaced with the type of board found in ``boardtype``.

.. note::
  If ``boardtype`` contains ``edison``, then the settings file is named
  ``settings_edison.json``

This file is located in ``/etc/wyliodrin``. The path is set in the
``SETTINGS_PATH`` directive in `wtalk.c`_.


`wyliodrin.json`
----------------
This file contains configuration data in JSON format such as the XMPP connetion
credentials (jid and password), privacy preference (whether to send back logs
or not), etc.

The path to this file is given by the value of the ``config_file`` key in
``settings_<boardtype>.json``.



.. _CMake: http://www.cmake.org
.. _`wtalk.c`: https://github.com/alexandruradovici/wyliodrin-server/blob/clean/wtalk.c
