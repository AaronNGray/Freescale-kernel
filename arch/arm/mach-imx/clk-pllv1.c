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

static unsigned long clk_pllv1_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_pllv1 *pll = to_clk_pllv1(hw);

	return mxc_decode_pll(readl(pll->base), parent_rate);
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
