/*
 *  Copyright (C) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/clkdev.h>

#include <mach/hardware.h>
#include <mach/common.h>
#include "clk.h"

/* CCM register addresses */
#define IO_ADDR_CCM(off)	(MX1_IO_ADDRESS(MX1_CCM_BASE_ADDR + (off)))

#define CCM_CSCR	IO_ADDR_CCM(0x0)
#define CCM_MPCTL0	IO_ADDR_CCM(0x4)
#define CCM_SPCTL0	IO_ADDR_CCM(0xc)
#define CCM_PCDR	IO_ADDR_CCM(0x20)

/* SCM register addresses */
#define IO_ADDR_SCM(off)	(MX1_IO_ADDRESS(MX1_SCM_BASE_ADDR + (off)))

#define SCM_GCCR	IO_ADDR_SCM(0xc)

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	},

static struct clk_lookup lookups[] = {
	clkdev(NULL, "dma", "dma_gate")
	clkdev("mx1-camera.0", NULL, "csi_gate")
	clkdev(NULL, "mma", "mma_gate")
	clkdev("imx_udc.0", NULL, "usbd_gate")
	clkdev("imx-gpt.0", "per", "per1")
	clkdev("imx-gpt.0", "ipg", "hclk")
	clkdev("imx1-uart.0", "per", "per1")
	clkdev("imx1-uart.0", "ipg", "hclk")
	clkdev("imx1-uart.1", "per", "per1")
	clkdev("imx1-uart.1", "ipg", "hclk")
	clkdev("imx1-uart.2", "per", "per1")
	clkdev("imx1-uart.2", "ipg", "hclk")
	clkdev("imx-i2c.0", NULL, "hclk")
	clkdev("imx1-cspi.0", "per", "per2")
	clkdev("imx1-cspi.0", "ipg", "dummy")
	clkdev("imx1-cspi.1", "per", "per2")
	clkdev("imx1-cspi.1", "ipg", "dummy")
	clkdev("imx-mmc.0", NULL, "per2")
	clkdev("imx-fb.0", "per", "per2")
	clkdev("imx-fb.0", "ipg", "dummy")
	clkdev("imx-fb.0", "ahb", "dummy")
	clkdev(NULL, "mshc", "hclk")
	clkdev(NULL, "ssi", "per3")
	clkdev("mxc_rtc.0", NULL, "clk32")
	clkdev(NULL, "clko", "clko")
};

static char *prem_sel_clks[] = { "clk32_premult", "clk16m", };
static char *clko_sel_clks[] = { "per1", "hclk", "clk48m", "clk16m", "prem",
				"fclk", };

int __init mx1_clocks_init(unsigned long fref)
{
	imx_clk_fixed("dummy", 0);
	imx_clk_fixed("clk32", fref);
	imx_clk_fixed("clk16m_ext", 16000000);
	imx_clk_gate("clk16m", "clk16m_ext", CCM_CSCR, 17);
	imx_clk_fixed_factor("clk32_premult", "clk32", 512, 1);
	imx_clk_mux("prem", CCM_CSCR, 16, 1, prem_sel_clks, ARRAY_SIZE(prem_sel_clks));
	imx_clk_pllv1("mpll", "clk32_premult", CCM_MPCTL0);
	imx_clk_pllv1("spll", "prem", CCM_SPCTL0);
	imx_clk_divider("mcu", "clk32_premult", CCM_CSCR, 15, 1);
	imx_clk_divider("fclk", "mpll", CCM_CSCR, 15, 1);
	imx_clk_divider("hclk", "spll", CCM_CSCR, 10, 4);
	imx_clk_divider("clk48m", "spll", CCM_CSCR, 26, 3);
	imx_clk_divider("per1", "spll", CCM_PCDR, 0, 4);
	imx_clk_divider("per2", "spll", CCM_PCDR, 4, 4);
	imx_clk_divider("per3", "spll", CCM_PCDR, 16, 7);
	imx_clk_mux("clko", CCM_CSCR, 29, 3, clko_sel_clks, ARRAY_SIZE(clko_sel_clks));
	imx_clk_gate("dma_gate", "hclk", SCM_GCCR, 4);
	imx_clk_gate("csi_gate", "hclk", SCM_GCCR, 2);
	imx_clk_gate("mma_gate", "hclk", SCM_GCCR, 1);
	imx_clk_gate("usbd_gate", "clk48m", SCM_GCCR, 0);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	mxc_timer_init(NULL, MX1_IO_ADDRESS(MX1_TIM1_BASE_ADDR),
			MX1_TIM1_INT);

	return 0;
}
