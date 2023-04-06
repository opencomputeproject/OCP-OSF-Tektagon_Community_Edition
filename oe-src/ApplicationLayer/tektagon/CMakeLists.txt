# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/Common.cmake)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(tektagon-oe)

set(TEKTAGON_ROOT ${CMAKE_CURRENT_LIST_DIR})

FILE(GLOB_RECURSE app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
set(CERBERUS_ROOT $ENV{ZEPHYR_BASE}/FunctionalBlocks/Cerberus)

#message(STATUS "Printing app_sources - ${app_sources}")

target_include_directories(
	app
	PRIVATE
		$ENV{ZEPHYR_BASE}/FunctionalBlocks/Common
		$ENV{ZEPHYR_BASE}/FunctionalBlocks/ManifestProcessor
		$ENV{ZEPHYR_BASE}/FunctionalBlocks/Pfr
		${CERBERUS_ROOT}/core
		${CERBERUS_ROOT}/projects/zephyr
		$ENV{ZEPHYR_BASE}/Wrapper/Tektagon-OE
		$ENV{ZEPHYR_BASE}/Silicon/AST1060
		${TEKTAGON_ROOT}/src
		${TEKTAGON_ROOT}/src/state_machine
		${AMI_PORT_ROOT}
		$ENV{ZEPHYR_BASE}
		$ENV{ZEPHYR_BASE}/HardwareAbstraction/smf
		$ENV{ZEPHYR_BASE}/HardwareAbstraction/Hal
)

target_compile_options(
	app
	PRIVATE
 		-fno-builtin
		-fdata-sections
		-Wall
		-Wextra
# 		-Werror
		-Wno-unused-parameter
		-g -ggdb3
)
