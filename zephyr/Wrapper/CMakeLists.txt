#***********************************************************************
#*                                                                     *
#*        Copyright © 2022 American Megatrends International LLC       *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(${ZEPHYR_PRESENCE})
add_subdirectory_ifdef(CONFIG_TEKTAGONOE Tektagon-OE)
else()
add_subdirectory(BareMetal)
endif()
