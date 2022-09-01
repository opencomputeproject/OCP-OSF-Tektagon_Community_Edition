#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************
# SPDX-License-Identifier: Apache-2.0

include(../../../Common.cmake)

CommonInterfaceNamed(intel_2.0)
CommonWrapperLibrary()


file(GLOB_RECURSE INTEL_2_0_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

CommonWrapperLibrarySources(${INTEL_2_0_SOURCES} )

CommonCoreLibraryIncludeDirectories(intel_2.0 
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
					${COMMON_ROOT}/HardwareAbstraction/Smf
					)

CommonLibraryLinkLibraries(intel_2.0)
