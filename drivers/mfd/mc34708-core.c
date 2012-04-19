/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mc34708.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>

struct mc34708 {
	struct i2c_client *i2c_client;
	struct mutex lock;
	int irq;

	irq_handler_t irqhandler[MC34708_NUM_IRQ];
	void *irqdata[MC34708_NUM_IRQ];
};

/*!
 * This is the enumeration of versions of PMIC
 */
enum mc34708_id {
	MC_PMIC_ID_MC34708,
	MC_PMIC_ID_INVALID,
};

struct mc34708_version_t {
	/*!
	 * PMIC version identifier.
	 */
	enum mc34708_id id;
	/*!
	 * Revision of the PMIC.
	 */
	int revision;
};

#define PMIC_I2C_RETRY_TIMES		10

static const struct i2c_device_id mc34708_device_id[] = {
	{"mc34708", MC_PMIC_ID_MC34708},
	{},
};

static const char * const mc34708_chipname[] = {
	[MC_PMIC_ID_MC34708] = "mc34708",
};

void mc34708_lock(struct mc34708 *mc_pmic)
{
	if (!mutex_trylock(&mc_pmic->lock)) {
		dev_dbg(&mc_pmic->i2c_client->dev, "wait for %s from %pf\n",
			__func__, __builtin_return_address(0));

		mutex_lock(&mc_pmic->lock);
	}
	dev_dbg(&mc_pmic->i2c_client->dev, "%s from %pf\n",
		__func__, __builtin_return_address(0));
}
EXPORT_SYMBOL(mc34708_lock);

void mc34708_unlock(struct mc34708 *mc_pmic)
{
	dev_dbg(&mc_pmic->i2c_client->dev, "%s from %pf\n",
		__func__, __builtin_return_address(0));
	mutex_unlock(&mc_pmic->lock);
}
EXPORT_SYMBOL(mc34708_unlock);

static int mc34708_i2c_24bit_read(struct i2c_client *client,
				  unsigned int offset,
				  unsigned int *value)
{
	unsigned char buf[3];
	int ret;
	int i;

	memset(buf, 0, 3);
	for (i = 0; i < PMIC_I2C_RETRY_TIMES; i++) {
		ret = i2c_smbus_read_i2c_block_data(client, offset, 3, buf);
		if (ret == 3)
			break;
		usleep_range(1000, 1500);
	}

	if (ret == 3) {
		*value = buf[0] << 16 | buf[1] << 8 | buf[2];
		return ret;
	} else {
		dev_err(&client->dev, "24bit read error, ret = %d\n", ret);
		return -1;	/* return -1 on failure */
	}
}

int mc34708_reg_read(struct mc34708 *mc_pmic, unsigned int offset,
		     u32 *val)
{
	BUG_ON(!mutex_is_locked(&mc_pmic->lock));

	if (offset > MC34708_NUMREGS)
		return -EINVAL;

	if (mc34708_i2c_24bit_read(mc_pmic->i2c_client, offset, val) == -1)
		return -1;
	*val &= 0xffffff;

	dev_vdbg(&mc_pmic->i2c_client->dev, "mc_pmic read [%02d] -> 0x%06x\n",
		 offset, *val);

	return 0;
}
EXPORT_SYMBOL(mc34708_reg_read);

static int mc34708_i2c_24bit_write(struct i2c_client *client,
				   unsigned int offset, unsigned int reg_val)
{
	char buf[3];
	int ret;
	int i;

	buf[0] = (reg_val >> 16) & 0xff;
	buf[1] = (reg_val >> 8) & 0xff;
	buf[2] = (reg_val) & 0xff;

	for (i = 0; i < PMIC_I2C_RETRY_TIMES; i++) {
		ret = i2c_smbus_write_i2c_block_data(client, offset, 3, buf);
		if (ret == 0)
			break;
		usleep_range(1000, 1500);
	}
	if (i == PMIC_I2C_RETRY_TIMES)
		dev_err(&client->dev, "24bit write error, ret = %d\n", ret);

	return ret;
}

int mc34708_reg_write(struct mc34708 *mc_pmic, unsigned int offset, u32 val)
{
	BUG_ON(!mutex_is_locked(&mc_pmic->lock));

	if (offset > MC34708_NUMREGS)
		return -EINVAL;

	if (mc34708_i2c_24bit_write(mc_pmic->i2c_client, offset, val))
		return -1;
	val &= 0xffffff;

	dev_vdbg(&mc_pmic->i2c_client->dev, "mc_pmic write[%02d] -> 0x%06x\n",
		 offset, val);

	return 0;
}
EXPORT_SYMBOL(mc34708_reg_write);

int mc34708_reg_rmw(struct mc34708 *mc_pmic, unsigned int offset, u32 mask,
		    u32 val)
{
	int ret;
	u32 valread;

	BUG_ON(val & ~mask);

	ret = mc34708_reg_read(mc_pmic, offset, &valread);
	if (ret)
		return ret;

	valread = (valread & ~mask) | val;

	return mc34708_reg_write(mc_pmic, offset, valread);
}
EXPORT_SYMBOL(mc34708_reg_rmw);

int mc34708_irq_mask(struct mc34708 *mc_pmic, int irq)
{
	int ret;
	unsigned int offmask =
	    irq < 24 ? MC34708_REG_INT_MASK0 : MC34708_REG_INT_MASK1;
	u32 irqbit = 1 << (irq < 24 ? irq : irq - 24);
	u32 mask;

	if (irq < 0 || irq >= MC34708_NUM_IRQ)
		return -EINVAL;

	ret = mc34708_reg_read(mc_pmic, offmask, &mask);
	if (ret)
		return ret;

	if (mask & irqbit)
		/* already masked */
		return 0;

	return mc34708_reg_write(mc_pmic, offmask, mask | irqbit);
}
EXPORT_SYMBOL(mc34708_irq_mask);

int mc34708_irq_unmask(struct mc34708 *mc_pmic, int irq)
{
	int ret;
	unsigned int offmask =
	    irq < 24 ? MC34708_REG_INT_MASK0 : MC34708_REG_INT_MASK1;
	u32 irqbit = 1 << (irq < 24 ? irq : irq - 24);
	u32 mask;

	if (irq < 0 || irq >= MC34708_NUM_IRQ)
		return -EINVAL;

	ret = mc34708_reg_read(mc_pmic, offmask, &mask);
	if (ret)
		return ret;

	if (!(mask & irqbit))
		/* already unmasked */
		return 0;

	return mc34708_reg_write(mc_pmic, offmask, mask & ~irqbit);
}
EXPORT_SYMBOL(mc34708_irq_unmask);

int mc34708_irq_status(struct mc34708 *mc_pmic, int irq, int *enabled,
		       int *pending)
{
	int ret;
	unsigned int offmask =
	    irq < 24 ? MC34708_REG_INT_MASK0 : MC34708_REG_INT_MASK1;
	unsigned int offstat =
	    irq < 24 ? MC34708_REG_INT_STATUS0 : MC34708_REG_INT_STATUS1;
	u32 irqbit = 1 << (irq < 24 ? irq : irq - 24);

	if (irq < 0 || irq >= MC34708_NUM_IRQ)
		return -EINVAL;

	if (enabled) {
		u32 mask;

		ret = mc34708_reg_read(mc_pmic, offmask, &mask);
		if (ret)
			return ret;

		*enabled = mask & irqbit;
	}

	if (pending) {
		u32 stat;

		ret = mc34708_reg_read(mc_pmic, offstat, &stat);
		if (ret)
			return ret;

		*pending = stat & irqbit;
	}

	return 0;
}
EXPORT_SYMBOL(mc34708_irq_status);

int mc34708_irq_ack(struct mc34708 *mc_pmic, int irq)
{
	unsigned int offstat =
	    irq < 24 ? MC34708_REG_INT_STATUS0 : MC34708_REG_INT_STATUS1;
	unsigned int val = 1 << (irq < 24 ? irq : irq - 24);

	BUG_ON(irq < 0 || irq >= MC34708_NUM_IRQ);

	return mc34708_reg_write(mc_pmic, offstat, val);
}
EXPORT_SYMBOL(mc34708_irq_ack);

int mc34708_irq_request_nounmask(struct mc34708 *mc_pmic, int irq,
				 irq_handler_t handler, const char *name,
				 void *dev)
{
	BUG_ON(!mutex_is_locked(&mc_pmic->lock));
	BUG_ON(!handler);

	if (irq < 0 || irq >= MC34708_NUM_IRQ)
		return -EINVAL;

	if (mc_pmic->irqhandler[irq])
		return -EBUSY;

	mc_pmic->irqhandler[irq] = handler;
	mc_pmic->irqdata[irq] = dev;

	return 0;
}
EXPORT_SYMBOL(mc34708_irq_request_nounmask);

int mc34708_irq_request(struct mc34708 *mc_pmic, int irq,
			irq_handler_t handler, const char *name, void *dev)
{
	int ret;

	ret = mc34708_irq_request_nounmask(mc_pmic, irq, handler, name, dev);
	if (ret)
		return ret;

	ret = mc34708_irq_unmask(mc_pmic, irq);
	if (ret) {
		mc_pmic->irqhandler[irq] = NULL;
		mc_pmic->irqdata[irq] = NULL;
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mc34708_irq_request);

int mc34708_irq_free(struct mc34708 *mc_pmic, int irq, void *dev)
{
	int ret;
	BUG_ON(!mutex_is_locked(&mc_pmic->lock));

	if (irq < 0 || irq >= MC34708_NUM_IRQ || !mc_pmic->irqhandler[irq] ||
	    mc_pmic->irqdata[irq] != dev)
		return -EINVAL;

	ret = mc34708_irq_mask(mc_pmic, irq);
	if (ret)
		return ret;

	mc_pmic->irqhandler[irq] = NULL;
	mc_pmic->irqdata[irq] = NULL;

	return 0;
}
EXPORT_SYMBOL(mc34708_irq_free);

static inline irqreturn_t mc34708_irqhandler(struct mc34708 *mc_pmic, int irq)
{
	return mc_pmic->irqhandler[irq] (irq, mc_pmic->irqdata[irq]);
}

/*
 * returns: number of handled irqs or negative error
 * locking: holds mc_pmic->lock
 */
static int mc34708_irq_handle(struct mc34708 *mc_pmic,
			      unsigned int offstat, unsigned int offmask,
			      int baseirq)
{
	u32 stat, mask;
	int ret = mc34708_reg_read(mc_pmic, offstat, &stat);
	int num_handled = 0;

	if (ret)
		return ret;

	ret = mc34708_reg_read(mc_pmic, offmask, &mask);
	if (ret)
		return ret;

	while (stat & ~mask) {
		int irq = __ffs(stat & ~mask);

		stat &= ~(1 << irq);

		if (likely(mc_pmic->irqhandler[baseirq + irq])) {
			irqreturn_t handled;

			handled = mc34708_irqhandler(mc_pmic, baseirq + irq);
			if (handled == IRQ_HANDLED)
				num_handled++;
		} else {
			dev_err(&mc_pmic->i2c_client->dev,
				"BUG: irq %u but no handler\n", baseirq + irq);

			mask |= 1 << irq;

			ret = mc34708_reg_write(mc_pmic, offmask, mask);
		}
	}

	return num_handled;
}

static irqreturn_t mc34708_irq_thread(int irq, void *data)
{
	struct mc34708 *mc_pmic = data;
	irqreturn_t ret;
	int handled = 0;

	mc34708_lock(mc_pmic);

	ret = mc34708_irq_handle(mc_pmic, MC34708_REG_INT_STATUS0,
				 MC34708_REG_INT_MASK0, 0);
	if (ret > 0)
		handled = 1;

	ret = mc34708_irq_handle(mc_pmic, MC34708_REG_INT_STATUS1,
				 MC34708_REG_INT_MASK1, 24);
	if (ret > 0)
		handled = 1;

	mc34708_unlock(mc_pmic);

	return IRQ_RETVAL(handled);
}

#define maskval(reg, mask)	(((reg) & (mask)) >> __ffs(mask))
static int mc34708_identify(struct mc34708 *mc_pmic,
			    struct mc34708_version_t *ver)
{
	int rev_id = 0;
	int rev1 = 0;
	int rev2 = 0;
	int finid = 0;
	int icid = 0;
	int ret;
	ret = mc34708_reg_read(mc_pmic, MC34708_REG_IDENTIFICATION, &rev_id);
	if (ret) {
		dev_dbg(&mc_pmic->i2c_client->dev, "read ID error!%x\n", ret);
		return ret;
	}
	rev1 = (rev_id & 0x018) >> 3;
	rev2 = (rev_id & 0x007);
	icid = (rev_id & 0x01C0) >> 6;
	finid = (rev_id & 0x01E00) >> 9;
	ver->id = MC_PMIC_ID_MC34708;

	ver->revision = ((rev1 * 10) + rev2);
	dev_dbg(&mc_pmic->i2c_client->dev,
		"mc_pmic Rev %d.%d FinVer %x detected\n", rev1, rev2, finid);

	return 0;
}

static const struct i2c_device_id *i2c_match_id(const struct i2c_device_id *id,
						const struct i2c_client *client)
{
	while (id->name[0]) {
		if (strcmp(client->name, id->name) == 0)
			return id;
		id++;
	}

	return NULL;
}

static const struct i2c_device_id *i2c_get_device_id(const struct i2c_client
						     *idev)
{
	const struct i2c_driver *idrv = to_i2c_driver(idev->dev.driver);

	return i2c_match_id(idrv->id_table, idev);
}

static const char *mc34708_get_chipname(struct mc34708 *mc_pmic)
{
	const struct i2c_device_id *devid =
	    i2c_get_device_id(mc_pmic->i2c_client);

	if (!devid)
		return NULL;

	return mc34708_chipname[devid->driver_data];
}

int mc34708_get_flags(struct mc34708 *mc_pmic)
{
	struct mc34708_platform_data *pdata =
	    dev_get_platdata(&mc_pmic->i2c_client->dev);

	return pdata->flags;
}
EXPORT_SYMBOL(mc34708_get_flags);

static int mc34708_add_subdevice_pdata(struct mc34708 *mc_pmic,
				       const char *format, void *pdata,
				       size_t pdata_size)
{
	char buf[30];
	const char *name = mc34708_get_chipname(mc_pmic);

	struct mfd_cell cell = {
		.platform_data = pdata,
		.pdata_size = pdata_size,
	};

	/* there is no asnprintf in the kernel :-( */
	if (snprintf(buf, sizeof(buf), format, name) > sizeof(buf))
		return -E2BIG;

	cell.name = kmemdup(buf, strlen(buf) + 1, GFP_KERNEL);
	if (!cell.name)
		return -ENOMEM;

	return mfd_add_devices(&mc_pmic->i2c_client->dev, -1, &cell, 1, NULL,
			       0);
}

static int mc34708_add_subdevice(struct mc34708 *mc_pmic, const char *format)
{
	return mc34708_add_subdevice_pdata(mc_pmic, format, NULL, 0);
}

static int mc34708_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct mc34708 *mc_pmic;
	struct mc34708_version_t version;
	struct mc34708_platform_data *pdata = client->dev.platform_data;
	struct device_node *np = client->dev.of_node;
	int ret;

	mc_pmic = kzalloc(sizeof(*mc_pmic), GFP_KERNEL);
	if (!mc_pmic)
		return -ENOMEM;
	i2c_set_clientdata(client, mc_pmic);
	mc_pmic->i2c_client = client;

	mutex_init(&mc_pmic->lock);
	mc34708_lock(mc_pmic);
	mc34708_identify(mc_pmic, &version);
	/* mask all irqs */
	ret = mc34708_reg_write(mc_pmic, MC34708_REG_INT_MASK0, 0x00ffffff);
	if (ret)
		goto err_mask;

	ret = mc34708_reg_write(mc_pmic, MC34708_REG_INT_MASK1, 0x00ffffff);
	if (ret)
		goto err_mask;

	ret = request_threaded_irq(client->irq, NULL, mc34708_irq_thread,
				   IRQF_ONESHOT | IRQF_TRIGGER_HIGH, "mc_pmic",
				   mc_pmic);

	if (ret) {
 err_mask:
		mc34708_unlock(mc_pmic);
		dev_set_drvdata(&client->dev, NULL);
		kfree(mc_pmic);
		return ret;
	}

	mc34708_unlock(mc_pmic);

	if (pdata && pdata->flags & MC34708_USE_ADC)
		mc34708_add_subdevice(mc_pmic, "%s-adc");
	else if (of_get_property(np, "fsl,mc34708-uses-adc", NULL))
		mc34708_add_subdevice(mc_pmic, "%s-adc");


	if (pdata && pdata->flags & MC34708_USE_REGULATOR) {
		struct mc34708_regulator_platform_data regulator_pdata = {
			.num_regulators = pdata->num_regulators,
			.regulators = pdata->regulators,
		};

		mc34708_add_subdevice_pdata(mc_pmic, "%s-regulator",
					    &regulator_pdata,
					    sizeof(regulator_pdata));
	} else if (of_find_node_by_name(np, "regulators")) {
		mc34708_add_subdevice(mc_pmic, "%s-regulator");
	}

	if (pdata && pdata->flags & MC34708_USE_RTC)
		mc34708_add_subdevice(mc_pmic, "%s-rtc");
	else if (of_get_property(np, "fsl,mc34708-uses-rtc", NULL))
		mc34708_add_subdevice(mc_pmic, "%s-rtc");

	if (pdata && pdata->flags & MC34708_USE_TOUCHSCREEN)
		mc34708_add_subdevice(mc_pmic, "%s-ts");
	else if (of_get_property(np, "fsl,mc34708-uses-ts", NULL))
		mc34708_add_subdevice(mc_pmic, "%s-ts");

	return 0;
}

static int __devexit mc34708_remove(struct i2c_client *client)
{
	struct mc34708 *mc_pmic = dev_get_drvdata(&client->dev);

	free_irq(mc_pmic->i2c_client->irq, mc_pmic);

	mfd_remove_devices(&client->dev);

	kfree(mc_pmic);

	return 0;
}

static const struct of_device_id mc34708_dt_ids[] = {
	{ .compatible = "fsl,mc34708" },
	{ /* sentinel */ }
};

static struct i2c_driver mc34708_driver = {
	.id_table = mc34708_device_id,
	.driver = {
		   .name = "mc34708",
		   .owner = THIS_MODULE,
		   .of_match_table = mc34708_dt_ids,
		   },
	.probe = mc34708_probe,
	.remove = __devexit_p(mc34708_remove),
};

static int __init mc34708_init(void)
{
	return i2c_add_driver(&mc34708_driver);
}
subsys_initcall(mc34708_init);

static void __exit mc34708_exit(void)
{
	i2c_del_driver(&mc34708_driver);
}
module_exit(mc34708_exit);

MODULE_DESCRIPTION("Core driver for Freescale MC34708 PMIC");
MODULE_AUTHOR("Robin Gong <B38343@freescale.com>, "
	      "Ying-Chun Liu (PaulLiu) <paul.liu@linaro.org>");
MODULE_LICENSE("GPL v2");
