# cmake_minimum_required(VERSION 3.13.1)

include(../../Common.cmake)
zephyr_interface_library_named(Cerberus)
zephyr_library()

set(TARGET_NAME Cerberus)

include (${CMAKE_CURRENT_LIST_DIR}/Cerberus.cmake)
target_include_directories(
	app
	PRIVATE
	${HAL_CERBERUS_ROOT}/core
	${HAL_CERBERUS_ROOT}/projects/zephyr
	)
set(CORE_DIR ${CERBERUS_ROOT}/core)
set(PLATFORM_DIR ${CERBERUS_ROOT}/projects/zephyr)

# Collect the Source Build Files
# Exclude Testing Directory and files ending with _mbedtls.c
set(EXCLUDE_DIR "/testing/")
set(EXCLUDE_FILE "_mbedtls.c")
file(GLOB_RECURSE CERBERUS_CORE_SOURCES "${CORE_DIR}/*.c")
foreach (TMP_PATH ${CERBERUS_CORE_SOURCES})
	string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
	if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
		list (REMOVE_ITEM CERBERUS_CORE_SOURCES ${TMP_PATH})
	endif ()

	string (FIND ${TMP_PATH} ${EXCLUDE_FILE} EXCLUDE_DIR_FOUND)

	if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
		list (REMOVE_ITEM CERBERUS_CORE_SOURCES ${TMP_PATH})
	endif ()
endforeach(TMP_PATH)

set(CERBERUS_CORE_INCLUDES ${CORE_DIR})

file(GLOB_RECURSE CERBERUS_PLATFORM_SOURCES "${PLATFORM_DIR}/*.c")
set(CERBERUS_PLATFORM_INCLUDES ${PLATFORM_DIR})

get_filename_component(PARENT_DIR $ENV{ZEPHYR_BASE} DIRECTORY)

find_package(Threads REQUIRED)

zephyr_library_sources(
		${CERBERUS_CORE_SOURCES}
		${CERBERUS_PLATFORM_SOURCES}
	)

zephyr_library_include_directories(
		${CERBERUS_CORE_INCLUDES}
		${CERBERUS_ROOT}/project/zephyr
		${CERBERUS_PLATFORM_INCLUDES}
		${AMI_PORT_ROOT}
	)
zephyr_compile_options(
		-fno-builtin
		-fdata-sections
		-Wall
		-Wextra
		-Wno-sign-compare
#		-Werror
#		-Wno-unused-parameter
#		-Wno-implicit-function-declaration
#		-Wno-maybe-uninitialized
#		-Wno-incompatible-pointer-types	
#		-Wno-int-conversion
#		-Wno-type-limits	
		-g -ggdb3
	)
zephyr_compile_definitions(
		ENABLE_DEBUG_COMMANDS
		ECC_ENABLE_GENERATE_KEY_PAIR
		ECC_ENABLE_ECDH
		HASH_ENABLE_SHA256
		HASH_ENABLE_SHA384
		HASH_ENABLE_SHA1
		X509_ENABLE_CREATE_CERTIFICATES
		X509_ENABLE_AUTHENTICATION
#		CONFIG_ARM
#		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=200000000
#		CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
#		CONFIG_MP_NUM_CPUS=1
#		unix		
	)
zephyr_library_link_libraries(Cerberus)
