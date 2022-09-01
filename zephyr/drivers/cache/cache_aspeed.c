/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <kernel.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#ifdef CONFIG_SOC_SERIES_AST26XX
/*
 * cache area control: each bit controls 16MB cache area
 *	1: cacheable
 *	0: no-cache
 *
 *	bit[0]: 1st 16MB from 0x0000_0000 to 0x00ff_ffff
 *	bit[1]: 2nd 16MB from 0x0100_0000 to 0x01ff_ffff
 *	...
 *	bit[30]: 31th 16MB from 0x1e00_0000 to 0x1eff_ffff
 *	bit[31]: 32th 16MB from 0x1f00_0000 to 0x1fff_ffff
 */
 #define CACHE_AREA_SIZE_LOG2   24
 #define CACHE_AREA_CTRL_REG    0xa40
 #define CACHE_INVALID_REG      0xa44
 #define CACHE_FUNC_CTRL_REG    0xa48

 #define CACHED_SRAM_ADDR       DT_REG_ADDR_BY_IDX(DT_NODELABEL(sdram0), 0)
 #define CACHED_SRAM_SIZE       DT_REG_SIZE_BY_IDX(DT_NODELABEL(sdram0), 0)
 #define CACHED_SRAM_END        (CACHED_SRAM_ADDR + CACHED_SRAM_SIZE - 1)
#elif defined(CONFIG_SOC_SERIES_AST10X0)
/*
 * cache area control: each bit controls 32KB cache area
 *	1: cacheable
 *	0: no-cache
 *
 *	bit[0]: 1st 32KB from 0x0000_0000 to 0x0000_7fff
 *	bit[1]: 2nd 32KB from 0x0000_8000 to 0x0000_ffff
 *	...
 *	bit[22]: 23th 32KB from 0x000a_8000 to 0x000a_ffff
 *	bit[23]: 24th 32KB from 0x000b_0000 to 0x000b_ffff
 */
 #define CACHE_AREA_SIZE_LOG2   15
 #define CACHE_AREA_CTRL_REG    0xa50
 #define CACHE_INVALID_REG      0xa54
 #define CACHE_FUNC_CTRL_REG    0xa58

 #define CACHED_SRAM_ADDR       DT_REG_ADDR_BY_IDX(DT_NODELABEL(sram0), 0)
 #define CACHED_SRAM_SIZE       DT_REG_SIZE_BY_IDX(DT_NODELABEL(sram0), 0)
 #define CACHED_SRAM_END        (CACHED_SRAM_ADDR + CACHED_SRAM_SIZE - 1)
#else
 #error "Unsupported SOC series"
#endif

#define CACHE_AREA_SIZE         (1 << CACHE_AREA_SIZE_LOG2)

#define DCACHE_INVALID(addr)    (BIT(31) | ((addr & GENMASK(10, 0)) << 16))
#define ICACHE_INVALID(addr)    (BIT(15) | ((addr & GENMASK(10, 0)) << 0))

#define ICACHE_CLEAN            BIT(2)
#define DCACHE_CLEAN            BIT(1)
#define CACHE_EANABLE           BIT(0)

/* cache size = 32B * 128 = 4KB */
#define CACHE_LINE_SIZE_LOG2    5
#define CACHE_LINE_SIZE         (1 << CACHE_LINE_SIZE_LOG2)
#define N_CACHE_LINE            128
#define CACHE_ALIGNED_ADDR(addr) \
	((addr >> CACHE_LINE_SIZE_LOG2) << CACHE_LINE_SIZE_LOG2)

/* prefetch buffer */
#define PREFETCH_BUF_SIZE       CACHE_LINE_SIZE

static void aspeed_cache_init(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint32_t start_bit, end_bit;

	/* set all cache areas to no-cache by default */
	sys_write32(0, base + CACHE_FUNC_CTRL_REG);

	/* calculate how many areas need to be set */
	start_bit = CACHED_SRAM_ADDR >> CACHE_AREA_SIZE_LOG2;
	end_bit = CACHED_SRAM_END >> CACHE_AREA_SIZE_LOG2;
	sys_write32(GENMASK(end_bit, start_bit), base + CACHE_AREA_CTRL_REG);

	/* enable cache */
	sys_write32(CACHE_EANABLE, base + CACHE_FUNC_CTRL_REG);
}

#ifndef CONFIG_CACHE_ASPEED_FIXUP_INVALID
/**
 * @brief get aligned address and the number of cachline to be invalied
 * @param [IN] addr - start address to be invalidated
 * @param [IN] size - size in byte
 * @param [OUT] p_aligned_addr - pointer to the cacheline aligned address variable
 * @return number of cacheline to be invalidated
 *
 *  * addr
 *   |--------size-------------|
 * |-----|-----|-----|-----|-----|
 *  \                             \
 *   head                          tail
 *
 * example 1:
 * addr = 0x100 (cacheline aligned), size = 64
 * then head = 0x100, number of cache line to be invalidated = 64 / 32 = 2
 * which means range [0x100, 0x140) will be invalidated
 *
 * example 2:
 * addr = 0x104 (cacheline unaligned), size = 64
 * then head = 0x100, number of cache line to be invalidated = 1 + 64 / 32 = 3
 * which means range [0x100, 0x160) will be invalidated
 */
static uint32_t get_n_cacheline(uint32_t addr, uint32_t size, uint32_t *p_head)
{
	uint32_t n = 0;
	uint32_t tail;

	/* head */
	*p_head = CACHE_ALIGNED_ADDR(addr);

	/* roundup the tail address */
	tail = addr + size + (CACHE_LINE_SIZE - 1);
	tail = CACHE_ALIGNED_ADDR(tail);

	n = (tail - *p_head) >> CACHE_LINE_SIZE_LOG2;

	return n;
}
#endif

void cache_data_enable(void)
{
	aspeed_cache_init();
}

void cache_data_disable(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));

	sys_write32(0, base + CACHE_FUNC_CTRL_REG);
}

void cache_instr_enable(void)
{
	aspeed_cache_init();
}

void cache_instr_disable(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));

	sys_write32(0, base + CACHE_FUNC_CTRL_REG);
}

int cache_data_all(int op)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint32_t ctrl = sys_read32(base + CACHE_FUNC_CTRL_REG);
	unsigned int key = 0;

	ARG_UNUSED(op);

	/* enter critical section */
	if (!k_is_in_isr())
		key = irq_lock();

	ctrl &= ~DCACHE_CLEAN;
	sys_write32(ctrl, base + CACHE_FUNC_CTRL_REG);

	__DSB();
	ctrl |= DCACHE_CLEAN;
	sys_write32(ctrl, base + CACHE_FUNC_CTRL_REG);
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr())
		irq_unlock(key);

	return 0;
}

#ifdef CONFIG_CACHE_ASPEED_FIXUP_INVALID
int cache_data_range(void *addr, size_t size, int op)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return cache_data_all(op);
}
#else
int cache_data_range(void *addr, size_t size, int op)
{
	uint32_t aligned_addr, i, n;
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	unsigned int key = 0;

	ARG_UNUSED(op);

	/* enter critical section */
	if (!k_is_in_isr())
		key = irq_lock();

	if (((uint32_t)addr < CACHED_SRAM_ADDR) ||
	    ((uint32_t)addr > CACHED_SRAM_END)) {
		return 0;
	}

	n = get_n_cacheline((uint32_t)addr, size, &aligned_addr);

	for (i = 0; i < n; i++) {
		sys_write32(0, base + CACHE_INVALID_REG);
		sys_write32(DCACHE_INVALID(aligned_addr), base + CACHE_INVALID_REG);
		aligned_addr += CACHE_LINE_SIZE;
	}
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr())
		irq_unlock(key);

	return 0;
}
#endif

int cache_instr_all(int op)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint32_t ctrl = sys_read32(base + CACHE_FUNC_CTRL_REG);
	unsigned int key = 0;

	ARG_UNUSED(op);

	/* enter critical section */
	if (!k_is_in_isr())
		key = irq_lock();

	ctrl &= ~ICACHE_CLEAN;
	sys_write32(ctrl, base + CACHE_FUNC_CTRL_REG);
	__ISB();
	ctrl |= ICACHE_CLEAN;
	sys_write32(ctrl, base + CACHE_FUNC_CTRL_REG);
	__ISB();

	/* exit critical section */
	if (!k_is_in_isr())
		irq_unlock(key);

	return 0;
}

#ifdef CONFIG_CACHE_ASPEED_FIXUP_INVALID
int cache_instr_range(void *addr, size_t size, int op)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return cache_instr_all(op);
}
#else
int cache_instr_range(void *addr, size_t size, int op)
{
	uint32_t aligned_addr, i, n;
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	unsigned int key = 0;

	ARG_UNUSED(op);

	if (((uint32_t)addr < CACHED_SRAM_ADDR) ||
	    ((uint32_t)addr > CACHED_SRAM_END)) {
		return 0;
	}

	n = get_n_cacheline((uint32_t)addr, size, &aligned_addr);

	/* enter critical section */
	if (!k_is_in_isr())
		key = irq_lock();

	for (i = 0; i < n; i++) {
		sys_write32(0, base + CACHE_INVALID_REG);
		sys_write32(ICACHE_INVALID(aligned_addr), base + CACHE_INVALID_REG);
		aligned_addr += CACHE_LINE_SIZE;
	}
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr())
		irq_unlock(key);

	return 0;
}
#endif

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
size_t cache_data_line_size_get(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint32_t ctrl = sys_read32(base + CACHE_FUNC_CTRL_REG);

	return (ctrl & CACHE_EANABLE) ? CACHE_LINE_SIZE : 0;
}
#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
size_t cache_instr_line_size_get(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint32_t ctrl = sys_read32(base + CACHE_FUNC_CTRL_REG);

	return (ctrl & CACHE_EANABLE) ? CACHE_LINE_SIZE : 0;
}
#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */
