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

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/fsl_devices.h>
#include <linux/memblock.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/iram.h>
#include <mach/iomux-mx6q.h>
#include <mach/ipu-v3.h>
#include <mach/mxc_vpu.h>
#include <mach/viv_gpu.h>
#include "devices-imx6q.h"

static iomux_v3_cfg_t imx6q_sabrelite_pads[] = {
	/* AUDMUX */
	MX6Q_PAD_SD2_DAT0__AUDMUX_AUD4_RXD,
	MX6Q_PAD_SD2_DAT3__AUDMUX_AUD4_TXC,
	MX6Q_PAD_SD2_DAT2__AUDMUX_AUD4_TXD,
	MX6Q_PAD_SD2_DAT1__AUDMUX_AUD4_TXFS,

	/* DISPLAY */
	MX6Q_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6Q_PAD_DI0_PIN15__IPU1_DI0_PIN15,
	MX6Q_PAD_DI0_PIN2__IPU1_DI0_PIN2,
	MX6Q_PAD_DI0_PIN3__IPU1_DI0_PIN3,
	MX6Q_PAD_DISP0_DAT0__IPU1_DISP0_DAT_0,
	MX6Q_PAD_DISP0_DAT1__IPU1_DISP0_DAT_1,
	MX6Q_PAD_DISP0_DAT2__IPU1_DISP0_DAT_2,
	MX6Q_PAD_DISP0_DAT3__IPU1_DISP0_DAT_3,
	MX6Q_PAD_DISP0_DAT4__IPU1_DISP0_DAT_4,
	MX6Q_PAD_DISP0_DAT5__IPU1_DISP0_DAT_5,
	MX6Q_PAD_DISP0_DAT6__IPU1_DISP0_DAT_6,
	MX6Q_PAD_DISP0_DAT7__IPU1_DISP0_DAT_7,
	MX6Q_PAD_DISP0_DAT8__IPU1_DISP0_DAT_8,
	MX6Q_PAD_DISP0_DAT9__IPU1_DISP0_DAT_9,
	MX6Q_PAD_DISP0_DAT10__IPU1_DISP0_DAT_10,
	MX6Q_PAD_DISP0_DAT11__IPU1_DISP0_DAT_11,
	MX6Q_PAD_DISP0_DAT12__IPU1_DISP0_DAT_12,
	MX6Q_PAD_DISP0_DAT13__IPU1_DISP0_DAT_13,
	MX6Q_PAD_DISP0_DAT14__IPU1_DISP0_DAT_14,
	MX6Q_PAD_DISP0_DAT15__IPU1_DISP0_DAT_15,
	MX6Q_PAD_DISP0_DAT16__IPU1_DISP0_DAT_16,
	MX6Q_PAD_DISP0_DAT17__IPU1_DISP0_DAT_17,
	MX6Q_PAD_DISP0_DAT18__IPU1_DISP0_DAT_18,
	MX6Q_PAD_DISP0_DAT19__IPU1_DISP0_DAT_19,
	MX6Q_PAD_DISP0_DAT20__IPU1_DISP0_DAT_20,
	MX6Q_PAD_DISP0_DAT21__IPU1_DISP0_DAT_21,
	MX6Q_PAD_DISP0_DAT22__IPU1_DISP0_DAT_22,
	MX6Q_PAD_DISP0_DAT23__IPU1_DISP0_DAT_23,
	/* PWM1 */
	MX6Q_PAD_SD1_CMD__PWM4_PWMO,
	/* sgtl5000 clock */
	MX6Q_PAD_GPIO_0__CCM_CLKO,
       /* I2C1, SGTL5000 */
        MX6Q_PAD_EIM_D21__I2C1_SCL,     /* GPIO3[21] */
        MX6Q_PAD_EIM_D28__I2C1_SDA,     /* GPIO3[28] */
	/* I2C2 */
	MX6Q_PAD_KEY_COL3__I2C2_SCL,
	MX6Q_PAD_KEY_ROW3__I2C2_SDA,
	/* I2C3 */
	MX6Q_PAD_GPIO_5__I2C3_SCL,
	MX6Q_PAD_GPIO_16__I2C3_SDA,
	/* ECSPI1 */
	MX6Q_PAD_EIM_D17__ECSPI1_MISO,
	MX6Q_PAD_EIM_D18__ECSPI1_MOSI,
	MX6Q_PAD_EIM_D16__ECSPI1_SCLK,
	MX6Q_PAD_EIM_D19__GPIO_3_19,    /*SS1*/
	/* GPIO */
	MX6Q_PAD_NANDF_D0__GPIO_2_0,
	MX6Q_PAD_EIM_D23__GPIO_3_23,
	 /* reset pin for usb hub */
	MX6Q_PAD_GPIO_17__GPIO_7_12,
	/* USB OTG power pin */
	MX6Q_PAD_EIM_D22__GPIO_3_22
};

/* For imx6q sabrelite board: set KSZ9021RN RGMII pad skew */
static int ksz9021rn_phy_fixup(struct phy_device *phydev)
{
       /* prefer master mode, disable 1000 Base-T capable */
        phy_write(phydev, 0x9, 0x1c00);

	/* min rx data delay */
	phy_write(phydev, 0x0b, 0x8105);
	phy_write(phydev, 0x0c, 0x0000);

	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(phydev, 0x0b, 0x8104);
	phy_write(phydev, 0x0c, 0xf0f0);
	phy_write(phydev, 0x0b, 0x104);

	return 0;
}

void imx6q_restart(char mode, const char *cmd)
{
	struct device_node *np;
	void __iomem *wdog_base;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-wdt");
	wdog_base = of_iomap(np, 0);
	if (!wdog_base)
		goto soft;

	imx_src_prepare_restart();

	/* enable wdog */
	writew_relaxed(1 << 2, wdog_base);
	/* write twice to ensure the request will not get ignored */
	writew_relaxed(1 << 2, wdog_base);

	/* wait for reset to assert ... */
	mdelay(500);

	pr_err("Watchdog reset failed to assert reset\n");

	/* delay to allow the serial port to show the message */
	mdelay(50);

soft:
	/* we'll take a jump through zero as a poor second */
	soft_restart(0);
}

static int mx6q_ipuv3_init(int id)
{
	imx_reset_ipu(id);
	return 0;
}

static void mx6q_ipuv3_pg(int enable)
{
	/*TODO*/
}

static struct imx_ipuv3_platform_data ipuv3_pdata = {
	.rev = 4,
	.init = mx6q_ipuv3_init,
	.pg = mx6q_ipuv3_pg,
};

static void mx6q_vpu_reset(void)
{
	imx_reset_vpu();
}

static struct mxc_vpu_platform_data vpu_pdata = {
	.iram_enable = true,
	.iram_size = 0x21000,
	.reset = mx6q_vpu_reset,
};

static struct viv_gpu_platform_data gpu_pdata = {
	.reserved_mem_size = SZ_128M,
};

extern struct mxc_usbh_platform_data imx6q_usbh1_pdata;
extern struct mxc_usbh_platform_data imx6q_usbdr_pdata;
void imx6q_usbh1_pm_init(void);
void imx6q_usbdr_pm_init(void);

static const struct of_dev_auxdata imx6q_auxdata_lookup[] __initconst = {
	OF_DEV_AUXDATA("fsl,imx6q-ipu", MX6Q_IPU1_BASE_ADDR, "imx-ipuv3.0", &ipuv3_pdata),
	OF_DEV_AUXDATA("fsl,imx6q-ipu", MX6Q_IPU2_BASE_ADDR, "imx-ipuv3.1", &ipuv3_pdata),
	OF_DEV_AUXDATA("fsl,vpu", MX6Q_VPU_BASE_ADDR, "mxc_vpu.0", &vpu_pdata),
	OF_DEV_AUXDATA("viv,galcore", MX6Q_GPU_3D_BASE_ADDR, "galcore", &gpu_pdata),
	OF_DEV_AUXDATA("fsl,imx6q-ahci", MX6Q_SATA_BASE_ADDR, "imx6q-ahci", &imx_sata_pdata),
	OF_DEV_AUXDATA("fsl,ehci-mxc", MX6Q_USB_OTG_BASE_ADDR, "ehci-mxc.0", &imx6q_usbdr_pdata),
	OF_DEV_AUXDATA("fsl,ehci-mxc", MX6Q_USB_H1_BASE_ADDR, "ehci-mxc.1", &imx6q_usbh1_pdata),
};

static struct mxc_audio_platform_data mx6_sabrelite_audio_data;

static int mx6_sabrelite_sgtl5000_init(void)
{
	struct clk *clko;
	int rate;

	clko = clk_get(NULL, "clko_clk");
	if (IS_ERR(clko)) {
		pr_err("can't get CLKO clock.\n");
		return PTR_ERR(clko);
	}
	rate = clk_round_rate(clko, 16000000);
	if (rate < 8000000 || rate > 27000000) {
		pr_err("Error:SGTL5000 mclk freq %d out of range!\n", rate);
		clk_put(clko);
		return -1;
	}
	mx6_sabrelite_audio_data.sysclk = rate;
	clk_set_rate(clko, rate);
	clk_enable(clko);
	return 0;
}

static struct mxc_audio_platform_data mx6_sabrelite_audio_data = {
	.ssi_num = 1,
	.src_port = 2,
	.ext_port = 4,
	.init = mx6_sabrelite_sgtl5000_init,
	.hp_gpio = -1,
};

static struct platform_device mx6_sabrelite_audio_device = {
	.name = "imx-sgtl5000",
	.dev = {
		.platform_data = &mx6_sabrelite_audio_data,
	},
};

static void __init imx6q_init_machine(void)
{
	if (of_machine_is_compatible("fsl,imx6q-sabrelite")) {
		phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);
		mxc_iomux_v3_setup_multiple_pads(imx6q_sabrelite_pads,
					ARRAY_SIZE(imx6q_sabrelite_pads));
	}

	of_platform_populate(NULL, of_default_bus_match_table,
					imx6q_auxdata_lookup, NULL);

	iram_init(MX6Q_IRAM_BASE_ADDR, MX6Q_IRAM_SIZE);

	if (of_machine_is_compatible("fsl,imx6q-sabrelite"))
		platform_device_register(&mx6_sabrelite_audio_device);

	imx6q_pm_init();
	imx6q_usbh1_pm_init();
	imx6q_usbdr_pm_init();
}

static struct map_desc imx_mx6q_iomux_desc[] = {
	{
		.virtual	= MX6Q_IO_P2V(MX6Q_IOMUXC_BASE_ADDR),
		.pfn		= __phys_to_pfn(MX6Q_IOMUXC_BASE_ADDR),
		.length		= MX6Q_IOMUXC_SIZE,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= MX6Q_IO_P2V(MX6Q_USB_PHY1_BASE_ADDR),
		.pfn		= __phys_to_pfn(MX6Q_USB_PHY1_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= MX6Q_IO_P2V(MX6Q_USB_PHY2_BASE_ADDR),
		.pfn		= __phys_to_pfn(MX6Q_USB_PHY2_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= MX6Q_IO_P2V(MX6Q_USB_OTGCTRL_BASE_ADDR),
		.pfn		= __phys_to_pfn(MX6Q_USB_OTGCTRL_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void __init imx6q_iomux_map_io(void)
{
	iotable_init(imx_mx6q_iomux_desc, ARRAY_SIZE(imx_mx6q_iomux_desc));
	mxc_iomux_v3_init(MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR));
}

static void __init imx6q_map_io(void)
{
	imx_lluart_map_io();
	imx_scu_map_io();
	imx6q_clock_map_io();
	imx6q_iomux_map_io();

	init_consistent_dma_size(SZ_64M);

	mxc_set_cpu_type(MXC_CPU_MX6Q);

	if (!system_rev)
		system_rev = 0x63000;
}

static int __init imx6q_gpio_add_irq_domain(struct device_node *np,
				struct device_node *interrupt_parent)
{
	static int gpio_irq_base = MXC_GPIO_IRQ_START + ARCH_NR_GPIOS;

	gpio_irq_base -= 32;
	irq_domain_add_simple(np, gpio_irq_base);

	return 0;
}

static const struct of_device_id imx6q_irq_match[] __initconst = {
	{ .compatible = "arm,cortex-a9-gic", .data = gic_of_init, },
	{ .compatible = "fsl,imx6q-gpio", .data = imx6q_gpio_add_irq_domain, },
	{ /* sentinel */ }
};

static void __init imx6q_init_irq(void)
{
	l2x0_of_init(0, ~0UL);
	imx_src_init();
	imx_gpc_init();
	of_irq_init(imx6q_irq_match);
}

static void __init imx6q_timer_init(void)
{
	mx6q_clocks_init();
}

static struct sys_timer imx6q_timer = {
	.init = imx6q_timer_init,
};

static const char *imx6q_dt_compat[] __initdata = {
	"fsl,imx6q-arm2",
	"fsl,imx6q-sabrelite",
	NULL,
};

static void __init imx6q_sabrelite_reserve(void)
{
	phys_addr_t phys;

	if (gpu_pdata.reserved_mem_size) {
		phys = memblock_alloc_base(gpu_pdata.reserved_mem_size,
					   SZ_4K, SZ_1G);
		memblock_free(phys, gpu_pdata.reserved_mem_size);
		memblock_remove(phys, gpu_pdata.reserved_mem_size);
		gpu_pdata.reserved_mem_base = phys;
	}
}

DT_MACHINE_START(IMX6Q, "Freescale i.MX6 Quad (Device Tree)")
	.map_io		= imx6q_map_io,
	.init_irq	= imx6q_init_irq,
	.handle_irq	= imx6q_handle_irq,
	.timer		= &imx6q_timer,
	.reserve 	= imx6q_sabrelite_reserve,
	.init_machine	= imx6q_init_machine,
	.dt_compat	= imx6q_dt_compat,
	.restart	= imx6q_restart,
MACHINE_END
