#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************
# Module Name:
#
#	CMakeLists.txt
#
# Abstract:
#
#	CMake script for Cerberus
#
# --

set(COMMON_CMAKE_ROOT ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(COMMON_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CERBERUS_ROOT ${CMAKE_CURRENT_LIST_DIR}/FunctionalBlocks/Cerberus)
set(APPLICATION_SOURCE_INC ${CMAKE_CURRENT_LIST_DIR}/ApplicationLayer/tektagon/src )
set(ZEPHYR_PRESENCE 1)

#include (${COMMON_CMAKE_ROOT}/extensions.cmake)

function(CommonDummyFunction)
	message(STATUS "Dummy")
endfunction()

function(CommonInterfaceNamed Name)
if(${ZEPHYR_PRESENCE})
	zephyr_interface_library_named(${Name}) 
endif()	
endfunction()

function(CommonWrapperLibrarySources)
if(${ZEPHYR_PRESENCE})
	zephyr_library_sources(${ARGN})
endif()
endfunction()

function(CommonLibraryLinkLibraries Item)
if(${ZEPHYR_PRESENCE})
	zephyr_library_link_libraries(${Item})
else()
	CommonDummyFunction()
endif()
endfunction()

function(CommonCoreCompileOptions Item)
if(${ZEPHYR_PRESENCE})
	zephyr_compile_options(${ARGN})
else()
	target_compile_options(${Item} PUBLIC ${ARGN})
endif()
endfunction()

function(CommonCoreCompileDefinitions)
if(${ZEPHYR_PRESENCE})
	zephyr_compile_definitions(${ARGN})
else()
	CommonDummyFunction()
endif()
endfunction()

function(CommonCore_Library Value ${ARGN})
  if(NOT ${Value})
    add_library(
		${Value}
	OBJECT
		${ARGN}
	)
  endif()
endfunction()

function(CommonCoreLibraryIncludeDirectories)
if(${ZEPHYR_PRESENCE})
  zephyr_library_include_directories(${ARGN})
else()
  target_include_directories(${Value} PUBLIC ${ARGN})
endif()
endfunction()

function(CommonCore_Link_Libraries)
if(${ZEPHYR_PRESENCE})
	CommonDummyFunction()
else()
	target_link_libraries(InterfaceValue PUBLIC ${ARGV})
endif()
endfunction()

macro(CommonWrapperLibrary)
	zephyr_library(${ARGV})
endmacro()