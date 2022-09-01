# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(svf)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/ivm_core.c)
target_sources(app PRIVATE src/hardware.c)
FILE(GLOB app_sources src/file/vme*.c)
target_sources_ifdef(CONFIG_FIXED_VME_FILE app PRIVATE ${app_sources})