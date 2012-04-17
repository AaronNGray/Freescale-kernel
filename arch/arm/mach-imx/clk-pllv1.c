#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/clock.h>
#include "clk.h"

/**
 * pll v1
 *
 * @clk_hw	clock source
 * @parent	the parent clock name
 * @base	base address of pll registers
 *
 * PLL clock version 1, found on i.MX1/21/25/27/31/35
 */
struct clk_pllv1 {
	struct clk_hw	hw;
	void __iomem	*base;
	char		*parent;
};

#define to_clk_pllv1(clk) (container_of(clk, struct clk_pllv1, clk))

/*
 * Get the resulting clock rate from a PLL register value and the input
 * frequency. PLLs with this register layout can at least be found on
 * MX1, MX21, MX27 and MX31
 *
 *                  mfi + mfn / (mfd + 1)
 *  f = 2 * f_ref * --------------------
 *                        pd + 1
 */
static unsigned long clk_pllv1_decode(unsigned int reg_val, u32 freq)
{
	long long ll;
	int mfn_abs;
	unsigned int mfi, mfn, mfd, pd;

	mfi = (reg_val >> 10) & 0xf;
	mfn = reg_val & 0x3ff;
	mfd = (reg_val >> 16) & 0x3ff;
	pd =  (reg_val >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	mfn_abs = mfn;

	/* On all i.MXs except i.MX1 and i.MX21 mfn is a 10bit
	 * 2's complements number
	 */
	if (!cpu_is_mx1() && !cpu_is_mx21() && mfn >= 0x200)
		mfn_abs = 0x400 - mfn;

	freq *= 2;
	freq /= pd + 1;

	ll = (unsigned long long)freq * mfn_abs;

	do_div(ll, mfd + 1);

	if (!cpu_is_mx1() && !cpu_is_mx21() && mfn >= 0x200)
		ll = -ll;

	ll = (freq * mfi) + ll;

	return ll;
}

static unsigned long clk_pllv1_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_pllv1 *pll = to_clk_pllv1(hw);

	return clk_pllv1_decode(readl(pll->base), parent_rate);
}

struct clk_ops clk_pllv1_ops = {
	.recalc_rate = clk_pllv1_recalc_rate,
};

struct clk *imx_clk_pllv1(const char *name, char *parent,
		void __iomem *base)
{
	struct clk_pllv1 *pll;
	struct clk *clk;

	pll = kmalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return NULL;

	pll->base = base;
	pll->parent = parent;

	clk = clk_register(NULL, name, &clk_pllv1_ops, &pll->hw, &pll->parent,
			1, 0);
	if (!clk)
		kfree(pll);

	return clk;
}
