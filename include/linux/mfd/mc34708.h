/* For mc34708's pmic driver
 * Copyright (C) 2004-2011 Freescale Semiconductor, Inc.
 *
 * based on:
 * Copyright 2009-2010 Pengutronix, Uwe Kleine-Koenig
 * <u.kleine-koenig@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#ifndef __LINUX_MFD_MC_34708_H
#define __LINUX_MFD_MC_34708_H

#include <linux/interrupt.h>
#include <linux/regulator/driver.h>

struct mc34708;

void mc34708_lock(struct mc34708 *mc_pmic);
void mc34708_unlock(struct mc34708 *mc_pmic);

int mc34708_reg_read(struct mc34708 *mc_pmic, unsigned int offset, u32 *val);
int mc34708_reg_write(struct mc34708 *mc_pmic, unsigned int offset, u32 val);
int mc34708_reg_rmw(struct mc34708 *mc_pmic, unsigned int offset,
		    u32 mask, u32 val);

int mc34708_get_flags(struct mc34708 *mc_pmic);

int mc34708_irq_request(struct mc34708 *mc_pmic, int irq,
			irq_handler_t handler, const char *name, void *dev);
int mc34708_irq_request_nounmask(struct mc34708 *mc_pmic, int irq,
				 irq_handler_t handler, const char *name,
				 void *dev);
int mc34708_irq_free(struct mc34708 *mc_pmic, int irq, void *dev);
int mc34708_irq_mask(struct mc34708 *mc_pmic, int irq);
int mc34708_irq_unmask(struct mc34708 *mc_pmic, int irq);
int mc34708_irq_status(struct mc34708 *mc_pmic, int irq,
		       int *enabled, int *pending);
int mc34708_irq_ack(struct mc34708 *mc_pmic, int irq);

int mc34708_get_flags(struct mc34708 *mc_pmic);

#define MC34708_SW1A		0
#define MC34708_SW1B		1
#define MC34708_SW2		2
#define MC34708_SW3		3
#define MC34708_SW4A		4
#define MC34708_SW4B		5
#define MC34708_SW5		6
#define MC34708_SWBST		7
#define MC34708_VPLL		8
#define MC34708_VREFDDR		9
#define MC34708_VUSB		10
#define MC34708_VUSB2		11
#define MC34708_VDAC		12
#define MC34708_VGEN1		13
#define MC34708_VGEN2		14
#define MC34708_REGU_NUM	15

#define MC34708_REG_INT_STATUS0		0
#define MC34708_REG_INT_MASK0		1
#define MC34708_REG_INT_STATUS1		3
#define MC34708_REG_INT_MASK1		4
#define MC34708_REG_IDENTIFICATION	7

#define MC34708_IRQ_ADCDONE	0
#define MC34708_IRQ_TSDONE	1
#define MC34708_IRQ_TSPENDET	2
#define MC34708_IRQ_USBDET	3
#define MC34708_IRQ_AUXDET	4
#define MC34708_IRQ_USBOVP	5
#define MC34708_IRQ_AUXOVP	6
#define MC34708_IRQ_CHRTIMEEXP	7
#define MC34708_IRQ_BATTOTP	8
#define MC34708_IRQ_BATTOVP	9
#define MC34708_IRQ_CHRCMPL	10
#define MC34708_IRQ_WKVBUSDET	11
#define MC34708_IRQ_WKAUXDET	12
#define MC34708_IRQ_LOWBATT	13
#define MC34708_IRQ_VBUSREGMI	14
#define MC34708_IRQ_ATTACH	15
#define MC34708_IRQ_DETACH	16
#define MC34708_IRQ_KP		17
#define MC34708_IRQ_LKP		18
#define MC34708_IRQ_LKR		19
#define MC34708_IRQ_UKNOW_ATTA	20
#define MC34708_IRQ_ADC_CHANGE	21
#define MC34708_IRQ_STUCK_KEY	22
#define MC34708_IRQ_STUCK_KEY_RCV	23
#define MC34708_IRQ_1HZ		24
#define MC34708_IRQ_TODA	25
#define MC34708_IRQ_UNUSED1	26
#define MC34708_IRQ_PWRON1	27
#define MC34708_IRQ_PWRON2	28
#define MC34708_IRQ_WDIRESET	29
#define MC34708_IRQ_SYSRST	30
#define MC34708_IRQ_RTCRST	31
#define MC34708_IRQ_PCI		32
#define MC34708_IRQ_WARM	33
#define MC34708_IRQ_MEMHLD	34
#define MC34708_IRQ_UNUSED2	35
#define MC34708_IRQ_THWARNL	36
#define MC34708_IRQ_THWARNH	37
#define MC34708_IRQ_CLK		38
#define MC34708_IRQ_UNUSED3	39
#define MC34708_IRQ_SCP		40
#define MC34708_NUMREGS		0x3f
#define MC34708_NUM_IRQ		46

struct mc34708_regulator_init_data {
	int id;
	struct regulator_init_data *init_data;
};

struct mc34708_regulator_platform_data {
	int num_regulators;
	struct mc34708_regulator_init_data *regulators;
};

struct mc34708_platform_data {
#define MC34708_USE_TOUCHSCREEN (1 << 0)
#define MC34708_USE_CODEC	(1 << 1)
#define MC34708_USE_ADC		(1 << 2)
#define MC34708_USE_RTC		(1 << 3)
#define MC34708_USE_REGULATOR	(1 << 4)
#define MC34708_USE_LED		(1 << 5)
	unsigned int flags;

	int num_regulators;
	struct mc34708_regulator_init_data *regulators;
};

#endif				/* ifndef __LINUX_MFD_MC_PMIC_H */
