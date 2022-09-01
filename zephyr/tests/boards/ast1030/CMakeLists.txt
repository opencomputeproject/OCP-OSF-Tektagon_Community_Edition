# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ast1030_evb_test)

FILE(GLOB app_sources src/*.c src/test_gpio/test*.c)
target_sources(app PRIVATE ${app_sources})
