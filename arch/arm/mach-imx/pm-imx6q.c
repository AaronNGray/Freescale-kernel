/*
 * Copyright 2011 Freescale Semiconductor, Inc.
 * Copyright 2011 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <asm/proc-fns.h>
#include <asm/suspend.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/tlb.h>
#include <asm/mach/map.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/iram.h>

#define ANATOP_REG_2P5		0x130
#define ANATOP_REG_CORE		0x140

extern unsigned long phys_l2x0_saved_regs;

static void __iomem *anatop_base;
static u32 anatop[2];

static void *suspend_iram_base;
static unsigned long iram_paddr, cpaddr;
static void (*suspend_in_iram)(suspend_state_t state,
	unsigned long iram_paddr, unsigned long suspend_iram_base) = NULL;

extern void mx6q_suspend(suspend_state_t state);

static void imx6q_anatop_pre_suspend(void)
{
	u32 reg;

	/* save registers */
	anatop[0] = readl_relaxed(anatop_base + ANATOP_REG_2P5);
	anatop[1] = readl_relaxed(anatop_base + ANATOP_REG_CORE);

	/* Enable weak 2P5 linear regulator */
	reg = readl_relaxed(anatop_base + ANATOP_REG_2P5);
	reg |= 1 << 18;
	writel_relaxed(reg, anatop_base + ANATOP_REG_2P5);
	/* Make sure ARM and SOC domain has same voltage */
	reg = readl_relaxed(anatop_base + ANATOP_REG_CORE);
	reg &= ~(0x1f << 18);
	reg |= (reg & 0x1f) << 18;
	writel_relaxed(reg, anatop_base + ANATOP_REG_CORE);

	/* gpu: power off pu reg1 */
	reg = readl_relaxed(anatop_base + ANATOP_REG_CORE);
	reg &= ~0x0003fe00;
	writel_relaxed(reg, anatop_base + ANATOP_REG_CORE);
}

static void imx6q_anatop_post_resume(void)
{
	u32 reg;

	/* restore registers */
	writel_relaxed(anatop[0], anatop_base + ANATOP_REG_2P5);
	writel_relaxed(anatop[1], anatop_base + ANATOP_REG_CORE);

	/*gpu: power on pu reg1 */
	reg = readl_relaxed(anatop_base + ANATOP_REG_CORE);
	reg &= ~0x0003fe00;
	reg |= 0x10 << 9; /* 1.1v */
	writel_relaxed(reg, anatop_base + ANATOP_REG_CORE);
	mdelay(10);
}

static int imx6q_pm_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_MEM:
		if (imx_gpc_wake_irq_pending())
			return 0;

		imx6q_ccm_pre_suspend(state);
		imx6q_set_lpm(ARM_POWER_OFF);
		imx_gpc_pre_suspend(state);
		imx6q_ccm_gpu_pre_suspend();
		imx6q_anatop_pre_suspend();

		suspend_in_iram(state, (unsigned long)iram_paddr,
			(unsigned long)suspend_iram_base);

		imx_smp_prepare();

		imx6q_anatop_post_resume();
		imx6q_ccm_gpu_post_resume();
		imx_gpc_post_resume(state);
		udelay(10);
		imx6q_ccm_post_resume();
		break;
	case PM_SUSPEND_STANDBY:
		if (imx_gpc_wake_irq_pending())
			return 0;

		imx6q_ccm_pre_suspend(state);
		imx6q_set_lpm(STOP_POWER_OFF);
		imx_gpc_pre_suspend(state);

		suspend_in_iram(state, (unsigned long)iram_paddr,
			(unsigned long)suspend_iram_base);

		imx_smp_prepare();

		imx_gpc_post_resume(state);
		imx6q_ccm_post_resume();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int imx6q_pm_valid(suspend_state_t state)
{
	return (state > PM_SUSPEND_ON && state <= PM_SUSPEND_MAX);
}

static const struct platform_suspend_ops imx6q_pm_ops = {
	.enter = imx6q_pm_enter,
	.valid = imx6q_pm_valid,
};

void __init imx6q_pm_init(void)
{
	/*
	 * The l2x0 core code provides an infrastucture to save and restore
	 * l2x0 registers across suspend/resume cycle.  But because imx6q
	 * retains L2 content during suspend and needs to resume L2 before
	 * MMU is enabled, it can only utilize register saving support and
	 * have to take care of restoring on its own.  So we save physical
	 * address of the data structure used by l2x0 core to save registers,
	 * and later restore the necessary ones in imx6q resume entry.
	 * Need to run the suspend code from IRAM as the DDR needs
	 * to be put into low power mode manually.
	 */
	phys_l2x0_saved_regs = __pa(&l2x0_saved_regs);

	anatop_base = IMX_IO_ADDRESS(MX6Q_ANATOP_BASE_ADDR);

	suspend_set_ops(&imx6q_pm_ops);

	/* Move suspend routine into iRAM */
	cpaddr = (unsigned long)iram_alloc(SZ_4K, &iram_paddr);
	/* Need to remap the area here since we want the memory region
		 to be executable. */
	suspend_iram_base = __arm_ioremap(iram_paddr, SZ_4K,
					  MT_MEMORY_NONCACHED);
	pr_info("cpaddr = %x suspend_iram_base=%x\n",
		(unsigned int)cpaddr, (unsigned int)suspend_iram_base);

	/*
	 * Need to run the suspend code from IRAM as the DDR needs
	 * to be put into low power mode manually.
	 */
	memcpy((void *)cpaddr, mx6q_suspend, SZ_4K);

	suspend_in_iram = (void *)suspend_iram_base;
}
