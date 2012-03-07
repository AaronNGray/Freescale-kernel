/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Standard functionality for the common clock API.
 */
#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>

struct clk_fixed_factor {
	struct clk_hw	hw;
	unsigned int	mult;
	unsigned int	div;
	char		*parent[1];
};

#define to_clk_fixed_factor(_hw) container_of(_hw, struct clk_fixed_factor, hw)

static unsigned long clk_factor_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_fixed_factor *fix = to_clk_fixed_factor(hw);

	return (parent_rate / fix->div) * fix->mult;
}

static long clk_factor_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct clk_fixed_factor *fix = to_clk_fixed_factor(hw);

	if (prate) {
		unsigned long best_parent;
		best_parent = (rate / fix->mult) * fix->div;
		*prate = __clk_round_rate(__clk_get_parent(hw->clk),
				best_parent);
		return (*prate / fix->div) * fix->mult;
	} else {
		return (__clk_get_rate(__clk_get_parent(hw->clk)) / fix->div) * fix->mult;
	}
}

static int clk_factor_set_rate(struct clk_hw *hw, unsigned long rate)
{
	return 0;
}

struct clk_ops clk_fixed_factor_ops = {
	.round_rate = clk_factor_round_rate,
	.set_rate = clk_factor_set_rate,
	.recalc_rate = clk_factor_recalc_rate,
};
EXPORT_SYMBOL_GPL(clk_fixed_factor_ops);

struct clk *clk_register_fixed_factor(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		unsigned int mult, unsigned int div)
{
	struct clk_fixed_factor *fix;
	struct clk *clk;

	fix = kmalloc(sizeof(struct clk_gate), GFP_KERNEL);

	if (!fix) {
		pr_err("%s: could not allocate gated clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	/* struct clk_gate assignments */
	fix->mult = mult;
	fix->div = div;

	if (parent_name) {
		fix->parent[0] = kstrdup(parent_name, GFP_KERNEL);
		if (!fix->parent[0])
			goto out;
	}

	clk = clk_register(dev, name,
			&clk_fixed_factor_ops, &fix->hw,
			fix->parent,
			(parent_name ? 1 : 0),
			flags);
	if (clk)
		return clk;

out:
	kfree(fix->parent[0]);
	kfree(fix);

	return NULL;
}
