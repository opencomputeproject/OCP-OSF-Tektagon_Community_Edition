# cmake_minimum_required(VERSION 3.13.1)
include(../../Common.cmake)

zephyr_interface_library_named(TE-Wrapper)
zephyr_library()

set(WRAPPER_DIR ${CMAKE_CURRENT_LIST_DIR})
#target_link_libraries(Cerberus INTERFACE)

# Collect the Source Build Files
file(GLOB_RECURSE TE_WRAPPER_SOURCES "${WRAPPER_DIR}/*.c")
set(WRAPPER_INCLUDES ${WRAPPER_DIR})

zephyr_library_sources(
		${TE_WRAPPER_SOURCES}
	)

zephyr_library_include_directories(
		${CERBERUS_ROOT}/core/
		${CERBERUS_ROOT}/projects/
		${CERBERUS_ROOT}/projects/Baremetal
		${CERBERUS_ROOT}/projects/zephyr
		${COMMON_ROOT}/Silicon/AST1060
		${WRAPPER_INCLUDES}/
		${COMMON_ROOT}/HardwareAbstraction/Hal
		${COMMON_ROOT}/FunctionalBlocks/Cerberus/core/manifest
		${COMMON_ROOT}/FunctionalBlocks/Cerberus/core/manifest/pfm
	)

zephyr_compile_options(
		-fno-builtin
		-fdata-sections
		-Wall
		-Wextra
		-Wno-sign-compare
		-g -ggdb3
	)

zephyr_library_link_libraries(TE-Wrapper)
