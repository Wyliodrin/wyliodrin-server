cmake_minimum_required (VERSION 2.8.8)
project (wyliodrin-connect)

set (WYLIODRIN_HYPERVISOR_VERSION_MAJOR 3)
set (WYLIODRIN_HYPERVISOR_VERSION_MINOR 0)

configure_file (
  "${PROJECT_SOURCE_DIR}/wyliodrin_hypervisor_config.h.in"
  "${PROJECT_BINARY_DIR}/wyliodrin_hypervisor_config.h"
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

set (wyliodrin_hypervisor_SRCS
  ${PROJECT_SOURCE_DIR}/wyliodrin_hypervisor.c
  ${PROJECT_SOURCE_DIR}/wredis/wredis.c
  ${PROJECT_SOURCE_DIR}/cmp/cmp.c
  ${PROJECT_SOURCE_DIR}/shells/shells.c
  ${PROJECT_SOURCE_DIR}/base64/base64.c
  ${PROJECT_SOURCE_DIR}/wjson/wjson.c
  ${PROJECT_SOURCE_DIR}/logs/logs.c
)

add_library (dbs STATIC
  ${PROJECT_SOURCE_DIR}/../libds/treetest.c
  ${PROJECT_SOURCE_DIR}/../libds/listtest.c
  ${PROJECT_SOURCE_DIR}/../libds/maptest.c
  ${PROJECT_SOURCE_DIR}/../libds/heap.c
  ${PROJECT_SOURCE_DIR}/../libds/hashmap.c
  ${PROJECT_SOURCE_DIR}/../libds/vector.c
  ${PROJECT_SOURCE_DIR}/../libds/strutiltest.c
  ${PROJECT_SOURCE_DIR}/../libds/heaptest.c
  ${PROJECT_SOURCE_DIR}/../libds/strutils.c
  ${PROJECT_SOURCE_DIR}/../libds/vectest.c
  ${PROJECT_SOURCE_DIR}/../libds/tree.c
  ${PROJECT_SOURCE_DIR}/../libds/list.c
)

add_executable (wyliodrin_hypervisor
  ${wyliodrin_hypervisor_SRCS}
)

install (TARGETS wyliodrin_hypervisor DESTINATION bin)

add_definitions ("-Wall -g -DLOG -DERR -DSHELLS -DFILES -DMAKE -DCOMMUNICATION -DPS -D_FILE_OFFSET_BITS=64")

target_link_libraries (wyliodrin_hypervisor
  ${CMAKE_THREAD_LIBS_INIT}
  ${CURL_LIBRARIES}
  -lutil
  -lwyliodrin
  ${HIREDIS_LIBRARIES}
  ${EVENT_LIBRARIES}
  ${FUSE_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${STROPHE_LIBRARIES}
  ${EXPAT_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_BINARY_DIR}/libdbs.a
)
