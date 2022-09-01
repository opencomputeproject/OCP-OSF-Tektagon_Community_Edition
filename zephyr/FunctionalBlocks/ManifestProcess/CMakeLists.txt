#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

include (../../Common.cmake)

CommonInterfaceNamed(ManifestProcess)
CommonWrapperLibrary()

CommonWrapperLibrarySources(
    ${COMMON_ROOT}/FunctionalBlocks/ManifestProcess/ManifestProcess.c
    ${COMMON_ROOT}/FunctionalBlocks/ManifestProcess/PfmFlash/PfmFlash.c
  )
  
CommonCoreLibraryIncludeDirectories(
	${COMMON_ROOT}
	${COMMON_ROOT}/ApplicationLayer/tektagon/src/common
	${COMMON_ROOT}/FunctionalBlocks/ManifestProcess
	${COMMON_ROOT}/FunctionalBlocks/ManifestProcess/PfmFlash
	${COMMON_ROOT}/ApplicationLayer/tektagon/src/include
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/core
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/core/manifest
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/core/manifest/pfm
	${COMMON_ROOT}/FunctionalBlocks/Cerberus/projects/zephyr
	${COMMON_ROOT}/Wrapper/Tektagon-OE/crypto
)

CommonLibraryLinkLibraries(ManifestProcess)

