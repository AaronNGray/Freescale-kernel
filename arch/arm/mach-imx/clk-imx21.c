/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 * Copyright 2008 Martin Fuzzey, mfuzzey@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/clkdev.h>

#include <mach/hardware.h>
#include <mach/common.h>
#include "clk.h"

#define IO_ADDR_CCM(off)	(MX21_IO_ADDRESS(MX21_CCM_BASE_ADDR + (off)))

/* Register offsets */
#define CCM_CSCR		IO_ADDR_CCM(0x0)
#define CCM_MPCTL0		IO_ADDR_CCM(0x4)
#define CCM_MPCTL1		IO_ADDR_CCM(0x8)
#define CCM_SPCTL0		IO_ADDR_CCM(0xc)
#define CCM_SPCTL1		IO_ADDR_CCM(0x10)
#define CCM_OSC26MCTL		IO_ADDR_CCM(0x14)
#define CCM_PCDR0		IO_ADDR_CCM(0x18)
#define CCM_PCDR1		IO_ADDR_CCM(0x1c)
#define CCM_PCCR0		IO_ADDR_CCM(0x20)
#define CCM_PCCR1		IO_ADDR_CCM(0x24)
#define CCM_CCSR		IO_ADDR_CCM(0x28)
#define CCM_PMCTL		IO_ADDR_CCM(0x2c)
#define CCM_PMCOUNT		IO_ADDR_CCM(0x30)
#define CCM_WKGDCTL		IO_ADDR_CCM(0x34)

static char *mpll_sel_clks[] = { "fpm", "ckih", };
static char *spll_sel_clks[] = { "fpm", "ckih", };

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	},

static struct clk_lookup lookups[] = {
	clkdev(NULL, "per1", "per1")
	clkdev(NULL, "per2", "per2")
	clkdev(NULL, "per3", "per3")
	clkdev(NULL, "per4", "per4")
	clkdev("imx21-uart.0", "per", "per1")
	clkdev("imx21-uart.0", "ipg", "uart1_ipg_gate")
	clkdev("imx21-uart.1", "per", "per1")
	clkdev("imx21-uart.1", "ipg", "uart2_ipg_gate")
	clkdev("imx21-uart.2", "per", "per1")
	clkdev("imx21-uart.2", "ipg", "uart3_ipg_gate")
	clkdev("imx21-uart.3", "per", "per1")
	clkdev("imx21-uart.3", "ipg", "uart4_ipg_gate")
	clkdev("imx-gpt.0", "ipg", "gpt1_ipg_gate")
	clkdev("imx-gpt.0", "per", "per1")
	clkdev("imx-gpt.1", "ipg", "gpt2_ipg_gate")
	clkdev("imx-gpt.1", "per", "per1")
	clkdev("imx-gpt.2", "ipg", "gpt3_ipg_gate")
	clkdev("imx-gpt.2", "per", "per1")
	clkdev("mxc_pwm.0", "pwm", "pwm_ipg_gate")
	clkdev("imx21-cspi.0", "per", "per2")
	clkdev("imx21-cspi.0", "ipg", "cspi1_ipg_gate")
	clkdev("imx21-cspi.1", "per", "per2")
	clkdev("imx21-cspi.1", "ipg", "cspi2_ipg_gate")
	clkdev("imx21-cspi.2", "per", "per2")
	clkdev("imx21-cspi.2", "ipg", "cspi3_ipg_gate")
	clkdev("imx-fb.0", "per", "per3")
	clkdev("imx-fb.0", "ipg", "lcdc_ipg_gate")
	clkdev("imx-fb.0", "ahb", "lcdc_hclk_gate")
	clkdev("imx21-hcd.0", "per", "usb_gate")
	clkdev("imx21-hcd.0", "ahb", "usb_hclk_gate")
	clkdev("mxc_nand.0", NULL, "nfc_gate")
	clkdev("imx-dma", "ahb", "dma_hclk_gate")
	clkdev("imx-dma", "ipg", "dma_gate")
	clkdev("imx2-wdt.0", NULL, "wdog_gate")
	clkdev("imx-i2c.0", NULL, "i2c_gate")
	clkdev("mxc-keypad", NULL, "kpp_gate")
	clkdev("mxc_w1.0", NULL, "owire_gate")
	clkdev(NULL, "brom", "brom_gate")
	clkdev(NULL, "emma", "emma_gate")
	clkdev(NULL, "slcdc", "slcdc_gate")
	clkdev(NULL, "gpio", "gpio_gate")
	clkdev(NULL, "rtc", "rtc_gate")
	clkdev(NULL, "csi", "csi")
	clkdev(NULL, "ssi1", "ssi1_gate")
	clkdev(NULL, "ssi2", "ssi2_gate")
	clkdev(NULL, "sdhc1", "sdhc1")
	clkdev(NULL, "sdhc2", "sdhc2")
};

/*
 * must be called very early to get information about the
 * available clock rate when the timer framework starts
 */
int __init mx21_clocks_init(unsigned long lref, unsigned long href)
{
	imx_clk_fixed("ckil", lref);
	imx_clk_fixed("ckih", href);
	imx_clk_fixed_factor("fpm", "ckil", 512, 1);
	imx_clk_mux("mpll_sel", CCM_CSCR, 16, 1, mpll_sel_clks, ARRAY_SIZE(mpll_sel_clks));
	imx_clk_mux("spll_sel", CCM_CSCR, 17, 1, spll_sel_clks, ARRAY_SIZE(spll_sel_clks));
	imx_clk_pllv1("mpll", "mpll_sel", CCM_MPCTL0);
	imx_clk_pllv1("spll", "spll_sel", CCM_SPCTL0);
	imx_clk_divider("fclk", "mpll", CCM_CSCR, 29, 3);
	imx_clk_divider("hclk", "fclk", CCM_CSCR, 10, 4);
	imx_clk_divider("ipg", "hclk", CCM_CSCR, 9, 1);
	imx_clk_divider("per1", "mpll", CCM_PCDR1, 0, 6);
	imx_clk_divider("per2", "mpll", CCM_PCDR1, 8, 6);
	imx_clk_divider("per3", "mpll", CCM_PCDR1, 16, 6);
	imx_clk_divider("per4", "mpll", CCM_PCDR1, 24, 6);
	imx_clk_gate("uart1_ipg_gate", "ipg", CCM_PCCR0, 0);
	imx_clk_gate("uart2_ipg_gate", "ipg", CCM_PCCR0, 1);
	imx_clk_gate("uart3_ipg_gate", "ipg", CCM_PCCR0, 2);
	imx_clk_gate("uart4_ipg_gate", "ipg", CCM_PCCR0, 3);
	imx_clk_gate("gpt1_ipg_gate", "ipg", CCM_PCCR1, 25);
	imx_clk_gate("gpt2_ipg_gate", "ipg", CCM_PCCR1, 26);
	imx_clk_gate("gpt3_ipg_gate", "ipg", CCM_PCCR1, 27);
	imx_clk_gate("pwm_ipg_gate", "ipg", CCM_PCCR1, 28);
	imx_clk_gate("sdhc1_ipg_gate", "ipg", CCM_PCCR0, 9);
	imx_clk_gate("sdhc2_ipg_gate", "ipg", CCM_PCCR0, 10);
	imx_clk_gate("lcdc_ipg_gate", "ipg", CCM_PCCR0, 18);
	imx_clk_gate("lcdc_hclk_gate", "hclk", CCM_PCCR0, 26);
	imx_clk_gate("cspi3_ipg_gate", "ipg", CCM_PCCR1, 23);
	imx_clk_gate("cspi2_ipg_gate", "ipg", CCM_PCCR0, 5);
	imx_clk_gate("cspi1_ipg_gate", "ipg", CCM_PCCR0, 4);
	imx_clk_gate("per4_gate", "per4", CCM_PCCR0, 22);
	imx_clk_gate("csi_hclk_gate", "hclk", CCM_PCCR0, 31);
	imx_clk_divider("usb_div", "spll", CCM_CSCR, 26, 3);
	imx_clk_gate("usb_gate", "usb_div", CCM_PCCR0, 14);
	imx_clk_gate("usb_hclk_gate", "hclk", CCM_PCCR0, 24);
	imx_clk_gate("ssi1_gate", "ipg", CCM_PCCR0, 6);
	imx_clk_gate("ssi2_gate", "ipg", CCM_PCCR0, 7);
	imx_clk_divider("nfc_div", "ipg", CCM_PCDR0, 12, 4);
	imx_clk_gate("nfc_gate", "nfc_div", CCM_PCCR0, 19);
	imx_clk_gate("dma_gate", "ipg", CCM_PCCR0, 13);
	imx_clk_gate("dma_hclk_gate", "hclk", CCM_PCCR0, 30);
	imx_clk_gate("brom_gate", "hclk", CCM_PCCR0, 28);
	imx_clk_gate("emma_gate", "ipg", CCM_PCCR0, 15);
	imx_clk_gate("emma_hclk_gate", "hclk", CCM_PCCR0, 27);
	imx_clk_gate("slcdc_gate", "ipg", CCM_PCCR0, 25);
	imx_clk_gate("slcdc_hclk_gate", "hclk", CCM_PCCR0, 21);
	imx_clk_gate("wdog_gate", "ipg", CCM_PCCR1, 24);
	imx_clk_gate("gpio_gate", "ipg", CCM_PCCR0, 11);
	imx_clk_gate("i2c_gate", "ipg", CCM_PCCR0, 12);
	imx_clk_gate("kpp_gate", "ipg", CCM_PCCR1, 30);
	imx_clk_gate("owire_gate", "ipg", CCM_PCCR1, 31);
	imx_clk_gate("rtc_gate", "ipg", CCM_PCCR1, 29);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	mxc_timer_init(NULL, MX21_IO_ADDRESS(MX21_GPT1_BASE_ADDR),
			MX21_INT_GPT1);
	return 0;
}
