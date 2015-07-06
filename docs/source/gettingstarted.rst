***************
Getting Started
***************

.. highlight:: c



Compiling and Installing Jansson
================================

The wyliodrin-server source is available at
https://github.com/alexandruradovici/wyliodrin-server.


CMake (various platforms, including Windows)
--------------------------------------------

wyliodrin-server can be built using CMake_. Create a build directory for an
out-of-tree build, change to that directory, and run ``cmake`` to configure
the project.

Example::

  cd wyliodrin-server
  mkdir build
  cd build
  cmake ..
  make
  make install

.. note:: In the example above ``..`` is used as an argument for ``cmake``.
          This is the path to the project root directory.

.. _CMake: http://www.cmake.org
