/* drivers/staging/taos/tsl277x.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/pm.h>
//#include "tsl277x.h"

#include <linux/i2c/tsl2772.h>
#include <linux/earlysuspend.h>
//#include <mach/regs-gpio.h>
//#include <mach/gpio.h>
//#include <mach/irqs.h>
//#define YANG_DEBUG

enum tsl277x_regs {
	TSL277X_ENABLE,
	TSL277X_ALS_TIME,
	TSL277X_PRX_TIME,
	TSL277X_WAIT_TIME,
	TSL277X_ALS_MINTHRESHLO,
	TSL277X_ALS_MINTHRESHHI,
	TSL277X_ALS_MAXTHRESHLO,
	TSL277X_ALS_MAXTHRESHHI,
	TSL277X_PRX_MINTHRESHLO,
	TSL277X_PRX_MINTHRESHHI,
	TSL277X_PRX_MAXTHRESHLO,
	TSL277X_PRX_MAXTHRESHHI,
	TSL277X_PERSISTENCE,
	TSL277X_CONFIG,
	TSL277X_PRX_PULSE_COUNT,
	TSL277X_CONTROL,

	TSL277X_REVID = 0x11,
	TSL277X_CHIPID,
	TSL277X_STATUS,
	TSL277X_ALS_CHAN0LO,
	TSL277X_ALS_CHAN0HI,
	TSL277X_ALS_CHAN1LO,
	TSL277X_ALS_CHAN1HI,
	TSL277X_PRX_LO,
	TSL277X_PRX_HI,

	TSL277X_REG_PRX_OFFS = 0x1e,
	TSL277X_REG_MAX,
};

enum tsl277x_cmd_reg {
	TSL277X_CMD_REG           = (1 << 7),
	TSL277X_CMD_INCR          = (0x1 << 5),
	TSL277X_CMD_SPL_FN        = (0x3 << 5),
	TSL277X_CMD_PROX_INT_CLR  = (0x5 << 0),
	TSL277X_CMD_ALS_INT_CLR   = (0x6 << 0),
};

enum tsl277x_en_reg {
	TSL277X_EN_PWR_ON   = (1 << 0),
	TSL277X_EN_ALS      = (1 << 1),
	TSL277X_EN_PRX      = (1 << 2),
	TSL277X_EN_WAIT     = (1 << 3),
	TSL277X_EN_ALS_IRQ  = (1 << 4),
	TSL277X_EN_PRX_IRQ  = (1 << 5),
	TSL277X_EN_SAI      = (1 << 6),
};

enum tsl277x_status {
	TSL277X_ST_ALS_VALID  = (1 << 0),
	TSL277X_ST_PRX_VALID  = (1 << 1),
	TSL277X_ST_ALS_IRQ    = (1 << 4),
	TSL277X_ST_PRX_IRQ    = (1 << 5),
	TSL277X_ST_PRX_SAT    = (1 << 6),
};

enum {
	TSL277X_ALS_GAIN_MASK = (3 << 0),
	TSL277X_ALS_AGL_MASK  = (1 << 2),
	TSL277X_ALS_AGL_SHIFT = 2,
	TSL277X_ATIME_PER_100 = 273,
	TSL277X_ATIME_DEFAULT_MS = 50,
	SCALE_SHIFT = 11,
	RATIO_SHIFT = 10,
	MAX_ALS_VALUE = 0xffff,
	MIN_ALS_VALUE = 10,
	GAIN_SWITCH_LEVEL = 100,
	GAIN_AUTO_INIT_VALUE = 16,
};

static u8 const tsl277x_ids[] = {
	0x39,
	0x30,
};

static char const *tsl277x_names[] = {
	"tsl27721 / tsl27725",
	"tsl27723 / tsl2777",
};

static u8 const restorable_regs[] = {
	TSL277X_ALS_TIME,
	TSL277X_PRX_TIME,
	TSL277X_WAIT_TIME,
	TSL277X_PERSISTENCE,
	TSL277X_CONFIG,
	TSL277X_PRX_PULSE_COUNT,
	TSL277X_CONTROL,
	TSL277X_REG_PRX_OFFS,
};

static u8 const als_gains[] = {
	1,
	8,
	16,
	120
};

struct taos_als_info {
	int ch0;
	int ch1;
	u32 cpl;
	u32 saturation;
	int lux;
	int ga;
};

struct taos_prox_info {
	int raw;
	int detected;
};

static struct lux_segment segment_default[] = {
	{
		.ratio = (435 << RATIO_SHIFT) / 1000,
		.k0 = (46516 << SCALE_SHIFT) / 1000,
		.k1 = (95381 << SCALE_SHIFT) / 1000,
	},
	{
		.ratio = (551 << RATIO_SHIFT) / 1000,
		.k0 = (23740 << SCALE_SHIFT) / 1000,
		.k1 = (43044 << SCALE_SHIFT) / 1000,
	},
};

struct tsl2772_chip {
	struct mutex lock;
	struct i2c_client *client;
	struct taos_prox_info prx_inf;
	struct taos_als_info als_inf;
	struct taos_parameters params;
	struct tsl2772_i2c_platform_data *pdata;
	u8 shadow[TSL277X_REG_MAX];
	struct input_dev *p_idev;
	struct input_dev *a_idev;
	int in_early_suspend;
	int in_suspend;
	int wake_irq;
	int irq_pending;
	bool unpowered;
	bool als_enabled;
	bool prx_enabled;
	struct lux_segment *segment;
	int segment_num;
	int seg_num_max;
	bool als_gain_auto;
	struct early_suspend suspend_desc;
};
static bool prox_is_calibrated = false;
static void taos_early_suspend(struct early_suspend *desc);
static void taos_late_resume(struct early_suspend *desc);
static int  taos_suspend(struct i2c_client *client,pm_message_t state);
static int  taos_resume(struct i2c_client *client);
static int taos_als_enable(struct tsl2772_chip *chip, int on);
static int taos_i2c_read(struct tsl2772_chip *chip, u8 reg, u8 *val)
{
	int ret;
	s32 read;
	struct i2c_client *client = chip->client;

	ret = i2c_smbus_write_byte(client, (TSL277X_CMD_REG | reg));
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to write register %x\n",
				__func__, reg);
		return ret;
	}
	read = i2c_smbus_read_byte(client);
	if (read < 0) {
		dev_err(&client->dev, "%s: failed to read from register %x\n",
				__func__, reg);
		return ret;
	}
	*val = read;
	return 0;
}

static int taos_i2c_blk_read(struct tsl2772_chip *chip,
		u8 reg, u8 *val, int size)
{
	s32 ret;
	struct i2c_client *client = chip->client;

	ret =  i2c_smbus_read_i2c_block_data(client,
			TSL277X_CMD_REG | TSL277X_CMD_INCR | reg, size, val);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed at address %x (%d bytes)\n",
				__func__, reg, size);
	return ret;
}

static int taos_i2c_write(struct tsl2772_chip *chip, u8 reg, u8 val)
{
	int ret;
	struct i2c_client *client = chip->client;

	ret = i2c_smbus_write_byte_data(client, TSL277X_CMD_REG | reg, val);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed to write register %x\n",
				__func__, reg);
	return ret;
}

static int taos_i2c_blk_write(struct tsl2772_chip *chip,
		u8 reg, u8 *val, int size)
{
	s32 ret;
	struct i2c_client *client = chip->client;

	ret =  i2c_smbus_write_i2c_block_data(client,
			TSL277X_CMD_REG | TSL277X_CMD_INCR | reg, size, val);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed at address %x (%d bytes)\n",
				__func__, reg, size);
	return ret;
}

static int set_segment_table(struct tsl2772_chip *chip,
		struct lux_segment *segment, int seg_num)
{
	int i;
	struct device *dev = &chip->client->dev;

	chip->seg_num_max = chip->pdata->segment_num ?
			chip->pdata->segment_num : ARRAY_SIZE(segment_default);

	if (!chip->segment) {
		dev_dbg(dev, "%s: allocating segment table\n", __func__);
		chip->segment = kzalloc(sizeof(*chip->segment) *
				chip->seg_num_max, GFP_KERNEL);
		if (!chip->segment) {
			dev_err(dev, "%s: no memory!\n", __func__);
			return -ENOMEM;
		}
	}
	if (seg_num > chip->seg_num_max) {
		dev_warn(dev, "%s: %d segment requested, %d applied\n",
				__func__, seg_num, chip->seg_num_max);
		chip->segment_num = chip->seg_num_max;
	} else {
		chip->segment_num = seg_num;
	}
	memcpy(chip->segment, segment,
			chip->segment_num * sizeof(*chip->segment));
	dev_dbg(dev, "%s: %d segment requested, %d applied\n", __func__,
			seg_num, chip->seg_num_max);
	for (i = 0; i < chip->segment_num; i++)
		dev_dbg(dev, "segment %d: ratio %6u, k0 %6u, k1 %6u\n",
				i, chip->segment[i].ratio,
				chip->segment[i].k0, chip->segment[i].k1);
	return 0;
}

static void taos_calc_cpl(struct tsl2772_chip *chip)
{
	u32 cpl;
	u32 sat;
	u8 atime = chip->shadow[TSL277X_ALS_TIME];
	u8 agl = (chip->shadow[TSL277X_CONFIG] & TSL277X_ALS_AGL_MASK)
			>> TSL277X_ALS_AGL_SHIFT;
	
//	u32 time_scale = ((256 - atime) << SCALE_SHIFT) *
//		TSL277X_ATIME_PER_100 / (TSL277X_ATIME_DEFAULT_MS * 100);

	u32 time_scale = (256 - atime ) * 2730 / 600; 

	cpl = time_scale * chip->params.als_gain;
	if (agl)
		cpl = cpl * 16 / 1000;
	sat = min_t(u32, MAX_ALS_VALUE, (u32)(256 - atime) << 10);
	sat = sat * 8 / 10;
	dev_dbg(&chip->client->dev,
			"%s: cpl = %u [time_scale %u, gain %u, agl %u], "
			"saturation %u\n", __func__, cpl, time_scale,
			chip->params.als_gain, agl, sat);
	chip->als_inf.cpl = cpl;
	chip->als_inf.saturation = sat;
}

static int set_als_gain(struct tsl2772_chip *chip, int gain)
{
	int rc;
	u8 ctrl_reg  = chip->shadow[TSL277X_CONTROL] & ~TSL277X_ALS_GAIN_MASK;

	switch (gain) {
	case 1:
		ctrl_reg |= AGAIN_1;
		break;
	case 8:
		ctrl_reg |= AGAIN_8;
		break;
	case 16:
		ctrl_reg |= AGAIN_16;
		break;
	case 120:
		ctrl_reg |= AGAIN_120;
		break;
	default:
		dev_err(&chip->client->dev, "%s: wrong als gain %d\n",
				__func__, gain);
		return -EINVAL;
	}
	rc = taos_i2c_write(chip, TSL277X_CONTROL, ctrl_reg);
	if (!rc) {
		chip->shadow[TSL277X_CONTROL] = ctrl_reg;
		chip->params.als_gain = gain;
		dev_dbg(&chip->client->dev, "%s: new gain %d\n",
				__func__, gain);
	}
	return rc;
}

static int taos_get_lux(struct tsl2772_chip *chip)
{
	unsigned i;
	int ret = 0;
	struct device *dev = &chip->client->dev;
	struct lux_segment *s = chip->segment;
	u32 c0 = chip->als_inf.ch0;
	u32 c1 = chip->als_inf.ch1;
	u32 sat = chip->als_inf.saturation;
	u32 ratio;
	u64 lux_0, lux_1;
	u32 cpl = chip->als_inf.cpl;
	u32 lux, k0 = 0, k1 = 0;

	if (!chip->als_gain_auto) {
		if (c0 <= MIN_ALS_VALUE) {
			dev_dbg(dev, "%s: darkness\n", __func__);
			lux = 0;
			goto exit;
		} else if (c0 >= sat) {
			dev_dbg(dev, "%s: saturation, keep lux\n", __func__);
			lux = chip->als_inf.lux;
			goto exit;
		}
	} else {
		u8 gain = chip->params.als_gain;
		int rc = -EIO;

		if (gain == 16 && c0 >= sat) {
			rc = set_als_gain(chip, 1);
		} else if (gain == 16 && c0 < GAIN_SWITCH_LEVEL) {
			rc = set_als_gain(chip, 120);
		} else if ((gain == 120 && c0 >= sat) ||
				(gain == 1 && c0 < GAIN_SWITCH_LEVEL)) {
			rc = set_als_gain(chip, 16);
		}
		if (!rc) {
			dev_dbg(dev, "%s: gain adjusted, skip\n", __func__);
			taos_calc_cpl(chip);
			ret = -EAGAIN;
			lux = chip->als_inf.lux;
			goto exit;
		}

		if (c0 <= MIN_ALS_VALUE) {
			dev_dbg(dev, "%s: darkness\n", __func__);
			lux = 0;
			goto exit;
		} else if (c0 >= sat) {
			dev_dbg(dev, "%s: saturation, keep lux\n", __func__);
			lux = chip->als_inf.lux;
			goto exit;
		}
	}

	ratio = (c1 << RATIO_SHIFT) / c0;
	for (i = 0; i < chip->segment_num; i++, s++) {
		if (ratio <= s->ratio) {
			dev_dbg(&chip->client->dev, "%s: ratio %u segment %u "
					"[r %u, k0 %u, k1 %u]\n", __func__,
					ratio, i, s->ratio, s->k0, s->k1);
			k0 = s->k0;
			k1 = s->k1;
			break;
		}
	}
	if (i >= chip->segment_num) {
		dev_dbg(&chip->client->dev, "%s: ratio %u - darkness\n",
				__func__, ratio);
		lux = 0;
		goto exit;
	}

/*	
	lux_0 = k0 * (s64)c0;
	lux_1 = k1 * (s64)c1;
	if (lux_1 >= lux_0) {
		dev_dbg(&chip->client->dev, "%s: negative result - darkness\n",
				__func__);
		lux = 0;
		goto exit;
	}
	lux_0 -= lux_1;
	while (lux_0 & ((u64)0xffffffff << 32)) {
		dev_dbg(&chip->client->dev, "%s: overwlow lux64 = 0x%16llx",
				__func__, lux_0);
		lux_0 >>= 1;
		cpl >>= 1;
	}
	if (!cpl) {
		dev_dbg(&chip->client->dev, "%s: zero cpl - darkness\n",
				__func__);
		lux = 0;
		goto exit;
	}
	lux = lux_0;
	lux = lux / cpl;
*/
	lux_0 = ( ( ( c0 * 100 ) - ( c1 * 187 ) ) * 10 ) / cpl; 
	lux_1 = ( ( ( c0 *  63 ) - ( c1 * 100 ) ) * 10 ) / cpl; 

	lux = max(lux_0, lux_1);							//20120830	lux
	lux = max(lux , (u32)0);							//20120830	lux	
 
	if( chip->als_inf.ga <= 0 ) 						//20120829	cal
		chip->als_inf.ga = 1 ;

	lux *= chip->als_inf.ga;						//20120829	cal

	
exit:
	dev_dbg(&chip->client->dev, "%s: lux %u (%u x %u - %u x %u) / %u\n",
		__func__, lux, k0, c0, k1, c1, cpl);
	chip->als_inf.lux = lux;
	return ret;
}


static int pltf_power_on(struct tsl2772_chip *chip)
{
	int rc = 0;
	if (chip->pdata->platform_power) {
		rc = chip->pdata->platform_power(&chip->client->dev,
			POWER_ON);
		msleep(10);
	}
	chip->unpowered = rc != 0;
	return rc;
}

static int pltf_power_off(struct tsl2772_chip *chip)
{
	int rc = 0;
	if (chip->pdata->platform_power) {
		rc = chip->pdata->platform_power(&chip->client->dev,
			POWER_OFF);
		chip->unpowered = rc == 0;
	} else {
		chip->unpowered = false;
	}
	return rc;
}

static int taos_irq_clr(struct tsl2772_chip *chip, u8 bits)
{
	int ret = i2c_smbus_write_byte(chip->client, TSL277X_CMD_REG |
			TSL277X_CMD_SPL_FN | bits);
	if (ret < 0)
		dev_err(&chip->client->dev, "%s: failed, bits %x\n",
				__func__, bits);
	return ret;
}

static void taos_get_als(struct tsl2772_chip *chip)
{
	u32 ch0, ch1;
	u8 *buf = &chip->shadow[TSL277X_ALS_CHAN0LO];

	ch0 = le16_to_cpup((const __le16 *)&buf[0]);
	ch1 = le16_to_cpup((const __le16 *)&buf[2]);
	chip->als_inf.ch0 = ch0;
	chip->als_inf.ch1 = ch1;
	dev_dbg(&chip->client->dev, "%s: ch0 %u, ch1 %u\n", __func__, ch0, ch1);
}

static void taos_get_prox(struct tsl2772_chip *chip)
{
	u8 *buf = &chip->shadow[TSL277X_PRX_LO];
	bool d = chip->prx_inf.detected;

	chip->prx_inf.raw = (buf[1] << 8) | buf[0];
        //printk(KERN_INFO "before:%d\n",d);
	chip->prx_inf.detected =
			(d && (chip->prx_inf.raw > chip->params.prox_th_min)) ||
			(!d && (chip->prx_inf.raw > chip->params.prox_th_max));
       // printk(KERN_INFO "end:%d\n",chip->prx_inf.detected );
	dev_dbg(&chip->client->dev, "%s: raw %d, detected %d\n", __func__,
			chip->prx_inf.raw, chip->prx_inf.detected);
}

static int taos_read_all(struct tsl2772_chip *chip)
{
	struct i2c_client *client = chip->client;
	s32 ret;

	dev_dbg(&client->dev, "%s\n", __func__);
	ret = taos_i2c_blk_read(chip, TSL277X_STATUS,
			&chip->shadow[TSL277X_STATUS],
			TSL277X_PRX_HI - TSL277X_STATUS + 1);
	return (ret < 0) ? ret : 0;
}


static int update_prox_thresh(struct tsl2772_chip *chip, bool on_enable)
{
	s32 ret;
	u8 *buf = &chip->shadow[TSL277X_PRX_MINTHRESHLO];
	u16 from, to;

	if (on_enable) {
		/* zero gate to force irq */
		from = to = 0;
	} else {
		if (chip->prx_inf.detected) {
			from = chip->params.prox_th_min;
			to = 0xffff;
		} else {
			from = 0;
			to = chip->params.prox_th_max;
		}
	}
	dev_dbg(&chip->client->dev, "%s: %u - %u\n", __func__, from, to);
	*buf++ = from & 0xff;
	*buf++ = from >> 8;
	*buf++ = to & 0xff;
	*buf++ = to >> 8;
	ret = taos_i2c_blk_write(chip, TSL277X_PRX_MINTHRESHLO,
			&chip->shadow[TSL277X_PRX_MINTHRESHLO],
			TSL277X_PRX_MAXTHRESHHI - TSL277X_PRX_MINTHRESHLO + 1);
#ifdef YANG_DEBUG
	printk(KERN_ALERT "the detected is %d\n",chip->prx_inf.detected);
	printk(KERN_ALERT "the from is %d,the to is %d!\n",from,to);
#endif
	return (ret < 0) ? ret : 0;
}

static int update_als_thres(struct tsl2772_chip *chip, bool on_enable)
{
	s32 ret;
	u8 *buf = &chip->shadow[TSL277X_ALS_MINTHRESHLO];
	u16 gate = chip->params.als_gate;
	u16 from, to, cur;

	cur = chip->als_inf.ch0;
	if (on_enable) {
		/* zero gate far away form current position to force an irq */
		from = to = cur > 0xffff / 2 ? 0 : 0xffff;
	} else {
		gate = cur * gate / 100;
		if (!gate)
			gate = 1;
		if (cur > gate)
			from = cur - gate;
		else
			from = 0;
		if (cur < (0xffff - gate))
			to = cur + gate;
		else
			to = 0xffff;
	}
	dev_dbg(&chip->client->dev, "%s: [%u - %u]\n", __func__, from, to);
	*buf++ = from & 0xff;
	*buf++ = from >> 8;
	*buf++ = to & 0xff;
	*buf++ = to >> 8;
	ret = taos_i2c_blk_write(chip, TSL277X_ALS_MINTHRESHLO,
			&chip->shadow[TSL277X_ALS_MINTHRESHLO],
			TSL277X_ALS_MAXTHRESHHI - TSL277X_ALS_MINTHRESHLO + 1);
	return (ret < 0) ? ret : 0;
}

static void report_prox(struct tsl2772_chip *chip)
{
	if (chip->p_idev) {
		input_report_abs(chip->p_idev, ABS_DISTANCE,
				chip->prx_inf.detected ? 0 : 1);
		input_sync(chip->p_idev);
	}
}

static void report_als(struct tsl2772_chip *chip)
{
	if (chip->a_idev) {
		int rc = taos_get_lux(chip);
		if (!rc) {
			int lux = chip->als_inf.lux;
			input_report_abs(chip->a_idev, ABS_MISC, lux);
			input_sync(chip->a_idev);
			update_als_thres(chip, 0);
		} else {
			update_als_thres(chip, 1);
		}
	}
}
static int taos_check_and_report(struct tsl2772_chip *chip)
{
	u8 status;
	
	int ret = taos_read_all(chip);
	if (ret)
		goto exit_clr;


	status = chip->shadow[TSL277X_STATUS];
	dev_dbg(&chip->client->dev, "%s: status 0x%02x\n", __func__, status);
#ifdef YANG_DEBUG
	printk(KERN_INFO "prox before %s: status 0x%02x\n", __func__, status);
#endif
	if ((status & (TSL277X_ST_PRX_VALID | TSL277X_ST_PRX_IRQ)) ==
			(TSL277X_ST_PRX_VALID | TSL277X_ST_PRX_IRQ)) {
		taos_get_prox(chip);
		report_prox(chip);
		update_prox_thresh(chip, 0);
#ifdef YANG_DEBUG
    printk(KERN_INFO "prox %s: status 0x%02x\n", __func__, status);
#endif
	}

	if ((status & (TSL277X_ST_ALS_VALID | TSL277X_ST_ALS_IRQ)) ==
			(TSL277X_ST_ALS_VALID | TSL277X_ST_ALS_IRQ)) {
		taos_get_als(chip);
		report_als(chip);
#ifdef YANG_DEBUG
	printk(KERN_INFO "als %s: status 0x%02x\n", __func__, status);
#endif

	}
exit_clr:
	taos_irq_clr(chip, TSL277X_CMD_PROX_INT_CLR | TSL277X_CMD_ALS_INT_CLR);
	return ret;
}

static irqreturn_t taos_irq(int irq, void *handle)
{
	struct tsl2772_chip *chip = handle;
	struct device *dev = &chip->client->dev;

	mutex_lock(&chip->lock);
/* while early susped, disable als*/
	if (chip->in_early_suspend) {
		dev_dbg(dev, "%s: in early_suspend\n", __func__);
		chip->irq_pending = 1;
		taos_als_enable(chip, 0);
	}
/* while suspend, disable both pros and als */	
 	if (chip->in_suspend) {
		dev_dbg(dev, "%s: in suspend\n", __func__); 
		disable_irq_nosync(chip->client->irq);
		goto bypass;
	} 
	dev_dbg(dev, "%s\n", __func__);
	/*************fx add**************/
#ifdef YANG_DEBUG
	printk(KERN_ALERT "####%s####\n",__func__);
#endif
	(void)taos_check_and_report(chip);
 bypass:
	mutex_unlock(&chip->lock);
	return IRQ_HANDLED;
}

static void set_pltf_settings(struct tsl2772_chip *chip)
{
	struct taos_raw_settings const *s = chip->pdata->raw_settings;
	u8 *sh = chip->shadow;
	struct device *dev = &chip->client->dev;

	if (s) {
		dev_dbg(dev, "%s: form pltf data\n", __func__);
		sh[TSL277X_ALS_TIME] = s->als_time;
		sh[TSL277X_PRX_TIME] = s->prx_time;
		sh[TSL277X_WAIT_TIME] = s->wait_time;
		sh[TSL277X_PERSISTENCE] = s->persist;
		sh[TSL277X_CONFIG] = s->cfg_reg;
		sh[TSL277X_PRX_PULSE_COUNT] = s->prox_pulse_cnt;
		sh[TSL277X_CONTROL] = s->ctrl_reg;
		sh[TSL277X_REG_PRX_OFFS] = s->prox_offs;
	} else {
		dev_dbg(dev, "%s: use defaults\n", __func__);
		sh[TSL277X_ALS_TIME] = 238; /* ~50 ms */
		sh[TSL277X_PRX_TIME] = 255;
		sh[TSL277X_WAIT_TIME] = 0;
		sh[TSL277X_PERSISTENCE] = PRX_PERSIST(1) | ALS_PERSIST(3);
		sh[TSL277X_CONFIG] = 0;//modified by pcyang 
		sh[TSL277X_PRX_PULSE_COUNT] = 0x05; //pcyang modify default value
		/*sh[TSL277X_CONTROL] = AGAIN_8 | PGAIN_1 | 
				PDIOD_CH0 | PDRIVE_30MA;*/ //25mA for again noise//PDRIVE_120MA;
		sh[TSL277X_CONTROL] = 0x60;
		sh[TSL277X_REG_PRX_OFFS] = 0x00;//goof modify default value - should be re-calibrated
	}
	chip->params.als_gate = chip->pdata->parameters.als_gate;
	chip->params.prox_th_max = chip->pdata->parameters.prox_th_max;
	chip->params.prox_th_min = chip->pdata->parameters.prox_th_min;
	chip->params.als_gain = chip->pdata->parameters.als_gain;
	if (chip->pdata->parameters.als_gain) {
		chip->params.als_gain = chip->pdata->parameters.als_gain;
	} else {
		chip->als_gain_auto = true;
		chip->params.als_gain = GAIN_AUTO_INIT_VALUE;
		dev_dbg(&chip->client->dev, "%s: auto als gain.\n", __func__);
	}
	(void)set_als_gain(chip, chip->params.als_gain);
	taos_calc_cpl(chip);
}

static int flush_regs(struct tsl2772_chip *chip)
{
unsigned i;
int rc;
u8 reg;

	dev_dbg(&chip->client->dev, "%s\n", __func__);
	for (i = 0; i < ARRAY_SIZE(restorable_regs); i++) {
		reg = restorable_regs[i];
		rc = taos_i2c_write(chip, reg, chip->shadow[reg]);
		if (rc) {
			dev_err(&chip->client->dev, "%s: err on reg 0x%02x\n",
					__func__, reg);
			break;
		}
	}
	return rc;
}

static int update_enable_reg(struct tsl2772_chip *chip)
{
	dev_dbg(&chip->client->dev, "%s: %02x\n", __func__,
			chip->shadow[TSL277X_ENABLE]);
	return taos_i2c_write(chip, TSL277X_ENABLE,
			chip->shadow[TSL277X_ENABLE]);
}

/*the function is added for calibrate the threshold---added by pcyang 2013-01-25*/
static void taos_prox_calibration(struct tsl2772_chip *chip)
{
    u16 prox_data[20];
    int prox_sum=0;
    int prox_mean=0;
    int i;
    u8 *buf;
    //dev_dbg(&chip->client->dev, "%s:\n", __func__);
#ifdef YANG_DEBUG
	printk(KERN_ALERT "taos_prox_calibration is called!\n");
#endif
    msleep(20);
    prox_is_calibrated = true;	
    for(i=0;i<20;i++)
    {   
#ifdef YANG_DEBUG
		printk(KERN_ALERT "get mutex lock!\n");
#endif
        //mutex_lock(&chip->lock);	can't lock here,because it is locked before call enable p-sensor
        taos_read_all(chip);
        buf = &chip->shadow[TSL277X_PRX_LO];
        chip->prx_inf.raw = (buf[1] << 8) | buf[0];
        //taos_get_prox(chip);
        prox_data[i]=chip->prx_inf.raw;
        prox_sum=prox_sum+prox_data[i];
#ifdef YANG_DEBUG
		printk(KERN_ALERT "will release the mutex!\n");
#endif
        //mutex_unlock(&chip->lock);
#ifdef YANG_DEBUG
		printk(KERN_ALERT "calibration raw[%d]=%d\n",i,prox_data[i]);
#endif
        msleep(3);
    }
    //printk(KERN_ALERT "calibration is ok!\n");
    prox_mean=prox_sum/20;
#ifdef YANG_DEBUG
    printk(KERN_ALERT "prox_mean=%d\n",prox_mean);
#endif
 /*when the prox_mean is so small,set the threshold is a fix value.by pcyang 2013.01.25*/
	if(prox_mean<5)
	{
	chip->params.prox_th_max= 120;
	chip->params.prox_th_min= 40;
	}
	else if(prox_mean<40){
	chip->params.prox_th_max = prox_mean*8;
	chip->params.prox_th_min = prox_mean*2;
	}
	else if(prox_mean<45){
	chip->params.prox_th_max = prox_mean*75/10;
	chip->params.prox_th_min = prox_mean*19/10;
	}
	else if(prox_mean<50){
	chip->params.prox_th_max = prox_mean*7;
	chip->params.prox_th_min = prox_mean*19/10;
	}
	else if(prox_mean<60){
	chip->params.prox_th_max = prox_mean*6;
	chip->params.prox_th_min = prox_mean*18/10;
	}
	else if(prox_mean<70){
	chip->params.prox_th_max = prox_mean*55/10;
	chip->params.prox_th_min = prox_mean*17/10;
	}
	else if(prox_mean<80){
	chip->params.prox_th_max = prox_mean*5;
	chip->params.prox_th_min = prox_mean*17/10;
	}
	else if(prox_mean<100){
	chip->params.prox_th_max = prox_mean*25/10;
	chip->params.prox_th_min = prox_mean*18/10;
	}
	/*if the prox_mean is more than 100,I modified the threshold calculation methold.It 
	depends on your handset test data.----modified by pcyang 2013-01-25*/
	else if(prox_mean<150){
	chip->params.prox_th_max = prox_mean*2;
	chip->params.prox_th_min = prox_mean*15/10;
	}
	else if(prox_mean<200){
	chip->params.prox_th_max = prox_mean*16/10;
	chip->params.prox_th_min = prox_mean*13/10;
	}
	else if(prox_mean<300){
	chip->params.prox_th_max = prox_mean*15/10;
	chip->params.prox_th_min = prox_mean*12/10;
	}
	else if(prox_mean<400){
	chip->params.prox_th_max = prox_mean*14/10;
	chip->params.prox_th_min = prox_mean*11/10;
	}
	else if(prox_mean<=600){
	chip->params.prox_th_max = prox_mean*13/10;
	chip->params.prox_th_min = prox_mean*11/10;
	}
	else {
	prox_is_calibrated = false;
	}
#ifdef YANG_DEBUG
	printk(KERN_ALERT "the threshold is prox_th_max=%d,prox_th_min=%d\n",chip->params.prox_th_max,chip->params.prox_th_min);
#endif
    
}

static int taos_prox_enable(struct tsl2772_chip *chip, int on)
{
	int rc;
	dev_dbg(&chip->client->dev, "%s: on = %d\n", __func__, on);
#ifdef YANG_DEBUG
	WARN(1,"taos_prox_enable-stack");
#endif
	if (on) {
		taos_irq_clr(chip, TSL277X_CMD_PROX_INT_CLR);
		update_prox_thresh(chip, 1);
		chip->shadow[TSL277X_ENABLE] |=
				(TSL277X_EN_PWR_ON | TSL277X_EN_PRX |
				TSL277X_EN_PRX_IRQ);
		rc = update_enable_reg(chip);
                /*added by pcyang for calibrate the threshold----2013-02-22*/
#ifdef YANG_DEBUG
        printk(KERN_ALERT "the params.prox_th_max=%d,the params.prox_th_min=%d\n",chip->params.prox_th_max,chip->params.prox_th_min);
        printk(KERN_ALERT "enable:the status is %d\n",chip->shadow[TSL277X_ENABLE]);
#endif
        //printk(KERN_ALERT "start to read2222\n");
        if(!prox_is_calibrated)
        {
	        taos_prox_calibration(chip);
        }
        if(chip->params.prox_th_max>600)
        {
            chip->params.prox_th_max = chip->pdata->parameters.prox_th_max;
            chip->params.prox_th_min = chip->pdata->parameters.prox_th_min;
        }
        //printk(KERN_ALERT "taos_prox_enable true!\n");
        /*pcyang added end*/
		if (rc)
			return rc;
		msleep(3);        
	} else {
		chip->shadow[TSL277X_ENABLE] &=
				~(TSL277X_EN_PRX_IRQ | TSL277X_EN_PRX);
		if (!(chip->shadow[TSL277X_ENABLE] & TSL277X_EN_ALS))
			chip->shadow[TSL277X_ENABLE] &= ~TSL277X_EN_PWR_ON;
		rc = update_enable_reg(chip);
		if (rc)
			return rc;
		taos_irq_clr(chip, TSL277X_CMD_PROX_INT_CLR);
	}
	if (!rc)
		chip->prx_enabled = on;
	return rc;
}

static int taos_als_enable(struct tsl2772_chip *chip, int on)
{
	int rc;

	dev_dbg(&chip->client->dev, "%s: on = %d\n", __func__, on);
	if (on) {
		taos_irq_clr(chip, TSL277X_CMD_ALS_INT_CLR);
		update_als_thres(chip, 1);
		chip->shadow[TSL277X_ENABLE] |=
				(TSL277X_EN_PWR_ON | TSL277X_EN_ALS |
				TSL277X_EN_ALS_IRQ);
		rc = update_enable_reg(chip);
		if (rc)
			return rc;
		msleep(3);
	} else {
		chip->shadow[TSL277X_ENABLE] &=
				~(TSL277X_EN_ALS_IRQ | TSL277X_EN_ALS);
		if (!(chip->shadow[TSL277X_ENABLE] & TSL277X_EN_PRX))
			chip->shadow[TSL277X_ENABLE] &= ~TSL277X_EN_PWR_ON;
		rc = update_enable_reg(chip);
		if (rc)
			return rc;
		taos_irq_clr(chip, TSL277X_CMD_ALS_INT_CLR);
	}
	if (!rc)
		chip->als_enabled = on;
	return rc;
}

static ssize_t taos_device_als_ch0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->als_inf.ch0);
}

static ssize_t taos_device_als_ch1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->als_inf.ch1);
}

static ssize_t taos_device_als_cpl(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->als_inf.cpl);
}

static ssize_t taos_device_als_lux(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->als_inf.lux);
}

static ssize_t taos_lux_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	struct lux_segment *s = chip->segment;
	int i, k;

	for (i = k = 0; i < chip->segment_num; i++)
		k += snprintf(buf + k, PAGE_SIZE - k, "%d:%u,%u,%u\n", i,
				(s[i].ratio * 1000) >> RATIO_SHIFT,
				(s[i].k0 * 1000) >> SCALE_SHIFT,
				(s[i].k1 * 1000) >> SCALE_SHIFT);
	return k;
}

static ssize_t taos_lux_table_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int i;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	u32 ratio, k0, k1;

	if (4 != sscanf(buf, "%10d:%10u,%10u,%10u", &i, &ratio, &k0, &k1))
		return -EINVAL;
	if (i >= chip->segment_num)
		return -EINVAL;
	mutex_lock(&chip->lock);
	chip->segment[i].ratio = (ratio << RATIO_SHIFT) / 1000;
	chip->segment[i].k0 = (k0 << SCALE_SHIFT) / 1000;
	chip->segment[i].k1 = (k1 << SCALE_SHIFT) / 1000;
	mutex_unlock(&chip->lock);
	return size;
}

static ssize_t taos_als_gain_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d (%s)\n", chip->params.als_gain,
			chip->als_gain_auto ? "auto" : "manual");
}

static ssize_t taos_als_gain_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	unsigned long gain;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);

	rc = strict_strtoul(buf, 10, &gain);
	if (rc)
		return -EINVAL;
	if (gain != 0 && gain != 1 && gain != 8 && gain != 16 && gain != 120)
		return -EINVAL;
	mutex_lock(&chip->lock);
	if (gain) {
		chip->als_gain_auto = false;
		rc = set_als_gain(chip, gain);
		if (!rc)
			taos_calc_cpl(chip);
	} else {
		chip->als_gain_auto = true;
	}
	mutex_unlock(&chip->lock);
	return rc ? rc : size;
}

static ssize_t taos_als_gate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d (in %%)\n", chip->params.als_gate);
}

static ssize_t taos_als_gate_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	unsigned long gate;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);

	rc = strict_strtoul(buf, 10, &gate);
	if (rc || gate > 100)
		return -EINVAL;
	mutex_lock(&chip->lock);
	chip->params.als_gate = gate;
	mutex_unlock(&chip->lock);
	return size;
}

static ssize_t taos_device_prx_raw(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        mutex_lock(&chip->lock);
	taos_read_all(chip);//fill up tao's status
	taos_get_prox(chip);//get tao's raw data
	mutex_unlock(&chip->lock);          
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->prx_inf.raw);
}

static ssize_t taos_device_prx_detected(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", chip->prx_inf.detected);
}
//add for pulse count set/get
static ssize_t taos_device_prx_pulse_count_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    u8 prx_pulse_count;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
    mutex_lock(&chip->lock);
    taos_i2c_read(chip, TSL277X_PRX_PULSE_COUNT, &prx_pulse_count);
    mutex_unlock(&chip->lock);         
	return snprintf(buf, PAGE_SIZE, "%d\n", prx_pulse_count);
}
static ssize_t taos_device_prx_pulse_count_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	unsigned char prx_pulse_count=0;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	rc = kstrtou8(buf, 10, &prx_pulse_count);
	if (rc){
        dev_err(&chip->client->dev, "%s: err on taos_device_prx_pulse_count_storek strtou8 \n",__func__);
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
    rc = taos_i2c_write(chip, TSL277X_PRX_PULSE_COUNT, prx_pulse_count);
    mutex_unlock(&chip->lock);
    if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_PRX_PULSE_COUNT \n",__func__);
	return rc ? rc : size;
}
/*goof add for offset */
static ssize_t taos_device_prx_offset_get(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    	u8 prx_offset=0;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        mutex_lock(&chip->lock);
    	taos_i2c_read(chip, TSL277X_REG_PRX_OFFS, &prx_offset);
        mutex_unlock(&chip->lock);
	return snprintf(buf, PAGE_SIZE, "%d\n",prx_offset);
}
static ssize_t taos_device_prx_offset_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	u8 prx_offset=0;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        
	rc = kstrtou8(buf, 10, &prx_offset);
	if (rc){
        dev_err(&chip->client->dev, "%s: err on taos_device_prx_offset_storek strtou8 \n",__func__);
		return -EINVAL;
	}
        mutex_lock(&chip->lock);
    	rc = taos_i2c_write(chip, TSL277X_REG_PRX_OFFS, prx_offset);
        mutex_unlock(&chip->lock);
	if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_REG_PRX_OFFS \n",__func__);
	return rc ? rc : size;
}
/*end*/
/*goof add for taos control */
static ssize_t taos_device_prx_control_get(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    	u8 prx_control=0;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        mutex_lock(&chip->lock);
    	taos_i2c_read(chip, TSL277X_CONTROL, &prx_control);
        mutex_unlock(&chip->lock);
	return snprintf(buf, PAGE_SIZE, "%d\n",prx_control);
}
static ssize_t taos_device_prx_control_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	u8 prx_control=0;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        
	rc = kstrtou8(buf, 10, &prx_control);
	if (rc){
        dev_err(&chip->client->dev, "%s: err on taos_device_prx_control_storek strtou8 \n",__func__);
		return -EINVAL;
	}
        mutex_lock(&chip->lock);
    	rc = taos_i2c_write(chip, TSL277X_CONTROL, prx_control);
        mutex_unlock(&chip->lock);
	if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_REG_PRX_CONTROL \n",__func__);
	return rc ? rc : size;
}
/*end*/
/*pcyang added*/
static ssize_t taos_device_prx_config_get(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    	u8 prx_config=0;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        mutex_lock(&chip->lock);
    	taos_i2c_read(chip, TSL277X_CONFIG, &prx_config);
        mutex_unlock(&chip->lock);
	return snprintf(buf, PAGE_SIZE, "%d\n",prx_config);
}
static ssize_t taos_device_prx_config_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	u8 prx_config=0;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        
	rc = kstrtou8(buf, 10, &prx_config);
	if (rc){
        dev_err(&chip->client->dev, "%s: err on taos_device_prx_config_storek strtou8 \n",__func__);
		return -EINVAL;
	}
        mutex_lock(&chip->lock);
    	rc = taos_i2c_write(chip, TSL277X_CONFIG, prx_config);
        mutex_unlock(&chip->lock);
        
	if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_REG_PRX_CONFIG \n",__func__);
	return rc ? rc : size;
}
/* bug 418385 begin */
static ssize_t taos_device_prx_enable_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{   
	s8 pro_enable = -1;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	rc = kstrtou8(buf, 10, &pro_enable);
	printk("taos_device_prx_enable pro_enable =%d \n",pro_enable);
	if (rc){
	dev_err(&chip->client->dev, "%s: err on taos_device_prx_config_storek strtou8 \n",__func__);
	return -EINVAL;
	}
	mutex_lock(&chip->lock);
	rc = taos_prox_enable(chip, pro_enable);
	mutex_unlock(&chip->lock);
	if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_REG_PRX_ENABLE \n",__func__);
	return rc ? rc : size;
}

static ssize_t taos_device_enable_reg_get(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    	u8 enable_reg=0;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
        mutex_lock(&chip->lock);
    	taos_i2c_read(chip, TSL277X_ENABLE, &enable_reg);
        mutex_unlock(&chip->lock);
	return snprintf(buf, PAGE_SIZE, "%d\n",enable_reg);
}

static ssize_t taos_device_als_enable_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{   
	s8 als_enable = -1;
	int rc;
	struct tsl2772_chip *chip = dev_get_drvdata(dev);
	rc = kstrtou8(buf, 10, &als_enable);
	printk("taos_device_als_enable_set als_enable =%d \n",als_enable);
	if (rc){
	dev_err(&chip->client->dev, "%s: err on taos_device_prx_config_storek strtou8 \n",__func__);
	return -EINVAL;
	}
	mutex_lock(&chip->lock);
	rc = taos_als_enable(chip, als_enable);
	mutex_unlock(&chip->lock);
	if (rc) 
		dev_err(&chip->client->dev, "%s: err on TSL277X_REG_PRX_ENABLE \n",__func__);
	return rc ? rc : size;
}
 /*bug 418385 end*/
 
/*pcyang added for get the threshold---2013-02-22*/
static ssize_t taos_device_prx_threshold(struct device *dev,
	struct device_attribute *attr, char *buf)
{       
        //s32 ret;
        u16 from=0,to=0;
        u8 low_threshold_low[1], low_threshold_high[1],high_threshold_low[1],high_threshold_high[1];
	    struct tsl2772_chip *chip = dev_get_drvdata(dev);
        //u8 *buffer = &chip->shadow[TSL277X_PRX_MINTHRESHLO];
        mutex_lock(&chip->lock);
        //ret=taos_i2c_blk_read(chip,TSL277X_PRX_MINTHRESHLO,&chip->shadow[TSL277X_PRX_MINTHRESHLO],
			//TSL277X_PRX_MAXTHRESHHI - TSL277X_PRX_MINTHRESHLO + 1);
        taos_i2c_read(chip, TSL277X_PRX_MINTHRESHLO, low_threshold_low);
        taos_i2c_read(chip, TSL277X_PRX_MINTHRESHHI, low_threshold_high);
#ifdef YANG_DEBUG
		printk(KERN_ALERT "the low_threshold_low=%d, low_threshold_high= %d\n",low_threshold_low[0],low_threshold_high[0]);
#endif
        mutex_unlock(&chip->lock);
        /*get the low threshold*/
        from=low_threshold_low[0] | (low_threshold_high[0]<<8);
        
        mutex_lock(&chip->lock);
        taos_i2c_read(chip, TSL277X_PRX_MAXTHRESHLO, high_threshold_low);   
        taos_i2c_read(chip, TSL277X_PRX_MAXTHRESHHI, high_threshold_high);
#ifdef YANG_DEBUG
		printk(KERN_ALERT "the high_threshold_low=%d, high_threshold_high= %d\n",high_threshold_low[0],high_threshold_high[0]);
#endif
        mutex_unlock(&chip->lock);
        to=high_threshold_low[0] | (high_threshold_high[0]<<8);
        
	return snprintf(buf, PAGE_SIZE, "threshold_low=%d,threshold_high=%d\n", from,to);
}
/*pcyang added end*/

static struct device_attribute prox_attrs[] = {
	__ATTR(prx_raw, 0444, taos_device_prx_raw, NULL),
	__ATTR(prx_detect, 0444, taos_device_prx_detected, NULL),
	__ATTR(offset_prx, 0644, taos_device_prx_offset_get, taos_device_prx_offset_set),
	__ATTR(control_prx, 0644, taos_device_prx_control_get, taos_device_prx_control_set),
	__ATTR(pulse_count_prx, 0644, taos_device_prx_pulse_count_show, taos_device_prx_pulse_count_store),
	__ATTR(config_prx, 0644, taos_device_prx_config_get, taos_device_prx_config_set),
	__ATTR(threshold_prx, 0444, taos_device_prx_threshold, NULL),
	__ATTR(enable, 0644, taos_device_enable_reg_get, taos_device_prx_enable_set),
};

static struct device_attribute als_attrs[] = {
	__ATTR(als_ch0, 0444, taos_device_als_ch0, NULL),
	__ATTR(als_ch1, 0444, taos_device_als_ch1, NULL),
	__ATTR(als_cpl, 0444, taos_device_als_cpl, NULL),
	__ATTR(als_lux, 0444, taos_device_als_lux, NULL),
	__ATTR(als_gain, 0644, taos_als_gain_show, taos_als_gain_store),
	__ATTR(als_gate, 0644, taos_als_gate_show, taos_als_gate_store),
	__ATTR(lux_table, 0644, taos_lux_table_show, taos_lux_table_store),
	__ATTR(enable, 0644, taos_device_enable_reg_get, taos_device_als_enable_set),
};

static int add_sysfs_interfaces(struct device *dev,
	struct device_attribute *a, int size)
{
	int i;
	for (i = 0; i < size; i++)
		if (device_create_file(dev, a + i))
			goto undo;
	return 0;
undo:
	for (; i >= 0 ; i--)
		device_remove_file(dev, a + i);
	dev_err(dev, "%s: failed to create sysfs interface\n", __func__);
	return -ENODEV;
}

static void remove_sysfs_interfaces(struct device *dev,
	struct device_attribute *a, int size)
{
	int i;
	for (i = 0; i < size; i++)
		device_remove_file(dev, a + i);
}

static int taos_get_id(struct tsl2772_chip *chip, u8 *id, u8 *rev)
{
	int rc = taos_i2c_read(chip, TSL277X_REVID, rev);
	if (rc)
		return rc;
	return taos_i2c_read(chip, TSL277X_CHIPID, id);
}

static int power_on(struct tsl2772_chip *chip)
{
	int rc;
	rc = pltf_power_on(chip);
	if (rc)
		return rc;
	dev_dbg(&chip->client->dev, "%s: chip was off, restoring regs\n",
			__func__);
	return flush_regs(chip);
}

static int prox_idev_open(struct input_dev *idev)
{
	struct tsl2772_chip *chip = dev_get_drvdata(&idev->dev);
	int rc;
	bool als = chip->a_idev && chip->a_idev->users; 
	dev_dbg(&idev->dev, "%s\n", __func__);
#ifdef YANG_DEBUG
	WARN(1,"prox_idev_open-stack");
	printk(KERN_ALERT "prox_idev_open is called!\n");
#endif
	mutex_lock(&chip->lock);
	if (chip->unpowered) {
		rc = power_on(chip);
		if (rc)
			goto chip_on_err;
	}
	rc = taos_prox_enable(chip, 1);
	if (rc && !als)
		pltf_power_off(chip);
chip_on_err:
	mutex_unlock(&chip->lock);
	return rc;
}

static void prox_idev_close(struct input_dev *idev)
{
	struct tsl2772_chip *chip = dev_get_drvdata(&idev->dev);
 	dev_dbg(&idev->dev, "%s\n", __func__);
	mutex_lock(&chip->lock);
	taos_prox_enable(chip, 0);
	if (!chip->a_idev || !chip->a_idev->users)
		pltf_power_off(chip);
	mutex_unlock(&chip->lock);
}

static int als_idev_open(struct input_dev *idev)
{
	struct tsl2772_chip *chip = dev_get_drvdata(&idev->dev);
	int rc;
	bool prox = chip->p_idev && chip->p_idev->users;

	dev_dbg(&idev->dev, "%s\n", __func__);
	mutex_lock(&chip->lock);
	if (chip->unpowered) {
		rc = power_on(chip);
		if (rc)
			goto chip_on_err;
	}
	rc = taos_als_enable(chip, 1);
	if (rc && !prox)
		pltf_power_off(chip);
chip_on_err:
	mutex_unlock(&chip->lock);
	return rc;
}

static void als_idev_close(struct input_dev *idev)
{
	struct tsl2772_chip *chip = dev_get_drvdata(&idev->dev);
	dev_dbg(&idev->dev, "%s\n", __func__);
	mutex_lock(&chip->lock);
	taos_als_enable(chip, 0);
	if (!chip->p_idev || !chip->p_idev->users)
		pltf_power_off(chip);
	mutex_unlock(&chip->lock);
}

static int __devinit taos_probe(struct i2c_client *client,
	const struct i2c_device_id *idp)
{
	int i, ret;
	u8 id, rev;
	struct device *dev = &client->dev;
	static struct tsl2772_chip *chip;
	struct tsl2772_i2c_platform_data *pdata = dev->platform_data;
	bool powered = 0;
	printk(KERN_ALERT "AAAAAAAAAAAAA\n");
	dev_info(dev, "%s: client->irq = %d\n", __func__, client->irq);
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "%s: i2c smbus byte data unsupported\n", __func__);
		ret = -EOPNOTSUPP;
		goto init_failed;
	}
	if (!pdata) {
		dev_err(dev, "%s: platform data required\n", __func__);
		ret = -EINVAL;
		goto init_failed;
	}
	if (!(pdata->prox_name || pdata->als_name) || client->irq < 0) {
		dev_err(dev, "%s: no reason to run.\n", __func__);
		ret = -EINVAL;
		goto init_failed;
	}
	if (pdata->platform_init) {
		ret = pdata->platform_init(dev);
		if (ret)
			goto init_failed;
	}
	if (pdata->platform_power) {
		ret = pdata->platform_power(dev, POWER_ON);
		if (ret) {
			dev_err(dev, "%s: pltf power on failed\n", __func__);
			goto pon_failed;
		}
		powered = true;
		msleep(10);
	}
	chip = kzalloc(sizeof(struct tsl2772_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto malloc_failed;
	}
	chip->client = client;
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);

	chip->seg_num_max = chip->pdata->segment_num ?
			chip->pdata->segment_num : ARRAY_SIZE(segment_default);
	if (chip->pdata->segment)
		ret = set_segment_table(chip, chip->pdata->segment,
			chip->pdata->segment_num);
	else
		ret =  set_segment_table(chip, segment_default,
			ARRAY_SIZE(segment_default));
	if (ret)
		goto set_segment_failed;

	ret = taos_get_id(chip, &id, &rev);
	if (ret)
		goto id_failed;
	for (i = 0; i < ARRAY_SIZE(tsl277x_ids); i++) {
		if (id == tsl277x_ids[i])
			break;
	}
	if (i < ARRAY_SIZE(tsl277x_names)) {
		dev_info(dev, "%s: '%s rev. %d' detected\n", __func__,
			tsl277x_names[i], rev);
	} else {
		dev_err(dev, "%s: not supported chip id\n", __func__);
		ret = -EOPNOTSUPP;
		goto id_failed;
	}
	mutex_init(&chip->lock);
	set_pltf_settings(chip);
	ret = flush_regs(chip);
	if (ret)
		goto flush_regs_failed;
	if (pdata->platform_power) {
		pdata->platform_power(dev, POWER_OFF);
		powered = false;
		chip->unpowered = true;
	}
	if (!pdata->prox_name)
		goto bypass_prox_idev;
	chip->p_idev = input_allocate_device();
	if (!chip->p_idev) {
		dev_err(dev, "%s: no memory for input_dev '%s'\n",
				__func__, pdata->prox_name);
		ret = -ENODEV;
		goto input_p_alloc_failed;
	}
	chip->p_idev->name = pdata->prox_name;
	chip->p_idev->id.bustype = BUS_I2C;
	set_bit(EV_ABS, chip->p_idev->evbit);
	set_bit(ABS_DISTANCE, chip->p_idev->absbit);
	input_set_abs_params(chip->p_idev, ABS_DISTANCE, 0, 1, 0, 0);
	chip->p_idev->open = prox_idev_open;
	chip->p_idev->close = prox_idev_close;
	dev_set_drvdata(&chip->p_idev->dev, chip);
	ret = input_register_device(chip->p_idev);
	if (ret) {
		input_free_device(chip->p_idev);
		dev_err(dev, "%s: cant register input '%s'\n",
				__func__, pdata->prox_name);
		goto input_p_alloc_failed;
	}
	ret = add_sysfs_interfaces(&chip->p_idev->dev,
			prox_attrs, ARRAY_SIZE(prox_attrs));
	if (ret)
		goto input_p_sysfs_failed;
bypass_prox_idev:
	if (!pdata->als_name)
		goto bypass_als_idev;
	chip->a_idev = input_allocate_device();
	if (!chip->a_idev) {
		dev_err(dev, "%s: no memory for input_dev '%s'\n",
				__func__, pdata->als_name);
		ret = -ENODEV;
		goto input_a_alloc_failed;
	}
	chip->a_idev->name = pdata->als_name;
	chip->a_idev->id.bustype = BUS_I2C;
	set_bit(EV_ABS, chip->a_idev->evbit);
	set_bit(ABS_MISC, chip->a_idev->absbit);
	input_set_abs_params(chip->a_idev, ABS_MISC, 0, 65535, 0, 0);
	chip->a_idev->open = als_idev_open;
	chip->a_idev->close = als_idev_close;
	dev_set_drvdata(&chip->a_idev->dev, chip);
	ret = input_register_device(chip->a_idev);
	if (ret) {
		input_free_device(chip->a_idev);
		dev_err(dev, "%s: cant register input '%s'\n",
				__func__, pdata->prox_name);
		goto input_a_alloc_failed;
	}
	ret = add_sysfs_interfaces(&chip->a_idev->dev,
			als_attrs, ARRAY_SIZE(als_attrs));
	if (ret)
		goto input_a_sysfs_failed;


bypass_als_idev:
	ret = request_threaded_irq(client->irq, NULL, taos_irq,
		      IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED,
		      dev_name(dev), chip);
	if (ret) {
		dev_info(dev, "Failed to request irq %d\n", client->irq);
		goto irq_register_fail;
	}
	chip->suspend_desc.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	chip->suspend_desc.suspend = taos_early_suspend;
	chip->suspend_desc.resume = taos_late_resume;
	register_early_suspend(&chip->suspend_desc);

	taos_als_enable(chip,1);
	taos_prox_enable(chip,1);
	printk(KERN_ALERT "start to calibration\n");
	mutex_lock(&chip->lock);
	taos_prox_calibration(chip);
	mutex_unlock(&chip->lock);
	printk(KERN_ALERT "calibration is end!\n");
	dev_info(dev, "Probe ok.\n");
	return 0;

irq_register_fail:
	if (chip->a_idev) {
		remove_sysfs_interfaces(&chip->a_idev->dev,
			als_attrs, ARRAY_SIZE(als_attrs));
input_a_sysfs_failed:
		input_unregister_device(chip->a_idev);
	}
input_a_alloc_failed:
	if (chip->p_idev) {
		remove_sysfs_interfaces(&chip->p_idev->dev,
			prox_attrs, ARRAY_SIZE(prox_attrs));
input_p_sysfs_failed:
		input_unregister_device(chip->p_idev);
	}
input_p_alloc_failed:
flush_regs_failed:
id_failed:
	kfree(chip->segment);
set_segment_failed:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
malloc_failed:
	if (powered && pdata->platform_power)
		pdata->platform_power(dev, POWER_OFF);
pon_failed:
	if (pdata->platform_teardown)
		pdata->platform_teardown(dev);
init_failed:
	dev_err(dev, "Probe failed.\n");
	return ret;
}

static void taos_early_suspend(struct early_suspend *desc)
{
	struct tsl2772_chip *chip = container_of(desc,struct tsl2772_chip,suspend_desc);
	struct tsl2772_i2c_platform_data *pdata = chip->pdata;
#ifdef YANG_DEBUG
    printk(KERN_ALERT "taos_early_suspend is called!\n");
#endif
	mutex_lock(&chip->lock);
	chip->in_early_suspend = 1;
	if (chip->a_idev && chip->a_idev->users) {
		if (pdata->als_can_wake) {
#ifdef YANG_DEBUG
			printk( "set wake on als\n");
#endif
			chip->wake_irq = 1;
		} else {
#ifdef YANG_DEBUG
			printk("als off\n");
#endif
			taos_als_enable(chip, 0);
		}
	}
	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 1);
	} else if (!chip->unpowered) {
#ifdef YANG_DEBUG
		printk("powering off\n");
#endif
		pltf_power_off(chip);
	}
	mutex_unlock(&chip->lock);
}

static void taos_late_resume(struct early_suspend *desc)
{
	struct tsl2772_chip *chip = container_of(desc,struct tsl2772_chip,suspend_desc);
	bool als_on;
	int rc = 0;
#ifdef YANG_DEBUG
    printk(KERN_ALERT "taos_late_resume is called!\n");
#endif
	mutex_lock(&chip->lock);
	als_on = chip->a_idev && chip->a_idev->users;
	chip->in_early_suspend = 0;
	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 0);
		chip->wake_irq = 0;
	}
	if (chip->unpowered && (als_on)) {
		rc = power_on(chip);
		if (rc)
			goto err_power;
	}
	if (als_on && !chip->als_enabled)
		(void)taos_als_enable(chip, 1);
	if (chip->irq_pending) {
		chip->irq_pending = 0;
		(void)taos_check_and_report(chip);
		(void)taos_als_enable(chip, 1);
	}
err_power:
	mutex_unlock(&chip->lock);
}

static int  taos_suspend(struct i2c_client *client,pm_message_t state)
{
	struct tsl2772_chip *chip = i2c_get_clientdata(client);
	struct tsl2772_i2c_platform_data *pdata = chip->pdata;
#ifdef YANG_DEBUG
	printk(KERN_ALERT "taos_suspend is called!\n");
#endif
	disable_irq_nosync(chip->client->irq);
	chip->in_suspend = 1;
	mutex_lock(&chip->lock); 
	if (chip->p_idev && chip->p_idev->users) {
		if (pdata->proximity_can_wake) {
#ifdef YANG_DEBUG
			printk( "set wake on proximity\n");
#endif
			chip->wake_irq = 1;
		} else {
#ifdef YANG_DEBUG
			printk( "proximity off\n");
#endif
			taos_prox_enable(chip, 0);
		}
	}
	if (chip->a_idev && chip->a_idev->users) {
		if (pdata->als_can_wake) {
#ifdef YANG_DEBUG
			printk( "set wake on als\n");
#endif
			chip->wake_irq = 1;
		} else {
#ifdef YANG_DEBUG
			printk("als off\n");
#endif
			taos_als_enable(chip, 0);
		}
	}
	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 1);
	} else if (!chip->unpowered) {
#ifdef YANG_DEBUG
		printk("powering off\n");
#endif
		pltf_power_off(chip);
	}

	mutex_unlock(&chip->lock);
	return 0;
}

static int  taos_resume(struct i2c_client *client)
{
	struct tsl2772_chip *chip = i2c_get_clientdata(client);
	bool als_on, prx_on;
	int rc = 0;
#ifdef YANG_DEBUG
    printk(KERN_ALERT "taos_resume is called!\n");
#endif
    chip->in_suspend = 0;
	mutex_lock(&chip->lock);
	prx_on = chip->p_idev && chip->p_idev->users;
	als_on = chip->a_idev && chip->a_idev->users;
	if (chip->wake_irq) {
		chip->wake_irq = 0;
	}
	if (chip->unpowered && (prx_on || als_on)) {
		rc = power_on(chip);
		if (rc)
			goto err_power;
	}
/*bug 418385	
	if (prx_on && !chip->prx_enabled)
		(void)taos_prox_enable(chip, 1);
	if (als_on && !chip->als_enabled)
*/
		(void)taos_als_enable(chip, 1);
	enable_irq(chip->client->irq);
err_power:
	mutex_unlock(&chip->lock);

	return 0;
}



static int __devexit taos_remove(struct i2c_client *client)
{
	struct tsl2772_chip *chip = i2c_get_clientdata(client);
	free_irq(client->irq, chip);
	if (chip->a_idev) {
		remove_sysfs_interfaces(&chip->a_idev->dev,
			als_attrs, ARRAY_SIZE(als_attrs));
		input_unregister_device(chip->a_idev);
	}
	if (chip->p_idev) {
		remove_sysfs_interfaces(&chip->p_idev->dev,
			prox_attrs, ARRAY_SIZE(prox_attrs));
		input_unregister_device(chip->p_idev);
	}
	if (chip->pdata->platform_teardown)
		chip->pdata->platform_teardown(&client->dev);
	i2c_set_clientdata(client, NULL);
	kfree(chip->segment);
	kfree(chip);
	return 0;
}

static struct i2c_device_id taos_idtable[] = {
	{ "tsl2772", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, taos_idtable);
/*
static const struct dev_pm_ops taos_pm_ops = {
	.suspend = taos_suspend,
	.resume  = taos_resume,
};
*/
static struct i2c_driver taos_driver = {
	.driver = {
		.name = "tsl2772",
//		.pm = &taos_pm_ops,
	},
	.id_table = taos_idtable,
	.suspend = taos_suspend,
	.resume  = taos_resume,
	.probe = taos_probe,
	.remove = __devexit_p(taos_remove),
};

static int __init taos_init(void)
{
	return i2c_add_driver(&taos_driver);
}

static void __exit taos_exit(void)
{
	i2c_del_driver(&taos_driver);
}

module_init(taos_init);
module_exit(taos_exit);

MODULE_AUTHOR("Aleksej Makarov <aleksej.makarov@sonyericsson.com>");
MODULE_DESCRIPTION("TAOS tsl2772 ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");
