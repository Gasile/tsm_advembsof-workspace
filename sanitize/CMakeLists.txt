cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    sanitize
    VERSION 0.1
    DESCRIPTION "CPP Zephyr Sanitize application."
    LANGUAGES C CXX
)

target_sources(
    app
    PRIVATE
    src/main.cpp
)

zephyr_compile_options(-fsanitize=bounds -fsanitize=bounds-strict -fsanitize=shift -fsanitize=signed-integer-overflow)
zephyr_link_libraries(-fsanitize=bounds -fsanitize=bounds-strict -fsanitize=shift -fsanitize=signed-integer-overflow)
