#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************
# SPDX-License-Identifier: Apache-2.0

include(../../Common.cmake)

CommonInterfaceNamed(smf)

CommonCoreLibraryIncludeDirectories(smf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

CommonWrapperLibrary()

CommonWrapperLibrarySources(smf.c)

CommonLibraryLinkLibraries(smf)
