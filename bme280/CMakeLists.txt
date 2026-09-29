cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    bme280
    VERSION 0.1
    DESCRIPTION "CPP Zephyr BME280 application."
    LANGUAGES C CXX
)

target_sources(
    app
    PRIVATE
    src/main.cpp
)
