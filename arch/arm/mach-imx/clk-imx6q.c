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
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mach/common.h>
#include "clk.h"

#define CCGR0				0x68
#define CCGR1				0x6c
#define CCGR2				0x70
#define CCGR3				0x74
#define CCGR4				0x78
#define CCGR5				0x7c
#define CCGR6				0x80
#define CCGR7				0x84

#define CLPCR				0x54
#define BP_CLPCR_LPM			0
#define BM_CLPCR_LPM			(0x3 << 0)
#define BM_CLPCR_BYPASS_PMIC_READY	(0x1 << 2)
#define BM_CLPCR_ARM_CLK_DIS_ON_LPM	(0x1 << 5)
#define BM_CLPCR_SBYOS			(0x1 << 6)
#define BM_CLPCR_DIS_REF_OSC		(0x1 << 7)
#define BM_CLPCR_VSTBY			(0x1 << 8)
#define BP_CLPCR_STBY_COUNT		9
#define BM_CLPCR_STBY_COUNT		(0x3 << 9)
#define BM_CLPCR_COSC_PWRDOWN		(0x1 << 11)
#define BM_CLPCR_WB_PER_AT_LPM		(0x1 << 16)
#define BM_CLPCR_WB_CORE_AT_LPM		(0x1 << 17)
#define BM_CLPCR_BYP_MMDC_CH0_LPM_HS	(0x1 << 19)
#define BM_CLPCR_BYP_MMDC_CH1_LPM_HS	(0x1 << 21)
#define BM_CLPCR_MASK_CORE0_WFI		(0x1 << 22)
#define BM_CLPCR_MASK_CORE1_WFI		(0x1 << 23)
#define BM_CLPCR_MASK_CORE2_WFI		(0x1 << 24)
#define BM_CLPCR_MASK_CORE3_WFI		(0x1 << 25)
#define BM_CLPCR_MASK_SCU_IDLE		(0x1 << 26)
#define BM_CLPCR_MASK_L2CC_IDLE		(0x1 << 27)

static void __iomem *ccm_base;

void __init imx6q_clock_map_io(void) { }

int imx6q_set_lpm(enum mxc_cpu_pwr_mode mode)
{
	u32 val = readl_relaxed(ccm_base + CLPCR);

	val &= ~BM_CLPCR_LPM;
	switch (mode) {
	case WAIT_CLOCKED:
		break;
	case WAIT_UNCLOCKED:
		val |= 0x1 << BP_CLPCR_LPM;
		break;
	case STOP_POWER_ON:
		val |= 0x2 << BP_CLPCR_LPM;
		break;
	case WAIT_UNCLOCKED_POWER_OFF:
		val |= 0x1 << BP_CLPCR_LPM;
		val &= ~BM_CLPCR_VSTBY;
		val &= ~BM_CLPCR_SBYOS;
		break;
	case STOP_POWER_OFF:
		val |= 0x2 << BP_CLPCR_LPM;
		val |= 0x3 << BP_CLPCR_STBY_COUNT;
		val |= BM_CLPCR_VSTBY;
		val |= BM_CLPCR_SBYOS;
		break;
	default:
		return -EINVAL;
	}

	writel_relaxed(val, ccm_base + CLPCR);

	return 0;
}

#define clkdev(d, n, c) \
	{ \
		.dev_id = d, \
		.con_id = n, \
		.clkname = c, \
	}

static struct clk_lookup lookups[] = {
	clkdev("mmdc_ch0_axi", NULL, "mmdc_ch0_axi"),
	clkdev("mmdc_ch1_axi", NULL, "mmdc_ch1_axi"),
	clkdev("uart_ipg", NULL, "uart_ipg"),
	clkdev("uart_serial", NULL, "uart_serial"),
	clkdev("imx-gpt.0", "ipg", "gpt_ipg"),
	clkdev("imx-gpt.0", "per", "gpt_ipg_per"),
	clkdev("smp_twd", NULL, "twd"),
	clkdev("2020000.uart", NULL, "uart_serial"),
	clkdev("21e8000.uart", NULL, "uart_serial"),
	clkdev("21ec000.uart", NULL, "uart_serial"),
	clkdev("21f0000.uart", NULL, "uart_serial"),
	clkdev("21f4000.uart", NULL, "uart_serial"),
	clkdev("2188000.enet", NULL, "enet"),
	clkdev("2190000.usdhc", NULL, "usdhc1"),
	clkdev("2194000.usdhc", NULL, "usdhc2"),
	clkdev("2198000.usdhc", NULL, "usdhc3"),
	clkdev("219c000.usdhc", NULL, "usdhc4"),
	clkdev("21a0000.i2c", NULL, "i2c1"),
	clkdev("21a4000.i2c", NULL, "i2c2"),
	clkdev("21a8000.i2c", NULL, "i2c3"),
	clkdev("2008000.ecspi", NULL, "ecspi1"),
	clkdev("200c000.ecspi", NULL, "ecspi2"),
	clkdev("2010000.ecspi", NULL, "ecspi3"),
	clkdev("2014000.ecspi", NULL, "ecspi4"),
	clkdev("2018000.ecspi", NULL, "ecspi5"),
	clkdev("20ec000.sdma", NULL, "sdma"),
	clkdev("20bc000.wdog", NULL, "dummy"),
	clkdev("20c0000.wdog", NULL, "dummy"),
};

static char *step_sels[]	= { "osc", "pll2_pfd2_396m", };
static char *pll1_sw_sels[]	= { "pll1_sys", "step", };
static char *periph_pre_sels[]	= { "pll2_bus", "pll2_pfd2_396m", "pll2_pfd0_352m", "pll2_198m", };
static char *periph_clk2_sels[]	= { "pll3_usb_otg", "osc", };
static char *periph_sels[]	= { "periph_pre", "periph_clk2", };
static char *periph2_sels[]	= { "periph2_pre", "periph2_clk2", };
static char *axi_sels[]		= { "periph", "pll2_pfd2_396m", "pll3_pfd1_540m", };
static char *audio_sels[]	= { "pll4_audio", "pll3_pfd2_508m", "pll3_pfd3_454m", "pll3_usb_otg", };
static char *gpu_axi_sels[]	= { "axi", "ahb", };
static char *gpu2d_core_sels[]	= { "axi", "pll3_usb_otg", "pll2_pfd0_352m", "pll2_pfd2_396m", };
static char *gpu3d_core_sels[]	= { "mmdc_ch0_axi", "pll3_usb_otg", "pll2_pfd1_594m", "pll2_pfd2_396m", };
static char *gpu3d_shader_sels[] = { "mmdc_ch0_axi", "pll3_usb_otg", "pll2_pfd1_594m", "pll2_pfd9_720m", };
static char *ipu_sels[]		= { "mmdc_ch0_axi", "pll2_pfd2_396m", "pll3_120m", "pll3_pfd1_540m", };
static char *ldb_di_sels[]	= { "pll5_video", "pll2_pfd0_352m", "pll2_pfd2_396m", "pll3_pfd1_540m", };
static char *ipu_di_pre_sels[]	= { "mmdc_ch0_axi", "pll3_usb_otg", "pll5_video", "pll2_pfd0_352m", "pll2_pfd2_396m", "pll3_pfd1_540m", };
static char *ipu1_di0_sels[]	= { "ipu1_di0_pre", "dummy", "dummy", "ldb_di0", "ldb_di1", };
static char *ipu1_di1_sels[]	= { "ipu1_di1_pre", "dummy", "dummy", "ldb_di0", "ldb_di1", };
static char *ipu2_di0_sels[]	= { "ipu2_di0_pre", "dummy", "dummy", "ldb_di0", "ldb_di1", };
static char *ipu2_di1_sels[]	= { "ipu2_di1_pre", "dummy", "dummy", "ldb_di0", "ldb_di1", };
static char *hsi_tx_sels[]	= { "pll3_120m", "pll2_pfd2_396m", };
static char *pcie_axi_sels[]	= { "axi", "ahb", };
static char *ssi_sels[]		= { "pll3_pfd2_508m", "pll3_pfd3_454m", "pll4_audio", };
static char *usdhc_sels[]	= { "pll2_pfd2_396m", "pll2_pfd0_352m", };
static char *enfc_sels[]	= { "pll2_pfd0_352m", "pll2_bus", "pll3_usb_otg", "pll2_pfd2_396m", };
static char *emi_sels[]		= { "axi", "pll3_usb_otg", "pll2_pfd2_396m", "pll2_pfd0_352m", };
static char *vdo_axi_sels[]	= { "axi", "ahb", };
static char *vpu_axi_sels[]	= { "axi", "pll2_pfd2_396m", "pll2_pfd0_352m", };
static char *cko1_sels[]	= { "pll3_usb_otg", "pll2_bus", "pll1_sys", "pll5_video",
				    "dummy", "axi", "enfc", "ipu1_di0", "ipu1_di1", "ipu2_di0",
				    "ipu2_di1", "ahb", "ipg", "ipg_per", "ckil", "pll4_audio", };

static const char * const clks_init_on[] __initconst = {
	"mmdc_ch0_axi", "mmdc_ch1_axi", "uart_ipg", "uart_serial",
};

int __init mx6q_clocks_init(void)
{
	struct device_node *np;
	struct clk *clk;
	void __iomem *base;
	int i, irq;

	imx_clk_fixed("dummy", 0);

	/* retrieve the freqency of fixed clocks from device tree */
	for_each_compatible_node(np, NULL, "fixed-clock") {
		u32 rate;
		if (of_property_read_u32(np, "clock-frequency", &rate))
			continue;

		if (of_device_is_compatible(np, "fsl,imx-ckil"))
			imx_clk_fixed("ckil", rate);
		else if (of_device_is_compatible(np, "fsl,imx-ckih1"))
			imx_clk_fixed("ckih", rate);
		else if (of_device_is_compatible(np, "fsl,imx-osc"))
			imx_clk_fixed("osc", rate);
	}

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-anatop");
	base = of_iomap(np, 0);
	WARN_ON(!base);

	/*            type               name         parent_name  base     gate_mask div_mask */
	imx_clk_pllv3(IMX_PLLV3_SYS,	 "pll1_sys",	  "osc", base,	      0x2000,   0x7f);
	imx_clk_pllv3(IMX_PLLV3_GENERIC, "pll2_bus",	  "osc", base + 0x30, 0x2000,   0x1);
	imx_clk_pllv3(IMX_PLLV3_USB,	 "pll3_usb_otg",  "osc", base + 0x10, 0x2000,   0x3);
	imx_clk_pllv3(IMX_PLLV3_AV,	 "pll4_audio",	  "osc", base + 0x70, 0x2000,   0x7f);
	imx_clk_pllv3(IMX_PLLV3_AV,	 "pll5_video",	  "osc", base + 0xa0, 0x2000,   0x7f);
	imx_clk_pllv3(IMX_PLLV3_MLB,	 "pll6_mlb",	  "osc", base + 0xd0, 0x2000,   0x0);
	imx_clk_pllv3(IMX_PLLV3_USB,	 "pll7_usb_host", "osc", base + 0x20, 0x2000,   0x3);
	imx_clk_pllv3(IMX_PLLV3_ENET,	 "pll8_enet",	  "osc", base + 0xe0, 0x182000, 0x3);

	/*          name              parent_name        reg       idx */
	imx_clk_pfd("pll2_pfd0_352m", "pll2_bus",     base + 0x100, 0);
	imx_clk_pfd("pll2_pfd1_594m", "pll2_bus",     base + 0x100, 1);
	imx_clk_pfd("pll2_pfd2_396m", "pll2_bus",     base + 0x100, 2);
	imx_clk_pfd("pll3_pfd0_720m", "pll3_usb_otg", base + 0xf0,  0);
	imx_clk_pfd("pll3_pfd1_540m", "pll3_usb_otg", base + 0xf0,  1);
	imx_clk_pfd("pll3_pfd2_508m", "pll3_usb_otg", base + 0xf0,  2);
	imx_clk_pfd("pll3_pfd3_454m", "pll3_usb_otg", base + 0xf0,  3);

	/*                   name         parent_name     mult div */
	imx_clk_fixed_factor("pll2_198m", "pll2_pfd2_396m", 1, 2);
	imx_clk_fixed_factor("pll3_120m", "pll3_usb_otg",   1, 4);
	imx_clk_fixed_factor("pll3_80m",  "pll3_usb_otg",   1, 6);
	imx_clk_fixed_factor("pll3_60m",  "pll3_usb_otg",   1, 8);
	imx_clk_fixed_factor("twd",       "arm",            1, 2);

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ccm");
	base = of_iomap(np, 0);
	WARN_ON(!base);
	ccm_base = base;

	/*          name                reg       shift width parent_names     num_parents */
	imx_clk_mux("step",	        base + 0xc,  8,  1, step_sels,	       ARRAY_SIZE(step_sels));
	imx_clk_mux("pll1_sw",	        base + 0xc,  2,  1, pll1_sw_sels,      ARRAY_SIZE(pll1_sw_sels));
	imx_clk_mux("periph_pre",       base + 0x18, 18, 2, periph_pre_sels,   ARRAY_SIZE(periph_pre_sels));
	imx_clk_mux("periph2_pre",      base + 0x18, 21, 2, periph_pre_sels,   ARRAY_SIZE(periph_pre_sels));
	imx_clk_mux("periph_clk2_sel",  base + 0x18, 12, 1, periph_clk2_sels,  ARRAY_SIZE(periph_clk2_sels));
	imx_clk_mux("periph2_clk2_sel", base + 0x18, 20, 1, periph_clk2_sels,  ARRAY_SIZE(periph_clk2_sels));
	imx_clk_mux("axi_sel",          base + 0x14, 6,  2, axi_sels,          ARRAY_SIZE(axi_sels));
	imx_clk_mux("esai_sel",         base + 0x20, 19, 2, audio_sels,        ARRAY_SIZE(audio_sels));
	imx_clk_mux("asrc_sel",         base + 0x30, 7,  2, audio_sels,        ARRAY_SIZE(audio_sels));
	imx_clk_mux("spdif_sel",        base + 0x30, 20, 2, audio_sels,        ARRAY_SIZE(audio_sels));
	imx_clk_mux("gpu2d_axi",        base + 0x18, 0,  1, gpu_axi_sels,      ARRAY_SIZE(gpu_axi_sels));
	imx_clk_mux("gpu3d_axi",        base + 0x18, 1,  1, gpu_axi_sels,      ARRAY_SIZE(gpu_axi_sels));
	imx_clk_mux("gpu2d_core_sel",   base + 0x18, 16, 2, gpu2d_core_sels,   ARRAY_SIZE(gpu2d_core_sels));
	imx_clk_mux("gpu3d_core_sel",   base + 0x18, 4,  2, gpu3d_core_sels,   ARRAY_SIZE(gpu3d_core_sels));
	imx_clk_mux("gpu3d_shader_sel", base + 0x18, 8,  2, gpu3d_shader_sels, ARRAY_SIZE(gpu3d_shader_sels));
	imx_clk_mux("ipu1_sel",         base + 0x3c, 9,  2, ipu_sels,          ARRAY_SIZE(ipu_sels));
	imx_clk_mux("ipu2_sel",         base + 0x3c, 14, 2, ipu_sels,          ARRAY_SIZE(ipu_sels));
	imx_clk_mux("ldb_di0_sel",      base + 0x2c, 9,  3, ldb_di_sels,       ARRAY_SIZE(ldb_di_sels));
	imx_clk_mux("ldb_di1_sel",      base + 0x2c, 12, 3, ldb_di_sels,       ARRAY_SIZE(ldb_di_sels));
	imx_clk_mux("ipu1_di0_pre_sel", base + 0x34, 6,  3, ipu_di_pre_sels,   ARRAY_SIZE(ipu_di_pre_sels));
	imx_clk_mux("ipu1_di1_pre_sel", base + 0x34, 15, 3, ipu_di_pre_sels,   ARRAY_SIZE(ipu_di_pre_sels));
	imx_clk_mux("ipu2_di0_pre_sel", base + 0x38, 6,  3, ipu_di_pre_sels,   ARRAY_SIZE(ipu_di_pre_sels));
	imx_clk_mux("ipu2_di1_pre_sel", base + 0x38, 15, 3, ipu_di_pre_sels,   ARRAY_SIZE(ipu_di_pre_sels));
	imx_clk_mux("ipu1_di0_sel",     base + 0x34, 0,  3, ipu1_di0_sels,     ARRAY_SIZE(ipu1_di0_sels));
	imx_clk_mux("ipu1_di1_sel",     base + 0x34, 9,  3, ipu1_di1_sels,     ARRAY_SIZE(ipu1_di1_sels));
	imx_clk_mux("ipu2_di0_sel",     base + 0x38, 0,  3, ipu2_di0_sels,     ARRAY_SIZE(ipu2_di0_sels));
	imx_clk_mux("ipu2_di1_sel",     base + 0x38, 9,  3, ipu2_di1_sels,     ARRAY_SIZE(ipu2_di1_sels));
	imx_clk_mux("hsi_tx_sel",       base + 0x30, 28, 1, hsi_tx_sels,       ARRAY_SIZE(hsi_tx_sels));
	imx_clk_mux("pcie_axi_sel",     base + 0x18, 10, 1, pcie_axi_sels,     ARRAY_SIZE(pcie_axi_sels));
	imx_clk_mux("ssi1_sel",         base + 0x1c, 10, 2, ssi_sels,          ARRAY_SIZE(ssi_sels));
	imx_clk_mux("ssi2_sel",         base + 0x1c, 12, 2, ssi_sels,          ARRAY_SIZE(ssi_sels));
	imx_clk_mux("ssi3_sel",         base + 0x1c, 14, 2, ssi_sels,          ARRAY_SIZE(ssi_sels));
	imx_clk_mux("usdhc1_sel",       base + 0x1c, 16, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	imx_clk_mux("usdhc2_sel",       base + 0x1c, 17, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	imx_clk_mux("usdhc3_sel",       base + 0x1c, 18, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	imx_clk_mux("usdhc4_sel",       base + 0x1c, 19, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels));
	imx_clk_mux("enfc_sel",         base + 0x2c, 16, 2, enfc_sels,         ARRAY_SIZE(enfc_sels));
	imx_clk_mux("emi_sel",          base + 0x1c, 27, 2, emi_sels,          ARRAY_SIZE(emi_sels));
	imx_clk_mux("emi_slow_sel",     base + 0x1c, 29, 2, emi_sels,          ARRAY_SIZE(emi_sels));
	imx_clk_mux("vdo_axi_sel",      base + 0x18, 11, 1, vdo_axi_sels,      ARRAY_SIZE(vdo_axi_sels));
	imx_clk_mux("vpu_axi_sel",      base + 0x18, 14, 2, vpu_axi_sels,      ARRAY_SIZE(vpu_axi_sels));
	imx_clk_mux("cko1_sel",         base + 0x60, 0,  4, cko1_sels,         ARRAY_SIZE(cko1_sels));

	/*               name         reg      shift width busy: reg, shift parent_names  num_parents */
	imx_clk_busy_mux("periph",  base + 0x14, 25,  1,   base + 0x48, 5,  periph_sels,  ARRAY_SIZE(periph_sels));
	imx_clk_busy_mux("periph2", base + 0x14, 26,  1,   base + 0x48, 3,  periph2_sels, ARRAY_SIZE(periph2_sels));

	/*              name                parent_name          reg       shift width */
	imx_clk_divider("periph_clk2",      "periph_clk2_sel",   base + 0x14, 27, 3);
	imx_clk_divider("periph2_clk2",     "periph2_clk2_sel",  base + 0x14, 0,  3);
	imx_clk_divider("ipg",              "ahb",               base + 0x14, 8,  2);
	imx_clk_divider("ipg_per",          "ipg",               base + 0x1c, 0,  6);
	imx_clk_divider("esai_pred",        "esai_sel",          base + 0x28, 9,  3);
	imx_clk_divider("esai_podf",        "esai_pred",         base + 0x28, 25, 3);
	imx_clk_divider("asrc_pred",        "asrc_sel",          base + 0x30, 12, 3);
	imx_clk_divider("asrc_podf",        "asrc_pred",         base + 0x30, 9,  3);
	imx_clk_divider("spdif_pred",       "spdif_sel",         base + 0x30, 25, 3);
	imx_clk_divider("spdif_podf",       "spdif_pred",        base + 0x30, 22, 3);
	imx_clk_divider("can_root",         "pll3_usb_otg",      base + 0x20, 2,  6);
	imx_clk_divider("ecspi_root",       "pll3_60m",          base + 0x38, 19, 6);
	imx_clk_divider("gpu2d_core_podf",  "gpu2d_core_sel",    base + 0x18, 23, 3);
	imx_clk_divider("gpu3d_core_podf",  "gpu3d_core_sel",    base + 0x18, 26, 3);
	imx_clk_divider("gpu3d_shader",     "gpu3d_shader_sel",  base + 0x18, 29, 3);
	imx_clk_divider("ipu1_podf",        "ipu1_sel",          base + 0x3c, 11, 3);
	imx_clk_divider("ipu2_podf",        "ipu2_sel",          base + 0x3c, 16, 3);
	imx_clk_divider("ldb_di0_podf",     "ldb_di0_sel",       base + 0x20, 10, 1);
	imx_clk_divider("ldb_di1_podf",     "ldb_di1_sel",       base + 0x20, 11, 1);
	imx_clk_divider("ipu1_di0_pre",     "ipu1_di0_pre_sel",  base + 0x34, 3,  3);
	imx_clk_divider("ipu1_di1_pre",     "ipu1_di1_pre_sel",  base + 0x34, 12, 3);
	imx_clk_divider("ipu2_di0_pre",     "ipu2_di0_pre_sel",  base + 0x38, 3,  3);
	imx_clk_divider("ipu2_di1_pre",     "ipu2_di1_pre_sel",  base + 0x38, 12, 3);
	imx_clk_divider("hsi_tx_podf",      "hsi_tx_sel",        base + 0x30, 29, 3);
	imx_clk_divider("ssi1_pred",        "ssi1_sel",          base + 0x28, 6,  3);
	imx_clk_divider("ssi1_podf",        "ssi1_pred",         base + 0x28, 0,  6);
	imx_clk_divider("ssi2_pred",        "ssi2_sel",          base + 0x2c, 6,  3);
	imx_clk_divider("ssi2_podf",        "ssi2_pred",         base + 0x2c, 0,  6);
	imx_clk_divider("ssi3_pred",        "ssi3_sel",          base + 0x28, 22, 3);
	imx_clk_divider("ssi3_podf",        "ssi3_pred",         base + 0x28, 16, 6);
	imx_clk_divider("uart_serial_podf", "pll3_80m",          base + 0x24, 0,  6);
	imx_clk_divider("usdhc1_podf",      "usdhc1_sel",        base + 0x24, 11, 3);
	imx_clk_divider("usdhc2_podf",      "usdhc2_sel",        base + 0x24, 16, 3);
	imx_clk_divider("usdhc3_podf",      "usdhc3_sel",        base + 0x24, 19, 3);
	imx_clk_divider("usdhc4_podf",      "usdhc4_sel",        base + 0x24, 22, 3);
	imx_clk_divider("enfc_pred",        "enfc_sel",          base + 0x2c, 18, 3);
	imx_clk_divider("enfc_podf",        "enfc_pred",         base + 0x2c, 21, 6);
	imx_clk_divider("emi_podf",         "emi_sel",           base + 0x1c, 20, 3);
	imx_clk_divider("emi_slow_podf",    "emi_slow_sel",      base + 0x1c, 23, 3);
	imx_clk_divider("vpu_axi_podf",     "vpu_axi_sel",       base + 0x24, 25, 3);
	imx_clk_divider("cko1_podf",        "cko1_sel",          base + 0x60, 4,  3);

	/*                   name                 parent_name    reg        shift width busy: reg, shift */
	imx_clk_busy_divider("axi",               "axi_sel",     base + 0x14, 16,  3,   base + 0x48, 0);
	imx_clk_busy_divider("mmdc_ch0_axi_podf", "periph",      base + 0x14, 19,  3,   base + 0x48, 4);
	imx_clk_busy_divider("mmdc_ch1_axi_podf", "periph2",     base + 0x14, 3,   3,   base + 0x48, 2);
	imx_clk_busy_divider("arm",               "pll1_sw",     base + 0x10, 0,   3,   base + 0x48, 16);
	imx_clk_busy_divider("ahb",               "periph",      base + 0x14, 10,  3,   base + 0x48, 1);

	/*            name             parent_name          reg         shift */
	imx_clk_gate2("apbh_dma",      "ahb",               base + 0x68, 4);
	imx_clk_gate2("asrc",          "asrc_podf",         base + 0x68, 6);
	imx_clk_gate2("can1_ipg",      "ipg",               base + 0x68, 14);
	imx_clk_gate2("can1_serial",   "can_root",          base + 0x68, 16);
	imx_clk_gate2("can2_ipg",      "ipg",               base + 0x68, 18);
	imx_clk_gate2("can2_serial",   "can_root",          base + 0x68, 20);
	imx_clk_gate2("ecspi1",        "ecspi_root",        base + 0x6c, 0);
	imx_clk_gate2("ecspi2",        "ecspi_root",        base + 0x6c, 2);
	imx_clk_gate2("ecspi3",        "ecspi_root",        base + 0x6c, 4);
	imx_clk_gate2("ecspi4",        "ecspi_root",        base + 0x6c, 6);
	imx_clk_gate2("ecspi5",        "ecspi_root",        base + 0x6c, 8);
	imx_clk_gate2("enet",          "ipg",               base + 0x6c, 10);
	imx_clk_gate2("esai",          "esai_podf",         base + 0x6c, 16);
	imx_clk_gate2("gpt_ipg",       "ipg",               base + 0x6c, 20);
	imx_clk_gate2("gpt_ipg_per",   "ipg_per",           base + 0x6c, 22);
	imx_clk_gate2("gpu2d_core",    "gpu2d_core_podf",   base + 0x6c, 24);
	imx_clk_gate2("gpu3d_core",    "gpu3d_core_podf",   base + 0x6c, 26);
	imx_clk_gate2("hdmi_iahb",     "ahb",               base + 0x70, 0);
	imx_clk_gate2("hdmi_isfr",     "pll3_pfd1_540m",    base + 0x70, 4);
	imx_clk_gate2("i2c1",          "ipg_per",           base + 0x70, 6);
	imx_clk_gate2("i2c2",          "ipg_per",           base + 0x70, 8);
	imx_clk_gate2("i2c3",          "ipg_per",           base + 0x70, 10);
	imx_clk_gate2("iim",           "ipg",               base + 0x70, 12);
	imx_clk_gate2("enfc",          "enfc_podf",         base + 0x70, 14);
	imx_clk_gate2("ipu1",          "ipu1_podf",         base + 0x74, 0);
	imx_clk_gate2("ipu1_di0",      "ipu1_di0_sel",      base + 0x74, 2);
	imx_clk_gate2("ipu1_di1",      "ipu1_di1_sel",      base + 0x74, 4);
	imx_clk_gate2("ipu2",          "ipu2_podf",         base + 0x74, 6);
	imx_clk_gate2("ipu2_di0",      "ipu2_di0_sel",      base + 0x74, 8);
	imx_clk_gate2("ldb_di0",       "ldb_di0_podf",      base + 0x74, 12);
	imx_clk_gate2("ldb_di1",       "ldb_di1_podf",      base + 0x74, 14);
	imx_clk_gate2("ipu2_di1",      "ipu2_di1_sel",      base + 0x74, 10);
	imx_clk_gate2("hsi_tx",        "hsi_tx_podf",       base + 0x74, 16);
	imx_clk_gate2("mlb",           "pll6_mlb",          base + 0x74, 18);
	imx_clk_gate2("mmdc_ch0_axi",  "mmdc_ch0_axi_podf", base + 0x74, 20);
	imx_clk_gate2("mmdc_ch1_axi",  "mmdc_ch1_axi_podf", base + 0x74, 22);
	imx_clk_gate2("ocram",         "ahb",               base + 0x74, 28);
	imx_clk_gate2("openvg_axi",    "axi",               base + 0x74, 30);
	imx_clk_gate2("pcie_axi",      "pcie_axi_sel",      base + 0x78, 0);
	imx_clk_gate2("pwm1",          "ipg_per",           base + 0x78, 16);
	imx_clk_gate2("pwm2",          "ipg_per",           base + 0x78, 18);
	imx_clk_gate2("pwm3",          "ipg_per",           base + 0x78, 20);
	imx_clk_gate2("pwm4",          "ipg_per",           base + 0x78, 22);
	imx_clk_gate2("gpmi_bch_apb",  "usdhc3",            base + 0x78, 24);
	imx_clk_gate2("gpmi_bch",      "usdhc4",            base + 0x78, 26);
	imx_clk_gate2("gpmi_io",       "enfc",              base + 0x78, 28);
	imx_clk_gate2("gpmi_apb",      "usdhc3",            base + 0x78, 30);
	imx_clk_gate2("sata",          "ipg",               base + 0x7c, 4);
	imx_clk_gate2("sdma",          "ahb",               base + 0x7c, 6);
	imx_clk_gate2("spba",          "ipg",               base + 0x7c, 12);
	imx_clk_gate2("ssi1",          "ssi1_podf",         base + 0x7c, 18);
	imx_clk_gate2("ssi2",          "ssi2_podf",         base + 0x7c, 20);
	imx_clk_gate2("ssi3",          "ssi3_podf",         base + 0x7c, 22);
	imx_clk_gate2("uart_ipg",      "ipg",               base + 0x7c, 24);
	imx_clk_gate2("uart_serial",   "uart_serial_podf",  base + 0x7c, 26);
	imx_clk_gate2("usboh3",        "ipg",               base + 0x80, 0);
	imx_clk_gate2("usdhc1",        "usdhc1_podf",       base + 0x80, 2);
	imx_clk_gate2("usdhc2",        "usdhc2_podf",       base + 0x80, 4);
	imx_clk_gate2("usdhc3",        "usdhc3_podf",       base + 0x80, 6);
	imx_clk_gate2("usdhc4",        "usdhc4_podf",       base + 0x80, 8);
	imx_clk_gate2("vdo_axi",       "vdo_axi_sel",       base + 0x80, 12);
	imx_clk_gate2("vpu_axi",       "vpu_axi_podf",      base + 0x80, 14);
	imx_clk_gate("cko1",           "cko1_podf",         base + 0x60, 7);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	for (i = 0; i < ARRAY_SIZE(clks_init_on); i++) {
		clk = clk_get_sys(clks_init_on[i], NULL);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to get clk %s", __func__,
			       clks_init_on[i]);
			return PTR_ERR(clk);
		}
		clk_prepare_enable(clk);
	}

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-gpt");
	base = of_iomap(np, 0);
	WARN_ON(!base);
	irq = irq_of_parse_and_map(np, 0);
	mxc_timer_init(base, irq);

	return 0;
}
