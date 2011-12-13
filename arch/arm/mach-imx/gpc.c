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

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/suspend.h>
#include <asm/hardware/gic.h>

#define GPC_CNTR		0x000
#define GPC_IMR1		0x008
#define GPC_ISR1		0x018
#define GPC_ISR2		0x01c
#define GPC_ISR3		0x020
#define GPC_ISR4		0x024
#define GPC_PGC_GPU_PGCR	0x260
#define GPC_PGC_CPU_PDN		0x2a0
#define GPC_PGC_CPU_PUPSCR	0x2a4
#define GPC_PGC_CPU_PDNSCR	0x2a8

#define IMR_NUM			4
#define ISR_NUM			4

static void __iomem *gpc_base;
static u32 gpc_wake_irqs[IMR_NUM];
static u32 gpc_saved_imrs[IMR_NUM];
static u32 gpc_saved_cntr;
static u32 gpc_saved_cpu_pdn;
static u32 gpc_saved_cpu_pupscr;
static u32 gpc_saved_cpu_pdnscr;

bool imx_gpc_wake_irq_pending(void)
{
	void __iomem *reg_isr1 = gpc_base + GPC_ISR1;
	int i;
	u32 val;

	for (i = 0; i < ISR_NUM; i++) {
		val = readl_relaxed(reg_isr1 + i * 4);
		if (val & gpc_wake_irqs[i])
			return true;
	}

	return false;
}

void imx_gpc_pre_suspend(suspend_state_t state)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	gpc_saved_cntr = readl_relaxed(gpc_base + GPC_CNTR);
	gpc_saved_cpu_pdn = readl_relaxed(gpc_base + GPC_PGC_CPU_PDN);
	gpc_saved_cpu_pupscr = readl_relaxed(gpc_base + GPC_PGC_CPU_PUPSCR);
	gpc_saved_cpu_pdnscr = readl_relaxed(gpc_base + GPC_PGC_CPU_PDNSCR);

	for (i = 0; i < IMR_NUM; i++) {
		gpc_saved_imrs[i] = readl_relaxed(reg_imr1 + i * 4);
		writel_relaxed(~gpc_wake_irqs[i], reg_imr1 + i * 4);
	}

	/* Power down and power up sequence */
	writel_relaxed(0xFFFFFFFF, gpc_base + GPC_PGC_CPU_PUPSCR);
	writel_relaxed(0xFFFFFFFF, gpc_base + GPC_PGC_CPU_PDNSCR);

	if (state == PM_SUSPEND_MEM) {
		/* Tell GPC to power off ARM core when suspend */
		writel_relaxed(0x1, gpc_base + GPC_PGC_CPU_PDN);

		/* GPU PGCR */
		writel_relaxed(0x1, gpc_base + GPC_PGC_GPU_PGCR);
		/* GPU/VPU power down request */
		writel_relaxed(0x1, gpc_base + GPC_CNTR);
	}
}

void imx_gpc_post_resume(suspend_state_t state)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	writel_relaxed(gpc_saved_cpu_pdnscr, gpc_base + GPC_PGC_CPU_PDNSCR);
	writel_relaxed(gpc_saved_cpu_pupscr, gpc_base + GPC_PGC_CPU_PUPSCR);
	writel_relaxed(gpc_saved_cpu_pdn, gpc_base + GPC_PGC_CPU_PDN);
	writel_relaxed(gpc_saved_cntr, gpc_base + GPC_CNTR);

	for (i = 0; i < IMR_NUM; i++)
		writel_relaxed(gpc_saved_imrs[i], reg_imr1 + i * 4);

	if (state == PM_SUSPEND_MEM) {
		/* GPU PGCR */
		writel_relaxed(0x1, gpc_base + GPC_PGC_GPU_PGCR);
		/* GPU/VPU power down request */
		writel_relaxed(gpc_saved_cntr | 0x2, gpc_base + GPC_CNTR);
	}
}

static int imx_gpc_irq_set_wake(struct irq_data *d, unsigned int on)
{
	unsigned int idx = d->irq / 32 - 1;
	u32 mask;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return -EINVAL;

	mask = 1 << d->irq % 32;
	gpc_wake_irqs[idx] = on ? gpc_wake_irqs[idx] | mask :
				  gpc_wake_irqs[idx] & ~mask;

	return 0;
}

static void imx_gpc_irq_unmask(struct irq_data *d)
{
	void __iomem *reg;
	u32 val;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return;

	reg = gpc_base + GPC_IMR1 + (d->irq / 32 - 1) * 4;
	val = readl_relaxed(reg);
	val &= ~(1 << d->irq % 32);
	writel_relaxed(val, reg);
}

static void imx_gpc_irq_mask(struct irq_data *d)
{
	void __iomem *reg;
	u32 val;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return;

	reg = gpc_base + GPC_IMR1 + (d->irq / 32 - 1) * 4;
	val = readl_relaxed(reg);
	val |= 1 << (d->irq % 32);
	writel_relaxed(val, reg);
}

void __init imx_gpc_init(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-gpc");
	gpc_base = of_iomap(np, 0);
	WARN_ON(!gpc_base);

	/* Register GPC as the secondary interrupt controller behind GIC */
	gic_arch_extn.irq_mask = imx_gpc_irq_mask;
	gic_arch_extn.irq_unmask = imx_gpc_irq_unmask;
	gic_arch_extn.irq_set_wake = imx_gpc_irq_set_wake;
}
