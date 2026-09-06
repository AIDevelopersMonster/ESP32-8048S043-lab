cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{KONTAKTS_APP_VERSION})
    set(PROJECT_VER "$ENV{KONTAKTS_APP_VERSION}")
else()
    set(PROJECT_VER "0.1.0")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(app06_ota_recovery)
