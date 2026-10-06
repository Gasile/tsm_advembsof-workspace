cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptr_test VERSION 0.1 DESCRIPTION "Zephyr Ptr Test App" LANGUAGES C CXX)
target_sources(app PRIVATE src/main.cpp)
