// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2010
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * (C) Copyright 2002
 * Rich Ireland, Enterasys Networks, rireland@enterasys.com.
 *
 * ispVM functions adapted from Lattice's ispmVMEmbedded code:
 * Copyright 2009 Lattice Semiconductor Corp.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmopcode.h"
#include <portability/cmsis_os2.h>
#include <drivers/jtag.h>

const struct device *jtag_dev;

/*
 * ispVMDelay
 *
 * Users must implement a delay to observe a_usTimeDelay, where
 * bit 15 of the a_usTimeDelay defines the unit.
 *      1 = milliseconds
 *      0 = microseconds
 * Example:
 *      a_usTimeDelay = 0x0001 = 1 microsecond delay.
 *      a_usTimeDelay = 0x8001 = 1 millisecond delay.
 *
 * This subroutine is called upon to provide a delay from 1 millisecond to a few
 * hundreds milliseconds each time.
 * It is understood that due to a_usTimeDelay is defined as unsigned short, a 16
 * bits integer, this function is restricted to produce a delay to 64000
 * micro-seconds or 32000 milli-second maximum. The VME file will never pass on
 * to this function a delay time > those maximum number. If it needs more than
 * those maximum, the VME file will launch the delay function several times to
 * realize a larger delay time cummulatively.
 * It is perfectly alright to provide a longer delay than required. It is not
 * acceptable if the delay is shorter.
 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
void ispVMDelay(unsigned short delay)
{
	uint32_t delay_ms = delay;

	if (delay_ms & 0x8000) {
		delay_ms = (delay_ms & ~0x8000) * 1000;
	}
	k_msleep(DIV_ROUND_UP(delay_ms, 1000));
}

void writePort(unsigned char a_ucPins, unsigned char a_ucValue)
{
	a_ucValue = a_ucValue ? 1 : 0;

	jtag_sw_xfer(jtag_dev, a_ucPins, a_ucValue);
}

unsigned char readPort(void)
{
	uint8_t tdo;

	jtag_tdo_get(jtag_dev, &tdo);
	return tdo;
}

void sclock(void)
{
	writePort(g_ucPinTCK, 0x01);
	writePort(g_ucPinTCK, 0x00);
}

void calibration(void)
{
	/* Apply 2 pulses to TCK. */
	writePort(g_ucPinTCK, 0x00);
	writePort(g_ucPinTCK, 0x01);
	writePort(g_ucPinTCK, 0x00);
	writePort(g_ucPinTCK, 0x01);
	writePort(g_ucPinTCK, 0x00);

	ispVMDelay(0x8001);

	/* Apply 2 pulses to TCK. */
	writePort(g_ucPinTCK, 0x01);
	writePort(g_ucPinTCK, 0x00);
	writePort(g_ucPinTCK, 0x01);
	writePort(g_ucPinTCK, 0x00);
}