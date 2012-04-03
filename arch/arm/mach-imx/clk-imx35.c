/*
 * Copyright (C) 2012 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/clkdev.h>
#include <linux/of.h>

#include <mach/hardware.h>
#include <mach/common.h>

#include "crmregs-imx3.h"
#include "clk.h"

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	},

static struct clk_lookup lookups[] = {
	clkdev("pata_imx", NULL, "pata_gate")
	clkdev("flexcan.0", NULL, "can1_gate")
	clkdev("flexcan.1", NULL, "can2_gate")
	clkdev("imx35-cspi.0", "per", "cspi1_gate")
	clkdev("imx35-cspi.0", "ipg", "cspi1_gate")
	clkdev("imx35-cspi.1", "per", "cspi2_gate")
	clkdev("imx35-cspi.1", "ipg", "cspi2_gate")
	clkdev("imx-epit.0", NULL, "epit1_gate")
	clkdev("imx-epit.1", NULL, "epit2_gate")
	clkdev("sdhci-esdhc-imx35.0", "per", "esdhc1_gate")
	clkdev("sdhci-esdhc-imx35.0", "ipg", "ipg")
	clkdev("sdhci-esdhc-imx35.0", "ahb", "ahb")
	clkdev("sdhci-esdhc-imx35.1", "per", "esdhc2_gate")
	clkdev("sdhci-esdhc-imx35.1", "ipg", "ipg")
	clkdev("sdhci-esdhc-imx35.1", "ahb", "ahb")
	clkdev("sdhci-esdhc-imx35.2", "per", "esdhc3_gate")
	clkdev("sdhci-esdhc-imx35.2", "ipg", "ipg")
	clkdev("sdhci-esdhc-imx35.2", "ahb", "ahb")
	/* i.mx35 has the i.mx27 type fec */
	clkdev("imx27-fec.0", NULL, "fec_gate")
	clkdev("imx-gpt.0", "per", "gpt_gate")
	clkdev("imx-gpt.0", "ipg", "ipg")
	clkdev("imx-i2c.0", NULL, "i2c1_gate")
	clkdev("imx-i2c.1", NULL, "i2c2_gate")
	clkdev("imx-i2c.2", NULL, "i2c3_gate")
	clkdev("ipu-core", NULL, "ipu_gate")
	clkdev("mx3_sdc_fb", NULL, "ipu_gate")
	clkdev("mxc_w1", NULL, "owire_gate")
	clkdev("imx35-sdma", NULL, "sdma_gate")
	clkdev("imx-ssi.0", "ipg", "ipg")
	clkdev("imx-ssi.0", "per", "ssi1_div_post")
	clkdev("imx-ssi.1", "ipg", "ipg")
	clkdev("imx-ssi.1", "per", "ssi2_div_post")
	/* i.mx35 has the i.mx21 type uart */
	clkdev("imx21-uart.0", "per", "uart1_gate")
	clkdev("imx21-uart.0", "ipg", "ipg")
	clkdev("imx21-uart.1", "per", "uart2_gate")
	clkdev("imx21-uart.1", "ipg", "ipg")
	clkdev("imx21-uart.2", "per", "uart3_gate")
	clkdev("imx21-uart.2", "ipg", "ipg")
	clkdev("mxc-ehci.0", "per", "usb_div")
	clkdev("mxc-ehci.0", "ipg", "ipg")
	clkdev("mxc-ehci.0", "ahb", "usbotg_gate")
	clkdev("mxc-ehci.1", "per", "usb_div")
	clkdev("mxc-ehci.1", "ipg", "ipg")
	clkdev("mxc-ehci.1", "ahb", "usbotg_gate")
	clkdev("mxc-ehci.2", "per", "usb_div")
	clkdev("mxc-ehci.2", "ipg", "ipg")
	clkdev("mxc-ehci.2", "ahb", "usbotg_gate")
	clkdev("fsl-usb2-udc", "per", "usb_div")
	clkdev("fsl-usb2-udc", "ipg", "ipg")
	clkdev("fsl-usb2-udc", "ahb", "usbahb_gate")
	clkdev("imx2-wdt.0", NULL, "wdog_gate")
	clkdev("mxc_nand.0", NULL, "nfc_div")
};

struct arm_ahb_div {
	unsigned char arm, ahb, sel;
};

static struct arm_ahb_div clk_consumer[] = {
	{ .arm = 1, .ahb = 4, .sel = 0},
	{ .arm = 1, .ahb = 3, .sel = 1},
	{ .arm = 2, .ahb = 2, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 4, .ahb = 1, .sel = 0},
	{ .arm = 1, .ahb = 5, .sel = 0},
	{ .arm = 1, .ahb = 8, .sel = 0},
	{ .arm = 1, .ahb = 6, .sel = 1},
	{ .arm = 2, .ahb = 4, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 4, .ahb = 2, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
};

static char hsp_div_532[] = { 4, 8, 3, 0 };
static char hsp_div_400[] = { 3, 6, 3, 0 };

static char *std_sel[] = {"ppll", "arm"};
static char *ipg_per_sel[] = {"ahb_per_div", "arm_per_div"};

int __init mx35_clocks_init()
{
	void __iomem *base = MX35_IO_ADDRESS(MX35_CCM_BASE_ADDR);
	u32 pdr0, consumer_sel, hsp_sel;
	struct arm_ahb_div *aad;
	struct clk *spba, *gpio1, *gpio2, *gpio3, *arm, *iim, *emi;
	unsigned char *hsp_div;

	pdr0 = __raw_readl(base + MXC_CCM_PDR0);
	consumer_sel = (pdr0 >> 16) & 0xf;
	aad = &clk_consumer[consumer_sel];
	if (!aad->arm) {
		pr_err("i.MX35 clk: illegal consumer mux selection 0x%x\n", consumer_sel);
		/*
		 * We are basically stuck. Continue with a default entry and hope we
		 * get far enough to actually show the above message
		 */
		aad = &clk_consumer[0];
	}

	imx_clk_fixed("ckih", 24000000);
	imx_clk_pllv1("mpll", "ckih", base + MX35_CCM_MPCTL);
	imx_clk_pllv1("ppll", "ckih", base + MX35_CCM_PPCTL);

	imx_clk_fixed_factor("mpll", "mpll_075", 3, 4);

	if (aad->sel)
		arm = imx_clk_fixed_factor("arm", "mpll_075", 1, aad->arm);
	else
		arm = imx_clk_fixed_factor("arm", "mpll", 1, aad->arm);

	if (clk_get_rate(arm) > 400000000)
		hsp_div = hsp_div_532;
	else
		hsp_div = hsp_div_400;

	hsp_sel = (pdr0 >> 20) & 0x3;
	if (!hsp_div[hsp_sel]) {
		pr_err("i.MX35 clk: illegal hsp clk selection 0x%x\n", hsp_sel);
		hsp_sel = 0;
	}

	imx_clk_fixed_factor("hsp", "arm", 1, hsp_div[hsp_sel]);

	imx_clk_fixed_factor("ahb", "arm", 1, aad->ahb);
	imx_clk_fixed_factor("ipg", "ahb", 1, 2);

	imx_clk_divider("arm_per_div", "arm", base + MX35_CCM_PDR4, 16, 6);
	imx_clk_divider("ahb_per_div", "ahb", base + MXC_CCM_PDR0, 12, 3);
	imx_clk_mux("ipg_per", base + MXC_CCM_PDR0, 26, 1, ipg_per_sel, ARRAY_SIZE(ipg_per_sel));

	imx_clk_mux("uart_sel", base + MX35_CCM_PDR3, 14, 1, std_sel, ARRAY_SIZE(std_sel));
	imx_clk_divider("uart_div", "uart_sel", base + MX35_CCM_PDR4, 10, 6);

	imx_clk_mux("esdhc_sel", base + MX35_CCM_PDR4, 9, 1, std_sel, ARRAY_SIZE(std_sel));
	imx_clk_divider("esdhc1_div", "esdhc_sel", base + MX35_CCM_PDR3, 0, 6);
	imx_clk_divider("esdhc2_div", "esdhc_sel", base + MX35_CCM_PDR3, 8, 6);
	imx_clk_divider("esdhc3_div", "esdhc_sel", base + MX35_CCM_PDR3, 16, 6);

	imx_clk_mux("ssi_sel", base + MX35_CCM_PDR2, 6, 1, std_sel, ARRAY_SIZE(std_sel));
	imx_clk_divider("ssi1_div_pre", "ssi_sel", base + MX35_CCM_PDR2, 24, 3);
	imx_clk_divider("ssi1_div_post", "ssi1_div_pre", base + MX35_CCM_PDR2, 0, 6);
	imx_clk_divider("ssi2_div_pre", "ssi_sel", base + MX35_CCM_PDR2, 27, 3);
	imx_clk_divider("ssi2_div_post", "ssi2_div_pre", base + MX35_CCM_PDR2, 8, 6);

	imx_clk_mux("usb_sel", base + MX35_CCM_PDR4, 9, 1, std_sel, ARRAY_SIZE(std_sel));
	imx_clk_divider("usb_div", "usb_sel", base + MX35_CCM_PDR4, 22, 6);

	imx_clk_divider("nfc_div", "ahb", base + MX35_CCM_PDR4, 28, 4);

	imx_clk_gate2("asrc_gate", "ipg", base + MX35_CCM_CGR0,  0);
	imx_clk_gate2("pata_gate", "ipg", base + MX35_CCM_CGR0,  2);
	imx_clk_gate2("audmux_gate", "ipg", base + MX35_CCM_CGR0,  4);
	imx_clk_gate2("can1_gate", "ipg", base + MX35_CCM_CGR0,  6);
	imx_clk_gate2("can2_gate", "ipg", base + MX35_CCM_CGR0,  8);
	imx_clk_gate2("cspi1_gate", "ipg", base + MX35_CCM_CGR0, 10);
	imx_clk_gate2("cspi2_gate", "ipg", base + MX35_CCM_CGR0, 12);
	imx_clk_gate2("ect_gate", "ipg", base + MX35_CCM_CGR0, 14);
	imx_clk_gate2("edio_gate",   "ipg", base + MX35_CCM_CGR0, 16);
	emi = imx_clk_gate2("emi_gate", "ipg", base + MX35_CCM_CGR0, 18);
	imx_clk_gate2("epit1_gate", "ipg", base + MX35_CCM_CGR0, 20);
	imx_clk_gate2("epit2_gate", "ipg", base + MX35_CCM_CGR0, 22);
	imx_clk_gate2("esai_gate",   "ipg", base + MX35_CCM_CGR0, 24);
	imx_clk_gate2("esdhc1_gate", "esdhc1_div", base + MX35_CCM_CGR0, 26);
	imx_clk_gate2("esdhc2_gate", "esdhc2_div", base + MX35_CCM_CGR0, 28);
	imx_clk_gate2("esdhc3_gate", "esdhc3_div", base + MX35_CCM_CGR0, 30);

	imx_clk_gate2("fec_gate", "ipg", base + MX35_CCM_CGR1,  0);
	gpio1 = imx_clk_gate2("gpio1_gate", "ipg", base + MX35_CCM_CGR1,  2);
	gpio2 = imx_clk_gate2("gpio2_gate", "ipg", base + MX35_CCM_CGR1,  4);
	gpio3 = imx_clk_gate2("gpio3_gate", "ipg", base + MX35_CCM_CGR1,  6);
	imx_clk_gate2("gpt_gate", "ipg", base + MX35_CCM_CGR1,  8);
	imx_clk_gate2("i2c1_gate", "ipg_per", base + MX35_CCM_CGR1, 10);
	imx_clk_gate2("i2c2_gate", "ipg_per", base + MX35_CCM_CGR1, 12);
	imx_clk_gate2("i2c3_gate", "ipg_per", base + MX35_CCM_CGR1, 14);
	imx_clk_gate2("iomuxc_gate", "ipg", base + MX35_CCM_CGR1, 16);
	imx_clk_gate2("ipu_gate", "hsp", base + MX35_CCM_CGR1, 18);
	imx_clk_gate2("kpp_gate", "ipg", base + MX35_CCM_CGR1, 20);
	imx_clk_gate2("mlb_gate", "ahb", base + MX35_CCM_CGR1, 22);
	imx_clk_gate2("mshc_gate", "dummy", base + MX35_CCM_CGR1, 24);
	imx_clk_gate2("owire_gate", "ipg_per", base + MX35_CCM_CGR1, 26);
	imx_clk_gate2("pwm_gate", "ipg_per", base + MX35_CCM_CGR1, 28);
	imx_clk_gate2("rngc_gate", "ipg", base + MX35_CCM_CGR1, 30);

	imx_clk_gate2("rtc_gate", "ipg", base + MX35_CCM_CGR2,  0);
	imx_clk_gate2("rtic_gate", "ahb", base + MX35_CCM_CGR2,  2);
	imx_clk_gate2("scc_gate", "ipg", base + MX35_CCM_CGR2,  4);
	imx_clk_gate2("sdma_gate", "ahb", base + MX35_CCM_CGR2,  6);
	spba = imx_clk_gate2("spba_gate", "ipg", base + MX35_CCM_CGR2,  8);
	imx_clk_gate2("spdif_gate", "spdif_div_post", base + MX35_CCM_CGR2, 10);
	imx_clk_gate2("ssi1_gate", "ssi1_div_post", base + MX35_CCM_CGR2, 12);
	imx_clk_gate2("ssi2_gate", "ssi2_div_post", base + MX35_CCM_CGR2, 14);
	imx_clk_gate2("uart1_gate", "uart_div", base + MX35_CCM_CGR2, 16);
	imx_clk_gate2("uart2_gate", "uart_div", base + MX35_CCM_CGR2, 18);
	imx_clk_gate2("uart3_gate", "uart_div", base + MX35_CCM_CGR2, 20);
	imx_clk_gate2("usbotg_gate", "ahb", base + MX35_CCM_CGR2, 22);
	imx_clk_gate2("wdog_gate", "ipg", base + MX35_CCM_CGR2, 24);
	imx_clk_gate2("max_gate", "dummy", base + MX35_CCM_CGR2, 26);
	imx_clk_gate2("audmux_gate", "ipg", base + MX35_CCM_CGR2, 30);

	imx_clk_gate2("csi_gate", "ipg", base + MX35_CCM_CGR3,  0);
	iim = imx_clk_gate2("iim_gate", "ipg", base + MX35_CCM_CGR3,  2);
	imx_clk_gate2("gpu2d_gate", "ahb", base + MX35_CCM_CGR3,  4);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	clk_prepare_enable(spba);
	clk_prepare_enable(gpio1);
	clk_prepare_enable(gpio2);
	clk_prepare_enable(gpio3);
	clk_prepare_enable(iim);
	clk_prepare_enable(emi);

	imx_print_silicon_rev("i.MX35", mx35_revision());

#ifdef CONFIG_MXC_USE_EPIT
	epit_timer_init(&epit1_clk,
			MX35_IO_ADDRESS(MX35_EPIT1_BASE_ADDR), MX35_INT_EPIT1);
#else
	mxc_timer_init(NULL, MX35_IO_ADDRESS(MX35_GPT1_BASE_ADDR),
			MX35_INT_GPT);
#endif

	return 0;
}
