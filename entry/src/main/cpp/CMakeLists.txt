cmake_minimum_required(VERSION 3.10)
project(influxdbstudio_harmony)

set_property(GLOBAL PROPERTY JOB_POOLS compile_pool=2 link_pool=1)
set(CMAKE_JOB_POOL_COMPILE compile_pool)
set(CMAKE_JOB_POOL_LINK link_pool)
set(CMAKE_AUTOGEN_PARALLEL 2)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

get_filename_component(HARMONY_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
get_filename_component(APP_ROOT "${HARMONY_PROJECT_ROOT}/app" ABSOLUTE)

if (NOT DEFINED QT_PREFIX OR "${QT_PREFIX}" STREQUAL "")
    set(QT_PREFIX "qtforharmony_sdk")
endif()

if (NOT IS_ABSOLUTE "${QT_PREFIX}")
    get_filename_component(QT_PREFIX "${HARMONY_PROJECT_ROOT}/${QT_PREFIX}" ABSOLUTE)
endif()

if (NOT EXISTS "${QT_PREFIX}/lib/cmake/Qt5/Qt5Config.cmake")
    message(FATAL_ERROR "QT_PREFIX is invalid: ${QT_PREFIX}")
endif()

list(PREPEND CMAKE_PREFIX_PATH ${QT_PREFIX})
list(APPEND CMAKE_FIND_ROOT_PATH ${QT_PREFIX})

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Network Widgets)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Network OhExtras Widgets)

include(${APP_ROOT}/CMakeLists.txt)

add_library(entry SHARED ${INFLUXDBSTUDIO_SOURCES} ${INFLUXDBSTUDIO_HEADERS})

foreach(TOOL MOC RCC UIC)
    set(QT_HOST_${TOOL} "" CACHE FILEPATH "Build-host Qt 5 tool")
    if(CMAKE_HOST_WIN32 AND NOT EXISTS "${QT_HOST_${TOOL}}")
        message(FATAL_ERROR "Windows requires QT_HOST_${TOOL}")
    endif()
endforeach()
if(QT_HOST_MOC)
    set_property(TARGET entry PROPERTY AUTOMOC_EXECUTABLE "${QT_HOST_MOC}")
endif()
if(QT_HOST_RCC)
    set_property(TARGET entry PROPERTY AUTORCC_EXECUTABLE "${QT_HOST_RCC}")
endif()
if(QT_HOST_UIC)
    set_property(TARGET entry PROPERTY AUTOUIC_EXECUTABLE "${QT_HOST_UIC}")
endif()

target_include_directories(entry PRIVATE
    ${APP_ROOT}/include
)

target_compile_features(entry PRIVATE cxx_std_17)
target_compile_definitions(entry PRIVATE OPENHARMONY NAPI_DISABLE_CPP_EXCEPTIONS)

set_target_properties(entry PROPERTIES
    BUILD_RPATH "$ORIGIN"
)

target_link_libraries(entry PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::OhExtras
    Qt5::Widgets
    Qt5::QOpenHarmonyPlatformIntegrationPlugin
    Qt5::QICOPlugin
)
