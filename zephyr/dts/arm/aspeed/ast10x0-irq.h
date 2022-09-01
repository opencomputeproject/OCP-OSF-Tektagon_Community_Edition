/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 ASPEED Technology Inc.
 */
#ifndef _AST10X0_IRQ_H_
#define _AST10X0_IRQ_H_

#define AST10X0_IRQ_DEFAULT_PRIORITY	1


#define INTR_MMC			0
#define INTR_RESV_1			1
#define INTR_MAC			2

#define INTR_UART5			8
#define INTR_USBDEV			9
#define INTR_RESV_10			10
#define INTR_GPIO			11
#define INTR_SCU			12

#define INTR_TIMER0			16
#define INTR_TIMER1			17
#define INTR_TIMER2			18
#define INTR_TIMER3			19
#define INTR_TIMER4			20
#define INTR_TIMER5			21
#define INTR_TIMER6			22
#define INTR_TIMER7			23

#define INTR_WDT			24

#define INTR_ESPI			42

#define INTR_UART1			47
#define INTR_UART2			48
#define INTR_UART3			49
#define INTR_UART4			50

#define INTR_MBOX			54

#define INTR_UARTDMA			56
#define INTR_UART6			57
#define INTR_UART7			58
#define INTR_UART8			59
#define INTR_UART9			60
#define INTR_UART10			61
#define INTR_UART11			62
#define INTR_UART12			63
#define INTR_UART13			64

#define INTR_FMC			39
#define INTR_SPI1			65
#define INTR_SPI2			66

#define INTR_SPIM1			87
#define INTR_SPIM2			88
#define INTR_SPIM3			89
#define INTR_SPIM4			90

#define INTR_I2C0			110
#define INTR_I2C1			111
#define INTR_I2C2			112
#define INTR_I2C3			113
#define INTR_I2C4			114
#define INTR_I2C5			115
#define INTR_I2C6			116
#define INTR_I2C7			117
#define INTR_I2C8			118
#define INTR_I2C9			119
#define INTR_I2C10			120
#define INTR_I2C11			121
#define INTR_I2C12			122
#define INTR_I2C13			123
#define INTR_I2C14			124
#define INTR_I2C15			125

#define INTR_I2CFILTER			127
#define INTR_I2CMBX			128

#define INTR_KCS1			138
#define INTR_KCS2			139
#define INTR_KCS3			140
#define INTR_KCS4			141
#define INTR_BT				143
#define INTR_SNOOP			144
#define INTR_PCC			145

#define INTR_VUART1			147
#define INTR_VUART2			148

#endif /* #ifndef _AST10X0_IRQ_H_ */
