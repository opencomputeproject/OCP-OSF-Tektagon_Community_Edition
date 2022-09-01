#***********************************************************************
#*                                                                     *
#*        Copyright © 2022 American Megatrends International LLC       *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
include (../Common.cmake)

file(GLOB_RECURSE TestingCommonSource "${CMAKE_CURRENT_LIST_DIR}/*.c")

CommonCore_Library(Testing ${TestingCommonSource})

CommonCoreLibraryIncludeDirectories(Testing
	${CMAKE_CURRENT_LIST_DIR}
	${COMMON_ROOT}/Wrapper/BareMetal/StateManager
    	${COMMON_ROOT}/Wrapper/BareMetal/StateActions
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/projects/Baremetal
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/testing
	${COMMON_ROOT}/Configuration
)

