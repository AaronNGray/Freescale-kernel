/*
 * Copyright (C) 2009 by Sascha Hauer, Pengutronix
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/clkdev.h>

#include <mach/hardware.h>
#include <mach/common.h>
#include <mach/mx25.h>
#include "clk.h"

#define CRM_BASE	MX25_IO_ADDRESS(MX25_CRM_BASE_ADDR)

#define CCM_MPCTL	0x00
#define CCM_UPCTL	0x04
#define CCM_CCTL	0x08
#define CCM_CGCR0	0x0C
#define CCM_CGCR1	0x10
#define CCM_CGCR2	0x14
#define CCM_PCDR0	0x18
#define CCM_PCDR1	0x1C
#define CCM_PCDR2	0x20
#define CCM_PCDR3	0x24
#define CCM_RCSR	0x28
#define CCM_CRDR	0x2C
#define CCM_DCVR0	0x30
#define CCM_DCVR1	0x34
#define CCM_DCVR2	0x38
#define CCM_DCVR3	0x3c
#define CCM_LTR0	0x40
#define CCM_LTR1	0x44
#define CCM_LTR2	0x48
#define CCM_LTR3	0x4c
#define CCM_MCR		0x64

#define ccm(x)	(CRM_BASE + (x))

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	},

static struct clk_lookup lookups[] = {
	/* i.mx25 has the i.mx21 type uart */
	clkdev("imx21-uart.0", "ipg", "uart1_ipg")
	clkdev("imx21-uart.0", "per", "uart_ipg_per")
	clkdev("imx21-uart.1", "ipg", "uart2_ipg")
	clkdev("imx21-uart.1", "per", "uart_ipg_per")
	clkdev("imx21-uart.2", "ipg", "uart3_ipg")
	clkdev("imx21-uart.2", "per", "uart_ipg_per")
	clkdev("imx21-uart.3", "ipg", "uart4_ipg")
	clkdev("imx21-uart.3", "per", "uart_ipg_per")
	clkdev("imx21-uart.4", "ipg", "uart5_ipg")
	clkdev("imx21-uart.4", "per", "uart_ipg_per")
	clkdev("imx-gpt.0", "ipg", "ipg")
	clkdev("imx-gpt.0", "per", "gpt_ipg_per")
	clkdev("mxc-ehci.0", "ipg", "ipg")
	clkdev("mxc-ehci.0", "ahb", "usbotg_ahb")
	clkdev("mxc-ehci.0", "per", "usbdiv")
	clkdev("mxc-ehci.1", "ipg", "ipg")
	clkdev("mxc-ehci.1", "ahb", "usbotg_ahb")
	clkdev("mxc-ehci.1", "per", "usbdiv")
	clkdev("mxc-ehci.2", "ipg", "ipg")
	clkdev("mxc-ehci.2", "ahb", "usbotg_ahb")
	clkdev("mxc-ehci.2", "per", "usbdiv")
	clkdev("fsl-usb2-udc", "ipg", "ipg")
	clkdev("fsl-usb2-udc", "ahb", "usbotg_ahb")
	clkdev("fsl-usb2-udc", "per", "usbdiv")
	clkdev("mxc_nand.0", NULL, "nfc_ipg_per")
	/* i.mx25 has the i.mx35 type cspi */
	clkdev("imx35-cspi.0", NULL, "cspi1_ipg")
	clkdev("imx35-cspi.1", NULL, "cspi2_ipg")
	clkdev("imx35-cspi.2", NULL, "cspi3_ipg")
	clkdev("mxc_pwm.0", "ipg", "pwm1_ipg")
	clkdev("mxc_pwm.0", "per", "per10")
	clkdev("mxc_pwm.1", "ipg", "pwm1_ipg")
	clkdev("mxc_pwm.1", "per", "per10")
	clkdev("mxc_pwm.2", "ipg", "pwm1_ipg")
	clkdev("mxc_pwm.2", "per", "per10")
	clkdev("mxc_pwm.3", "ipg", "pwm1_ipg")
	clkdev("mxc_pwm.3", "per", "per10")
	clkdev("imx-keypad", NULL, "kpp_ipg")
	clkdev("mx25-adc", NULL, "tsc_ipg")
	clkdev("imx-i2c.0", NULL, "i2c_ipg_per")
	clkdev("imx-i2c.1", NULL, "i2c_ipg_per")
	clkdev("imx-i2c.2", NULL, "i2c_ipg_per")
	clkdev("imx25-fec.0", "ipg", "fec_ipg")
	clkdev("imx25-fec.0", "ahb", "fec_ahb")
	clkdev("imxdi_rtc.0", NULL, "dryice_ipg")
	clkdev("imx-fb.0", "per", "lcdc_ipg_per")
	clkdev("imx-fb.0", "ipg", "lcdc_ipg")
	clkdev("imx-fb.0", "ahb", "lcdc_ahb")
	clkdev("imx2-wdt.0", NULL, "wdt_ipg")
	clkdev("imx-ssi.0", "per", "ssi1_ipg_per")
	clkdev("imx-ssi.0", "ipg", "ssi1_ipg")
	clkdev("imx-ssi.1", "per", "ssi2_ipg_per")
	clkdev("imx-ssi.1", "ipg", "ssi2_ipg")
	clkdev("sdhci-esdhc-imx25.0", "per", "esdhc1_ipg_per")
	clkdev("sdhci-esdhc-imx25.0", "ipg", "esdhc1_ipg")
	clkdev("sdhci-esdhc-imx25.0", "ahb", "esdhc1_ahb")
	clkdev("sdhci-esdhc-imx25.1", "per", "esdhc2_ipg_per")
	clkdev("sdhci-esdhc-imx25.1", "ipg", "esdhc2_ipg")
	clkdev("sdhci-esdhc-imx25.1", "ahb", "esdhc2_ahb")
	clkdev("mx2-camera.0", "per", "csi_ipg_per")
	clkdev("mx2-camera.0", "ipg", "csi_ipg")
	clkdev("mx2-camera.0", "ahb", "csi_ahb")
	clkdev(NULL, "audmux", "dummy")
	clkdev("flexcan.0", NULL, "can1_ipg")
	clkdev("flexcan.1", NULL, "can2_ipg")
	/* i.mx25 has the i.mx35 type sdma */
	clkdev("imx35-sdma", "ipg", "sdma_ipg")
	clkdev("imx35-sdma", "ahb", "sdma_ahb")
	clkdev(NULL, "iim", "iim_ipg")
};

static char *cpu_sel_clks[] = { "mpll", "mpll_cpu_3_4", };
static char *per_sel_clks[] = { "ahb", "upll", };

int __init mx25_clocks_init(void)
{
	imx_clk_fixed("dummy", 0);
	imx_clk_fixed("osc", 24000000);
	imx_clk_pllv1("mpll", "osc", ccm(CCM_MPCTL));
	imx_clk_pllv1("upll", "osc", ccm(CCM_UPCTL));
	imx_clk_fixed_factor("mpll_cpu_3_4", "mpll", 3, 4);
	imx_clk_mux("cpu_sel", ccm(CCM_CCTL), 14, 1, cpu_sel_clks, ARRAY_SIZE(cpu_sel_clks));
	imx_clk_divider("cpu", "cpu_sel", ccm(CCM_CCTL), 30, 2);
	imx_clk_divider("ahb", "cpu", ccm(CCM_CCTL), 28, 2);
	imx_clk_fixed_factor("ipg", "ahb", 1, 2);
	imx_clk_mux("per0_sel", ccm(CCM_MCR), 0, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per1_sel", ccm(CCM_MCR), 1, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per2_sel", ccm(CCM_MCR), 2, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per3_sel", ccm(CCM_MCR), 3, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per4_sel", ccm(CCM_MCR), 4, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per5_sel", ccm(CCM_MCR), 5, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per6_sel", ccm(CCM_MCR), 6, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per7_sel", ccm(CCM_MCR), 7, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per8_sel", ccm(CCM_MCR), 8, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per9_sel", ccm(CCM_MCR), 9, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per10_sel", ccm(CCM_MCR), 10, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per11_sel", ccm(CCM_MCR), 11, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per12_sel", ccm(CCM_MCR), 12, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per13_sel", ccm(CCM_MCR), 13, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per14_sel", ccm(CCM_MCR), 14, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_mux("per15_sel", ccm(CCM_MCR), 15, 1, per_sel_clks, ARRAY_SIZE(per_sel_clks));
	imx_clk_divider("per0", "per0_sel", ccm(CCM_PCDR0), 0, 6);
	imx_clk_divider("per1", "per1_sel", ccm(CCM_PCDR0), 8, 6);
	imx_clk_divider("per2", "per2_sel", ccm(CCM_PCDR0), 16, 6);
	imx_clk_divider("per3", "per3_sel", ccm(CCM_PCDR0), 24, 6);
	imx_clk_divider("per4", "per4_sel", ccm(CCM_PCDR1), 0, 6);
	imx_clk_divider("per5", "per5_sel", ccm(CCM_PCDR1), 8, 6);
	imx_clk_divider("per6", "per6_sel", ccm(CCM_PCDR1), 16, 6);
	imx_clk_divider("per7", "per7_sel", ccm(CCM_PCDR1), 24, 6);
	imx_clk_divider("per8", "per8_sel", ccm(CCM_PCDR2), 0, 6);
	imx_clk_divider("per9", "per9_sel", ccm(CCM_PCDR2), 8, 6);
	imx_clk_divider("per10", "per10_sel", ccm(CCM_PCDR2), 16, 6);
	imx_clk_divider("per11", "per11_sel", ccm(CCM_PCDR2), 24, 6);
	imx_clk_divider("per12", "per12_sel", ccm(CCM_PCDR3), 0, 6);
	imx_clk_divider("per13", "per13_sel", ccm(CCM_PCDR3), 8, 6);
	imx_clk_divider("per14", "per14_sel", ccm(CCM_PCDR3), 16, 6);
	imx_clk_divider("per15", "per15_sel", ccm(CCM_PCDR3), 24, 6);
	imx_clk_gate("csi_ipg_per", "per0", ccm(CCM_CGCR0), 0);
	imx_clk_gate("esdhc1_ipg_per", "per3", ccm(CCM_CGCR0),  3);
	imx_clk_gate("esdhc2_ipg_per", "per4", ccm(CCM_CGCR0),  4);
	imx_clk_gate("gpt_ipg_per", "per5", ccm(CCM_CGCR0),  5);
	imx_clk_gate("i2c_ipg_per", "per6", ccm(CCM_CGCR0),  6);
	imx_clk_gate("lcdc_ipg_per", "per8", ccm(CCM_CGCR0),  7);
	imx_clk_gate("nfc_ipg_per", "ipg_per", ccm(CCM_CGCR0),  8);
	imx_clk_gate("ssi1_ipg_per", "per13", ccm(CCM_CGCR0), 13);
	imx_clk_gate("ssi2_ipg_per", "per14", ccm(CCM_CGCR0), 14);
	imx_clk_gate("uart_ipg_per", "per15", ccm(CCM_CGCR0), 15);
	imx_clk_gate("csi_ahb", "ahb", ccm(CCM_CGCR0), 18);
	imx_clk_gate("esdhc1_ahb", "ahb", ccm(CCM_CGCR0), 21);
	imx_clk_gate("esdhc2_ahb", "ahb", ccm(CCM_CGCR0), 22);
	imx_clk_gate("fec_ahb", "ahb", ccm(CCM_CGCR0), 23);
	imx_clk_gate("lcdc_ahb", "ahb", ccm(CCM_CGCR0), 24);
	imx_clk_gate("sdma_ahb", "ahb", ccm(CCM_CGCR0), 26);
	imx_clk_gate("usbotg_ahb", "ahb", ccm(CCM_CGCR0), 28);
	imx_clk_gate("can1_ipg", "ipg", ccm(CCM_CGCR1),  2);
	imx_clk_gate("can2_ipg", "ipg", ccm(CCM_CGCR1),  3);
	imx_clk_gate("csi_ipg", "ipg", ccm(CCM_CGCR1),  4);
	imx_clk_gate("cspi1_ipg", "ipg", ccm(CCM_CGCR1),  5);
	imx_clk_gate("cspi2_ipg", "ipg", ccm(CCM_CGCR1),  6);
	imx_clk_gate("cspi3_ipg", "ipg", ccm(CCM_CGCR1),  7);
	imx_clk_gate("dryice_ipg", "ipg", ccm(CCM_CGCR1),  8);
	imx_clk_gate("esdhc1_ipg", "ipg", ccm(CCM_CGCR1), 13);
	imx_clk_gate("esdhc2_ipg", "ipg", ccm(CCM_CGCR1), 14);
	imx_clk_gate("fec_ipg", "ipg", ccm(CCM_CGCR1), 15);
	imx_clk_gate("iim_ipg", "ipg", ccm(CCM_CGCR1), 26);
	imx_clk_gate("kpp_ipg", "ipg", ccm(CCM_CGCR1), 28);
	imx_clk_gate("lcdc_ipg", "ipg", ccm(CCM_CGCR1), 29);
	imx_clk_gate("pwm1_ipg", "ipg", ccm(CCM_CGCR1), 31);
	imx_clk_gate("pwm2_ipg", "ipg", ccm(CCM_CGCR2),  0);
	imx_clk_gate("pwm3_ipg", "ipg", ccm(CCM_CGCR2),  1);
	imx_clk_gate("pwm4_ipg", "ipg", ccm(CCM_CGCR2),  2);
	imx_clk_gate("sdma_ipg", "ipg", ccm(CCM_CGCR2),  6);
	imx_clk_gate("ssi1_ipg", "ipg", ccm(CCM_CGCR2), 11);
	imx_clk_gate("ssi2_ipg", "ipg", ccm(CCM_CGCR2), 12);
	imx_clk_gate("tsc_ipg", "ipg", ccm(CCM_CGCR2), 13);
	imx_clk_gate("uart1_ipg", "ipg", ccm(CCM_CGCR2), 14);
	imx_clk_gate("uart2_ipg", "ipg", ccm(CCM_CGCR2), 15);
	imx_clk_gate("uart3_ipg", "ipg", ccm(CCM_CGCR2), 16);
	imx_clk_gate("uart4_ipg", "ipg", ccm(CCM_CGCR2), 17);
	imx_clk_gate("uart5_ipg", "ipg", ccm(CCM_CGCR2), 18);
	imx_clk_gate("wdt_ipg", "ipg", ccm(CCM_CGCR2), 19);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	mxc_timer_init(NULL, MX25_IO_ADDRESS(MX25_GPT1_BASE_ADDR), 54);
	return 0;
}
