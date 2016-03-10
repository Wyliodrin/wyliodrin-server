cmake_minimum_required (VERSION 2.8.8)
project (wyliodrin-server)

set (WTALK_VERSION_MAJOR 3)
set (WTALK_VERSION_MINOR 21)

# file (WRITE "/etc/wyliodrin/version" "v${WTALK_VERSION_MAJOR}.${WTALK_VERSION_MINOR}\n")

configure_file (
  "${PROJECT_SOURCE_DIR}/wtalk_config.h.in"
  "${PROJECT_BINARY_DIR}/wtalk_config.h"
)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include (GNUInstallDirs)
include (FindPkgConfig)
include (FindEXPAT)
include (FindOpenSSL)

find_package (Threads REQUIRED)
find_package (EXPAT REQUIRED)
find_package (Hiredis REQUIRED)
pkg_check_modules (STROPHE REQUIRED libstrophe)
pkg_check_modules (JANSSON REQUIRED jansson)
pkg_check_modules (FUSE REQUIRED fuse)
pkg_check_modules (CURL REQUIRED libcurl)
pkg_check_modules (EVENT REQUIRED libevent)

include_directories (
  ${PROJECT_SOURCE_DIR}/libds
  ${PROJECT_BINARY_DIR}
  ${FUSE_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${EVENT_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
  ${STROPHE_INCLUDE_DIRS}
  ${EXPAT_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
  ${CURL_INCLUDE_DIRS}
)

set (wyliodrind_SRCS
  ${PROJECT_SOURCE_DIR}/wtalk.c
  ${PROJECT_SOURCE_DIR}/wxmpp/wxmpp.c
  ${PROJECT_SOURCE_DIR}/shells/shells.c
  ${PROJECT_SOURCE_DIR}/wjson/wjson.c
  ${PROJECT_SOURCE_DIR}/ps/ps.c
  ${PROJECT_SOURCE_DIR}/make/make.c
  ${PROJECT_SOURCE_DIR}/files/files.c
  ${PROJECT_SOURCE_DIR}/communication/communication.c
  ${PROJECT_SOURCE_DIR}/base64/base64.c
  ${PROJECT_SOURCE_DIR}/logs/logs.c
  ${PROJECT_SOURCE_DIR}/cmp/cmp.c
  ${PROJECT_SOURCE_DIR}/network/network.c
)

add_library (dbs STATIC
  ${PROJECT_SOURCE_DIR}/libds/treetest.c
  ${PROJECT_SOURCE_DIR}/libds/listtest.c
  ${PROJECT_SOURCE_DIR}/libds/maptest.c
  ${PROJECT_SOURCE_DIR}/libds/heap.c
  ${PROJECT_SOURCE_DIR}/libds/hashmap.c
  ${PROJECT_SOURCE_DIR}/libds/vector.c
  ${PROJECT_SOURCE_DIR}/libds/strutiltest.c
  ${PROJECT_SOURCE_DIR}/libds/heaptest.c
  ${PROJECT_SOURCE_DIR}/libds/strutils.c
  ${PROJECT_SOURCE_DIR}/libds/vectest.c
  ${PROJECT_SOURCE_DIR}/libds/tree.c
  ${PROJECT_SOURCE_DIR}/libds/list.c
)

if (DEVICEINTEL)
  add_definitions (-DDEVICEINTEL)
  pkg_check_modules (MRAA REQUIRED mraa>=0.4.0)
  message (INFO "found libmraa version: ${MRAA_VERSION}")
  set(FLAGS "-DDEVICEINTEL")
endif ()

add_executable (wyliodrind
  ${wyliodrind_SRCS}
)

install (TARGETS wyliodrind DESTINATION bin)

add_definitions ("-Wall -DERR -DSHELLS -DFILES -DMAKE -DCOMMUNICATION -DPS -D_FILE_OFFSET_BITS=64")

target_link_libraries (wyliodrind
  ${CMAKE_THREAD_LIBS_INIT}
  ${CURL_LIBRARIES}
  -lutil
  ${HIREDIS_LIBRARIES}
  ${EVENT_LIBRARIES}
  ${MRAA_LIBRARIES}
  ${FUSE_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${STROPHE_LIBRARIES}
  ${EXPAT_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_BINARY_DIR}/libdbs.a
)

add_subdirectory (hypervisor)

option (RASPBERRYPI "Build for the RaspberryPi." OFF)
option (UDOONEO     "Build for the UDOO NEO."    OFF)

install (FILES setup/settings_raspberrypi.json DESTINATION /etc/wyliodrin/ PERMISSIONS WORLD_READ)
install (FILES setup/settings_arduinogalileo.json DESTINATION /etc/wyliodrin/ PERMISSIONS WORLD_READ)
install (FILES setup/settings_edison.json DESTINATION /etc/wyliodrin/ PERMISSIONS WORLD_READ)
install (FILES setup/settings_minnowboardmax.json DESTINATION /etc/wyliodrin/ PERMISSIONS WORLD_READ)
install (FILES setup/settings_udooneo.json DESTINATION /etc/wyliodrin/ PERMISSIONS WORLD_READ)

if (RASPBERRYPI)
  install (FILES scripts/setup_wifi_rpi DESTINATION bin/ PERMISSIONS WORLD_READ WORLD_EXECUTE)
endif ()

if (UDOONEO)
  install (FILES scripts/setup_wifi_rpi DESTINATION bin/ PERMISSIONS WORLD_READ WORLD_EXECUTE)
  install (FILES setup/wyliodrin-server.conf DESTINATION /etc/supervisor/conf.d/ PERMISSIONS WORLD_READ)
endif ()
