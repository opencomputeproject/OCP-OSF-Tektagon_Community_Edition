#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

# HAL
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
include (../../Common.cmake)

if(${ZEPHYR_PRESENCE})
CommonInterfaceNamed(amihardwareinterface)
CommonWrapperLibrary()

CommonWrapperLibrarySources(
    ${COMMON_ROOT}/HardwareAbstraction/Hal/CommonCrypto/CommonHash.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/CommonCrypto/CommonRsa.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/CommonLogging/CommonLogging.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/CommonFlash/CommonFlash.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/CommonFlash/FlashMaster.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/SpiFilter/SpiFilter.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/I2cFilter/I2cFilter.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/I2c/I2c.c
    ${COMMON_ROOT}/HardwareAbstraction/Hal/I2c/I2cMaster.c
  )
  
CommonCoreLibraryIncludeDirectories(
	${COMMON_ROOT}
	${CERBERUS_ROOT}/projects/Baremetal
	${CERBERUS_ROOT}/projects/zephyr
	${COMMON_ROOT}/HardwareAbstraction/Hal
	${COMMON_ROOT}/FunctionalBlocks/Common
	${CERBERUS_ROOT}/core
	${COMMON_ROOT}/Wrapper/Tektagon-OE
	${COMMON_ROOT}/Silicon/AST1060
	)

CommonLibraryLinkLibraries(amihardwareinterface)
else()
file(GLOB_RECURSE HAL_COMMON_SOURCE "${CMAKE_CURRENT_LIST_DIR}/*.c")

CommonCore_Library(HARDWARE_ABSTRACTION ${HAL_COMMON_SOURCE})

CommonCoreLibraryIncludeDirectories(HARDWARE_ABSTRACTION
	${CMAKE_CURRENT_LIST_DIR}
	${CERBERUS_ROOT}/core
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/projects/Baremetal
	${COMMON_ROOT}/FunctionalBlocks/Common
	${COMMON_ROOT}/Wrapper/BareMetal	
	${COMMON_ROOT}/HardwareAbstraction/Qpc/Includes
    ${COMMON_ROOT}/HardwareAbstraction/Qpc/Ports/Arm
    ${COMMON_ROOT}/HardwareAbstraction/Qpc/Source
    ${COMMON_ROOT}/HardwareAbstraction/Qpc/Source/Qv
    ${COMMON_ROOT}/HardwareAbstraction/Qpc/Source/Qf
)
endif()
