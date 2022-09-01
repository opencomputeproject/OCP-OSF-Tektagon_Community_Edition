#***********************************************************************
#*                                                                     *
#*        Copyright © 2022 American Megatrends International LLC       *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

# Hardware Abstraction

add_subdirectory(Hal)

if(${ZEPHYR_PRESENCE})
add_subdirectory(smf)
else()
add_subdirectory(Qpc)
endif()



