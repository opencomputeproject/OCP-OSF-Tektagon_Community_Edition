# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(i3c)

zephyr_sources_ifdef(CONFIG_I3C_SAMPLE_IMX3102 src/i3c-imx3102.c src/i3c-spd5118.c)
zephyr_sources_ifdef(CONFIG_I3C_SAMPLE_IMX3112 src/i3c-imx3112.c src/i3c-spd5118.c)
target_sources(app PRIVATE src/main.c)
