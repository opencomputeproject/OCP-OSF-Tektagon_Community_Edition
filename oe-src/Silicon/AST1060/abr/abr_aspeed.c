#include "abr_aspeed.h"
#include <init.h>

void print_abr_wdt_info(void)
{
	/* ABR enable */
	if (sys_read32(HW_STRAP2_SCU510) & BIT(11)) // OTPSTRAP[43]
		printk("\r\n The boot SPI ABR is enabled");
	else
		printk("\r\n The boot SPI ABR is disabled");

	if (sys_read32(HW_STRAP2_SCU510) & BIT(12)) // OTPSTRAP[44]
		printk(", Single flash");
	else
		printk(", Dual flashes");
	// OTPSTRAP[3]
	printk(", Source: %s (%s)",
	       (sys_read32(ASPEED_FMC_WDT2_CTRL) & BIT(4)) ? "Alternate" : "Primary", // Boot flash source select indicator
	       (sys_read32(HW_STRAP1_SCU500) & BIT(3)) ? "1/3 flash layout" : "1/2 flash layout");

	if (sys_read32(HW_STRAP2_SCU510) & GENMASK(15, 13)) { // OTPSTRAP[47:45]
		printk(", Boot SPI flash size: %ldMB.",
		       BIT((sys_read32(HW_STRAP2_SCU510) >> 13) & 0x7) / 2);
	} else
		printk(", no define size.");

	printk("\n");

	if (sys_read32(ASPEED_FMC_WDT2_CTRL) & BIT(0))
		printk("\r\n WDT status: enabled.\n");
	else
		printk("\r\n WDT status: disabled.\n");
}

void clear_source_select_indicator(void)
{
	uint32_t reg_val;
	uint8_t bitmask;

	bitmask = 0xea;
	reg_val = sys_read32(ASPEED_FMC_WDT2_CTRL);
	reg_val |= bitmask << 16;
	sys_write32(reg_val, ASPEED_FMC_WDT2_CTRL);
	printk("\r\n The indicator is cleared.\n");
}

void disable_watchdog(void)
{
	uint32_t reg_val;
	uint8_t bitmask;

	bitmask = BIT(0);
	reg_val = sys_read32(ASPEED_FMC_WDT2_CTRL);
	reg_val &= ~bitmask;
	sys_write32(reg_val, ASPEED_FMC_WDT2_CTRL);
	printk("\r\n The WDT is disabled.\n");
}
