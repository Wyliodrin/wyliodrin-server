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
include (FindPkgConfig)
include (FindEXPAT)

find_package (EXPAT REQUIRED)
find_package (Hiredis REQUIRED)

pkg_check_modules (STROPHE REQUIRED libstrophe)
pkg_check_modules (JANSSON REQUIRED jansson)

include_directories (
  ${PROJECT_SOURCE_DIR}/libds
  ${PROJECT_BINARY_DIR}
  ${HIREDIS_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
  ${STROPHE_INCLUDE_DIRS}
  ${EXPAT_INCLUDE_DIRS}
)

set (wyliodrind_SRCS
  ${PROJECT_SOURCE_DIR}/wyapp.c
#  ${PROJECT_SOURCE_DIR}/wxmpp/wxmpp.c
#  ${PROJECT_SOURCE_DIR}/shells/shells.c
  ${PROJECT_SOURCE_DIR}/wjson/wjson.c
#  ${PROJECT_SOURCE_DIR}/base64/base64.c
#  ${PROJECT_SOURCE_DIR}/logs/logs.c
#  ${PROJECT_SOURCE_DIR}/cmp/cmp.c
)

if (DEVICEINTEL)
  add_definitions (-DDEVICEINTEL)
  pkg_check_modules (MRAA REQUIRED mraa>=0.4.0)
  message (INFO "found libmraa version: ${MRAA_VERSION}")
  set(FLAGS "-DDEVICEINTEL")
endif ()

add_executable (wyapp
 ${wyliodrind_SRCS}
)



target_link_libraries (wyapp
  -lutil
  ${HIREDIS_LIBRARIES}
  ${MRAA_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${STROPHE_LIBRARIES}
  ${EXPAT_LIBRARIES}
)

#add_subdirectory (hypervisor)

option (RASPBERRYPI     "Build for the RaspberryPi."     OFF)
option (UDOONEO         "Build for the UDOO NEO."        OFF)
option (BEAGLEBONEBLACK "Build for the BEAGLEBONEBLACK." OFF)

