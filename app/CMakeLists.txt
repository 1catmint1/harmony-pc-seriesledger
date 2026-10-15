cmake_minimum_required(VERSION 3.10)

project(influxdbstudio_qt_app LANGUAGES CXX)

set(INFLUXDBSTUDIO_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/connectiondialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/influxclient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/mainwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/querypage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/tablepage.cpp
)

set(INFLUXDBSTUDIO_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/include/appmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/include/connectiondialog.h
    ${CMAKE_CURRENT_LIST_DIR}/include/influxclient.h
    ${CMAKE_CURRENT_LIST_DIR}/include/jsonhelpers.h
    ${CMAKE_CURRENT_LIST_DIR}/include/mainwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/include/querypage.h
    ${CMAKE_CURRENT_LIST_DIR}/include/tablepage.h
)

