#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

include (../../Common.cmake)

CommonInterfaceNamed(common)
CommonWrapperLibrary()

CommonWrapperLibrarySources(
    ${COMMON_ROOT}/FunctionalBlocks/Common/Common.c
  )
  
CommonCoreLibraryIncludeDirectories(
	${COMMON_ROOT}
	${CERBERUS_ROOT}/projects/zephyr
	${COMMON_ROOT}/HardwareAbstraction/Hal
	${COMMON_ROOT}/FunctionalBlocks/Common
	${CERBERUS_ROOT}/core
	${COMMON_ROOT}/Wrapper/Tektagon-OE
	${APPLICATION_SOURCE_INC}/state_machine
)

CommonLibraryLinkLibraries(common)

