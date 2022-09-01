#***********************************************************************
#*                                                                     *
#*        Copyright © 2022 American Megatrends International LLC       *
#*                                                                     *
#*      All rights reserved. Subject to AMI licensing agreement.       *
#*                                                                     *
#***********************************************************************-
# file HardwareAbstraction.cmake
# Abstract:
#
#	Defines the full set of compiler definitions necessary to enable Hardware Abstraction to publish its tasks.
#	Remove unwanted items from the list if you are not using it.
#
# --

set(HARDWARE_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(
	HARDWARE_ABSTRACTION_LIST
	CRYPTO_INITILIZATION
	DICE_INITILIZATION
	FLASH_INITILIZATION
	GPIO_INITILIZATION
	I2C_INITILIZATION
	INTERRUPT_INITILIZATION
	SMBUS_INITILIZATION
	SMBUSFILTER_INITILIZATION
	SPI_INITILIZATION
	SPIFILTER_INITILIZATION
	TIMER_INITILIZATION
	WATCHDOG_INITILIZATION
)
