/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
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

#include "crm-regs-imx5.h"
#include "clk.h"

/* Low-power Audio Playback Mode clock */
static char *lp_apm_sel[] = { "osc", };

/* This is used multiple times */
static char *standard_pll_sel[] = { "pll1_sw", "pll2_sw", "pll3_sw", "lp_apm", };
static char *periph_apm_sel[] = { "pll1_sw", "pll3_sw", "lp_apm", };
static char *main_bus_sel[] = { "pll2_sw", "periph_apm", };
static char *esdhc_c_sel[] = { "esdhc_a_podf", "esdhc_b_podf", };
static char *esdhc_d_sel[] = { "esdhc_a_podf", "esdhc_b_podf", };
static char *emi_slow_sel[] = { "main_bus", "ahb", };
static char *usb_phy_sel[] = { "osc", "usb_phy_podf", };
static char *mx51_ipu_di0_sel[] = { "di_pred", "osc", "ckih1", "tve_di", };
static char *mx53_ipu_di0_sel[] = { "di_pred", "osc", "ckih1", "di_pll4_podf", "dummy", "ldb_di0", };
static char *mx53_ldb_di0_sel[] = { "pll3_sw", "pll4_sw", };
static char *mx51_ipu_di1_sel[] = { "di_pred", "osc", "ckih1", "tve_di", "ipp_di1", };
static char *mx53_ipu_di1_sel[] = { "di_pred", "osc", "ckih1", "tve_di", "ipp_di1", "ldb_di1", };
static char *mx53_ldb_di1_sel[] = { "pll3_sw", "pll4_sw", };
static char *mx51_tve_ext_sel[] = { "osc", "ckih1", };
static char *mx53_tve_ext_sel[] = { "pll4_sw", "ckih1", };
static char *tve_sel[] = { "tve_pred", "tve_ext_sel", };
static char *ipu_sel[] = { "axi_a", "axi_b", "emi_slow_gate", "ahb", };
static char *vpu_sel[] = { "axi_a", "axi_b", "emi_slow_gate", "ahb", };

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	},

static struct clk_lookup mx5_lookups[] = {
	clkdev("imx-gpt.0", "per", "gpt_gate")
	clkdev("imx-gpt.0", "ipg", "gpt_ipg_gate")
	clkdev("imx21-uart.0", "per", "uart1_per_gate")
	clkdev("imx21-uart.0", "ipg", "uart1_ipg_gate")
	clkdev("imx21-uart.1", "per", "uart2_per_gate")
	clkdev("imx21-uart.1", "ipg", "uart2_ipg_gate")
	clkdev("imx21-uart.2", "per", "uart3_per_gate")
	clkdev("imx21-uart.2", "ipg", "uart3_ipg_gate")
	clkdev("imx21-uart.3", "per", "uart4_per_gate")
	clkdev("imx21-uart.3", "ipg", "uart4_ipg_gate")
	clkdev("imx21-uart.4", "per", "uart5_per_gate")
	clkdev("imx21-uart.4", "ipg", "uart5_ipg_gate")
	clkdev("imx51-ecspi.0", "per", "ecspi1_per_gate")
	clkdev("imx51-ecspi.0", "ipg", "ecspi1_ipg_gate")
	clkdev("imx51-ecspi.1", "per", "ecspi2_per_gate")
	clkdev("imx51-ecspi.1", "ipg", "ecspi2_ipg_gate")
	clkdev("imx51-cspi.0", NULL, "cspi_ipg_gate")
	clkdev("mxc_pwm.0", "pwm", "pwm1_ipg_gate")
	clkdev("mxc_pwm.1", "pwm", "pwm2_ipg_gate")
	clkdev("imx-i2c.0", NULL, "i2c1_gate")
	clkdev("imx-i2c.1", NULL, "i2c2_gate")
	clkdev("mxc-ehci.0", "per", "usboh3_per_gate")
	clkdev("mxc-ehci.0", "ipg", "usboh3_gate")
	clkdev("mxc-ehci.0", "ahb", "usboh3_gate")
	clkdev("mxc-ehci.1", "per", "usboh3_per_gate")
	clkdev("mxc-ehci.1", "ipg", "usboh3_gate")
	clkdev("mxc-ehci.1", "ahb", "usboh3_gate")
	clkdev("mxc-ehci.2", "per", "usboh3_per_gate")
	clkdev("mxc-ehci.2", "ipg", "usboh3_gate")
	clkdev("mxc-ehci.2", "ahb", "usboh3_gate")
	clkdev("fsl-usb2-udc", "per", "usboh3_per_gate")
	clkdev("fsl-usb2-udc", "ipg", "usboh3_gate")
	clkdev("fsl-usb2-udc", "ahb", "usboh3_gate")
	clkdev("mxc_nand", NULL, "nfc_gate")
	clkdev("imx-ssi.0", NULL, "ssi1_ipg_gate")
	clkdev("imx-ssi.1", NULL, "ssi1_ipg_gate")
	clkdev("imx-ssi.2", NULL, "ssi1_ipg_gate")
	clkdev("imx35-sdma", NULL, "sdma_gate")
	clkdev(NULL, "cpu", "cpu_podf")
	clkdev(NULL, "iim", "iim_gate")
	clkdev("imx2-wdt.0", NULL, "dummy")
	clkdev("imx2-wdt.1", NULL, "dummy")
	clkdev("imx-keypad", NULL, "dummy")
	clkdev("imx-tve.0", NULL, "tve_gate")
	clkdev("imx-tve.0", "di1", "ipu_di1_gate")
};

static struct clk_lookup mx51_lookups[] = {
	clkdev("imx-i2c.2", NULL, "hsi2c_gate")
	clkdev(NULL, "mipi_hsp", "mx51_mipi")
	clkdev("imx51-vpu.0", NULL, "vpu")
	clkdev("imx27-fec.0", NULL, "fec_gate")
	clkdev(NULL, "gpc_dvfs", "gpc_dvfs")
	clkdev("imx51-ipu", NULL, "ipu")
	clkdev("imx51-ipu", "di0", "ipu_di0_gate")
	clkdev("imx51-ipu", "di1", "ipu_di1_gate")
	clkdev("imx51-ipu", "hsp", "ipu_gate")
	clkdev("mxc-ehci.0", "usb_phy1", "usb_phy_gate")
	clkdev("sdhci-esdhc-imx51.0", "ipg", "esdhc1_ipg_gate")
	clkdev("sdhci-esdhc-imx51.0", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx51.0", "per", "esdhc1_per_gate")
	clkdev("sdhci-esdhc-imx51.1", "ipg", "esdhc2_ipg_gate")
	clkdev("sdhci-esdhc-imx51.1", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx51.1", "per", "esdhc2_per_gate")
	clkdev("sdhci-esdhc-imx51.2", "ipg", "esdhc3_ipg_gate")
	clkdev("sdhci-esdhc-imx51.2", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx51.2", "per", "esdhc3_per_gate")
	clkdev("sdhci-esdhc-imx51.3", "ipg", "esdhc4_ipg_gate")
	clkdev("sdhci-esdhc-imx51.3", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx51.3", "per", "esdhc4_per_gate")
};

static struct clk_lookup mx53_lookups[] = {
	clkdev("imx53-vpu.0", NULL, "vpu")
	clkdev("imx-i2c.2", NULL, "i2c3_gate")
	clkdev("imx25-fec.0", NULL, "fec_gate")
	clkdev("imx53-ipu", NULL, "ipu")
	clkdev("imx53-ipu", "di0", "ipu_di0_gate")
	clkdev("imx53-ipu", "di1", "ipu_di1_gate")
	clkdev("imx53-ipu", "hsp", "ipu_gate")
	clkdev("mxc-ehci.0", "usb_phy1", "usb_phy1_gate")
	clkdev("sdhci-esdhc-imx53.0", "ipg", "esdhc1_ipg_gate")
	clkdev("sdhci-esdhc-imx53.0", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx53.0", "per", "esdhc1_per_gate")
	clkdev("sdhci-esdhc-imx53.1", "ipg", "esdhc2_ipg_gate")
	clkdev("sdhci-esdhc-imx53.1", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx53.1", "per", "esdhc2_per_gate")
	clkdev("sdhci-esdhc-imx53.2", "ipg", "esdhc3_ipg_gate")
	clkdev("sdhci-esdhc-imx53.2", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx53.2", "per", "esdhc3_per_gate")
	clkdev("sdhci-esdhc-imx53.3", "ipg", "esdhc4_ipg_gate")
	clkdev("sdhci-esdhc-imx53.3", "ahb", "dummy")
	clkdev("sdhci-esdhc-imx53.3", "per", "esdhc4_per_gate")
};

static struct clk *pll1, *pll2, *pll3, *pll4, *iim,
		  *emi_fast, *ahb_max, *aips_tz1, *aips_tz2,
		  *spba, *tmax1, *tmax2, *tmax3, *esdhc_a_sel, *esdhc_b_sel,
		  *esdhc_a_podf, *esdhc_b_podf;

static void __init mx5_clocks_common_init(unsigned long rate_ckil,
		unsigned long rate_osc, unsigned long rate_ckih1,
		unsigned long rate_ckih2)
{
	imx_clk_fixed("dummy", 0);
	imx_clk_fixed("ckil", rate_ckil);
	imx_clk_fixed("osc", rate_osc);
	imx_clk_fixed("ckih1", rate_ckih1);
	imx_clk_fixed("ckih2", rate_ckih2);

	imx_clk_mux("lp_apm", MXC_CCM_CCSR, 9, 1, lp_apm_sel,
			ARRAY_SIZE(lp_apm_sel));
	imx_clk_mux("periph_apm", MXC_CCM_CBCMR, 12, 2, periph_apm_sel,
			ARRAY_SIZE(periph_apm_sel));
	imx_clk_mux("main_bus", MXC_CCM_CBCDR, 25, 1, main_bus_sel,
			ARRAY_SIZE(main_bus_sel));
	imx_clk_divider("ahb", "main_bus", MXC_CCM_CBCDR, 10, 3);
	ahb_max = imx_clk_gate2("ahb_max", "ahb", MXC_CCM_CCGR0, 28);
	aips_tz1 = imx_clk_gate2("aips_tz1", "ahb", MXC_CCM_CCGR0, 24);
	aips_tz2 = imx_clk_gate2("aips_tz2", "ahb", MXC_CCM_CCGR0, 26);
	tmax1 = imx_clk_gate2("tmax1", "ahb", MXC_CCM_CCGR1, 0);
	tmax2 = imx_clk_gate2("tmax2", "ahb", MXC_CCM_CCGR1, 2);
	tmax3 = imx_clk_gate2("tmax3", "ahb", MXC_CCM_CCGR1, 4);
	spba = imx_clk_gate2("spba", "ipg", MXC_CCM_CCGR5, 0);
	imx_clk_divider("ipg", "ahb", MXC_CCM_CBCDR, 8, 2);
	imx_clk_divider("axi_a", "main_bus", MXC_CCM_CBCDR, 16, 3);
	imx_clk_divider("axi_b", "main_bus", MXC_CCM_CBCDR, 19, 3);
	imx_clk_mux("uart_sel", MXC_CCM_CSCMR1, 24, 2, standard_pll_sel,
			ARRAY_SIZE(standard_pll_sel));
	imx_clk_divider("uart_pred", "uart_sel", MXC_CCM_CSCDR1, 3, 3);
	imx_clk_divider("uart_root", "uart_pred", MXC_CCM_CSCDR1, 0, 3);

	esdhc_a_sel = imx_clk_mux("esdhc_a_sel", MXC_CCM_CSCMR1, 20, 2, standard_pll_sel,
			ARRAY_SIZE(standard_pll_sel));
	esdhc_b_sel = imx_clk_mux("esdhc_b_sel", MXC_CCM_CSCMR1, 16, 2, standard_pll_sel,
			ARRAY_SIZE(standard_pll_sel));
	imx_clk_divider("esdhc_a_pred", "esdhc_a_sel", MXC_CCM_CSCDR1, 16, 3);
	esdhc_a_podf = imx_clk_divider("esdhc_a_podf", "esdhc_a_pred", MXC_CCM_CSCDR1, 11, 3);
	imx_clk_divider("esdhc_b_pred", "esdhc_b_sel", MXC_CCM_CSCDR1, 22, 3);
	esdhc_b_podf = imx_clk_divider("esdhc_b_podf", "esdhc_b_pred", MXC_CCM_CSCDR1, 19, 3);
	imx_clk_mux("esdhc_c_sel", MXC_CCM_CSCMR1, 19, 1, esdhc_c_sel, ARRAY_SIZE(esdhc_c_sel));
	imx_clk_mux("esdhc_d_sel", MXC_CCM_CSCMR1, 18, 1, esdhc_d_sel, ARRAY_SIZE(esdhc_d_sel));

	imx_clk_mux("emi_sel", MXC_CCM_CBCDR, 26, 1, emi_slow_sel, ARRAY_SIZE(emi_slow_sel));
	imx_clk_divider("emi_slow_podf", "emi_sel", MXC_CCM_CBCDR, 22, 3);
	imx_clk_divider("nfc_podf", "emi_slow_podf", MXC_CCM_CBCDR, 13, 3);
	imx_clk_mux("ecspi_sel", MXC_CCM_CSCMR1, 4, 2, standard_pll_sel,
			ARRAY_SIZE(standard_pll_sel));
	imx_clk_divider("ecspi_pred", "ecspi_sel", MXC_CCM_CSCDR2, 25, 3);
	imx_clk_divider("ecspi_podf", "ecspi_pred", MXC_CCM_CSCDR2, 19, 6);
	imx_clk_mux("usboh3_sel", MXC_CCM_CSCMR1, 22, 2, standard_pll_sel,
			ARRAY_SIZE(standard_pll_sel));
	imx_clk_divider("usboh3_pred", "usboh3_sel", MXC_CCM_CSCDR1, 8, 3);
	imx_clk_divider("usboh3_podf", "usboh3_pred", MXC_CCM_CSCDR1, 6, 2);
	imx_clk_divider("usb_phy_pred", "pll3_sw", MXC_CCM_CDCDR, 3, 3);
	imx_clk_divider("usb_phy_podf", "usb_phy_pred", MXC_CCM_CDCDR, 0, 3);
	imx_clk_mux("usb_phy_sel", MXC_CCM_CSCMR1, 26, 1, usb_phy_sel,
			ARRAY_SIZE(usb_phy_sel));
	imx_clk_divider("cpu_podf", "pll1_sw", MXC_CCM_CACRR, 0, 3);
	imx_clk_divider("di_pred", "pll3_sw", MXC_CCM_CDCDR, 6, 3);
	imx_clk_fixed("tve_di", 65000000); /* FIXME */
	imx_clk_mux("tve_sel", MXC_CCM_CSCMR1, 7, 1, tve_sel, ARRAY_SIZE(tve_sel));
	iim = imx_clk_gate2("iim_gate", "ipg", MXC_CCM_CCGR0, 30);
	imx_clk_gate2("uart1_ipg_gate", "ipg", MXC_CCM_CCGR1, 6);
	imx_clk_gate2("uart1_per_gate", "uart_root", MXC_CCM_CCGR1, 8);
	imx_clk_gate2("uart2_ipg_gate", "ipg", MXC_CCM_CCGR1, 10);
	imx_clk_gate2("uart2_per_gate", "uart_root", MXC_CCM_CCGR1, 12);
	imx_clk_gate2("uart3_ipg_gate", "ipg", MXC_CCM_CCGR1, 14);
	imx_clk_gate2("uart3_per_gate", "uart_root", MXC_CCM_CCGR1, 16);
	imx_clk_gate2("i2c1_gate", "ipg", MXC_CCM_CCGR1, 18);
	imx_clk_gate2("i2c2_gate", "ipg", MXC_CCM_CCGR1, 20);
	imx_clk_gate2("gpt_ipg_gate", "ipg", MXC_CCM_CCGR2, 20);
	imx_clk_gate2("pwm1_ipg_gate", "ipg", MXC_CCM_CCGR2, 10);
	imx_clk_gate2("pwm1_hf_gate", "ipg", MXC_CCM_CCGR2, 12);
	imx_clk_gate2("pwm2_ipg_gate", "ipg", MXC_CCM_CCGR2, 14);
	imx_clk_gate2("pwm2_hf_gate", "ipg", MXC_CCM_CCGR2, 16);
	imx_clk_gate2("gpt_gate", "ipg", MXC_CCM_CCGR2, 18);
	imx_clk_gate2("fec_gate", "ipg", MXC_CCM_CCGR2, 24);
	imx_clk_gate2("usboh3_gate", "ipg", MXC_CCM_CCGR2, 26);
	imx_clk_gate2("usboh3_per_gate", "usboh3_podf", MXC_CCM_CCGR2, 28);
	imx_clk_gate2("esdhc1_ipg_gate", "ipg", MXC_CCM_CCGR3, 0);
	imx_clk_gate2("esdhc2_ipg_gate", "ipg", MXC_CCM_CCGR3, 4);
	imx_clk_gate2("esdhc3_ipg_gate", "ipg", MXC_CCM_CCGR3, 8);
	imx_clk_gate2("esdhc4_ipg_gate", "ipg", MXC_CCM_CCGR3, 12);
	imx_clk_gate2("ssi1_ipg_gate", "ipg", MXC_CCM_CCGR3, 16);
	imx_clk_gate2("ssi2_ipg_gate", "ipg", MXC_CCM_CCGR3, 18);
	imx_clk_gate2("ssi3_ipg_gate", "ipg", MXC_CCM_CCGR3, 20);
	imx_clk_gate2("mipi_hsc1_gate", "ipg", MXC_CCM_CCGR4, 6);
	imx_clk_gate2("mipi_hsc2_gate", "ipg", MXC_CCM_CCGR4, 8);
	imx_clk_gate2("mipi_esc_gate", "ipg", MXC_CCM_CCGR4, 10);
	imx_clk_gate2("mipi_hsp_gate", "ipg", MXC_CCM_CCGR4, 12);
	imx_clk_gate2("ecspi1_ipg_gate", "ipg", MXC_CCM_CCGR4, 18);
	imx_clk_gate2("ecspi1_per_gate", "ecspi_podf", MXC_CCM_CCGR4, 20);
	imx_clk_gate2("ecspi2_ipg_gate", "ipg", MXC_CCM_CCGR4, 22);
	imx_clk_gate2("ecspi2_per_gate", "ecspi_podf", MXC_CCM_CCGR4, 24);
	imx_clk_gate2("cspi_ipg_gate", "ipg", MXC_CCM_CCGR4, 26);
	imx_clk_gate2("sdma_gate", "ipg", MXC_CCM_CCGR4, 30);
	emi_fast = imx_clk_gate2("emi_fast_gate", "dummy", MXC_CCM_CCGR5, 14);
	imx_clk_gate2("emi_slow_gate", "emi_slow_podf", MXC_CCM_CCGR5, 16);
	imx_clk_mux("ipu_sel", MXC_CCM_CBCMR, 6, 2, ipu_sel, ARRAY_SIZE(ipu_sel));
	imx_clk_gate2("ipu_gate", "ipu_sel", MXC_CCM_CCGR5, 10);
	imx_clk_gate2("nfc_gate", "nfc_podf", MXC_CCM_CCGR5, 20);
	imx_clk_gate2("ipu_di0_gate", "ipu_di0_sel", MXC_CCM_CCGR6, 10);
	imx_clk_gate2("ipu_di1_gate", "ipu_di1_sel", MXC_CCM_CCGR6, 12);
	imx_clk_mux("vpu_sel", MXC_CCM_CBCMR, 14, 2, vpu_sel, ARRAY_SIZE(vpu_sel));
	imx_clk_gate2("vpu_gate", "vpu_sel", MXC_CCM_CCGR5, 6);
	imx_clk_gate2("vpu_reference_gate", "osc", MXC_CCM_CCGR5, 8);
	imx_clk_gate2("uart4_ipg_gate", "ipg", MXC_CCM_CCGR7, 8);
	imx_clk_gate2("uart4_per_gate", "uart_root", MXC_CCM_CCGR7, 10);
	imx_clk_gate2("uart5_ipg_gate", "ipg", MXC_CCM_CCGR7, 12);
	imx_clk_gate2("uart5_per_gate", "uart_root", MXC_CCM_CCGR7, 14);
	imx_clk_gate2("gpc_dvfs", "dummy", MXC_CCM_CCGR5, 24);

	clkdev_add_table(mx5_lookups, ARRAY_SIZE(mx5_lookups));

	/* Set SDHC parents to be PLL2 */
	clk_set_parent(esdhc_a_sel, pll2);
	clk_set_parent(esdhc_b_sel, pll2);

	clk_prepare_enable(ahb_max); /* esdhc3 */
	clk_prepare_enable(aips_tz1);
	clk_prepare_enable(aips_tz2); /* fec */
	clk_prepare_enable(spba);
	clk_prepare_enable(emi_fast); /* fec */
	clk_prepare_enable(tmax1);
	clk_prepare_enable(tmax2); /* esdhc2, fec */
	clk_prepare_enable(tmax3); /* esdhc1, esdhc4 */
}

int __init mx51_clocks_init(unsigned long rate_ckil, unsigned long rate_osc,
			unsigned long rate_ckih1, unsigned long rate_ckih2)
{
	pll1 = imx_clk_pllv2("pll1_sw", "osc", MX51_DPLL1_BASE);
	pll2 = imx_clk_pllv2("pll2_sw", "osc", MX51_DPLL2_BASE);
	pll3 = imx_clk_pllv2("pll3_sw", "osc", MX51_DPLL3_BASE);
	imx_clk_mux("ipu_di0_sel", MXC_CCM_CSCMR2, 26, 3, mx51_ipu_di0_sel,
			ARRAY_SIZE(mx51_ipu_di0_sel));
	imx_clk_mux("ipu_di1_sel", MXC_CCM_CSCMR2, 29, 3, mx51_ipu_di1_sel,
			ARRAY_SIZE(mx51_ipu_di1_sel));
	imx_clk_mux("tve_ext_sel", MXC_CCM_CSCMR1, 6, 1, mx51_tve_ext_sel,
			ARRAY_SIZE(mx51_tve_ext_sel));
	imx_clk_gate2("tve_gate", "tve_sel", MXC_CCM_CCGR2, 30);
	imx_clk_divider("tve_pred", "pll3_sw", MXC_CCM_CDCDR, 28, 3);
	imx_clk_gate2("esdhc1_per_gate", "esdhc_a_podf", MXC_CCM_CCGR3, 2);
	imx_clk_gate2("esdhc2_per_gate", "esdhc_b_podf", MXC_CCM_CCGR3, 6);
	imx_clk_gate2("esdhc3_per_gate", "esdhc_c_sel", MXC_CCM_CCGR3, 10);
	imx_clk_gate2("esdhc4_per_gate", "esdhc_d_sel", MXC_CCM_CCGR3, 14);
	imx_clk_gate2("usb_phy_gate", "usb_phy_sel", MXC_CCM_CCGR2, 0);
	imx_clk_gate2("hsi2c_gate", "ipg", MXC_CCM_CCGR1, 22);

	mx5_clocks_common_init(rate_ckil, rate_osc, rate_ckih1, rate_ckih2);

	clkdev_add_table(mx51_lookups, ARRAY_SIZE(mx51_lookups));

	/* set SDHC root clock to 166.25MHZ*/
	clk_set_rate(esdhc_a_podf, 166250000);
	clk_set_rate(esdhc_b_podf, 166250000);

	/* System timer */
	mxc_timer_init(NULL, MX51_IO_ADDRESS(MX51_GPT1_BASE_ADDR),
		MX51_INT_GPT);

	clk_prepare_enable(iim);
	imx_print_silicon_rev("i.MX51", mx51_revision());
	clk_disable_unprepare(iim);

	return 0;
}

int __init mx53_clocks_init(unsigned long rate_ckil, unsigned long rate_osc,
			unsigned long rate_ckih1, unsigned long rate_ckih2)
{
	pll1 = imx_clk_pllv2("pll1_sw", "osc", MX53_DPLL1_BASE);
	pll2 = imx_clk_pllv2("pll2_sw", "osc", MX53_DPLL2_BASE);
	pll3 = imx_clk_pllv2("pll3_sw", "osc", MX53_DPLL3_BASE);
	pll4 = imx_clk_pllv2("pll4_sw", "osc", MX53_DPLL4_BASE);

	imx_clk_mux("ldb_di1_sel", MXC_CCM_CSCMR2, 9, 1, mx53_ldb_di1_sel,
			ARRAY_SIZE(mx53_ldb_di1_sel));
	imx_clk_fixed_factor("ldb_di1_div_3_5", "ldb_di1_sel", 2, 7);
	imx_clk_divider("ldb_di1_div", "ldb_di1_div_3_5", MXC_CCM_CSCMR2, 11, 1);
	imx_clk_divider("di_pll4_podf", "pll4_sw", MXC_CCM_CDCDR, 16, 3);
	imx_clk_mux("ldb_di0_sel", MXC_CCM_CSCMR2, 8, 1, mx53_ldb_di0_sel,
			ARRAY_SIZE(mx53_ldb_di0_sel));
	imx_clk_fixed_factor("ldb_di0_div_3_5", "ldb_di0_sel", 2, 7);
	imx_clk_divider("ldb_di0_div", "ldb_di0_div_3_5", MXC_CCM_CSCMR2, 10, 1);
	imx_clk_gate2("ldb_di0_gate", "ldb_di0_div", MXC_CCM_CCGR6, 28);
	imx_clk_gate2("ldb_di1_gate", "ldb_di1_div", MXC_CCM_CCGR6, 30);
	imx_clk_mux("ipu_di0_sel", MXC_CCM_CSCMR2, 26, 3, mx53_ipu_di0_sel,
			ARRAY_SIZE(mx53_ipu_di0_sel));
	imx_clk_mux("ipu_di1_sel", MXC_CCM_CSCMR2, 29, 3, mx53_ipu_di1_sel,
			ARRAY_SIZE(mx53_ipu_di1_sel));
	imx_clk_mux("tve_ext_sel", MXC_CCM_CSCMR1, 6, 1, mx53_tve_ext_sel,
			ARRAY_SIZE(mx53_tve_ext_sel));
	imx_clk_gate2("tve_gate", "tve_pred", MXC_CCM_CCGR2, 30);
	imx_clk_divider("tve_pred", "tve_ext_sel", MXC_CCM_CDCDR, 28, 3);
	imx_clk_gate2("esdhc1_per_gate", "esdhc_a_podf", MXC_CCM_CCGR3, 2);
	imx_clk_gate2("esdhc2_per_gate", "esdhc_c_sel", MXC_CCM_CCGR3, 6);
	imx_clk_gate2("esdhc3_per_gate", "esdhc_b_podf", MXC_CCM_CCGR3, 10);
	imx_clk_gate2("esdhc4_per_gate", "esdhc_d_sel", MXC_CCM_CCGR3, 14);
	imx_clk_gate2("usb_phy1_gate", "usb_phy_sel", MXC_CCM_CCGR4, 10);
	imx_clk_gate2("usb_phy2_gate", "usb_phy_sel", MXC_CCM_CCGR4, 12);
	imx_clk_gate2("i2c3_gate", "ipg", MXC_CCM_CCGR1, 22);

	mx5_clocks_common_init(rate_ckil, rate_osc, rate_ckih1, rate_ckih2);

	clkdev_add_table(mx53_lookups, ARRAY_SIZE(mx53_lookups));

	/* set SDHC root clock to 200MHZ*/
	clk_set_rate(esdhc_a_podf, 200000000);
	clk_set_rate(esdhc_b_podf, 200000000);

	/* System timer */
	mxc_timer_init(NULL, MX53_IO_ADDRESS(MX53_GPT1_BASE_ADDR),
		MX53_INT_GPT);

	clk_prepare_enable(iim);
	imx_print_silicon_rev("i.MX53", mx53_revision());
	clk_disable_unprepare(iim);

	return 0;
}

#ifdef CONFIG_OF
static void __init clk_get_freq_dt(unsigned long *ckil, unsigned long *osc,
				   unsigned long *ckih1, unsigned long *ckih2)
{
	struct device_node *np;

	/* retrieve the freqency of fixed clocks from device tree */
	for_each_compatible_node(np, NULL, "fixed-clock") {
		u32 rate;
		if (of_property_read_u32(np, "clock-frequency", &rate))
			continue;

		if (of_device_is_compatible(np, "fsl,imx-ckil"))
			*ckil = rate;
		else if (of_device_is_compatible(np, "fsl,imx-osc"))
			*osc = rate;
		else if (of_device_is_compatible(np, "fsl,imx-ckih1"))
			*ckih1 = rate;
		else if (of_device_is_compatible(np, "fsl,imx-ckih2"))
			*ckih2 = rate;
	}
}

int __init mx51_clocks_init_dt(void)
{
	unsigned long ckil, osc, ckih1, ckih2;

	clk_get_freq_dt(&ckil, &osc, &ckih1, &ckih2);
	return mx51_clocks_init(ckil, osc, ckih1, ckih2);
}

int __init mx53_clocks_init_dt(void)
{
	unsigned long ckil, osc, ckih1, ckih2;

	clk_get_freq_dt(&ckil, &osc, &ckih1, &ckih2);
	return mx53_clocks_init(ckil, osc, ckih1, ckih2);
}
#endif
