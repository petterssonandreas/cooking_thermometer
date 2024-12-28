# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ct)

target_sources(app PRIVATE
    src/adc.c
    src/buttons.c
    src/cttime.c
    src/display.c
    src/main.c
)
zephyr_include_directories(src)
