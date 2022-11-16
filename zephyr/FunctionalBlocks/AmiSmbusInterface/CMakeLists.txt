#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************
# SPDX-License-Identifier: Apache-2.0

include(../../Common.cmake)

CommonInterfaceNamed(AmiSmbusInterface)
CommonWrapperLibrary()

CommonWrapperLibrarySources(
	AmiSmbusInterfaceSrcLib.c
	)

CommonCoreLibraryIncludeDirectories(cerberus_pfr 
	${CMAKE_CURRENT_SOURCE_DIR}
	${APPLICATION_SOURCE_INC}
	${COMMON_ROOT}
	${CERBERUS_ROOT}/projects/zephyr
	${COMMON_ROOT}/HardwareAbstraction/Hal/
	${COMMON_ROOT}/FunctionalBlocks/Common
	${CERBERUS_ROOT}/core	
	${COMMON_ROOT}/Wrapper/Tektagon-OE
	${APPLICATION_SOURCE_INC}/state_machine
	${COMMON_ROOT}/Silicon/AST1060
	${COMMON_ROOT}/HardwareAbstraction/smf
        ${COMMON_ROOT}/ApplicationLayer/tektagon/src 
        ${COMMON_ROOT}/ApplicationLayer/tektagon/src/state_machine                                 
	)

CommonLibraryLinkLibraries(AmiSmbusInterface)
