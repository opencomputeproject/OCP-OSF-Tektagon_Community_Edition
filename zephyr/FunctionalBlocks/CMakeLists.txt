#***********************************************************************
#*                                                                     *
#*                  Copyright (c) 1985-2022, AMI.                      *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

if(${ZEPHYR_PRESENCE})
add_subdirectory_ifdef(CONFIG_CERBERUS Cerberus)
add_subdirectory(ManifestProcess)
add_subdirectory(Common)
add_subdirectory(Pfr)
else()
add_subdirectory(Cerberus)
#add_subdirectory(Common)
endif()
