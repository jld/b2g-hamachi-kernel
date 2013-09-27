/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>

#include "msm_sensor.h"
#include "msm.h"
#include "s5k5ca_v4l2.h"

#define SENSOR_NAME "s5k5ca"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k5ca"
#define s5k5ca_obj s5k5ca_##obj
extern int lcd_camera_power_onoff(int on);
#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define S5K5CA_VERBOSE_DGB

#ifdef S5K5CA_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

static struct msm_sensor_ctrl_t s5k5ca_s_ctrl;
static int csi_config;
static int pwdn = 0;

DEFINE_MUTEX(s5k5ca_mut);


static struct msm_camera_i2c_reg_conf s5k5ca_prev_30fps_settings[] = {
	{0x0028,0x7000,0,0},
	{0x002A,0x023C,0,0},
	{0x0F12,0x0000,0,0},//#REG_TC_GP_ActivePrevConfig//Select preview configuration_0
	{0x002A,0x0240,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_PrevOpenAfterChange
	{0x002A,0x0230,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_NewConfigSync//Update preview configuration
	{0x002A,0x023E,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_PrevConfigChanged
	{0x002A,0x0220,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_EnablePreview//Start preview
	{0x0028,0xD000,0,0},
	{0x002A,0xb0a0,0,0},
	{0x0F12,0x0000,0,0},
	{0x0028,0x7000,0,0},
	{0x002A,0x0222,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_EnablePreviewChanged
};

static struct msm_camera_i2c_reg_conf s5k5ca_prev_60fps_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k5ca_prev_90fps_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k5ca_snap_settings[] = {
	{0x0028,0x7000,0,0},
	{0x002A,0x0244,0,0},
	{0x0F12,0x0000,0,0},//Value
	{0x002A,0x0240,0,0},
	{0x0F12,0x0001,0,0},
	{0x002A,0x0230,0,0},
	{0x0F12,0x0001,0,0},
	{0x002A,0x0246,0,0},
	{0x0F12,0x0001,0,0},
	{0x002A,0x0224,0,0},
	{0x0F12,0x0001,0,0},
	{0x0028,0xD000,0,0},
	{0x002A,0xb0a0,0,0},
	{0x0F12,0x0000,0,0},
	{0x0028,0x7000,0,0},
	{0x002A,0x0226,0,0},
	{0x0F12,0x0001,0,0},
};
	//set sensor init setting
	static struct msm_camera_i2c_reg_conf s5k5ca_init_settings[] = {
	{0xFCFC,0xD000,0,0},
	{0x0010,0x0001,0,0},//Reset
	{0x1030,0x0000,0,0},//Clear host interrupt so main will wait
	{0x0014,0x0001,0,0},//ARM go
	{0xFFFF,0x0014,0,20},//Min.10ms delay is required

	{0x0028,0xD000,0,0},
	{0x002A,0x1082,0,0},//driving ability
	{0x0F12,0x0155,0,0},//[9:8] D4 [7:6] D3 [5:4] D2 [3:2] D1 [1:0] D0
	{0x0F12,0x0155,0,0},//[9:8] D9 [7:6] D8 [5:4] D7 [3:2] D6 [1:0] D5
	{0x0F12,0x1555,0,0},//[5:4] GPIO3 [3:2] GPIO2 [1:0] GPIO1
	{0x0F12,0x05d5,0,0},//[11:10] SDA [9:8] SCA [7:6] PCLK [3:2] VSYNC [1:0] HSYNC
	// Start T&P part
	// DO NOT DELETE T&P SECTION COMMENTS! They are required to debug T&P related issues.
	{0x0028,0x7000,0,0},
	{0x002A,0x2CF8,0,0},
	{0x0F12,0xB510,0,0},
	{0x0F12,0x4827,0,0},
	{0x0F12,0x21C0,0,0},
	{0x0F12,0x8041,0,0},
	{0x0F12,0x4825,0,0},
	{0x0F12,0x4A26,0,0},
	{0x0F12,0x3020,0,0},
	{0x0F12,0x8382,0,0},
	{0x0F12,0x1D12,0,0},
	{0x0F12,0x83C2,0,0},
	{0x0F12,0x4822,0,0},
	{0x0F12,0x3040,0,0},
	{0x0F12,0x8041,0,0},
	{0x0F12,0x4821,0,0},
	{0x0F12,0x4922,0,0},
	{0x0F12,0x3060,0,0},
	{0x0F12,0x8381,0,0},
	{0x0F12,0x1D09,0,0},
	{0x0F12,0x83C1,0,0},
	{0x0F12,0x4821,0,0},
	{0x0F12,0x491D,0,0},
	{0x0F12,0x8802,0,0},
	{0x0F12,0x3980,0,0},
	{0x0F12,0x804A,0,0},
	{0x0F12,0x8842,0,0},
	{0x0F12,0x808A,0,0},
	{0x0F12,0x8882,0,0},
	{0x0F12,0x80CA,0,0},
	{0x0F12,0x88C2,0,0},
	{0x0F12,0x810A,0,0},
	{0x0F12,0x8902,0,0},
	{0x0F12,0x491C,0,0},
	{0x0F12,0x80CA,0,0},
	{0x0F12,0x8942,0,0},
	{0x0F12,0x814A,0,0},
	{0x0F12,0x8982,0,0},
	{0x0F12,0x830A,0,0},
	{0x0F12,0x89C2,0,0},
	{0x0F12,0x834A,0,0},
	{0x0F12,0x8A00,0,0},
	{0x0F12,0x4918,0,0},
	{0x0F12,0x8188,0,0},
	{0x0F12,0x4918,0,0},
	{0x0F12,0x4819,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xFA0E,0,0},
	{0x0F12,0x4918,0,0},
	{0x0F12,0x4819,0,0},
	{0x0F12,0x6341,0,0},
	{0x0F12,0x4919,0,0},
	{0x0F12,0x4819,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xFA07,0,0},
	{0x0F12,0x4816,0,0},
	{0x0F12,0x4918,0,0},
	{0x0F12,0x3840,0,0},
	{0x0F12,0x62C1,0,0},
	{0x0F12,0x4918,0,0},
	{0x0F12,0x3880,0,0},
	{0x0F12,0x63C1,0,0},
	{0x0F12,0x4917,0,0},
	{0x0F12,0x6301,0,0},
	{0x0F12,0x4917,0,0},
	{0x0F12,0x3040,0,0},
	{0x0F12,0x6181,0,0},
	{0x0F12,0x4917,0,0},
	{0x0F12,0x4817,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9F7,0,0},
	{0x0F12,0x4917,0,0},
	{0x0F12,0x4817,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9F3,0,0},
	{0x0F12,0x4917,0,0},
	{0x0F12,0x4817,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9EF,0,0},
	{0x0F12,0xBC10,0,0},
	{0x0F12,0xBC08,0,0},
	{0x0F12,0x4718,0,0},
	{0x0F12,0x1100,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x267C,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x2CE8,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x3274,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xF400,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0xF520,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x2DF1,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x89A9,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x2E43,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x0140,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2E7D,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xB4F7,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x2F07,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2F2B,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2FD1,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2FE5,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2FB9,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x013D,0,0},
	{0x0F12,0x0001,0,0},
	{0x0F12,0x306B,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x5823,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x30B9,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xD789,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0xB570,0,0},
	{0x0F12,0x6804,0,0},
	{0x0F12,0x6845,0,0},
	{0x0F12,0x6881,0,0},
	{0x0F12,0x6840,0,0},
	{0x0F12,0x2900,0,0},
	{0x0F12,0x6880,0,0},
	{0x0F12,0xD007,0,0},
	{0x0F12,0x49C3,0,0},
	{0x0F12,0x8949,0,0},
	{0x0F12,0x084A,0,0},
	{0x0F12,0x1880,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9BA,0,0},
	{0x0F12,0x80A0,0,0},
	{0x0F12,0xE000,0,0},
	{0x0F12,0x80A0,0,0},
	{0x0F12,0x88A0,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD010,0,0},
	{0x0F12,0x68A9,0,0},
	{0x0F12,0x6828,0,0},
	{0x0F12,0x084A,0,0},
	{0x0F12,0x1880,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9AE,0,0},
	{0x0F12,0x8020,0,0},
	{0x0F12,0x1D2D,0,0},
	{0x0F12,0xCD03,0,0},
	{0x0F12,0x084A,0,0},
	{0x0F12,0x1880,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9A7,0,0},
	{0x0F12,0x8060,0,0},
	{0x0F12,0xBC70,0,0},
	{0x0F12,0xBC08,0,0},
	{0x0F12,0x4718,0,0},
	{0x0F12,0x2000,0,0},
	{0x0F12,0x8060,0,0},
	{0x0F12,0x8020,0,0},
	{0x0F12,0xE7F8,0,0},
	{0x0F12,0xB510,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF9A2,0,0},
	{0x0F12,0x48B2,0,0},
	{0x0F12,0x8A40,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD00C,0,0},
	{0x0F12,0x48B1,0,0},
	{0x0F12,0x49B2,0,0},
	{0x0F12,0x8800,0,0},
	{0x0F12,0x4AB2,0,0},
	{0x0F12,0x2805,0,0},
	{0x0F12,0xD003,0,0},
	{0x0F12,0x4BB1,0,0},
	{0x0F12,0x795B,0,0},
	{0x0F12,0x2B00,0,0},
	{0x0F12,0xD005,0,0},
	{0x0F12,0x2001,0,0},
	{0x0F12,0x8008,0,0},
	{0x0F12,0x8010,0,0},
	{0x0F12,0xBC10,0,0},
	{0x0F12,0xBC08,0,0},
	{0x0F12,0x4718,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD1FA,0,0},
	{0x0F12,0x2000,0,0},
	{0x0F12,0x8008,0,0},
	{0x0F12,0x8010,0,0},
	{0x0F12,0xE7F6,0,0},
	{0x0F12,0xB5F8,0,0},
	{0x0F12,0x2407,0,0},
	{0x0F12,0x2C06,0,0},
	{0x0F12,0xD035,0,0},
	{0x0F12,0x2C07,0,0},
	{0x0F12,0xD033,0,0},
	{0x0F12,0x48A3,0,0},
	{0x0F12,0x8BC1,0,0},
	{0x0F12,0x2900,0,0},
	{0x0F12,0xD02A,0,0},
	{0x0F12,0x00A2,0,0},
	{0x0F12,0x1815,0,0},
	{0x0F12,0x4AA4,0,0},
	{0x0F12,0x6DEE,0,0},
	{0x0F12,0x8A92,0,0},
	{0x0F12,0x4296,0,0},
	{0x0F12,0xD923,0,0},
	{0x0F12,0x0028,0,0},
	{0x0F12,0x3080,0,0},
	{0x0F12,0x0007,0,0},
	{0x0F12,0x69C0,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF96B,0,0},
	{0x0F12,0x1C71,0,0},
	{0x0F12,0x0280,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF967,0,0},
	{0x0F12,0x0006,0,0},
	{0x0F12,0x4898,0,0},
	{0x0F12,0x0061,0,0},
	{0x0F12,0x1808,0,0},
	{0x0F12,0x8D80,0,0},
	{0x0F12,0x0A01,0,0},
	{0x0F12,0x0600,0,0},
	{0x0F12,0x0E00,0,0},
	{0x0F12,0x1A08,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF96A,0,0},
	{0x0F12,0x0002,0,0},
	{0x0F12,0x6DE9,0,0},
	{0x0F12,0x6FE8,0,0},
	{0x0F12,0x1A08,0,0},
	{0x0F12,0x4351,0,0},
	{0x0F12,0x0300,0,0},
	{0x0F12,0x1C49,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF953,0,0},
	{0x0F12,0x0401,0,0},
	{0x0F12,0x0430,0,0},
	{0x0F12,0x0C00,0,0},
	{0x0F12,0x4301,0,0},
	{0x0F12,0x61F9,0,0},
	{0x0F12,0xE004,0,0},
	{0x0F12,0x00A2,0,0},
	{0x0F12,0x4990,0,0},
	{0x0F12,0x1810,0,0},
	{0x0F12,0x3080,0,0},
	{0x0F12,0x61C1,0,0},
	{0x0F12,0x1E64,0,0},
	{0x0F12,0xD2C5,0,0},
	{0x0F12,0x2006,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF959,0,0},
	{0x0F12,0x2007,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF956,0,0},
	{0x0F12,0xBCF8,0,0},
	{0x0F12,0xBC08,0,0},
	{0x0F12,0x4718,0,0},
	{0x0F12,0xB510,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF958,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD00A,0,0},
	{0x0F12,0x4881,0,0},
	{0x0F12,0x8B81,0,0},
	{0x0F12,0x0089,0,0},
	{0x0F12,0x1808,0,0},
	{0x0F12,0x6DC1,0,0},
	{0x0F12,0x4883,0,0},
	{0x0F12,0x8A80,0,0},
	{0x0F12,0x4281,0,0},
	{0x0F12,0xD901,0,0},
	{0x0F12,0x2001,0,0},
	{0x0F12,0xE7A1,0,0},
	{0x0F12,0x2000,0,0},
	{0x0F12,0xE79F,0,0},
	{0x0F12,0xB5F8,0,0},
	{0x0F12,0x0004,0,0},
	{0x0F12,0x4F80,0,0},
	{0x0F12,0x227D,0,0},
	{0x0F12,0x8938,0,0},
	{0x0F12,0x0152,0,0},
	{0x0F12,0x4342,0,0},
	{0x0F12,0x487E,0,0},
	{0x0F12,0x9000,0,0},
	{0x0F12,0x8A01,0,0},
	{0x0F12,0x0848,0,0},
	{0x0F12,0x1810,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF91D,0,0},
	{0x0F12,0x210F,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF940,0,0},
	{0x0F12,0x497A,0,0},
	{0x0F12,0x8C49,0,0},
	{0x0F12,0x090E,0,0},
	{0x0F12,0x0136,0,0},
	{0x0F12,0x4306,0,0},
	{0x0F12,0x4979,0,0},
	{0x0F12,0x2C00,0,0},
	{0x0F12,0xD003,0,0},
	{0x0F12,0x2001,0,0},
	{0x0F12,0x0240,0,0},
	{0x0F12,0x4330,0,0},
	{0x0F12,0x8108,0,0},
	{0x0F12,0x4876,0,0},
	{0x0F12,0x2C00,0,0},
	{0x0F12,0x8D00,0,0},
	{0x0F12,0xD001,0,0},
	{0x0F12,0x2501,0,0},
	{0x0F12,0xE000,0,0},
	{0x0F12,0x2500,0,0},
	{0x0F12,0x4972,0,0},
	{0x0F12,0x4328,0,0},
	{0x0F12,0x8008,0,0},
	{0x0F12,0x207D,0,0},
	{0x0F12,0x00C0,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF92E,0,0},
	{0x0F12,0x2C00,0,0},
	{0x0F12,0x496E,0,0},
	{0x0F12,0x0328,0,0},
	{0x0F12,0x4330,0,0},
	{0x0F12,0x8108,0,0},
	{0x0F12,0x88F8,0,0},
	{0x0F12,0x2C00,0,0},
	{0x0F12,0x01AA,0,0},
	{0x0F12,0x4310,0,0},
	{0x0F12,0x8088,0,0},
	{0x0F12,0x9800,0,0},
	{0x0F12,0x8A01,0,0},
	{0x0F12,0x486A,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8F1,0,0},
	{0x0F12,0x496A,0,0},
	{0x0F12,0x8809,0,0},
	{0x0F12,0x4348,0,0},
	{0x0F12,0x0400,0,0},
	{0x0F12,0x0C00,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF918,0,0},
	{0x0F12,0x0020,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF91D,0,0},
	{0x0F12,0x4866,0,0},
	{0x0F12,0x7004,0,0},
	{0x0F12,0xE7A3,0,0},
	{0x0F12,0xB510,0,0},
	{0x0F12,0x0004,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF91E,0,0},
	{0x0F12,0x6020,0,0},
	{0x0F12,0x4963,0,0},
	{0x0F12,0x8B49,0,0},
	{0x0F12,0x0789,0,0},
	{0x0F12,0xD001,0,0},
	{0x0F12,0x0040,0,0},
	{0x0F12,0x6020,0,0},
	{0x0F12,0xE74C,0,0},
	{0x0F12,0xB510,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF91B,0,0},
	{0x0F12,0x485F,0,0},
	{0x0F12,0x8880,0,0},
	{0x0F12,0x0601,0,0},
	{0x0F12,0x4854,0,0},
	{0x0F12,0x1609,0,0},
	{0x0F12,0x8141,0,0},
	{0x0F12,0xE742,0,0},
	{0x0F12,0xB5F8,0,0},
	{0x0F12,0x000F,0,0},
	{0x0F12,0x4C55,0,0},
	{0x0F12,0x3420,0,0},
	{0x0F12,0x2500,0,0},
	{0x0F12,0x5765,0,0},
	{0x0F12,0x0039,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF913,0,0},
	{0x0F12,0x9000,0,0},
	{0x0F12,0x2600,0,0},
	{0x0F12,0x57A6,0,0},
	{0x0F12,0x4C4C,0,0},
	{0x0F12,0x42AE,0,0},
	{0x0F12,0xD01B,0,0},
	{0x0F12,0x4D54,0,0},
	{0x0F12,0x8AE8,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD013,0,0},
	{0x0F12,0x484D,0,0},
	{0x0F12,0x8A01,0,0},
	{0x0F12,0x8B80,0,0},
	{0x0F12,0x4378,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8B5,0,0},
	{0x0F12,0x89A9,0,0},
	{0x0F12,0x1A41,0,0},
	{0x0F12,0x484E,0,0},
	{0x0F12,0x3820,0,0},
	{0x0F12,0x8AC0,0,0},
	{0x0F12,0x4348,0,0},
	{0x0F12,0x17C1,0,0},
	{0x0F12,0x0D89,0,0},
	{0x0F12,0x1808,0,0},
	{0x0F12,0x1280,0,0},
	{0x0F12,0x8961,0,0},
	{0x0F12,0x1A08,0,0},
	{0x0F12,0x8160,0,0},
	{0x0F12,0xE003,0,0},
	{0x0F12,0x88A8,0,0},
	{0x0F12,0x0600,0,0},
	{0x0F12,0x1600,0,0},
	{0x0F12,0x8160,0,0},
	{0x0F12,0x200A,0,0},
	{0x0F12,0x5E20,0,0},
	{0x0F12,0x42B0,0,0},
	{0x0F12,0xD011,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8AB,0,0},
	{0x0F12,0x1D40,0,0},
	{0x0F12,0x00C3,0,0},
	{0x0F12,0x1A18,0,0},
	{0x0F12,0x214B,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF897,0,0},
	{0x0F12,0x211F,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8BA,0,0},
	{0x0F12,0x210A,0,0},
	{0x0F12,0x5E61,0,0},
	{0x0F12,0x0FC9,0,0},
	{0x0F12,0x0149,0,0},
	{0x0F12,0x4301,0,0},
	{0x0F12,0x483D,0,0},
	{0x0F12,0x81C1,0,0},
	{0x0F12,0x9800,0,0},
	{0x0F12,0xE74A,0,0},
	{0x0F12,0xB5F1,0,0},
	{0x0F12,0xB082,0,0},
	{0x0F12,0x2500,0,0},
	{0x0F12,0x483A,0,0},
	{0x0F12,0x9001,0,0},
	{0x0F12,0x2400,0,0},
	{0x0F12,0x2028,0,0},
	{0x0F12,0x4368,0,0},
	{0x0F12,0x4A39,0,0},
	{0x0F12,0x4925,0,0},
	{0x0F12,0x1887,0,0},
	{0x0F12,0x1840,0,0},
	{0x0F12,0x9000,0,0},
	{0x0F12,0x9800,0,0},
	{0x0F12,0x0066,0,0},
	{0x0F12,0x9A01,0,0},
	{0x0F12,0x1980,0,0},
	{0x0F12,0x218C,0,0},
	{0x0F12,0x5A09,0,0},
	{0x0F12,0x8A80,0,0},
	{0x0F12,0x8812,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8CA,0,0},
	{0x0F12,0x53B8,0,0},
	{0x0F12,0x1C64,0,0},
	{0x0F12,0x2C14,0,0},
	{0x0F12,0xDBF1,0,0},
	{0x0F12,0x1C6D,0,0},
	{0x0F12,0x2D03,0,0},
	{0x0F12,0xDBE6,0,0},
	{0x0F12,0x9802,0,0},
	{0x0F12,0x6800,0,0},
	{0x0F12,0x0600,0,0},
	{0x0F12,0x0E00,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8C5,0,0},
	{0x0F12,0xBCFE,0,0},
	{0x0F12,0xBC08,0,0},
	{0x0F12,0x4718,0,0},
	{0x0F12,0xB570,0,0},
	{0x0F12,0x6805,0,0},
	{0x0F12,0x2404,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8C5,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD103,0,0},
	{0x0F12,0xF000,0,0},
	{0x0F12,0xF8C9,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x2400,0,0},
	{0x0F12,0x3540,0,0},
	{0x0F12,0x88E8,0,0},
	{0x0F12,0x0500,0,0},
	{0x0F12,0xD403,0,0},
	{0x0F12,0x4822,0,0},
	{0x0F12,0x89C0,0,0},
	{0x0F12,0x2800,0,0},
	{0x0F12,0xD002,0,0},
	{0x0F12,0x2008,0,0},
	{0x0F12,0x4304,0,0},
	{0x0F12,0xE001,0,0},
	{0x0F12,0x2010,0,0},
	{0x0F12,0x4304,0,0},
	{0x0F12,0x481F,0,0},
	{0x0F12,0x8B80,0,0},
	{0x0F12,0x0700,0,0},
	{0x0F12,0x0F81,0,0},
	{0x0F12,0x2001,0,0},
	{0x0F12,0x2900,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x4304,0,0},
	{0x0F12,0x491C,0,0},
	{0x0F12,0x8B0A,0,0},
	{0x0F12,0x42A2,0,0},
	{0x0F12,0xD004,0,0},
	{0x0F12,0x0762,0,0},
	{0x0F12,0xD502,0,0},
	{0x0F12,0x4A19,0,0},
	{0x0F12,0x3220,0,0},
	{0x0F12,0x8110,0,0},
	{0x0F12,0x830C,0,0},
	{0x0F12,0xE691,0,0},
	{0x0F12,0x0C3C,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x3274,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x26E8,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x6100,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x6500,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x1A7C,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x1120,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xFFFF,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x3374,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x1D6C,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x167C,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xF400,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x2C2C,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x40A0,0,0},
	{0x0F12,0x00DD,0,0},
	{0x0F12,0xF520,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x2C29,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x1A54,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x1564,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xF2A0,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x2440,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x05A0,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x2894,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0x1224,0,0},
	{0x0F12,0x7000,0,0},
	{0x0F12,0xB000,0,0},
	{0x0F12,0xD000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x1A3F,0,0},
	{0x0F12,0x0001,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xF004,0,0},
	{0x0F12,0xE51F,0,0},
	{0x0F12,0x1F48,0,0},
	{0x0F12,0x0001,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x24BD,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x36DD,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xB4CF,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xB5D7,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x36ED,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xF53F,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xF5D9,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x013D,0,0},
	{0x0F12,0x0001,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xF5C9,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xFAA9,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x3723,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0x5823,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xD771,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x4778,0,0},
	{0x0F12,0x46C0,0,0},
	{0x0F12,0xC000,0,0},
	{0x0F12,0xE59F,0,0},
	{0x0F12,0xFF1C,0,0},
	{0x0F12,0xE12F,0,0},
	{0x0F12,0xD75B,0,0},
	{0x0F12,0x0000,0,0},
	{0x0F12,0x8117,0,0},
	{0x0F12,0x0000,0,0},
	      
	// End T&P part

	//========================================================        
	// CIs/APs/An setting    - 400LsB  sYsCLK 32MHz           
	//========================================================        
	// This regis are for FACTORY ONLY. If you change it without prior notification,
	// YOU are REsIBLE for the FAILURE that will happen in the future.      
	//========================================================        
	      
	{0x002A,0x157A,0,0},
	{0x0F12,0x0001,0,0},
	{0x002A,0x1578,0,0},
	{0x0F12,0x0001,0,0},
	{0x002A,0x1576,0,0},
	{0x0F12,0x0020,0,0},
	{0x002A,0x1574,0,0},
	{0x0F12,0x0006,0,0},
	{0x002A,0x156E,0,0},
	{0x0F12,0x0001,0,0}, // slope calibration tolerance in units of 1/256 
	{0x002A,0x1568,0,0},
	{0x0F12,0x00FC,0,0},
	  
	//ADC control 
	{0x002A,0x155A,0,0},
	{0x0F12,0x01CC,0,0}, //ADC sAT of 450mV for 10bit default in EVT1          
	{0x002A,0x157E,0,0},                        
	{0x0F12,0x0C80,0,0}, // 3200 Max. Reset ramp DCLK counts (default 2048 0x800)     
	{0x0F12,0x0578,0,0}, // 1400 Max. Reset ramp DCLK counts for x3.5         
	{0x002A,0x157C,0,0},                        
	{0x0F12,0x0190,0,0}, // 400 Reset ramp for x1 in DCLK counts          
	{0x002A,0x1570,0,0},                        
	{0x0F12,0x00A0,0,0}, // 160 LsB                     
	{0x0F12,0x0010,0,0}, // reset threshold                 
	{0x002A,0x12C4,0,0},                        
	{0x0F12,0x006A,0,0}, // 106 additional timing columns.            
	{0x002A,0x12C8,0,0},                        
	{0x0F12,0x08AC,0,0}, // 2220 ADC columns in normal mode including Hold & Latch    
	{0x0F12,0x0050,0,0}, // 80 addition of ADC columns in Y-ave mode (default 244 0x74)

	{0x002A,0x1696,0,0}, // based on APs guidelines        
	{0x0F12,0x0000,0,0}, // based on APs guidelines        
	{0x0F12,0x0000,0,0}, // default. 1492 used for ADC dark characteristics
	{0x0F12,0x00C6,0,0}, // default. 1492 used for ADC dark characteristics
	{0x0F12,0x00C6,0,0},                                   
	              
	{0x002A,0x1690,0,0}, // when set double sampling is activated - requires different set of pointers                 
	{0x0F12,0x0001,0,0},                   
	              
	{0x002A,0x12B0,0,0}, // comp and pixel bias control 0xF40E - default for EVT1                        
	{0x0F12,0x0055,0,0}, // comp and pixel bias control 0xF40E for binning mode                        
	{0x0F12,0x005A,0,0},                   
	              
	{0x002A,0x337A,0,0}, // [7] - is used for rest-only mode (EVT0 value is 0xD and HW 0x6)                    
	{0x0F12,0x0006,0,0},
	{0x0F12,0x0068,0,0},
	{0x002A,0x169E,0,0},
	{0x0F12,0x0007,0,0},
	{0x002A,0x0BF6,0,0},
	{0x0F12,0x0000,0,0},


	{0x002A,0x327C,0,0},
	{0x0F12,0x1000,0,0},
	{0x0F12,0x6998,0,0},
	{0x0F12,0x0078,0,0},
	{0x0F12,0x04FE,0,0},
	{0x0F12,0x8800,0,0},

	{0x002A,0x3274,0,0},
	{0x0F12,0x0155,0,0}, //set IO driving current 2mA for Gs500
	{0x0F12,0x0155,0,0}, //set IO driving current      
	{0x0F12,0x1555,0,0}, //set IO driving current      
	{0x0F12,0x05D5,0,0},//0555 //set IO driving current      

	{0x0028,0x7000,0,0},
	{0x002A,0x0572,0,0},
	{0x0F12,0x0007,0,0}, //#skl_usConfigStbySettings // Enable T&P code after HW stby + skip ZI part on HW wakeup.

	{0x0028,0x7000,0,0}, 
	{0x002A,0x12D2,0,0},  
	{0x0F12,0x0003,0,0}, //senHal_pContSenModesRegsArray[0][0]2 700012D2   
	{0x0F12,0x0003,0,0}, //senHal_pContSenModesRegsArray[0][1]2 700012D4  
	{0x0F12,0x0003,0,0}, //senHal_pContSenModesRegsArray[0][2]2 700012D6  
	{0x0F12,0x0003,0,0}, //senHal_pContSenModesRegsArray[0][3]2 700012D8  
	{0x0F12,0x0884,0,0}, //senHal_pContSenModesRegsArray[1][0]2 700012DA  
	{0x0F12,0x08CF,0,0}, //senHal_pContSenModesRegsArray[1][1]2 700012DC  
	{0x0F12,0x0500,0,0}, //senHal_pContSenModesRegsArray[1][2]2 700012DE  
	{0x0F12,0x054B,0,0}, //senHal_pContSenModesRegsArray[1][3]2 700012E0  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[2][0]2 700012E2  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[2][1]2 700012E4  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[2][2]2 700012E6  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[2][3]2 700012E8  
	{0x0F12,0x0885,0,0}, //senHal_pContSenModesRegsArray[3][0]2 700012EA  
	{0x0F12,0x0467,0,0}, //senHal_pContSenModesRegsArray[3][1]2 700012EC  
	{0x0F12,0x0501,0,0}, //senHal_pContSenModesRegsArray[3][2]2 700012EE  
	{0x0F12,0x02A5,0,0}, //senHal_pContSenModesRegsArray[3][3]2 700012F0  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[4][0]2 700012F2  
	{0x0F12,0x046A,0,0}, //senHal_pContSenModesRegsArray[4][1]2 700012F4  
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[4][2]2 700012F6  
	{0x0F12,0x02A8,0,0}, //senHal_pContSenModesRegsArray[4][3]2 700012F8  
	{0x0F12,0x0885,0,0}, //senHal_pContSenModesRegsArray[5][0]2 700012FA  
	{0x0F12,0x08D0,0,0}, //senHal_pContSenModesRegsArray[5][1]2 700012FC  
	{0x0F12,0x0501,0,0}, //senHal_pContSenModesRegsArray[5][2]2 700012FE  
	{0x0F12,0x054C,0,0}, //senHal_pContSenModesRegsArray[5][3]2 70001300  
	{0x0F12,0x0006,0,0}, //senHal_pContSenModesRegsArray[6][0]2 70001302  
	{0x0F12,0x0020,0,0}, //senHal_pContSenModesRegsArray[6][1]2 70001304  
	{0x0F12,0x0006,0,0}, //senHal_pContSenModesRegsArray[6][2]2 70001306  
	{0x0F12,0x0020,0,0}, //senHal_pContSenModesRegsArray[6][3]2 70001308  
	{0x0F12,0x0881,0,0}, //senHal_pContSenModesRegsArray[7][0]2 7000130A  
	{0x0F12,0x0463,0,0}, //senHal_pContSenModesRegsArray[7][1]2 7000130C  
	{0x0F12,0x04FD,0,0}, //senHal_pContSenModesRegsArray[7][2]2 7000130E  
	{0x0F12,0x02A1,0,0}, //senHal_pContSenModesRegsArray[7][3]2 70001310  
	{0x0F12,0x0006,0,0}, //senHal_pContSenModesRegsArray[8][0]2 70001312  
	{0x0F12,0x0489,0,0}, //senHal_pContSenModesRegsArray[8][1]2 70001314  
	{0x0F12,0x0006,0,0}, //senHal_pContSenModesRegsArray[8][2]2 70001316  
	{0x0F12,0x02C7,0,0}, //senHal_pContSenModesRegsArray[8][3]2 70001318  
	{0x0F12,0x0881,0,0}, //senHal_pContSenModesRegsArray[9][0]2 7000131A  
	{0x0F12,0x08CC,0,0}, //senHal_pContSenModesRegsArray[9][1]2 7000131C  
	{0x0F12,0x04FD,0,0}, //senHal_pContSenModesRegsArray[9][2]2 7000131E  
	{0x0F12,0x0548,0,0}, //senHal_pContSenModesRegsArray[9][3]2 70001320  
	{0x0F12,0x03A2,0,0}, //senHal_pContSenModesRegsArray[10][0] 2 70001322
	{0x0F12,0x01D3,0,0}, //senHal_pContSenModesRegsArray[10][1] 2 70001324
	{0x0F12,0x01E0,0,0}, //senHal_pContSenModesRegsArray[10][2] 2 70001326
	{0x0F12,0x00F2,0,0}, //senHal_pContSenModesRegsArray[10][3] 2 70001328
	{0x0F12,0x03F2,0,0}, //senHal_pContSenModesRegsArray[11][0] 2 7000132A
	{0x0F12,0x0223,0,0}, //senHal_pContSenModesRegsArray[11][1] 2 7000132C
	{0x0F12,0x0230,0,0}, //senHal_pContSenModesRegsArray[11][2] 2 7000132E
	{0x0F12,0x0142,0,0}, //senHal_pContSenModesRegsArray[11][3] 2 70001330
	{0x0F12,0x03A2,0,0}, //senHal_pContSenModesRegsArray[12][0] 2 70001332
	{0x0F12,0x063C,0,0}, //senHal_pContSenModesRegsArray[12][1] 2 70001334
	{0x0F12,0x01E0,0,0}, //senHal_pContSenModesRegsArray[12][2] 2 70001336
	{0x0F12,0x0399,0,0}, //senHal_pContSenModesRegsArray[12][3] 2 70001338
	{0x0F12,0x03F2,0,0}, //senHal_pContSenModesRegsArray[13][0] 2 7000133A
	{0x0F12,0x068C,0,0}, //senHal_pContSenModesRegsArray[13][1] 2 7000133C
	{0x0F12,0x0230,0,0}, //senHal_pContSenModesRegsArray[13][2] 2 7000133E
	{0x0F12,0x03E9,0,0}, //senHal_pContSenModesRegsArray[13][3] 2 70001340
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[14][0] 2 70001342
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[14][1] 2 70001344
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[14][2] 2 70001346
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[14][3] 2 70001348
	{0x0F12,0x003C,0,0}, //senHal_pContSenModesRegsArray[15][0] 2 7000134A
	{0x0F12,0x003C,0,0}, //senHal_pContSenModesRegsArray[15][1] 2 7000134C
	{0x0F12,0x003C,0,0}, //senHal_pContSenModesRegsArray[15][2] 2 7000134E
	{0x0F12,0x003C,0,0}, //senHal_pContSenModesRegsArray[15][3] 2 70001350
	{0x0F12,0x01D3,0,0}, //senHal_pContSenModesRegsArray[16][0] 2 70001352
	{0x0F12,0x01D3,0,0}, //senHal_pContSenModesRegsArray[16][1] 2 70001354
	{0x0F12,0x00F2,0,0}, //senHal_pContSenModesRegsArray[16][2] 2 70001356
	{0x0F12,0x00F2,0,0}, //senHal_pContSenModesRegsArray[16][3] 2 70001358
	{0x0F12,0x020B,0,0}, //senHal_pContSenModesRegsArray[17][0] 2 7000135A
	{0x0F12,0x024A,0,0}, //senHal_pContSenModesRegsArray[17][1] 2 7000135C
	{0x0F12,0x012A,0,0}, //senHal_pContSenModesRegsArray[17][2] 2 7000135E
	{0x0F12,0x0169,0,0}, //senHal_pContSenModesRegsArray[17][3] 2 70001360
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[18][0] 2 70001362
	{0x0F12,0x046B,0,0}, //senHal_pContSenModesRegsArray[18][1] 2 70001364
	{0x0F12,0x0002,0,0}, //senHal_pContSenModesRegsArray[18][2] 2 70001366
	{0x0F12,0x02A9,0,0}, //senHal_pContSenModesRegsArray[18][3] 2 70001368
	{0x0F12,0x0419,0,0}, //senHal_pContSenModesRegsArray[19][0] 2 7000136A
	{0x0F12,0x04A5,0,0}, //senHal_pContSenModesRegsArray[19][1] 2 7000136C
	{0x0F12,0x0257,0,0}, //senHal_pContSenModesRegsArray[19][2] 2 7000136E
	{0x0F12,0x02E3,0,0}, //senHal_pContSenModesRegsArray[19][3] 2 70001370
	{0x0F12,0x0630,0,0}, //senHal_pContSenModesRegsArray[20][0] 2 70001372
	{0x0F12,0x063C,0,0}, //senHal_pContSenModesRegsArray[20][1] 2 70001374
	{0x0F12,0x038D,0,0}, //senHal_pContSenModesRegsArray[20][2] 2 70001376
	{0x0F12,0x0399,0,0}, //senHal_pContSenModesRegsArray[20][3] 2 70001378
	{0x0F12,0x0668,0,0}, //senHal_pContSenModesRegsArray[21][0] 2 7000137A
	{0x0F12,0x06B3,0,0}, //senHal_pContSenModesRegsArray[21][1] 2 7000137C
	{0x0F12,0x03C5,0,0}, //senHal_pContSenModesRegsArray[21][2] 2 7000137E
	{0x0F12,0x0410,0,0}, //senHal_pContSenModesRegsArray[21][3] 2 70001380
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[22][0] 2 70001382
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[22][1] 2 70001384
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[22][2] 2 70001386
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[22][3] 2 70001388
	{0x0F12,0x03A2,0,0}, //senHal_pContSenModesRegsArray[23][0] 2 7000138A
	{0x0F12,0x01D3,0,0}, //senHal_pContSenModesRegsArray[23][1] 2 7000138C
	{0x0F12,0x01E0,0,0}, //senHal_pContSenModesRegsArray[23][2] 2 7000138E
	{0x0F12,0x00F2,0,0}, //senHal_pContSenModesRegsArray[23][3] 2 70001390
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[24][0] 2 70001392
	{0x0F12,0x0461,0,0}, //senHal_pContSenModesRegsArray[24][1] 2 70001394
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[24][2] 2 70001396
	{0x0F12,0x029F,0,0}, //senHal_pContSenModesRegsArray[24][3] 2 70001398
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[25][0] 2 7000139A
	{0x0F12,0x063C,0,0}, //senHal_pContSenModesRegsArray[25][1] 2 7000139C
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[25][2] 2 7000139E
	{0x0F12,0x0399,0,0}, //senHal_pContSenModesRegsArray[25][3] 2 700013A0
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[26][0] 2 700013A2
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[26][1] 2 700013A4
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[26][2] 2 700013A6
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[26][3] 2 700013A8
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[27][0] 2 700013AA
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[27][1] 2 700013AC
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[27][2] 2 700013AE
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[27][3] 2 700013B0
	{0x0F12,0x020C,0,0}, //senHal_pContSenModesRegsArray[28][0] 2 700013B2
	{0x0F12,0x024B,0,0}, //senHal_pContSenModesRegsArray[28][1] 2 700013B4
	{0x0F12,0x012B,0,0}, //senHal_pContSenModesRegsArray[28][2] 2 700013B6
	{0x0F12,0x016A,0,0}, //senHal_pContSenModesRegsArray[28][3] 2 700013B8
	{0x0F12,0x039F,0,0}, //senHal_pContSenModesRegsArray[29][0] 2 700013BA
	{0x0F12,0x045E,0,0}, //senHal_pContSenModesRegsArray[29][1] 2 700013BC
	{0x0F12,0x01DD,0,0}, //senHal_pContSenModesRegsArray[29][2] 2 700013BE
	{0x0F12,0x029C,0,0}, //senHal_pContSenModesRegsArray[29][3] 2 700013C0
	{0x0F12,0x041A,0,0}, //senHal_pContSenModesRegsArray[30][0] 2 700013C2
	{0x0F12,0x04A6,0,0}, //senHal_pContSenModesRegsArray[30][1] 2 700013C4
	{0x0F12,0x0258,0,0}, //senHal_pContSenModesRegsArray[30][2] 2 700013C6
	{0x0F12,0x02E4,0,0}, //senHal_pContSenModesRegsArray[30][3] 2 700013C8
	{0x0F12,0x062D,0,0}, //senHal_pContSenModesRegsArray[31][0] 2 700013CA
	{0x0F12,0x0639,0,0}, //senHal_pContSenModesRegsArray[31][1] 2 700013CC
	{0x0F12,0x038A,0,0}, //senHal_pContSenModesRegsArray[31][2] 2 700013CE
	{0x0F12,0x0396,0,0}, //senHal_pContSenModesRegsArray[31][3] 2 700013D0
	{0x0F12,0x0669,0,0}, //senHal_pContSenModesRegsArray[32][0] 2 700013D2
	{0x0F12,0x06B4,0,0}, //senHal_pContSenModesRegsArray[32][1] 2 700013D4
	{0x0F12,0x03C6,0,0}, //senHal_pContSenModesRegsArray[32][2] 2 700013D6
	{0x0F12,0x0411,0,0}, //senHal_pContSenModesRegsArray[32][3] 2 700013D8
	{0x0F12,0x087C,0,0}, //senHal_pContSenModesRegsArray[33][0] 2 700013DA
	{0x0F12,0x08C7,0,0}, //senHal_pContSenModesRegsArray[33][1] 2 700013DC
	{0x0F12,0x04F8,0,0}, //senHal_pContSenModesRegsArray[33][2] 2 700013DE
	{0x0F12,0x0543,0,0}, //senHal_pContSenModesRegsArray[33][3] 2 700013E0
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[34][0] 2 700013E2
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[34][1] 2 700013E4
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[34][2] 2 700013E6
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[34][3] 2 700013E8
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[35][0] 2 700013EA
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[35][1] 2 700013EC
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[35][2] 2 700013EE
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[35][3] 2 700013F0
	{0x0F12,0x020F,0,0}, //senHal_pContSenModesRegsArray[36][0] 2 700013F2
	{0x0F12,0x024E,0,0}, //senHal_pContSenModesRegsArray[36][1] 2 700013F4
	{0x0F12,0x012E,0,0}, //senHal_pContSenModesRegsArray[36][2] 2 700013F6
	{0x0F12,0x016D,0,0}, //senHal_pContSenModesRegsArray[36][3] 2 700013F8
	{0x0F12,0x039F,0,0}, //senHal_pContSenModesRegsArray[37][0] 2 700013FA
	{0x0F12,0x045E,0,0}, //senHal_pContSenModesRegsArray[37][1] 2 700013FC
	{0x0F12,0x01DD,0,0}, //senHal_pContSenModesRegsArray[37][2] 2 700013FE
	{0x0F12,0x029C,0,0}, //senHal_pContSenModesRegsArray[37][3] 2 70001400
	{0x0F12,0x041D,0,0}, //senHal_pContSenModesRegsArray[38][0] 2 70001402
	{0x0F12,0x04A9,0,0}, //senHal_pContSenModesRegsArray[38][1] 2 70001404
	{0x0F12,0x025B,0,0}, //senHal_pContSenModesRegsArray[38][2] 2 70001406
	{0x0F12,0x02E7,0,0}, //senHal_pContSenModesRegsArray[38][3] 2 70001408
	{0x0F12,0x062D,0,0}, //senHal_pContSenModesRegsArray[39][0] 2 7000140A
	{0x0F12,0x0639,0,0}, //senHal_pContSenModesRegsArray[39][1] 2 7000140C
	{0x0F12,0x038A,0,0}, //senHal_pContSenModesRegsArray[39][2] 2 7000140E
	{0x0F12,0x0396,0,0}, //senHal_pContSenModesRegsArray[39][3] 2 70001410
	{0x0F12,0x066C,0,0}, //senHal_pContSenModesRegsArray[40][0] 2 70001412
	{0x0F12,0x06B7,0,0}, //senHal_pContSenModesRegsArray[40][1] 2 70001414
	{0x0F12,0x03C9,0,0}, //senHal_pContSenModesRegsArray[40][2] 2 70001416
	{0x0F12,0x0414,0,0}, //senHal_pContSenModesRegsArray[40][3] 2 70001418
	{0x0F12,0x087C,0,0}, //senHal_pContSenModesRegsArray[41][0] 2 7000141A
	{0x0F12,0x08C7,0,0}, //senHal_pContSenModesRegsArray[41][1] 2 7000141C
	{0x0F12,0x04F8,0,0}, //senHal_pContSenModesRegsArray[41][2] 2 7000141E
	{0x0F12,0x0543,0,0}, //senHal_pContSenModesRegsArray[41][3] 2 70001420
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[42][0] 2 70001422
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[42][1] 2 70001424
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[42][2] 2 70001426
	{0x0F12,0x0040,0,0}, //senHal_pContSenModesRegsArray[42][3] 2 70001428
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[43][0] 2 7000142A
	{0x0F12,0x01D0,0,0}, //senHal_pContSenModesRegsArray[43][1] 2 7000142C
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[43][2] 2 7000142E
	{0x0F12,0x00EF,0,0}, //senHal_pContSenModesRegsArray[43][3] 2 70001430
	{0x0F12,0x020F,0,0}, //senHal_pContSenModesRegsArray[44][0] 2 70001432
	{0x0F12,0x024E,0,0}, //senHal_pContSenModesRegsArray[44][1] 2 70001434
	{0x0F12,0x012E,0,0}, //senHal_pContSenModesRegsArray[44][2] 2 70001436
	{0x0F12,0x016D,0,0}, //senHal_pContSenModesRegsArray[44][3] 2 70001438
	{0x0F12,0x039F,0,0}, //senHal_pContSenModesRegsArray[45][0] 2 7000143A
	{0x0F12,0x045E,0,0}, //senHal_pContSenModesRegsArray[45][1] 2 7000143C
	{0x0F12,0x01DD,0,0}, //senHal_pContSenModesRegsArray[45][2] 2 7000143E
	{0x0F12,0x029C,0,0}, //senHal_pContSenModesRegsArray[45][3] 2 70001440
	{0x0F12,0x041D,0,0}, //senHal_pContSenModesRegsArray[46][0] 2 70001442
	{0x0F12,0x04A9,0,0}, //senHal_pContSenModesRegsArray[46][1] 2 70001444
	{0x0F12,0x025B,0,0}, //senHal_pContSenModesRegsArray[46][2] 2 70001446
	{0x0F12,0x02E7,0,0}, //senHal_pContSenModesRegsArray[46][3] 2 70001448
	{0x0F12,0x062D,0,0}, //senHal_pContSenModesRegsArray[47][0] 2 7000144A
	{0x0F12,0x0639,0,0}, //senHal_pContSenModesRegsArray[47][1] 2 7000144C
	{0x0F12,0x038A,0,0}, //senHal_pContSenModesRegsArray[47][2] 2 7000144E
	{0x0F12,0x0396,0,0}, //senHal_pContSenModesRegsArray[47][3] 2 70001450
	{0x0F12,0x066C,0,0}, //senHal_pContSenModesRegsArray[48][0] 2 70001452
	{0x0F12,0x06B7,0,0}, //senHal_pContSenModesRegsArray[48][1] 2 70001454
	{0x0F12,0x03C9,0,0}, //senHal_pContSenModesRegsArray[48][2] 2 70001456
	{0x0F12,0x0414,0,0}, //senHal_pContSenModesRegsArray[48][3] 2 70001458
	{0x0F12,0x087C,0,0}, //senHal_pContSenModesRegsArray[49][0] 2 7000145A
	{0x0F12,0x08C7,0,0}, //senHal_pContSenModesRegsArray[49][1] 2 7000145C
	{0x0F12,0x04F8,0,0}, //senHal_pContSenModesRegsArray[49][2] 2 7000145E
	{0x0F12,0x0543,0,0}, //senHal_pContSenModesRegsArray[49][3] 2 70001460
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[50][0] 2 70001462
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[50][1] 2 70001464
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[50][2] 2 70001466
	{0x0F12,0x003D,0,0}, //senHal_pContSenModesRegsArray[50][3] 2 70001468
	{0x0F12,0x01D2,0,0}, //senHal_pContSenModesRegsArray[51][0] 2 7000146A
	{0x0F12,0x01D2,0,0}, //senHal_pContSenModesRegsArray[51][1] 2 7000146C
	{0x0F12,0x00F1,0,0}, //senHal_pContSenModesRegsArray[51][2] 2 7000146E
	{0x0F12,0x00F1,0,0}, //senHal_pContSenModesRegsArray[51][3] 2 70001470
	{0x0F12,0x020C,0,0}, //senHal_pContSenModesRegsArray[52][0] 2 70001472
	{0x0F12,0x024B,0,0}, //senHal_pContSenModesRegsArray[52][1] 2 70001474
	{0x0F12,0x012B,0,0}, //senHal_pContSenModesRegsArray[52][2] 2 70001476
	{0x0F12,0x016A,0,0}, //senHal_pContSenModesRegsArray[52][3] 2 70001478
	{0x0F12,0x03A1,0,0}, //senHal_pContSenModesRegsArray[53][0] 2 7000147A
	{0x0F12,0x0460,0,0}, //senHal_pContSenModesRegsArray[53][1] 2 7000147C
	{0x0F12,0x01DF,0,0}, //senHal_pContSenModesRegsArray[53][2] 2 7000147E
	{0x0F12,0x029E,0,0}, //senHal_pContSenModesRegsArray[53][3] 2 70001480
	{0x0F12,0x041A,0,0}, //senHal_pContSenModesRegsArray[54][0] 2 70001482
	{0x0F12,0x04A6,0,0}, //senHal_pContSenModesRegsArray[54][1] 2 70001484
	{0x0F12,0x0258,0,0}, //senHal_pContSenModesRegsArray[54][2] 2 70001486
	{0x0F12,0x02E4,0,0}, //senHal_pContSenModesRegsArray[54][3] 2 70001488
	{0x0F12,0x062F,0,0}, //senHal_pContSenModesRegsArray[55][0] 2 7000148A
	{0x0F12,0x063B,0,0}, //senHal_pContSenModesRegsArray[55][1] 2 7000148C
	{0x0F12,0x038C,0,0}, //senHal_pContSenModesRegsArray[55][2] 2 7000148E
	{0x0F12,0x0398,0,0}, //senHal_pContSenModesRegsArray[55][3] 2 70001490
	{0x0F12,0x0669,0,0}, //senHal_pContSenModesRegsArray[56][0] 2 70001492
	{0x0F12,0x06B4,0,0}, //senHal_pContSenModesRegsArray[56][1] 2 70001494
	{0x0F12,0x03C6,0,0}, //senHal_pContSenModesRegsArray[56][2] 2 70001496
	{0x0F12,0x0411,0,0}, //senHal_pContSenModesRegsArray[56][3] 2 70001498
	{0x0F12,0x087E,0,0}, //senHal_pContSenModesRegsArray[57][0] 2 7000149A
	{0x0F12,0x08C9,0,0}, //senHal_pContSenModesRegsArray[57][1] 2 7000149C
	{0x0F12,0x04FA,0,0}, //senHal_pContSenModesRegsArray[57][2] 2 7000149E
	{0x0F12,0x0545,0,0}, //senHal_pContSenModesRegsArray[57][3] 2 700014A0
	{0x0F12,0x03A2,0,0}, //senHal_pContSenModesRegsArray[58][0] 2 700014A2
	{0x0F12,0x01D3,0,0}, //senHal_pContSenModesRegsArray[58][1] 2 700014A4
	{0x0F12,0x01E0,0,0}, //senHal_pContSenModesRegsArray[58][2] 2 700014A6
	{0x0F12,0x00F2,0,0}, //senHal_pContSenModesRegsArray[58][3] 2 700014A8
	{0x0F12,0x03AF,0,0}, //senHal_pContSenModesRegsArray[59][0] 2 700014AA
	{0x0F12,0x01E0,0,0}, //senHal_pContSenModesRegsArray[59][1] 2 700014AC
	{0x0F12,0x01ED,0,0}, //senHal_pContSenModesRegsArray[59][2] 2 700014AE
	{0x0F12,0x00FF,0,0}, //senHal_pContSenModesRegsArray[59][3] 2 700014B0
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[60][0] 2 700014B2
	{0x0F12,0x0461,0,0}, //senHal_pContSenModesRegsArray[60][1] 2 700014B4
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[60][2] 2 700014B6
	{0x0F12,0x029F,0,0}, //senHal_pContSenModesRegsArray[60][3] 2 700014B8
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[61][0] 2 700014BA
	{0x0F12,0x046E,0,0}, //senHal_pContSenModesRegsArray[61][1] 2 700014BC
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[61][2] 2 700014BE
	{0x0F12,0x02AC,0,0}, //senHal_pContSenModesRegsArray[61][3] 2 700014C0
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[62][0] 2 700014C2
	{0x0F12,0x063C,0,0}, //senHal_pContSenModesRegsArray[62][1] 2 700014C4
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[62][2] 2 700014C6
	{0x0F12,0x0399,0,0}, //senHal_pContSenModesRegsArray[62][3] 2 700014C8
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[63][0] 2 700014CA
	{0x0F12,0x0649,0,0}, //senHal_pContSenModesRegsArray[63][1] 2 700014CC
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[63][2] 2 700014CE
	{0x0F12,0x03A6,0,0}, //senHal_pContSenModesRegsArray[63][3] 2 700014D0
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[64][0] 2 700014D2
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[64][1] 2 700014D4
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[64][2] 2 700014D6
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[64][3] 2 700014D8
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[65][0] 2 700014DA
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[65][1] 2 700014DC
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[65][2] 2 700014DE
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[65][3] 2 700014E0
	{0x0F12,0x03AA,0,0}, //senHal_pContSenModesRegsArray[66][0] 2 700014E2
	{0x0F12,0x01DB,0,0}, //senHal_pContSenModesRegsArray[66][1] 2 700014E4
	{0x0F12,0x01E8,0,0}, //senHal_pContSenModesRegsArray[66][2] 2 700014E6
	{0x0F12,0x00FA,0,0}, //senHal_pContSenModesRegsArray[66][3] 2 700014E8
	{0x0F12,0x03B7,0,0}, //senHal_pContSenModesRegsArray[67][0] 2 700014EA
	{0x0F12,0x01E8,0,0}, //senHal_pContSenModesRegsArray[67][1] 2 700014EC
	{0x0F12,0x01F5,0,0}, //senHal_pContSenModesRegsArray[67][2] 2 700014EE
	{0x0F12,0x0107,0,0}, //senHal_pContSenModesRegsArray[67][3] 2 700014F0
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[68][0] 2 700014F2
	{0x0F12,0x0469,0,0}, //senHal_pContSenModesRegsArray[68][1] 2 700014F4
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[68][2] 2 700014F6
	{0x0F12,0x02A7,0,0}, //senHal_pContSenModesRegsArray[68][3] 2 700014F8
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[69][0] 2 700014FA
	{0x0F12,0x0476,0,0}, //senHal_pContSenModesRegsArray[69][1] 2 700014FC
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[69][2] 2 700014FE
	{0x0F12,0x02B4,0,0}, //senHal_pContSenModesRegsArray[69][3] 2 70001500
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[70][0] 2 70001502
	{0x0F12,0x0644,0,0}, //senHal_pContSenModesRegsArray[70][1] 2 70001504
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[70][2] 2 70001506
	{0x0F12,0x03A1,0,0}, //senHal_pContSenModesRegsArray[70][3] 2 70001508
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[71][0] 2 7000150A
	{0x0F12,0x0651,0,0}, //senHal_pContSenModesRegsArray[71][1] 2 7000150C
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[71][2] 2 7000150E
	{0x0F12,0x03AE,0,0}, //senHal_pContSenModesRegsArray[71][3] 2 70001510
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[72][0] 2 70001512
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[72][1] 2 70001514
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[72][2] 2 70001516
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[72][3] 2 70001518
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[73][0] 2 7000151A
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[73][1] 2 7000151C
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[73][2] 2 7000151E
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[73][3] 2 70001520
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[74][0] 2 70001522
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[74][1] 2 70001524
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[74][2] 2 70001526
	{0x0F12,0x0001,0,0}, //senHal_pContSenModesRegsArray[74][3] 2 70001528
	{0x0F12,0x000F,0,0}, //senHal_pContSenModesRegsArray[75][0] 2 7000152A
	{0x0F12,0x000F,0,0}, //senHal_pContSenModesRegsArray[75][1] 2 7000152C
	{0x0F12,0x000F,0,0}, //senHal_pContSenModesRegsArray[75][2] 2 7000152E
	{0x0F12,0x000F,0,0}, //senHal_pContSenModesRegsArray[75][3] 2 70001530
	{0x0F12,0x05AD,0,0}, //senHal_pContSenModesRegsArray[76][0] 2 70001532
	{0x0F12,0x03DE,0,0}, //senHal_pContSenModesRegsArray[76][1] 2 70001534
	{0x0F12,0x030A,0,0}, //senHal_pContSenModesRegsArray[76][2] 2 70001536
	{0x0F12,0x021C,0,0}, //senHal_pContSenModesRegsArray[76][3] 2 70001538
	{0x0F12,0x062F,0,0}, //senHal_pContSenModesRegsArray[77][0] 2 7000153A
	{0x0F12,0x0460,0,0}, //senHal_pContSenModesRegsArray[77][1] 2 7000153C
	{0x0F12,0x038C,0,0}, //senHal_pContSenModesRegsArray[77][2] 2 7000153E
	{0x0F12,0x029E,0,0}, //senHal_pContSenModesRegsArray[77][3] 2 70001540
	{0x0F12,0x07FC,0,0}, //senHal_pContSenModesRegsArray[78][0] 2 70001542
	{0x0F12,0x0847,0,0}, //senHal_pContSenModesRegsArray[78][1] 2 70001544
	{0x0F12,0x0478,0,0}, //senHal_pContSenModesRegsArray[78][2] 2 70001546
	{0x0F12,0x04C3,0,0}, //senHal_pContSenModesRegsArray[78][3] 2 70001548
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[79][0] 2 7000154A
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[79][1] 2 7000154C
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[79][2] 2 7000154E
	{0x0F12,0x0000,0,0}, //senHal_pContSenModesRegsArray[79][3] 2 70001550

	{0x002A,0x12B8,0,0}, //disable CINTR 0           
	{0x0F12,0x1000,0,0},   
	//============================================================
	// ISP-FE Setting
	//============================================================
	{0x002A,0x158A,0,0},
	{0x0F12,0xEAF0,0,0},
	{0x002A,0x15C6,0,0},
	{0x0F12,0x0020,0,0},
	{0x0F12,0x0060,0,0},
	{0x002A,0x15BC,0,0},
	{0x0F12,0x0200,0,0}, // added by Shy.

	 //Analog Offset for MSM
	{0x002A,0x1608,0,0}, 
	{0x0F12,0x0100,0,0}, // #gisp_msm_sAnalogOffset[0] 
	{0x0F12,0x0100,0,0}, // #gisp_msm_sAnalogOffset[1]
	{0x0F12,0x0100,0,0}, // #gisp_msm_sAnalogOffset[2]
	{0x0F12,0x0100,0,0}, // #gisp_msm_sAnalogOffset[3]

	//============================================================
	// ISP-FE Setting END
	//============================================================
	//================================================================================================
	// SET JPEG & SPOOF
	//================================================================================================
	{0x002A,0x0454,0,0},
	{0x0F12,0x0055,0,0},// #REG_TC_BRC_usCaptureQuality // JPEG BRC (BitRateControl) value // 85

	//================================================================================================
	// SET THUMBNAIL
	// # Foramt : RGB565
	// # Size: VGA
	//================================================================================================
	//0F12 0001 // thumbnail enable
	//0F12 0140 // Width//320 //640
	//0F12 00F0 // Height //240 //480
	//0F12 0000 // Thumbnail format : 0RGB565 1 RGB888 2 Full-YUV (0-255)

	//================================================================================================
	// SET AE
	//================================================================================================
	// AE target 
	{0x002A,0x0F70,0,0},
	{0x0F12,0x003A,0,0}, // #TVAR_ae_BrAve
	// AE mode
	{0x002A,0x0F76,0,0},
	{0x0F12,0x0005,0,0}, //Disable illumination & contrast  // #ae_StatMode
	  // AE weight  
	{0x002A,0x0F7E,0,0},
	{0x0F12,0x0000,0,0}, //#ae_WeightTbl_16_0_
	{0x0F12,0x0000,0,0}, //#ae_WeightTbl_16_1_
	{0x0F12,0x0000,0,0}, //#ae_WeightTbl_16_2_
	{0x0F12,0x0000,0,0}, //#ae_WeightTbl_16_3_
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_4_
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_5_
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_6_
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_7_
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_8_
	{0x0F12,0x0201,0,0}, //#ae_WeightTbl_16_9_
	{0x0F12,0x0102,0,0}, //#ae_WeightTbl_16_10
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_11
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_12
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_13
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_14
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_15
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_16
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_17
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_18
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_19
	{0x0F12,0x0201,0,0}, //#ae_WeightTbl_16_20
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_21
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_22
	{0x0F12,0x0102,0,0}, //#ae_WeightTbl_16_23
	{0x0F12,0x0201,0,0}, //#ae_WeightTbl_16_24
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_25
	{0x0F12,0x0202,0,0}, //#ae_WeightTbl_16_26
	{0x0F12,0x0102,0,0}, //#ae_WeightTbl_16_27
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_28
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_29
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_30
	{0x0F12,0x0101,0,0}, //#ae_WeightTbl_16_31
	          
	//================================================================================================
	// SET FLICKER
	//================================================================================================
	{0x002A,0x0C18,0,0},
	{0x0F12,0x0001,0,0}, // 0001: 60Hz start auto / 0000: 50Hz start auto
	{0x002A,0x04D2,0,0},
	{0x0F12,0x067F,0,0},

	//================================================================================================
	// SET GAS
	//================================================================================================
	// GAS alpha
	// R, Gr, Gb, B per light source
	{0x002A,0x06CE,0,0},
	{0x0F12,0x0130,0,0}, // #TVAR_ash_GASAlpha[0] // Horizon
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[1]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[2]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[3]
	{0x0F12,0x0120,0,0}, // #TVAR_ash_GASAlpha[4] // IncandA
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[5]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[6]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[7]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[8] // WW
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[9]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[10]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[11]//*<BU5D05950 zhangsheng 20100422 begin*/
	{0x0F12,0x00FA,0,0}, // #TVAR_ash_GASAlpha[12]// CWF
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[13]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[14]
	{0x0F12,0x0120,0,0}, // #TVAR_ash_GASAlpha[15]//*BU5D05950 zhangsheng 20100422 end>*/
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[16]// D50
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[17]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[18]
	{0x0F12,0x0120,0,0}, // #TVAR_ash_GASAlpha[19]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[20]// D65
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[21]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[22]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[23]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[24]// D75
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[25]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[26]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASAlpha[27]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASOutdoorAlpha[0] // Outdoor
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASOutdoorAlpha[1]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASOutdoorAlpha[2]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_GASOutdoorAlpha[3]
	 // GAS beta
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[0]// Horizon
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[1]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[2]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[3]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[4]// IncandA
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[5]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[6]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[7]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[8]// WW 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[9]
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[10] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[11] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[12] // CWF
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[13] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[14] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[15] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[16] // D50
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[17] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[18] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[19] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[20] // D65
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[21] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[22] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[23] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[24] // D75
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[25] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[26] 
	{0x0F12,0x0000,0,0}, // #ash_GASBeta[27] 
	{0x0F12,0x0000,0,0}, // #ash_GASOutdoorBeta[0] // Outdoor
	{0x0F12,0x0000,0,0}, // #ash_GASOutdoorBeta[1]
	{0x0F12,0x0000,0,0}, // #ash_GASOutdoorBeta[2]
	{0x0F12,0x0000,0,0}, // #ash_GASOutdoorBeta[3]
	{0x002A,0x06B4,0,0},
	{0x0F12,0x0000,0,0}, // #wbt_bUseOutdoorASH ON:1 OFF:0

	// Parabloic function
	{0x002A,0x075A,0,0},
	{0x0F12,0x0000,0,0}, // #ash_bParabolicEstimation
	{0x0F12,0x0400,0,0}, // #ash_uParabolicCenterX
	{0x0F12,0x0300,0,0}, // #ash_uParabolicCenterY
	{0x0F12,0x0010,0,0}, // #ash_uParabolicScalingA
	{0x0F12,0x0011,0,0}, // #ash_uParabolicScalingB
	{0x002A,0x06C6,0,0},
	{0x0F12,0x0100,0,0}, //ash_CGrasAlphas_0_
	{0x0F12,0x0100,0,0}, //ash_CGrasAlphas_1_
	{0x0F12,0x0100,0,0}, //ash_CGrasAlphas_2_
	{0x0F12,0x0100,0,0}, //ash_CGrasAlphas_3_
	{0x002A,0x0E3C,0,0},
	{0x0F12,0x00C0,0,0}, // #awbb_Alpha_Comp_Mode
	{0x002A,0x074E,0,0}, 
	{0x0F12,0x0000,0,0}, // #ash_bLumaMode //use Beta : 0001 not use Beta : 0000

	// GAS LUT start address // 7000_347C 
	{0x002A,0x0754,0,0},
	{0x0F12,0x347C,0,0},
	{0x0F12,0x7000,0,0},
	             // GAS LUT
	{0x002A,0x347C,0,0}, //*<BU5D05950 zhangsheng 20100422 begin*/ 
	{0x0F12,0x0164,0,0}, // #TVAR_ash_pGAS[0]
	{0x0F12,0x016B,0,0}, // #TVAR_ash_pGAS[1]
	{0x0F12,0x013F,0,0}, // #TVAR_ash_pGAS[2]
	{0x0F12,0x010B,0,0}, // #TVAR_ash_pGAS[3]
	{0x0F12,0x00EF,0,0}, // #TVAR_ash_pGAS[4]
	{0x0F12,0x00DC,0,0}, // #TVAR_ash_pGAS[5]
	{0x0F12,0x00D4,0,0}, // #TVAR_ash_pGAS[6]
	{0x0F12,0x00D4,0,0}, // #TVAR_ash_pGAS[7]
	{0x0F12,0x00E1,0,0}, // #TVAR_ash_pGAS[8]
	{0x0F12,0x00FB,0,0}, // #TVAR_ash_pGAS[9]
	{0x0F12,0x0127,0,0}, // #TVAR_ash_pGAS[10]
	{0x0F12,0x014E,0,0}, // #TVAR_ash_pGAS[11]
	{0x0F12,0x0146,0,0}, // #TVAR_ash_pGAS[12]
	{0x0F12,0x0157,0,0}, // #TVAR_ash_pGAS[13]
	{0x0F12,0x0140,0,0}, // #TVAR_ash_pGAS[14]
	{0x0F12,0x0103,0,0}, // #TVAR_ash_pGAS[15]
	{0x0F12,0x00D5,0,0}, // #TVAR_ash_pGAS[16]
	{0x0F12,0x00B5,0,0}, // #TVAR_ash_pGAS[17]
	{0x0F12,0x00A1,0,0}, // #TVAR_ash_pGAS[18]
	{0x0F12,0x0097,0,0}, // #TVAR_ash_pGAS[19]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[20]
	{0x0F12,0x00AA,0,0}, // #TVAR_ash_pGAS[21]
	{0x0F12,0x00C5,0,0}, // #TVAR_ash_pGAS[22]
	{0x0F12,0x00F2,0,0}, // #TVAR_ash_pGAS[23]
	{0x0F12,0x0131,0,0}, // #TVAR_ash_pGAS[24]
	{0x0F12,0x0142,0,0}, // #TVAR_ash_pGAS[25]
	{0x0F12,0x0132,0,0}, // #TVAR_ash_pGAS[26]
	{0x0F12,0x0108,0,0}, // #TVAR_ash_pGAS[27]
	{0x0F12,0x00CA,0,0}, // #TVAR_ash_pGAS[28]
	{0x0F12,0x009B,0,0}, // #TVAR_ash_pGAS[29]
	{0x0F12,0x0079,0,0}, // #TVAR_ash_pGAS[30]
	{0x0F12,0x0061,0,0}, // #TVAR_ash_pGAS[31]
	{0x0F12,0x0059,0,0}, // #TVAR_ash_pGAS[32]
	{0x0F12,0x0060,0,0}, // #TVAR_ash_pGAS[33]
	{0x0F12,0x0075,0,0}, // #TVAR_ash_pGAS[34]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[35]
	{0x0F12,0x00C2,0,0}, // #TVAR_ash_pGAS[36]
	{0x0F12,0x0106,0,0}, // #TVAR_ash_pGAS[37]
	{0x0F12,0x0137,0,0}, // #TVAR_ash_pGAS[38]
	{0x0F12,0x0109,0,0}, // #TVAR_ash_pGAS[39]
	{0x0F12,0x00DA,0,0}, // #TVAR_ash_pGAS[40]
	{0x0F12,0x009E,0,0}, // #TVAR_ash_pGAS[41]
	{0x0F12,0x006D,0,0}, // #TVAR_ash_pGAS[42]
	{0x0F12,0x0048,0,0}, // #TVAR_ash_pGAS[43]
	{0x0F12,0x0030,0,0}, // #TVAR_ash_pGAS[44]
	{0x0F12,0x0026,0,0}, // #TVAR_ash_pGAS[45]
	{0x0F12,0x002D,0,0}, // #TVAR_ash_pGAS[46]
	{0x0F12,0x0048,0,0}, // #TVAR_ash_pGAS[47]
	{0x0F12,0x0072,0,0}, // #TVAR_ash_pGAS[48]
	{0x0F12,0x00A5,0,0}, // #TVAR_ash_pGAS[49]
	{0x0F12,0x00E7,0,0}, // #TVAR_ash_pGAS[50]
	{0x0F12,0x011F,0,0}, // #TVAR_ash_pGAS[51]
	{0x0F12,0x00F0,0,0}, // #TVAR_ash_pGAS[52]
	{0x0F12,0x00C0,0,0}, // #TVAR_ash_pGAS[53]
	{0x0F12,0x0082,0,0}, // #TVAR_ash_pGAS[54]
	{0x0F12,0x004F,0,0}, // #TVAR_ash_pGAS[55]
	{0x0F12,0x0029,0,0}, // #TVAR_ash_pGAS[56]
	{0x0F12,0x0012,0,0}, // #TVAR_ash_pGAS[57]
	{0x0F12,0x000C,0,0}, // #TVAR_ash_pGAS[58]
	{0x0F12,0x0015,0,0}, // #TVAR_ash_pGAS[59]
	{0x0F12,0x002D,0,0}, // #TVAR_ash_pGAS[60]
	{0x0F12,0x005B,0,0}, // #TVAR_ash_pGAS[61]
	{0x0F12,0x0091,0,0}, // #TVAR_ash_pGAS[62]
	{0x0F12,0x00D5,0,0}, // #TVAR_ash_pGAS[63]
	{0x0F12,0x0110,0,0}, // #TVAR_ash_pGAS[64]
	{0x0F12,0x00E4,0,0}, // #TVAR_ash_pGAS[65]
	{0x0F12,0x00B5,0,0}, // #TVAR_ash_pGAS[66]
	{0x0F12,0x0075,0,0}, // #TVAR_ash_pGAS[67]
	{0x0F12,0x0042,0,0}, // #TVAR_ash_pGAS[68]
	{0x0F12,0x001C,0,0}, // #TVAR_ash_pGAS[69]
	{0x0F12,0x0006,0,0}, // #TVAR_ash_pGAS[70]
	{0x0F12,0x0000,0,0}, // #TVAR_ash_pGAS[71]
	{0x0F12,0x000A,0,0}, // #TVAR_ash_pGAS[72]
	{0x0F12,0x0021,0,0}, // #TVAR_ash_pGAS[73]
	{0x0F12,0x0053,0,0}, // #TVAR_ash_pGAS[74]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[75]
	{0x0F12,0x00D0,0,0}, // #TVAR_ash_pGAS[76]
	{0x0F12,0x010D,0,0}, // #TVAR_ash_pGAS[77]
	{0x0F12,0x00E7,0,0}, // #TVAR_ash_pGAS[78]
	{0x0F12,0x00B8,0,0}, // #TVAR_ash_pGAS[79]
	{0x0F12,0x0079,0,0}, // #TVAR_ash_pGAS[80]
	{0x0F12,0x0047,0,0}, // #TVAR_ash_pGAS[81]
	{0x0F12,0x0021,0,0}, // #TVAR_ash_pGAS[82]
	{0x0F12,0x000C,0,0}, // #TVAR_ash_pGAS[83]
	{0x0F12,0x0004,0,0}, // #TVAR_ash_pGAS[84]
	{0x0F12,0x000E,0,0}, // #TVAR_ash_pGAS[85]
	{0x0F12,0x002B,0,0}, // #TVAR_ash_pGAS[86]
	{0x0F12,0x0059,0,0}, // #TVAR_ash_pGAS[87]
	{0x0F12,0x0093,0,0}, // #TVAR_ash_pGAS[88]
	{0x0F12,0x00DA,0,0}, // #TVAR_ash_pGAS[89]
	{0x0F12,0x0114,0,0}, // #TVAR_ash_pGAS[90]
	{0x0F12,0x00F9,0,0}, // #TVAR_ash_pGAS[91]
	{0x0F12,0x00C9,0,0}, // #TVAR_ash_pGAS[92]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[93]
	{0x0F12,0x005C,0,0}, // #TVAR_ash_pGAS[94]
	{0x0F12,0x0039,0,0}, // #TVAR_ash_pGAS[95]
	{0x0F12,0x0024,0,0}, // #TVAR_ash_pGAS[96]
	{0x0F12,0x001D,0,0}, // #TVAR_ash_pGAS[97]
	{0x0F12,0x0028,0,0}, // #TVAR_ash_pGAS[98]
	{0x0F12,0x0044,0,0}, // #TVAR_ash_pGAS[99]
	{0x0F12,0x0072,0,0}, // #TVAR_ash_pGAS[100]
	{0x0F12,0x00A9,0,0}, // #TVAR_ash_pGAS[101]
	{0x0F12,0x00F0,0,0}, // #TVAR_ash_pGAS[102]
	{0x0F12,0x0126,0,0}, // #TVAR_ash_pGAS[103]
	{0x0F12,0x0115,0,0}, // #TVAR_ash_pGAS[104]
	{0x0F12,0x00EE,0,0}, // #TVAR_ash_pGAS[105]
	{0x0F12,0x00AD,0,0}, // #TVAR_ash_pGAS[106]
	{0x0F12,0x0081,0,0}, // #TVAR_ash_pGAS[107]
	{0x0F12,0x0061,0,0}, // #TVAR_ash_pGAS[108]
	{0x0F12,0x004D,0,0}, // #TVAR_ash_pGAS[109]
	{0x0F12,0x0049,0,0}, // #TVAR_ash_pGAS[110]
	{0x0F12,0x0053,0,0}, // #TVAR_ash_pGAS[111]
	{0x0F12,0x006F,0,0}, // #TVAR_ash_pGAS[112]
	{0x0F12,0x0099,0,0}, // #TVAR_ash_pGAS[113]
	{0x0F12,0x00CF,0,0}, // #TVAR_ash_pGAS[114]
	{0x0F12,0x0118,0,0}, // #TVAR_ash_pGAS[115]
	{0x0F12,0x0140,0,0}, // #TVAR_ash_pGAS[116]
	{0x0F12,0x0136,0,0}, // #TVAR_ash_pGAS[117]
	{0x0F12,0x0122,0,0}, // #TVAR_ash_pGAS[118]
	{0x0F12,0x00E2,0,0}, // #TVAR_ash_pGAS[119]
	{0x0F12,0x00B6,0,0}, // #TVAR_ash_pGAS[120]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[121]
	{0x0F12,0x0089,0,0}, // #TVAR_ash_pGAS[122]
	{0x0F12,0x0086,0,0}, // #TVAR_ash_pGAS[123]
	{0x0F12,0x0090,0,0}, // #TVAR_ash_pGAS[124]
	{0x0F12,0x00AA,0,0}, // #TVAR_ash_pGAS[125]
	{0x0F12,0x00D1,0,0}, // #TVAR_ash_pGAS[126]
	{0x0F12,0x0108,0,0}, // #TVAR_ash_pGAS[127]
	{0x0F12,0x0150,0,0}, // #TVAR_ash_pGAS[128]
	{0x0F12,0x015D,0,0}, // #TVAR_ash_pGAS[129]
	{0x0F12,0x0141,0,0}, // #TVAR_ash_pGAS[130]
	{0x0F12,0x0150,0,0}, // #TVAR_ash_pGAS[131]
	{0x0F12,0x011A,0,0}, // #TVAR_ash_pGAS[132]
	{0x0F12,0x00EF,0,0}, // #TVAR_ash_pGAS[133]
	{0x0F12,0x00D5,0,0}, // #TVAR_ash_pGAS[134]
	{0x0F12,0x00C9,0,0}, // #TVAR_ash_pGAS[135]
	{0x0F12,0x00C6,0,0}, // #TVAR_ash_pGAS[136]
	{0x0F12,0x00D0,0,0}, // #TVAR_ash_pGAS[137]
	{0x0F12,0x00E6,0,0}, // #TVAR_ash_pGAS[138]
	{0x0F12,0x010D,0,0}, // #TVAR_ash_pGAS[139]
	{0x0F12,0x0142,0,0}, // #TVAR_ash_pGAS[140]
	{0x0F12,0x017A,0,0}, // #TVAR_ash_pGAS[141]
	{0x0F12,0x016D,0,0}, // #TVAR_ash_pGAS[142]
	{0x0F12,0x012E,0,0}, // #TVAR_ash_pGAS[143]
	{0x0F12,0x012A,0,0}, // #TVAR_ash_pGAS[144]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_pGAS[145]
	{0x0F12,0x00D3,0,0}, // #TVAR_ash_pGAS[146]
	{0x0F12,0x00B7,0,0}, // #TVAR_ash_pGAS[147]
	{0x0F12,0x00A5,0,0}, // #TVAR_ash_pGAS[148]
	{0x0F12,0x009F,0,0}, // #TVAR_ash_pGAS[149]
	{0x0F12,0x00A0,0,0}, // #TVAR_ash_pGAS[150]
	{0x0F12,0x00AB,0,0}, // #TVAR_ash_pGAS[151]
	{0x0F12,0x00C2,0,0}, // #TVAR_ash_pGAS[152]
	{0x0F12,0x00E6,0,0}, // #TVAR_ash_pGAS[153]
	{0x0F12,0x0108,0,0}, // #TVAR_ash_pGAS[154]
	{0x0F12,0x010B,0,0}, // #TVAR_ash_pGAS[155]
	{0x0F12,0x0125,0,0}, // #TVAR_ash_pGAS[156]
	{0x0F12,0x0105,0,0}, // #TVAR_ash_pGAS[157]
	{0x0F12,0x00D0,0,0}, // #TVAR_ash_pGAS[158]
	{0x0F12,0x00A7,0,0}, // #TVAR_ash_pGAS[159]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[160]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[161]
	{0x0F12,0x0071,0,0}, // #TVAR_ash_pGAS[162]
	{0x0F12,0x0074,0,0}, // #TVAR_ash_pGAS[163]
	{0x0F12,0x0081,0,0}, // #TVAR_ash_pGAS[164]
	{0x0F12,0x0097,0,0}, // #TVAR_ash_pGAS[165]
	{0x0F12,0x00BC,0,0}, // #TVAR_ash_pGAS[166]
	{0x0F12,0x00F2,0,0}, // #TVAR_ash_pGAS[167]
	{0x0F12,0x0107,0,0}, // #TVAR_ash_pGAS[168]
	{0x0F12,0x0108,0,0}, // #TVAR_ash_pGAS[169]
	{0x0F12,0x00D9,0,0}, // #TVAR_ash_pGAS[170]
	{0x0F12,0x00A3,0,0}, // #TVAR_ash_pGAS[171]
	{0x0F12,0x007D,0,0}, // #TVAR_ash_pGAS[172]
	{0x0F12,0x005E,0,0}, // #TVAR_ash_pGAS[173]
	{0x0F12,0x004A,0,0}, // #TVAR_ash_pGAS[174]
	{0x0F12,0x0043,0,0}, // #TVAR_ash_pGAS[175]
	{0x0F12,0x0048,0,0}, // #TVAR_ash_pGAS[176]
	{0x0F12,0x005A,0,0}, // #TVAR_ash_pGAS[177]
	{0x0F12,0x0074,0,0}, // #TVAR_ash_pGAS[178]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[179]
	{0x0F12,0x00D1,0,0}, // #TVAR_ash_pGAS[180]
	{0x0F12,0x00FE,0,0}, // #TVAR_ash_pGAS[181]
	{0x0F12,0x00E7,0,0}, // #TVAR_ash_pGAS[182]
	{0x0F12,0x00B4,0,0}, // #TVAR_ash_pGAS[183]
	{0x0F12,0x0083,0,0}, // #TVAR_ash_pGAS[184]
	{0x0F12,0x0058,0,0}, // #TVAR_ash_pGAS[185]
	{0x0F12,0x0037,0,0}, // #TVAR_ash_pGAS[186]
	{0x0F12,0x0022,0,0}, // #TVAR_ash_pGAS[187]
	{0x0F12,0x001A,0,0}, // #TVAR_ash_pGAS[188]
	{0x0F12,0x0022,0,0}, // #TVAR_ash_pGAS[189]
	{0x0F12,0x0039,0,0}, // #TVAR_ash_pGAS[190]
	{0x0F12,0x0059,0,0}, // #TVAR_ash_pGAS[191]
	{0x0F12,0x0083,0,0}, // #TVAR_ash_pGAS[192]
	{0x0F12,0x00BA,0,0}, // #TVAR_ash_pGAS[193]
	{0x0F12,0x00ED,0,0}, // #TVAR_ash_pGAS[194]
	{0x0F12,0x00D1,0,0}, // #TVAR_ash_pGAS[195]
	{0x0F12,0x00A1,0,0}, // #TVAR_ash_pGAS[196]
	{0x0F12,0x006C,0,0}, // #TVAR_ash_pGAS[197]
	{0x0F12,0x0041,0,0}, // #TVAR_ash_pGAS[198]
	{0x0F12,0x0020,0,0}, // #TVAR_ash_pGAS[199]
	{0x0F12,0x000C,0,0}, // #TVAR_ash_pGAS[200]
	{0x0F12,0x0009,0,0}, // #TVAR_ash_pGAS[201]
	{0x0F12,0x0012,0,0}, // #TVAR_ash_pGAS[202]
	{0x0F12,0x0026,0,0}, // #TVAR_ash_pGAS[203]
	{0x0F12,0x004A,0,0}, // #TVAR_ash_pGAS[204]
	{0x0F12,0x0074,0,0}, // #TVAR_ash_pGAS[205]
	{0x0F12,0x00A9,0,0}, // #TVAR_ash_pGAS[206]
	{0x0F12,0x00E0,0,0}, // #TVAR_ash_pGAS[207]
	{0x0F12,0x00C7,0,0}, // #TVAR_ash_pGAS[208]
	{0x0F12,0x0097,0,0}, // #TVAR_ash_pGAS[209]
	{0x0F12,0x0060,0,0}, // #TVAR_ash_pGAS[210]
	{0x0F12,0x0037,0,0}, // #TVAR_ash_pGAS[211]
	{0x0F12,0x0016,0,0}, // #TVAR_ash_pGAS[212]
	{0x0F12,0x0003,0,0}, // #TVAR_ash_pGAS[213]
	{0x0F12,0x0000,0,0}, // #TVAR_ash_pGAS[214]
	{0x0F12,0x000B,0,0}, // #TVAR_ash_pGAS[215]
	{0x0F12,0x001E,0,0}, // #TVAR_ash_pGAS[216]
	{0x0F12,0x0045,0,0}, // #TVAR_ash_pGAS[217]
	{0x0F12,0x0070,0,0}, // #TVAR_ash_pGAS[218]
	{0x0F12,0x00A8,0,0}, // #TVAR_ash_pGAS[219]
	{0x0F12,0x00DC,0,0}, // #TVAR_ash_pGAS[220]
	{0x0F12,0x00C7,0,0}, // #TVAR_ash_pGAS[221]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[222]
	{0x0F12,0x0064,0,0}, // #TVAR_ash_pGAS[223]
	{0x0F12,0x003B,0,0}, // #TVAR_ash_pGAS[224]
	{0x0F12,0x001B,0,0}, // #TVAR_ash_pGAS[225]
	{0x0F12,0x0009,0,0}, // #TVAR_ash_pGAS[226]
	{0x0F12,0x0004,0,0}, // #TVAR_ash_pGAS[227]
	{0x0F12,0x000E,0,0}, // #TVAR_ash_pGAS[228]
	{0x0F12,0x0028,0,0}, // #TVAR_ash_pGAS[229]
	{0x0F12,0x004C,0,0}, // #TVAR_ash_pGAS[230]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[231]
	{0x0F12,0x00B0,0,0}, // #TVAR_ash_pGAS[232]
	{0x0F12,0x00E5,0,0}, // #TVAR_ash_pGAS[233]
	{0x0F12,0x00D6,0,0}, // #TVAR_ash_pGAS[234]
	{0x0F12,0x00A8,0,0}, // #TVAR_ash_pGAS[235]
	{0x0F12,0x0073,0,0}, // #TVAR_ash_pGAS[236]
	{0x0F12,0x004C,0,0}, // #TVAR_ash_pGAS[237]
	{0x0F12,0x002F,0,0}, // #TVAR_ash_pGAS[238]
	{0x0F12,0x001C,0,0}, // #TVAR_ash_pGAS[239]
	{0x0F12,0x001A,0,0}, // #TVAR_ash_pGAS[240]
	{0x0F12,0x0024,0,0}, // #TVAR_ash_pGAS[241]
	{0x0F12,0x003D,0,0}, // #TVAR_ash_pGAS[242]
	{0x0F12,0x0060,0,0}, // #TVAR_ash_pGAS[243]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[244]
	{0x0F12,0x00C4,0,0}, // #TVAR_ash_pGAS[245]
	{0x0F12,0x00F5,0,0}, // #TVAR_ash_pGAS[246]
	{0x0F12,0x00EF,0,0}, // #TVAR_ash_pGAS[247]
	{0x0F12,0x00C6,0,0}, // #TVAR_ash_pGAS[248]
	{0x0F12,0x008F,0,0}, // #TVAR_ash_pGAS[249]
	{0x0F12,0x006B,0,0}, // #TVAR_ash_pGAS[250]
	{0x0F12,0x004F,0,0}, // #TVAR_ash_pGAS[251]
	{0x0F12,0x0040,0,0}, // #TVAR_ash_pGAS[252]
	{0x0F12,0x003E,0,0}, // #TVAR_ash_pGAS[253]
	{0x0F12,0x0048,0,0}, // #TVAR_ash_pGAS[254]
	{0x0F12,0x005F,0,0}, // #TVAR_ash_pGAS[255]
	{0x0F12,0x0080,0,0}, // #TVAR_ash_pGAS[256]
	{0x0F12,0x00A9,0,0}, // #TVAR_ash_pGAS[257]
	{0x0F12,0x00E5,0,0}, // #TVAR_ash_pGAS[258]
	{0x0F12,0x010D,0,0}, // #TVAR_ash_pGAS[259]
	{0x0F12,0x0109,0,0}, // #TVAR_ash_pGAS[260]
	{0x0F12,0x00F2,0,0}, // #TVAR_ash_pGAS[261]
	{0x0F12,0x00B9,0,0}, // #TVAR_ash_pGAS[262]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[263]
	{0x0F12,0x007F,0,0}, // #TVAR_ash_pGAS[264]
	{0x0F12,0x0071,0,0}, // #TVAR_ash_pGAS[265]
	{0x0F12,0x0071,0,0}, // #TVAR_ash_pGAS[266]
	{0x0F12,0x007B,0,0}, // #TVAR_ash_pGAS[267]
	{0x0F12,0x0090,0,0}, // #TVAR_ash_pGAS[268]
	{0x0F12,0x00AF,0,0}, // #TVAR_ash_pGAS[269]
	{0x0F12,0x00D8,0,0}, // #TVAR_ash_pGAS[270]
	{0x0F12,0x0114,0,0}, // #TVAR_ash_pGAS[271]
	{0x0F12,0x0125,0,0}, // #TVAR_ash_pGAS[272]
	{0x0F12,0x0113,0,0}, // #TVAR_ash_pGAS[273]
	{0x0F12,0x011A,0,0}, // #TVAR_ash_pGAS[274]
	{0x0F12,0x00EA,0,0}, // #TVAR_ash_pGAS[275]
	{0x0F12,0x00C3,0,0}, // #TVAR_ash_pGAS[276]
	{0x0F12,0x00AF,0,0}, // #TVAR_ash_pGAS[277]
	{0x0F12,0x00A4,0,0}, // #TVAR_ash_pGAS[278]
	{0x0F12,0x00A6,0,0}, // #TVAR_ash_pGAS[279]
	{0x0F12,0x00B1,0,0}, // #TVAR_ash_pGAS[280]
	{0x0F12,0x00C5,0,0}, // #TVAR_ash_pGAS[281]
	{0x0F12,0x00E4,0,0}, // #TVAR_ash_pGAS[282]
	{0x0F12,0x010E,0,0}, // #TVAR_ash_pGAS[283]
	{0x0F12,0x013C,0,0}, // #TVAR_ash_pGAS[284]
	{0x0F12,0x0134,0,0}, // #TVAR_ash_pGAS[285]
	{0x0F12,0x0112,0,0}, // #TVAR_ash_pGAS[286]
	{0x0F12,0x0111,0,0}, // #TVAR_ash_pGAS[287]
	{0x0F12,0x00E8,0,0}, // #TVAR_ash_pGAS[288]
	{0x0F12,0x00BC,0,0}, // #TVAR_ash_pGAS[289]
	{0x0F12,0x00A6,0,0}, // #TVAR_ash_pGAS[290]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[291]
	{0x0F12,0x0099,0,0}, // #TVAR_ash_pGAS[292]
	{0x0F12,0x00A1,0,0}, // #TVAR_ash_pGAS[293]
	{0x0F12,0x00B2,0,0}, // #TVAR_ash_pGAS[294]
	{0x0F12,0x00CD,0,0}, // #TVAR_ash_pGAS[295]
	{0x0F12,0x00F7,0,0}, // #TVAR_ash_pGAS[296]
	{0x0F12,0x011B,0,0}, // #TVAR_ash_pGAS[297]
	{0x0F12,0x0118,0,0}, // #TVAR_ash_pGAS[298]
	{0x0F12,0x010C,0,0}, // #TVAR_ash_pGAS[299]
	{0x0F12,0x00EF,0,0}, // #TVAR_ash_pGAS[300]
	{0x0F12,0x00BE,0,0}, // #TVAR_ash_pGAS[301]
	{0x0F12,0x0097,0,0}, // #TVAR_ash_pGAS[302]
	{0x0F12,0x007F,0,0}, // #TVAR_ash_pGAS[303]
	{0x0F12,0x0070,0,0}, // #TVAR_ash_pGAS[304]
	{0x0F12,0x006E,0,0}, // #TVAR_ash_pGAS[305]
	{0x0F12,0x0076,0,0}, // #TVAR_ash_pGAS[306]
	{0x0F12,0x008A,0,0}, // #TVAR_ash_pGAS[307]
	{0x0F12,0x00A3,0,0}, // #TVAR_ash_pGAS[308]
	{0x0F12,0x00CE,0,0}, // #TVAR_ash_pGAS[309]
	{0x0F12,0x0106,0,0}, // #TVAR_ash_pGAS[310]
	{0x0F12,0x0118,0,0}, // #TVAR_ash_pGAS[311]
	{0x0F12,0x00F5,0,0}, // #TVAR_ash_pGAS[312]
	{0x0F12,0x00C7,0,0}, // #TVAR_ash_pGAS[313]
	{0x0F12,0x0096,0,0}, // #TVAR_ash_pGAS[314]
	{0x0F12,0x0072,0,0}, // #TVAR_ash_pGAS[315]
	{0x0F12,0x0057,0,0}, // #TVAR_ash_pGAS[316]
	{0x0F12,0x0045,0,0}, // #TVAR_ash_pGAS[317]
	{0x0F12,0x0042,0,0}, // #TVAR_ash_pGAS[318]
	{0x0F12,0x004A,0,0}, // #TVAR_ash_pGAS[319]
	{0x0F12,0x0062,0,0}, // #TVAR_ash_pGAS[320]
	{0x0F12,0x007F,0,0}, // #TVAR_ash_pGAS[321]
	{0x0F12,0x00A9,0,0}, // #TVAR_ash_pGAS[322]
	{0x0F12,0x00E1,0,0}, // #TVAR_ash_pGAS[323]
	{0x0F12,0x0110,0,0}, // #TVAR_ash_pGAS[324]
	{0x0F12,0x00D7,0,0}, // #TVAR_ash_pGAS[325]
	{0x0F12,0x00AA,0,0}, // #TVAR_ash_pGAS[326]
	{0x0F12,0x0079,0,0}, // #TVAR_ash_pGAS[327]
	{0x0F12,0x0052,0,0}, // #TVAR_ash_pGAS[328]
	{0x0F12,0x0034,0,0}, // #TVAR_ash_pGAS[329]
	{0x0F12,0x0020,0,0}, // #TVAR_ash_pGAS[330]
	{0x0F12,0x0019,0,0}, // #TVAR_ash_pGAS[331]
	{0x0F12,0x0024,0,0}, // #TVAR_ash_pGAS[332]
	{0x0F12,0x003F,0,0}, // #TVAR_ash_pGAS[333]
	{0x0F12,0x0062,0,0}, // #TVAR_ash_pGAS[334]
	{0x0F12,0x008F,0,0}, // #TVAR_ash_pGAS[335]
	{0x0F12,0x00C7,0,0}, // #TVAR_ash_pGAS[336]
	{0x0F12,0x00FA,0,0}, // #TVAR_ash_pGAS[337]
	{0x0F12,0x00C7,0,0}, // #TVAR_ash_pGAS[338]
	{0x0F12,0x009A,0,0}, // #TVAR_ash_pGAS[339]
	{0x0F12,0x0067,0,0}, // #TVAR_ash_pGAS[340]
	{0x0F12,0x003F,0,0}, // #TVAR_ash_pGAS[341]
	{0x0F12,0x001F,0,0}, // #TVAR_ash_pGAS[342]
	{0x0F12,0x000C,0,0}, // #TVAR_ash_pGAS[343]
	{0x0F12,0x0009,0,0}, // #TVAR_ash_pGAS[344]
	{0x0F12,0x0013,0,0}, // #TVAR_ash_pGAS[345]
	{0x0F12,0x002A,0,0}, // #TVAR_ash_pGAS[346]
	{0x0F12,0x004F,0,0}, // #TVAR_ash_pGAS[347]
	{0x0F12,0x007C,0,0}, // #TVAR_ash_pGAS[348]
	{0x0F12,0x00B2,0,0}, // #TVAR_ash_pGAS[349]
	{0x0F12,0x00E8,0,0}, // #TVAR_ash_pGAS[350]
	{0x0F12,0x00C3,0,0}, // #TVAR_ash_pGAS[351]
	{0x0F12,0x0095,0,0}, // #TVAR_ash_pGAS[352]
	{0x0F12,0x0061,0,0}, // #TVAR_ash_pGAS[353]
	{0x0F12,0x0039,0,0}, // #TVAR_ash_pGAS[354]
	{0x0F12,0x0018,0,0}, // #TVAR_ash_pGAS[355]
	{0x0F12,0x0004,0,0}, // #TVAR_ash_pGAS[356]
	{0x0F12,0x0000,0,0}, // #TVAR_ash_pGAS[357]
	{0x0F12,0x000B,0,0}, // #TVAR_ash_pGAS[358]
	{0x0F12,0x0020,0,0}, // #TVAR_ash_pGAS[359]
	{0x0F12,0x0047,0,0}, // #TVAR_ash_pGAS[360]
	{0x0F12,0x0074,0,0}, // #TVAR_ash_pGAS[361]
	{0x0F12,0x00A9,0,0}, // #TVAR_ash_pGAS[362]
	{0x0F12,0x00DF,0,0}, // #TVAR_ash_pGAS[363]
	{0x0F12,0x00C9,0,0}, // #TVAR_ash_pGAS[364]
	{0x0F12,0x009D,0,0}, // #TVAR_ash_pGAS[365]
	{0x0F12,0x0069,0,0}, // #TVAR_ash_pGAS[366]
	{0x0F12,0x0041,0,0}, // #TVAR_ash_pGAS[367]
	{0x0F12,0x001F,0,0}, // #TVAR_ash_pGAS[368]
	{0x0F12,0x000B,0,0}, // #TVAR_ash_pGAS[369]
	{0x0F12,0x0005,0,0}, // #TVAR_ash_pGAS[370]
	{0x0F12,0x000E,0,0}, // #TVAR_ash_pGAS[371]
	{0x0F12,0x0026,0,0}, // #TVAR_ash_pGAS[372]
	{0x0F12,0x004B,0,0}, // #TVAR_ash_pGAS[373]
	{0x0F12,0x0076,0,0}, // #TVAR_ash_pGAS[374]
	{0x0F12,0x00AE,0,0}, // #TVAR_ash_pGAS[375]
	{0x0F12,0x00E2,0,0}, // #TVAR_ash_pGAS[376]
	{0x0F12,0x00DD,0,0}, // #TVAR_ash_pGAS[377]
	{0x0F12,0x00B2,0,0}, // #TVAR_ash_pGAS[378]
	{0x0F12,0x007D,0,0}, // #TVAR_ash_pGAS[379]
	{0x0F12,0x0056,0,0}, // #TVAR_ash_pGAS[380]
	{0x0F12,0x0035,0,0}, // #TVAR_ash_pGAS[381]
	{0x0F12,0x0020,0,0}, // #TVAR_ash_pGAS[382]
	{0x0F12,0x001A,0,0}, // #TVAR_ash_pGAS[383]
	{0x0F12,0x0023,0,0}, // #TVAR_ash_pGAS[384]
	{0x0F12,0x0039,0,0}, // #TVAR_ash_pGAS[385]
	{0x0F12,0x005A,0,0}, // #TVAR_ash_pGAS[386]
	{0x0F12,0x0084,0,0}, // #TVAR_ash_pGAS[387]
	{0x0F12,0x00B9,0,0}, // #TVAR_ash_pGAS[388]
	{0x0F12,0x00ED,0,0}, // #TVAR_ash_pGAS[389]
	{0x0F12,0x00FB,0,0}, // #TVAR_ash_pGAS[390]
	{0x0F12,0x00D3,0,0}, // #TVAR_ash_pGAS[391]
	{0x0F12,0x009C,0,0}, // #TVAR_ash_pGAS[392]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[393]
	{0x0F12,0x005B,0,0}, // #TVAR_ash_pGAS[394]
	{0x0F12,0x0046,0,0}, // #TVAR_ash_pGAS[395]
	{0x0F12,0x003F,0,0}, // #TVAR_ash_pGAS[396]
	{0x0F12,0x0046,0,0}, // #TVAR_ash_pGAS[397]
	{0x0F12,0x005A,0,0}, // #TVAR_ash_pGAS[398]
	{0x0F12,0x0077,0,0}, // #TVAR_ash_pGAS[399]
	{0x0F12,0x009E,0,0}, // #TVAR_ash_pGAS[400]
	{0x0F12,0x00D6,0,0}, // #TVAR_ash_pGAS[401]
	{0x0F12,0x0103,0,0}, // #TVAR_ash_pGAS[402]
	{0x0F12,0x0119,0,0}, // #TVAR_ash_pGAS[403]
	{0x0F12,0x0102,0,0}, // #TVAR_ash_pGAS[404]
	{0x0F12,0x00CA,0,0}, // #TVAR_ash_pGAS[405]
	{0x0F12,0x00A4,0,0}, // #TVAR_ash_pGAS[406]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[407]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[408]
	{0x0F12,0x0072,0,0}, // #TVAR_ash_pGAS[409]
	{0x0F12,0x0076,0,0}, // #TVAR_ash_pGAS[410]
	{0x0F12,0x0088,0,0}, // #TVAR_ash_pGAS[411]
	{0x0F12,0x00A2,0,0}, // #TVAR_ash_pGAS[412]
	{0x0F12,0x00C9,0,0}, // #TVAR_ash_pGAS[413]
	{0x0F12,0x0100,0,0}, // #TVAR_ash_pGAS[414]
	{0x0F12,0x0117,0,0}, // #TVAR_ash_pGAS[415]
	{0x0F12,0x0127,0,0}, // #TVAR_ash_pGAS[416]
	{0x0F12,0x012D,0,0}, // #TVAR_ash_pGAS[417]
	{0x0F12,0x00FE,0,0}, // #TVAR_ash_pGAS[418]
	{0x0F12,0x00D8,0,0}, // #TVAR_ash_pGAS[419]
	{0x0F12,0x00BE,0,0}, // #TVAR_ash_pGAS[420]
	{0x0F12,0x00B0,0,0}, // #TVAR_ash_pGAS[421]
	{0x0F12,0x00A8,0,0}, // #TVAR_ash_pGAS[422]
	{0x0F12,0x00AF,0,0}, // #TVAR_ash_pGAS[423]
	{0x0F12,0x00BB,0,0}, // #TVAR_ash_pGAS[424]
	{0x0F12,0x00D5,0,0}, // #TVAR_ash_pGAS[425]
	{0x0F12,0x00FC,0,0}, // #TVAR_ash_pGAS[426]
	{0x0F12,0x0126,0,0}, // #TVAR_ash_pGAS[427]
	{0x0F12,0x0125,0,0}, // #TVAR_ash_pGAS[428]
	{0x0F12,0x00DF,0,0}, // #TVAR_ash_pGAS[429]
	{0x0F12,0x00EC,0,0}, // #TVAR_ash_pGAS[430]
	{0x0F12,0x00C9,0,0}, // #TVAR_ash_pGAS[431]
	{0x0F12,0x00A5,0,0}, // #TVAR_ash_pGAS[432]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[433]
	{0x0F12,0x008B,0,0}, // #TVAR_ash_pGAS[434]
	{0x0F12,0x008C,0,0}, // #TVAR_ash_pGAS[435]
	{0x0F12,0x0093,0,0}, // #TVAR_ash_pGAS[436]
	{0x0F12,0x00A1,0,0}, // #TVAR_ash_pGAS[437]
	{0x0F12,0x00BA,0,0}, // #TVAR_ash_pGAS[438]
	{0x0F12,0x00DF,0,0}, // #TVAR_ash_pGAS[439]
	{0x0F12,0x0101,0,0}, // #TVAR_ash_pGAS[440]
	{0x0F12,0x00F6,0,0}, // #TVAR_ash_pGAS[441]
	{0x0F12,0x00DA,0,0}, // #TVAR_ash_pGAS[442]
	{0x0F12,0x00CE,0,0}, // #TVAR_ash_pGAS[443]
	{0x0F12,0x00A4,0,0}, // #TVAR_ash_pGAS[444]
	{0x0F12,0x0083,0,0}, // #TVAR_ash_pGAS[445]
	{0x0F12,0x0072,0,0}, // #TVAR_ash_pGAS[446]
	{0x0F12,0x0068,0,0}, // #TVAR_ash_pGAS[447]
	{0x0F12,0x0067,0,0}, // #TVAR_ash_pGAS[448]
	{0x0F12,0x006F,0,0}, // #TVAR_ash_pGAS[449]
	{0x0F12,0x007E,0,0}, // #TVAR_ash_pGAS[450]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[451]
	{0x0F12,0x00B8,0,0}, // #TVAR_ash_pGAS[452]
	{0x0F12,0x00EB,0,0}, // #TVAR_ash_pGAS[453]
	{0x0F12,0x00F0,0,0}, // #TVAR_ash_pGAS[454]
	{0x0F12,0x00C4,0,0}, // #TVAR_ash_pGAS[455]
	{0x0F12,0x00A7,0,0}, // #TVAR_ash_pGAS[456]
	{0x0F12,0x007D,0,0}, // #TVAR_ash_pGAS[457]
	{0x0F12,0x005F,0,0}, // #TVAR_ash_pGAS[458]
	{0x0F12,0x004C,0,0}, // #TVAR_ash_pGAS[459]
	{0x0F12,0x0040,0,0}, // #TVAR_ash_pGAS[460]
	{0x0F12,0x003F,0,0}, // #TVAR_ash_pGAS[461]
	{0x0F12,0x0048,0,0}, // #TVAR_ash_pGAS[462]
	{0x0F12,0x005A,0,0}, // #TVAR_ash_pGAS[463]
	{0x0F12,0x0070,0,0}, // #TVAR_ash_pGAS[464]
	{0x0F12,0x0092,0,0}, // #TVAR_ash_pGAS[465]
	{0x0F12,0x00C6,0,0}, // #TVAR_ash_pGAS[466]
	{0x0F12,0x00E2,0,0}, // #TVAR_ash_pGAS[467]
	{0x0F12,0x00A5,0,0}, // #TVAR_ash_pGAS[468]
	{0x0F12,0x0086,0,0}, // #TVAR_ash_pGAS[469]
	{0x0F12,0x0060,0,0}, // #TVAR_ash_pGAS[470]
	{0x0F12,0x0041,0,0}, // #TVAR_ash_pGAS[471]
	{0x0F12,0x002A,0,0}, // #TVAR_ash_pGAS[472]
	{0x0F12,0x001E,0,0}, // #TVAR_ash_pGAS[473]
	{0x0F12,0x001A,0,0}, // #TVAR_ash_pGAS[474]
	{0x0F12,0x0024,0,0}, // #TVAR_ash_pGAS[475]
	{0x0F12,0x0038,0,0}, // #TVAR_ash_pGAS[476]
	{0x0F12,0x0054,0,0}, // #TVAR_ash_pGAS[477]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[478]
	{0x0F12,0x00A8,0,0}, // #TVAR_ash_pGAS[479]
	{0x0F12,0x00CB,0,0}, // #TVAR_ash_pGAS[480]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[481]
	{0x0F12,0x0078,0,0}, // #TVAR_ash_pGAS[482]
	{0x0F12,0x004F,0,0}, // #TVAR_ash_pGAS[483]
	{0x0F12,0x002E,0,0}, // #TVAR_ash_pGAS[484]
	{0x0F12,0x0016,0,0}, // #TVAR_ash_pGAS[485]
	{0x0F12,0x000A,0,0}, // #TVAR_ash_pGAS[486]
	{0x0F12,0x000A,0,0}, // #TVAR_ash_pGAS[487]
	{0x0F12,0x0012,0,0}, // #TVAR_ash_pGAS[488]
	{0x0F12,0x0024,0,0}, // #TVAR_ash_pGAS[489]
	{0x0F12,0x0042,0,0}, // #TVAR_ash_pGAS[490]
	{0x0F12,0x0066,0,0}, // #TVAR_ash_pGAS[491]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[492]
	{0x0F12,0x00B9,0,0}, // #TVAR_ash_pGAS[493]
	{0x0F12,0x008F,0,0}, // #TVAR_ash_pGAS[494]
	{0x0F12,0x0071,0,0}, // #TVAR_ash_pGAS[495]
	{0x0F12,0x0047,0,0}, // #TVAR_ash_pGAS[496]
	{0x0F12,0x0026,0,0}, // #TVAR_ash_pGAS[497]
	{0x0F12,0x000E,0,0}, // #TVAR_ash_pGAS[498]
	{0x0F12,0x0002,0,0}, // #TVAR_ash_pGAS[499]
	{0x0F12,0x0000,0,0}, // #TVAR_ash_pGAS[500]
	{0x0F12,0x0009,0,0}, // #TVAR_ash_pGAS[501]
	{0x0F12,0x0018,0,0}, // #TVAR_ash_pGAS[502]
	{0x0F12,0x0039,0,0}, // #TVAR_ash_pGAS[503]
	{0x0F12,0x005C,0,0}, // #TVAR_ash_pGAS[504]
	{0x0F12,0x008A,0,0}, // #TVAR_ash_pGAS[505]
	{0x0F12,0x00B0,0,0}, // #TVAR_ash_pGAS[506]
	{0x0F12,0x0091,0,0}, // #TVAR_ash_pGAS[507]
	{0x0F12,0x0075,0,0}, // #TVAR_ash_pGAS[508]
	{0x0F12,0x004B,0,0}, // #TVAR_ash_pGAS[509]
	{0x0F12,0x002C,0,0}, // #TVAR_ash_pGAS[510]
	{0x0F12,0x0013,0,0}, // #TVAR_ash_pGAS[511]
	{0x0F12,0x0006,0,0}, // #TVAR_ash_pGAS[512]
	{0x0F12,0x0002,0,0}, // #TVAR_ash_pGAS[513]
	{0x0F12,0x000A,0,0}, // #TVAR_ash_pGAS[514]
	{0x0F12,0x001D,0,0}, // #TVAR_ash_pGAS[515]
	{0x0F12,0x003A,0,0}, // #TVAR_ash_pGAS[516]
	{0x0F12,0x005C,0,0}, // #TVAR_ash_pGAS[517]
	{0x0F12,0x008A,0,0}, // #TVAR_ash_pGAS[518]
	{0x0F12,0x00B1,0,0}, // #TVAR_ash_pGAS[519]
	{0x0F12,0x00A5,0,0}, // #TVAR_ash_pGAS[520]
	{0x0F12,0x0087,0,0}, // #TVAR_ash_pGAS[521]
	{0x0F12,0x005D,0,0}, // #TVAR_ash_pGAS[522]
	{0x0F12,0x003E,0,0}, // #TVAR_ash_pGAS[523]
	{0x0F12,0x0027,0,0}, // #TVAR_ash_pGAS[524]
	{0x0F12,0x0019,0,0}, // #TVAR_ash_pGAS[525]
	{0x0F12,0x0016,0,0}, // #TVAR_ash_pGAS[526]
	{0x0F12,0x001D,0,0}, // #TVAR_ash_pGAS[527]
	{0x0F12,0x002E,0,0}, // #TVAR_ash_pGAS[528]
	{0x0F12,0x0047,0,0}, // #TVAR_ash_pGAS[529]
	{0x0F12,0x0068,0,0}, // #TVAR_ash_pGAS[530]
	{0x0F12,0x0094,0,0}, // #TVAR_ash_pGAS[531]
	{0x0F12,0x00BC,0,0}, // #TVAR_ash_pGAS[532]
	{0x0F12,0x00BE,0,0}, // #TVAR_ash_pGAS[533]
	{0x0F12,0x00A6,0,0}, // #TVAR_ash_pGAS[534]
	{0x0F12,0x0079,0,0}, // #TVAR_ash_pGAS[535]
	{0x0F12,0x005C,0,0}, // #TVAR_ash_pGAS[536]
	{0x0F12,0x0047,0,0}, // #TVAR_ash_pGAS[537]
	{0x0F12,0x003A,0,0}, // #TVAR_ash_pGAS[538]
	{0x0F12,0x0036,0,0}, // #TVAR_ash_pGAS[539]
	{0x0F12,0x003B,0,0}, // #TVAR_ash_pGAS[540]
	{0x0F12,0x004B,0,0}, // #TVAR_ash_pGAS[541]
	{0x0F12,0x0060,0,0}, // #TVAR_ash_pGAS[542]
	{0x0F12,0x0080,0,0}, // #TVAR_ash_pGAS[543]
	{0x0F12,0x00AD,0,0}, // #TVAR_ash_pGAS[544]
	{0x0F12,0x00CF,0,0}, // #TVAR_ash_pGAS[545]
	{0x0F12,0x00DC,0,0}, // #TVAR_ash_pGAS[546]
	{0x0F12,0x00D2,0,0}, // #TVAR_ash_pGAS[547]
	{0x0F12,0x00A3,0,0}, // #TVAR_ash_pGAS[548]
	{0x0F12,0x0084,0,0}, // #TVAR_ash_pGAS[549]
	{0x0F12,0x0073,0,0}, // #TVAR_ash_pGAS[550]
	{0x0F12,0x0067,0,0}, // #TVAR_ash_pGAS[551]
	{0x0F12,0x0062,0,0}, // #TVAR_ash_pGAS[552]
	{0x0F12,0x0066,0,0}, // #TVAR_ash_pGAS[553]
	{0x0F12,0x0073,0,0}, // #TVAR_ash_pGAS[554]
	{0x0F12,0x0087,0,0}, // #TVAR_ash_pGAS[555]
	{0x0F12,0x00A6,0,0}, // #TVAR_ash_pGAS[556]
	{0x0F12,0x00D7,0,0}, // #TVAR_ash_pGAS[557]
	{0x0F12,0x00E7,0,0}, // #TVAR_ash_pGAS[558]
	{0x0F12,0x00ED,0,0}, // #TVAR_ash_pGAS[559]
	{0x0F12,0x00F8,0,0}, // #TVAR_ash_pGAS[560]
	{0x0F12,0x00D1,0,0}, // #TVAR_ash_pGAS[561]
	{0x0F12,0x00B1,0,0}, // #TVAR_ash_pGAS[562]
	{0x0F12,0x009F,0,0}, // #TVAR_ash_pGAS[563]
	{0x0F12,0x0095,0,0}, // #TVAR_ash_pGAS[564]
	{0x0F12,0x0091,0,0}, // #TVAR_ash_pGAS[565]
	{0x0F12,0x0093,0,0}, // #TVAR_ash_pGAS[566]
	{0x0F12,0x009E,0,0}, // #TVAR_ash_pGAS[567]
	{0x0F12,0x00B0,0,0}, // #TVAR_ash_pGAS[568]
	{0x0F12,0x00D1,0,0}, // #TVAR_ash_pGAS[569]
	{0x0F12,0x00F7,0,0}, // #TVAR_ash_pGAS[570]
	{0x0F12,0x00F5,0,0}, // #TVAR_ash_pGAS[571]
	{0x002A,0x074E,0,0}, 
	{0x0F12,0x0001,0,0}, //ash_bLumaMode
	{0x002A,0x0D30,0,0},
	{0x0F12,0x02A7,0,0}, // #awbb_GLocusR
	{0x0F12,0x0343,0,0}, // #awbb_GLocusB
	//Rgainproj
	{0x002A,0x06B8,0,0}, //06B8
	{0x0F12,0x00DF,0,0}, //00E0 // #TVAR_ash_AwbAshCord_0_ Horizon
	{0x0F12,0x00F0,0,0}, //00F4 // #TVAR_ash_AwbAshCord_1_ Incan A
	{0x0F12,0x0106,0,0}, //0114 // #TVAR_ash_AwbAshCord_2_ TL84
	{0x0F12,0x0139,0,0}, //013D // #TVAR_ash_AwbAshCord_3_ CWF
	{0x0F12,0x0171,0,0}, //0171 // #TVAR_ash_AwbAshCord_4_ D50
	{0x0F12,0x0189,0,0}, //0184 // #TVAR_ash_AwbAshCord_5_ D65
	{0x0F12,0x01A8,0,0}, //01A8 // #TVAR_ash_AwbAshCord_6_ / /*BU5D05950 zhangsheng 20100422 end>*/
	//================================================================================================
	// SET CCM
	//================================================================================================ 
	// CCM start address // 7000_33A4
	{0x002A,0x0698,0,0},
	{0x0F12,0x33A4,0,0},
	{0x0F12,0x7000,0,0},
	{0x002A,0x33A4,0,0},
	{0x0F12,0x0207,0,0},//#TVAR_wbt_pBaseCcms// Horizon
	{0x0F12,0xFF65,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFFD5,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFF5A,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x02CA,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFEF4,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFF4D,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFFA2,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x044D,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x01E8,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x0155,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFDE0,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x0159,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFFD9,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x02AB,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0xFE53,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x02E4,0,0},//#TVAR_wbt_pBaseCcms
	{0x0F12,0x00FE,0,0},//#TVAR_wbt_pBaseCcms//*<BU5D05950 zhangsheng 20100422 begin*/

	{0x0F12,0x01EA,0,0},//#TVAR_wbt_pBaseCcms[18]// Inca
	{0x0F12,0xFF67,0,0},//#TVAR_wbt_pBaseCcms[19]
	{0x0F12,0xFFF1,0,0},//#TVAR_wbt_pBaseCcms[20]
	{0x0F12,0xFF84,0,0},//#TVAR_wbt_pBaseCcms[21]
	{0x0F12,0x0297,0,0},//#TVAR_wbt_pBaseCcms[22]
	{0x0F12,0xFEFD,0,0},//#TVAR_wbt_pBaseCcms[23]
	{0x0F12,0xFFE5,0,0},//#TVAR_wbt_pBaseCcms[24]
	{0x0F12,0xFFF6,0,0},//#TVAR_wbt_pBaseCcms[25]
	{0x0F12,0x0360,0,0},//#TVAR_wbt_pBaseCcms[26]
	{0x0F12,0x01FB,0,0},//#TVAR_wbt_pBaseCcms[27]
	{0x0F12,0x012A,0,0},//#TVAR_wbt_pBaseCcms[28]
	{0x0F12,0xFDF7,0,0},//#TVAR_wbt_pBaseCcms[29]
	{0x0F12,0x015F,0,0},//#TVAR_wbt_pBaseCcms[30]//*BU5D05950 zhangsheng 20100422 end>*/
	{0x0F12,0xFFBE,0,0},//#TVAR_wbt_pBaseCcms[31]
	{0x0F12,0x02C2,0,0},//#TVAR_wbt_pBaseCcms[32]
	{0x0F12,0xFEC2,0,0},//#TVAR_wbt_pBaseCcms[33]
	{0x0F12,0x0299,0,0},//#TVAR_wbt_pBaseCcms[34]
	{0x0F12,0x00D9,0,0},//#TVAR_wbt_pBaseCcms[35]

	{0x0F12,0x020B,0,0},//#TVAR_wbt_pBaseCcms[36]// WW
	{0x0F12,0xFF5C,0,0},//#TVAR_wbt_pBaseCcms[37]
	{0x0F12,0xFFDC,0,0},//#TVAR_wbt_pBaseCcms[38]
	{0x0F12,0xFF92,0,0},//#TVAR_wbt_pBaseCcms[39]
	{0x0F12,0x0223,0,0},//#TVAR_wbt_pBaseCcms[40]
	{0x0F12,0xFF08,0,0},//#TVAR_wbt_pBaseCcms[41]
	{0x0F12,0xFFB2,0,0},//#TVAR_wbt_pBaseCcms[42]
	{0x0F12,0xFFF5,0,0},//#TVAR_wbt_pBaseCcms[43]
	{0x0F12,0x02C4,0,0},//#TVAR_wbt_pBaseCcms[44]
	{0x0F12,0x01DF,0,0},//#TVAR_wbt_pBaseCcms[45]
	{0x0F12,0x00EE,0,0},//#TVAR_wbt_pBaseCcms[46]
	{0x0F12,0xFE8B,0,0},//#TVAR_wbt_pBaseCcms[47]
	{0x0F12,0x01BE,0,0},//#TVAR_wbt_pBaseCcms[48]
	{0x0F12,0xFF8B,0,0},//#TVAR_wbt_pBaseCcms[49]
	{0x0F12,0x020B,0,0},//#TVAR_wbt_pBaseCcms[50]
	{0x0F12,0xFEEF,0,0},//#TVAR_wbt_pBaseCcms[51]
	{0x0F12,0x0221,0,0},//#TVAR_wbt_pBaseCcms[52]
	{0x0F12,0x0100,0,0},//#TVAR_wbt_pBaseCcms[53]//*<BU5D05950 zhangsheng 20100422 begin*/

	{0x0F12,0x01F2,0,0}, //#TVAR_wbt_pBaseCcms[54]// CWF
	{0x0F12,0xFF47,0,0}, //#TVAR_wbt_pBaseCcms[55]
	{0x0F12,0x0002,0,0}, //#TVAR_wbt_pBaseCcms[56]
	{0x0F12,0xFEF0,0,0}, //#TVAR_wbt_pBaseCcms[57]
	{0x0F12,0x0201,0,0}, //#TVAR_wbt_pBaseCcms[58]
	{0x0F12,0xFF68,0,0}, //#TVAR_wbt_pBaseCcms[59]
	{0x0F12,0xFF95,0,0}, //#TVAR_wbt_pBaseCcms[60]
	{0x0F12,0xFFB7,0,0}, //#TVAR_wbt_pBaseCcms[61]
	{0x0F12,0x0246,0,0}, //#TVAR_wbt_pBaseCcms[62]
	{0x0F12,0x01FD,0,0}, //#TVAR_wbt_pBaseCcms[63]
	{0x0F12,0x00A9,0,0}, //#TVAR_wbt_pBaseCcms[64]
	{0x0F12,0xFEE4,0,0}, //#TVAR_wbt_pBaseCcms[65]
	{0x0F12,0x016E,0,0}, //#TVAR_wbt_pBaseCcms[66]
	{0x0F12,0xFF77,0,0}, //#TVAR_wbt_pBaseCcms[67]
	{0x0F12,0x01DC,0,0}, //#TVAR_wbt_pBaseCcms[68]
	{0x0F12,0xFEE2,0,0}, //#TVAR_wbt_pBaseCcms[69]
	{0x0F12,0x01A2,0,0}, //#TVAR_wbt_pBaseCcms[70]
	{0x0F12,0x015E,0,0}, //#TVAR_wbt_pBaseCcms[71]//*BU5D05950 zhangsheng 20100422 end>*/

	{0x0F12,0x01C1,0,0},//#TVAR_wbt_pBaseCcms[72]// D50
	{0x0F12,0xFFB2,0,0},//#TVAR_wbt_pBaseCcms[73]
	{0x0F12,0xFFCB,0,0},//#TVAR_wbt_pBaseCcms[74]
	{0x0F12,0xFF41,0,0},//#TVAR_wbt_pBaseCcms[75]
	{0x0F12,0x01A4,0,0},//#TVAR_wbt_pBaseCcms[76]
	{0x0F12,0xFF7B,0,0},//#TVAR_wbt_pBaseCcms[77]
	{0x0F12,0xFFE0,0,0},//#TVAR_wbt_pBaseCcms[78]
	{0x0F12,0xFFCE,0,0},//#TVAR_wbt_pBaseCcms[79]
	{0x0F12,0x01E6,0,0},//#TVAR_wbt_pBaseCcms[80]
	{0x0F12,0x0114,0,0},//#TVAR_wbt_pBaseCcms[81]
	{0x0F12,0x0124,0,0},//#TVAR_wbt_pBaseCcms[82]
	{0x0F12,0xFF56,0,0},//#TVAR_wbt_pBaseCcms[83]
	{0x0F12,0x01AA,0,0},//#TVAR_wbt_pBaseCcms[84]
	{0x0F12,0xFF68,0,0},//#TVAR_wbt_pBaseCcms[85]
	{0x0F12,0x01B6,0,0},//#TVAR_wbt_pBaseCcms[86]
	{0x0F12,0xFF4F,0,0},//#TVAR_wbt_pBaseCcms[87]
	{0x0F12,0x0178,0,0},//#TVAR_wbt_pBaseCcms[88]
	{0x0F12,0x0121,0,0},//#TVAR_wbt_pBaseCcms[89]

	{0x0F12,0x01D5,0,0},//#TVAR_wbt_pBaseCcms[90]// D65
	{0x0F12,0xFF7B,0,0},//#TVAR_wbt_pBaseCcms[91]
	{0x0F12,0xFFEE,0,0},//#TVAR_wbt_pBaseCcms[92]
	{0x0F12,0xFF0A,0,0},//#TVAR_wbt_pBaseCcms[93]
	{0x0F12,0x01E0,0,0},//#TVAR_wbt_pBaseCcms[94]
	{0x0F12,0xFF76,0,0},//#TVAR_wbt_pBaseCcms[95]
	{0x0F12,0xFFD3,0,0},//#TVAR_wbt_pBaseCcms[96]
	{0x0F12,0xFFC0,0,0},//#TVAR_wbt_pBaseCcms[97]
	{0x0F12,0x0200,0,0},//#TVAR_wbt_pBaseCcms[98]
	{0x0F12,0x01A5,0,0},//#TVAR_wbt_pBaseCcms[99]
	{0x0F12,0x00AF,0,0},//#TVAR_wbt_pBaseCcms[100]
	{0x0F12,0xFF3B,0,0},//#TVAR_wbt_pBaseCcms[101]
	{0x0F12,0x01F5,0,0},//#TVAR_wbt_pBaseCcms[102]
	{0x0F12,0xFF50,0,0},//#TVAR_wbt_pBaseCcms[103]
	{0x0F12,0x0183,0,0},//#TVAR_wbt_pBaseCcms[104]
	{0x0F12,0xFEE7,0,0},//#TVAR_wbt_pBaseCcms[105]
	{0x0F12,0x01B9,0,0},//#TVAR_wbt_pBaseCcms[106]
	{0x0F12,0x0148,0,0},//#TVAR_wbt_pBaseCcms[107]

	{0x002A,0x06A0,0,0}, // Outdoor CCM address // 7000_3380  
	{0x0F12,0x3380,0,0},
	{0x0F12,0x7000,0,0},
	{0x002A,0x3380,0,0}, // Outdoor CCM
	{0x0F12,0x0218,0,0}, //#TVAR_wbt_pOutdoorCcm[0]
	{0x0F12,0xFF43,0,0}, //#TVAR_wbt_pOutdoorCcm[1]
	{0x0F12,0xFF92,0,0}, //#TVAR_wbt_pOutdoorCcm[2]
	{0x0F12,0xFF1D,0,0}, //#TVAR_wbt_pOutdoorCcm[3]
	{0x0F12,0x021E,0,0}, //#TVAR_wbt_pOutdoorCcm[4]
	{0x0F12,0xFF0F,0,0}, //#TVAR_wbt_pOutdoorCcm[5]
	{0x0F12,0xFFBE,0,0}, //#TVAR_wbt_pOutdoorCcm[6]
	{0x0F12,0x0006,0,0}, //#TVAR_wbt_pOutdoorCcm[7]
	{0x0F12,0x02AE,0,0}, //#TVAR_wbt_pOutdoorCcm[8]
	{0x0F12,0x0170,0,0}, //#TVAR_wbt_pOutdoorCcm[9]
	{0x0F12,0x010B,0,0}, //#TVAR_wbt_pOutdoorCcm[10] 
	{0x0F12,0xFED4,0,0}, //#TVAR_wbt_pOutdoorCcm[11] 
	{0x0F12,0x01B9,0,0}, //#TVAR_wbt_pOutdoorCcm[12] 
	{0x0F12,0xFF59,0,0}, //#TVAR_wbt_pOutdoorCcm[13] 
	{0x0F12,0x0241,0,0}, //#TVAR_wbt_pOutdoorCcm[14] 
	{0x0F12,0xFF01,0,0}, //#TVAR_wbt_pOutdoorCcm[15] 
	{0x0F12,0x01B8,0,0}, //#TVAR_wbt_pOutdoorCcm[16] 
	{0x0F12,0x0152,0,0}, //#TVAR_wbt_pOutdoorCcm[17] //White balance
	    // param_start awbb_IndoorGrZones_m_BGrid
	{0x002A,0x0C48,0,0},
	{0x0F12,0x03BD,0,0},//awbb_IndoorGrZones_m_BGrid[0]
	{0x0F12,0x03F2,0,0},//awbb_IndoorGrZones_m_BGrid[1]
	{0x0F12,0x036F,0,0},//awbb_IndoorGrZones_m_BGrid[2]
	{0x0F12,0x03F7,0,0},//awbb_IndoorGrZones_m_BGrid[3]
	{0x0F12,0x0335,0,0},//awbb_IndoorGrZones_m_BGrid[4]
	{0x0F12,0x03E0,0,0},//awbb_IndoorGrZones_m_BGrid[5]
	{0x0F12,0x0301,0,0},//awbb_IndoorGrZones_m_BGrid[6]
	{0x0F12,0x03B9,0,0},//awbb_IndoorGrZones_m_BGrid[7]
	{0x0F12,0x02D2,0,0},//awbb_IndoorGrZones_m_BGrid[8]
	{0x0F12,0x0392,0,0},//awbb_IndoorGrZones_m_BGrid[9]
	{0x0F12,0x02AE,0,0},//awbb_IndoorGrZones_m_BGrid[10]
	{0x0F12,0x0367,0,0},//awbb_IndoorGrZones_m_BGrid[11]
	{0x0F12,0x028F,0,0},//awbb_IndoorGrZones_m_BGrid[12]
	{0x0F12,0x033C,0,0},//awbb_IndoorGrZones_m_BGrid[13]
	{0x0F12,0x0275,0,0},//awbb_IndoorGrZones_m_BGrid[14]
	{0x0F12,0x0317,0,0},//awbb_IndoorGrZones_m_BGrid[15]
	{0x0F12,0x0259,0,0},//awbb_IndoorGrZones_m_BGrid[16]
	{0x0F12,0x02EF,0,0},//awbb_IndoorGrZones_m_BGrid[17]
	{0x0F12,0x0240,0,0},//awbb_IndoorGrZones_m_BGrid[18]
	{0x0F12,0x02D0,0,0},//awbb_IndoorGrZones_m_BGrid[19]
	{0x0F12,0x0229,0,0},//awbb_IndoorGrZones_m_BGrid[20]
	{0x0F12,0x02B1,0,0},//awbb_IndoorGrZones_m_BGrid[21]
	{0x0F12,0x0215,0,0},//awbb_IndoorGrZones_m_BGrid[22]
	{0x0F12,0x0294,0,0},//awbb_IndoorGrZones_m_BGrid[23]
	{0x0F12,0x0203,0,0},//awbb_IndoorGrZones_m_BGrid[24]
	{0x0F12,0x027F,0,0},//awbb_IndoorGrZones_m_BGrid[25]
	{0x0F12,0x01EF,0,0},//awbb_IndoorGrZones_m_BGrid[26]
	{0x0F12,0x0264,0,0},//awbb_IndoorGrZones_m_BGrid[27]
	{0x0F12,0x01E4,0,0},//awbb_IndoorGrZones_m_BGrid[28]
	{0x0F12,0x024C,0,0},//awbb_IndoorGrZones_m_BGrid[29]
	{0x0F12,0x01E5,0,0},//awbb_IndoorGrZones_m_BGrid[30]
	{0x0F12,0x0233,0,0},//awbb_IndoorGrZones_m_BGrid[31]
	{0x0F12,0x01EE,0,0},//awbb_IndoorGrZones_m_BGrid[32]
	{0x0F12,0x020F,0,0},//awbb_IndoorGrZones_m_BGrid[33]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[34]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[35]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[36]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[37]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[38]
	{0x0F12,0x0000,0,0},//awbb_IndoorGrZones_m_BGrid[39]
	{0x0F12,0x0005,0,0}, //awbb_IndoorGrZones_m_GridStep // param_end awbb_IndoorGrZones_m_BGrid
	{0x0F12,0x0000,0,0},
	{0x002A,0x0CA0,0,0},
	{0x0F12,0x0101,0,0},//011A //awbb_IndoorGrZones_m_Boffs
	{0x0F12,0x0000,0,0},
	{0x002A,0x0CE0,0,0}, // param_start awbb_LowBrGrZones_m_BGrid
	{0x0F12,0x0399,0,0}, //awbb_LowBrGrZones_m_BGrid[0]
	{0x0F12,0x0417,0,0}, //awbb_LowBrGrZones_m_BGrid[1]
	{0x0F12,0x0327,0,0}, //awbb_LowBrGrZones_m_BGrid[2]
	{0x0F12,0x0417,0,0}, //awbb_LowBrGrZones_m_BGrid[3]
	{0x0F12,0x02BD,0,0}, //awbb_LowBrGrZones_m_BGrid[4]
	{0x0F12,0x0409,0,0}, //awbb_LowBrGrZones_m_BGrid[5]
	{0x0F12,0x0271,0,0}, //awbb_LowBrGrZones_m_BGrid[6]
	{0x0F12,0x03BD,0,0}, //awbb_LowBrGrZones_m_BGrid[7]
	{0x0F12,0x0231,0,0}, //awbb_LowBrGrZones_m_BGrid[8]
	{0x0F12,0x036F,0,0}, //awbb_LowBrGrZones_m_BGrid[9]
	{0x0F12,0x0203,0,0}, //awbb_LowBrGrZones_m_BGrid[10]
	{0x0F12,0x0322,0,0}, //awbb_LowBrGrZones_m_BGrid[11]
	{0x0F12,0x01D0,0,0}, //awbb_LowBrGrZones_m_BGrid[12]
	{0x0F12,0x02DB,0,0}, //awbb_LowBrGrZones_m_BGrid[13]
	{0x0F12,0x01AD,0,0}, //awbb_LowBrGrZones_m_BGrid[14]
	{0x0F12,0x02A7,0,0}, //awbb_LowBrGrZones_m_BGrid[15]
	{0x0F12,0x01AA,0,0}, //awbb_LowBrGrZones_m_BGrid[16]
	{0x0F12,0x027D,0,0}, //awbb_LowBrGrZones_m_BGrid[17]
	{0x0F12,0x01B0,0,0}, //awbb_LowBrGrZones_m_BGrid[18]
	{0x0F12,0x0219,0,0}, //awbb_LowBrGrZones_m_BGrid[19]
	{0x0F12,0x0000,0,0}, //awbb_LowBrGrZones_m_BGrid[20]
	{0x0F12,0x0000,0,0}, //awbb_LowBrGrZones_m_BGrid[21]
	{0x0F12,0x0000,0,0}, //awbb_LowBrGrZones_m_BGrid[22]
	{0x0F12,0x0000,0,0}, //awbb_LowBrGrZones_m_BGrid[23]
	{0x0F12,0x0006,0,0}, //awbb_LowBrGrZones_m_GridStep // param_end awbb_LowBrGrZones_m_BGrid
	{0x0F12,0x0000,0,0},
	{0x002A,0x0D18,0,0},
	{0x0F12,0x00DF,0,0},//00FA //awbb_LowBrGrZones_m_Boffs
	{0x0F12,0x0000,0,0},
	{0x002A,0x0CA4,0,0}, // param_start awbb_OutdoorGrZones_m_BGrid
	{0x0F12,0x0291,0,0},//awbb_OutdoorGrZones_m_BGrid[0]
	{0x0F12,0x02C4,0,0},//awbb_OutdoorGrZones_m_BGrid[1]
	{0x0F12,0x0244,0,0},//awbb_OutdoorGrZones_m_BGrid[2]
	{0x0F12,0x02B6,0,0},//awbb_OutdoorGrZones_m_BGrid[3]
	{0x0F12,0x0215,0,0},//awbb_OutdoorGrZones_m_BGrid[4]
	{0x0F12,0x028F,0,0},//awbb_OutdoorGrZones_m_BGrid[5]
	{0x0F12,0x01F2,0,0},//awbb_OutdoorGrZones_m_BGrid[6]
	{0x0F12,0x025F,0,0},//awbb_OutdoorGrZones_m_BGrid[7]
	{0x0F12,0x01E4,0,0},//awbb_OutdoorGrZones_m_BGrid[8]
	{0x0F12,0x021E,0,0},//awbb_OutdoorGrZones_m_BGrid[9]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[10]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[11]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[12]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[13]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[14]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[15]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[16]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[17]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[18]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[19]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[20]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[21]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[22]
	{0x0F12,0x0000,0,0},//awbb_OutdoorGrZones_m_BGrid[23]
	{0x0F12,0x0006,0,0}, //awbb_OutdoorGrZones_m_GridStep // param_end awbb_OutdoorGrZones_m_BGrid
	{0x0F12,0x0000,0,0},
	{0x002A,0x0CDC,0,0},
	{0x0F12,0x01F2,0,0},//0212 //awbb_OutdoorGrZones_m_Boffs
	{0x0F12,0x0000,0,0},
	{0x002A,0x0D1C,0,0},
	{0x0F12,0x034D,0,0}, //awbb_CrclLowT_R_c
	{0x0F12,0x0000,0,0},
	{0x002A,0x0D20,0,0},
	{0x0F12,0x016C,0,0}, //awbb_CrclLowT_B_c
	{0x0F12,0x0000,0,0},
	{0x002A,0x0D24,0,0},
	{0x0F12,0x49D5,0,0}, //awbb_CrclLowT_Rad_c
	{0x0F12,0x0000,0,0},
	{0x002A,0x0D46,0,0},
	{0x0F12,0x0470,0,0}, //awbb_MvEq_RBthresh
	{0x002A,0x0D5C,0,0},
	{0x0F12,0x0534,0,0}, //awbb_LowTempRB
	{0x002A,0x0D2C,0,0},
	{0x0F12,0x0131,0,0}, //awbb_IntcR 
	{0x0F12,0x012F,0,0},  //012C //awbb_IntcB 

	{0x002A,0x0E36,0,0},    
	{0x0F12,0x0000,0,0},//0028  //R OFFSET 
	{0x0F12,0x0000,0,0},//FFD8  //B OFFSET 
	{0x0F12,0x0000,0,0},  //G OFFSET 

	{0x002A,0x0DD4,0,0},    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[0] //           
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[1] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[2] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[3] //    
	{0x0F12,0xFFF1,0,0},//awbb_GridCorr_R[4] //    
	{0x0F12,0x001E,0,0},//awbb_GridCorr_R[5] //     
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[6] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[7] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[8] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[9] //    
	{0x0F12,0xFFF1,0,0},//awbb_GridCorr_R[10] //   
	{0x0F12,0x001E,0,0},//awbb_GridCorr_R[11] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[12] //   
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[13] //   
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[14] //   
	{0x0F12,0x0000,0,0},//awbb_GridCorr_R[15] //   
	{0x0F12,0xFFF1,0,0},//awbb_GridCorr_R[16] //   
	{0x0F12,0x001E,0,0},//awbb_GridCorr_R[17] //   

	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[0] ////  
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[1] //    
	{0x0F12,0x006E,0,0},//awbb_GridCorr_B[2] //    
	{0x0F12,0x0046,0,0},//awbb_GridCorr_B[3] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[4] //    
	{0x0F12,0xFFC0,0,0},//awbb_GridCorr_B[5] // 
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[6] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[7] //    
	{0x0F12,0x006E,0,0},//awbb_GridCorr_B[8] //    
	{0x0F12,0x0046,0,0},//awbb_GridCorr_B[9] //    
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[10] //   
	{0x0F12,0xFFC0,0,0},//awbb_GridCorr_B[11] //
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[12] //   
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[13] //   
	{0x0F12,0x006E,0,0},//awbb_GridCorr_B[14] //   
	{0x0F12,0x0046,0,0},//awbb_GridCorr_B[15] //   
	{0x0F12,0x0000,0,0},//awbb_GridCorr_B[16] //   
	{0x0F12,0xFFC0,0,0},//awbb_GridCorr_B[17] //   

	{0x0F12,0x02F2,0,0},//awbb_GridConst_1[0] //             
	{0x0F12,0x0340,0,0},//awbb_GridConst_1[1] //             
	{0x0F12,0x0398,0,0},//awbb_GridConst_1[2] //             


	{0x0F12,0x0DF6,0,0},// awbb_GridConst_2[0] //         
	{0x0F12,0x0EAA,0,0},// awbb_GridConst_2[1] //         
	{0x0F12,0x0EB5,0,0},// awbb_GridConst_2[2] //         
	{0x0F12,0x0F33,0,0},// awbb_GridConst_2[3] //         
	{0x0F12,0x0F9D,0,0},// awbb_GridConst_2[4] //         
	{0x0F12,0x0FF2,0,0},// awbb_GridConst_2[5] //         

	{0x0F12,0x00AC,0,0}, //awbb_GridCoeff_R_1           
	{0x0F12,0x00BD,0,0}, //awbb_GridCoeff_B_1           
	{0x0F12,0x0049,0,0}, //awbb_GridCoeff_R_2           
	{0x0F12,0x00F5,0,0}, //awbb_GridCoeff_B_2           

	{0x002A,0x0E4A,0,0},           
	{0x0F12,0x0002,0,0}, //awbb_GridEnable//    
	//================================================================================================
	// SET GRID OFFSET
	//================================================================================================
	// Not used
	             //002A 0E4A
	             //0F12 0000 // #awbb_GridEnable

	//================================================================================================
	// SET GAMMA
	//================================================================================================
	   //Our //old //STW
	{0x002A,0x3288,0,0},
	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__0_ 0x70003288
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__1_ 0x7000328A
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__2_ 0x7000328C
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__3_ 0x7000328E
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__4_ 0x70003290
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__5_ 0x70003292
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__6_ 0x70003294
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__7_ 0x70003296
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__8_ 0x70003298
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__9_ 0x7000329A
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__10_ 0x7000329C
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__11_ 0x7000329E
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__12_ 0x700032A0
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__13_ 0x700032A2
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__14_ 0x700032A4
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__15_ 0x700032A6
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__16_ 0x700032A8
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__17_ 0x700032AA
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__18_ 0x700032AC
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBIndoor_0__19_ 0x700032AE
	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__0_ 0x700032B0
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__1_ 0x700032B2
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__2_ 0x700032B4
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__3_ 0x700032B6
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__4_ 0x700032B8
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__5_ 0x700032BA
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__6_ 0x700032BC
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__7_ 0x700032BE
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__8_ 0x700032C0
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__9_ 0x700032C2
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__10_ 0x700032C4
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__11_ 0x700032C6
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__12_ 0x700032C8
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__13_ 0x700032CA
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__14_ 0x700032CC
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__15_ 0x700032CE
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__16_ 0x700032D0
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__17_ 0x700032D2
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__18_ 0x700032D4
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBIndoor_1__19_ 0x700032D6
	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__0_ 0x700032D8
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__1_ 0x700032DA
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__2_ 0x700032DC
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__3_ 0x700032DE
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__4_ 0x700032E0
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__5_ 0x700032E2
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__6_ 0x700032E4
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__7_ 0x700032E6
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__8_ 0x700032E8
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__9_ 0x700032EA
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__10_ 0x700032EC
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__11_ 0x700032EE
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__12_ 0x700032F0
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__13_ 0x700032F2
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__14_ 0x700032F4
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__15_ 0x700032F6
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__16_ 0x700032F8
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__17_ 0x700032FA
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__18_ 0x700032FC
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBIndoor_2__19_ 0x700032FE

	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__0_ 0x70003300
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__1_ 0x70003302
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__2_ 0x70003304
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__3_ 0x70003306
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__4_ 0x70003308
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__5_ 0x7000330A
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__6_ 0x7000330C
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__7_ 0x7000330E
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__8_ 0x70003310
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__9_ 0x70003312
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__10_0x70003314
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__11_0x70003316
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__12_0x70003318
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__13_0x7000331A
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__14_0x7000331C
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__15_0x7000331E
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__16_0x70003320
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__17_0x70003322
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__18_0x70003324
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_0__19_0x70003326
	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__0_ 0x70003328
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__1_ 0x7000332A
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__2_ 0x7000332C
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__3_ 0x7000332E
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__4_ 0x70003330
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__5_ 0x70003332
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__6_ 0x70003334
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__7_ 0x70003336
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__8_ 0x70003338
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__9_ 0x7000333A
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__10_0x7000333C
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__11_0x7000333E
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__12_0x70003340
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__13_0x70003342
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__14_0x70003344
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__15_0x70003346
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__16_0x70003348
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__17_0x7000334A
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__18_0x7000334C
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_1__19_0x7000334E
	{0x0F12,0x0000,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__0_ 0x70003350
	{0x0F12,0x0004,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__1_ 0x70003352
	{0x0F12,0x0010,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__2_ 0x70003354
	{0x0F12,0x002A,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__3_ 0x70003356
	{0x0F12,0x0062,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__4_ 0x70003358
	{0x0F12,0x00D5,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__5_ 0x7000335A
	{0x0F12,0x0138,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__6_ 0x7000335C
	{0x0F12,0x0161,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__7_ 0x7000335E
	{0x0F12,0x0186,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__8_ 0x70003360
	{0x0F12,0x01BC,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__9_ 0x70003362
	{0x0F12,0x01E8,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__10_0x70003364
	{0x0F12,0x020F,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__11_0x70003366
	{0x0F12,0x0232,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__12_0x70003368
	{0x0F12,0x0273,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__13_0x7000336A
	{0x0F12,0x02AF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__14_0x7000336C
	{0x0F12,0x0309,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__15_0x7000336E
	{0x0F12,0x0355,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__16_0x70003370
	{0x0F12,0x0394,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__17_0x70003372
	{0x0F12,0x03CE,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__18_0x70003374
	{0x0F12,0x03FF,0,0}, //#SARR_usDualGammaLutRGBOutdoor_2__19_0x70003376

	{0x002A,0x06A6,0,0},
	{0x0F12,0x00D8,0,0}, // #SARR_AwbCcmCord_0_ H
	{0x0F12,0x00FC,0,0}, // #SARR_AwbCcmCord_1_ A
	{0x0F12,0x0120,0,0}, // #SARR_AwbCcmCord_2_ TL84/U30
	{0x0F12,0x014C,0,0}, // #SARR_AwbCcmCord_3_ CWF
	{0x0F12,0x0184,0,0}, // #SARR_AwbCcmCord_4_ D50
	{0x0F12,0x01AD,0,0}, // #SARR_AwbCcmCord_5_ D65

	{0x002A,0x1034,0,0},  // Hong  1123           
	{0x0F12,0x00B5,0,0}, // #SARR_IllumType[0]   
	{0x0F12,0x00CF,0,0}, // #SARR_IllumType[1]   
	{0x0F12,0x0116,0,0}, // #SARR_IllumType[2]   
	{0x0F12,0x0140,0,0}, // #SARR_IllumType[3]   
	{0x0F12,0x0150,0,0}, // #SARR_IllumType[4]   
	{0x0F12,0x0174,0,0}, // #SARR_IllumType[5]   
	{0x0F12,0x018E,0,0}, // #SARR_IllumType[6]   
	     
	{0x0F12,0x00B8,0,0}, // #SARR_IllumTypeF[0]  
	{0x0F12,0x00BA,0,0}, // #SARR_IllumTypeF[1]  
	{0x0F12,0x00C0,0,0}, // #SARR_IllumTypeF[2]  
	{0x0F12,0x00F0,0,0}, // #SARR_IllumTypeF[3]  
	{0x0F12,0x0100,0,0}, // #SARR_IllumTypeF[4]  
	{0x0F12,0x0100,0,0}, // #SARR_IllumTypeF[5]  
	{0x0F12,0x0100,0,0}, // #SARR_IllumTypeF[6]  

	   
	//================================================================================================
	// SET AFIT
	//================================================================================================
	// Noise index
	{0x002A,0x0764,0,0},
	{0x0F12,0x0049,0,0},   // #afit_uNoiseIndInDoor[0] // 64
	{0x0F12,0x005F,0,0},   // #afit_uNoiseIndInDoor[1] // 165
	{0x0F12,0x00CB,0,0},   // #afit_uNoiseIndInDoor[2] // 377
	{0x0F12,0x01E0,0,0},   // #afit_uNoiseIndInDoor[3] // 616
	{0x0F12,0x0220,0,0},   // #afit_uNoiseIndInDoor[4] // 700
	    
	{0x002A,0x0770,0,0},  // AFIT table start address // 7000_07C4
	{0x0F12,0x07C4,0,0},
	{0x0F12,0x7000,0,0},
	    // AFIT table (Variables)
	{0x002A,0x07C4,0,0},
	{0x0F12,0x0034,0,0},  // #TVAR_afit_pBaseVals[0]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[1]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[2]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[3]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[4]
	{0x0F12,0x00C1,0,0},  // #TVAR_afit_pBaseVals[5]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[6]
	{0x0F12,0x009C,0,0},  // #TVAR_afit_pBaseVals[7]
	{0x0F12,0x0251,0,0},  // #TVAR_afit_pBaseVals[8]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[9]
	{0x0F12,0x000C,0,0},  // #TVAR_afit_pBaseVals[10]
	{0x0F12,0x0010,0,0},  // #TVAR_afit_pBaseVals[11]
	{0x0F12,0x012C,0,0},  // #TVAR_afit_pBaseVals[12]
	{0x0F12,0x03E8,0,0},  // #TVAR_afit_pBaseVals[13]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[14]
	{0x0F12,0x005A,0,0},  // #TVAR_afit_pBaseVals[15]
	{0x0F12,0x0070,0,0},  // #TVAR_afit_pBaseVals[16]
	{0x0F12,0x0020,0,0},  // #TVAR_afit_pBaseVals[17]
	{0x0F12,0x0020,0,0},  // #TVAR_afit_pBaseVals[18]
	{0x0F12,0x01AA,0,0},  // #TVAR_afit_pBaseVals[19]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[20]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[21]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[22]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[23]
	{0x0F12,0x003E,0,0},  // #TVAR_afit_pBaseVals[24]
	{0x0F12,0x0008,0,0},  // #TVAR_afit_pBaseVals[25]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[26]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[27]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[28]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[29]
	{0x0F12,0x0A24,0,0},  // #TVAR_afit_pBaseVals[30]
	{0x0F12,0x1701,0,0},  // #TVAR_afit_pBaseVals[31]
	{0x0F12,0x0229,0,0},  // #TVAR_afit_pBaseVals[32]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[33]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[34]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[35]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[36]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[37]
	{0x0F12,0x045A,0,0},  // #TVAR_afit_pBaseVals[38]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[39]
	{0x0F12,0x0301,0,0},  // #TVAR_afit_pBaseVals[40]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[41]
	{0x0F12,0x081E,0,0},  // #TVAR_afit_pBaseVals[42]
	{0x0F12,0x0A14,0,0},  // #TVAR_afit_pBaseVals[43]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[44]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[45]
	{0x0F12,0x0032,0,0},  // #TVAR_afit_pBaseVals[46]
	{0x0F12,0x000E,0,0},  // #TVAR_afit_pBaseVals[47]
	{0x0F12,0x0002,0,0},  // #TVAR_afit_pBaseVals[48]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[49]
	{0x0F12,0x1102,0,0},  // #TVAR_afit_pBaseVals[50]
	{0x0F12,0x001B,0,0},  // #TVAR_afit_pBaseVals[51]
	{0x0F12,0x0900,0,0},  // #TVAR_afit_pBaseVals[52]
	{0x0F12,0x0600,0,0},  // #TVAR_afit_pBaseVals[53]
	{0x0F12,0x0504,0,0},  // #TVAR_afit_pBaseVals[54]
	{0x0F12,0x0306,0,0},  // #TVAR_afit_pBaseVals[55]
	{0x0F12,0x4603,0,0},  // #TVAR_afit_pBaseVals[56]
	{0x0F12,0x0480,0,0},  // #TVAR_afit_pBaseVals[57]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[58]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[59]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[60]
	{0x0F12,0x0707,0,0},  // #TVAR_afit_pBaseVals[61]
	{0x0F12,0x5001,0,0},  // #TVAR_afit_pBaseVals[62]
	{0x0F12,0xC850,0,0},  // #TVAR_afit_pBaseVals[63]
	{0x0F12,0x50C8,0,0},  // #TVAR_afit_pBaseVals[64]
	{0x0F12,0x0500,0,0},  // #TVAR_afit_pBaseVals[65]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[66]
	{0x0F12,0x5400,0,0},  // #TVAR_afit_pBaseVals[67]
	{0x0F12,0x0714,0,0},  // #TVAR_afit_pBaseVals[68]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[69]
	{0x0F12,0x5A04,0,0},  // #TVAR_afit_pBaseVals[70]
	{0x0F12,0x201E,0,0},  // #TVAR_afit_pBaseVals[71]
	{0x0F12,0x4012,0,0},  // #TVAR_afit_pBaseVals[72]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[73]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[74]
	{0x0F12,0x0114,0,0},  // #TVAR_afit_pBaseVals[75]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[76]
	{0x0F12,0x4446,0,0},  // #TVAR_afit_pBaseVals[77]
	{0x0F12,0x646E,0,0},  // #TVAR_afit_pBaseVals[78]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[79]
	{0x0F12,0x030A,0,0},  // #TVAR_afit_pBaseVals[80]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[81]
	{0x0F12,0x141E,0,0},  // #TVAR_afit_pBaseVals[82]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[83]
	{0x0F12,0x0432,0,0},  // #TVAR_afit_pBaseVals[84]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[85]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[86]
	{0x0F12,0x0440,0,0},  // #TVAR_afit_pBaseVals[87]
	{0x0F12,0x0302,0,0},  // #TVAR_afit_pBaseVals[88]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[89]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[90]
	{0x0F12,0x4601,0,0},  // #TVAR_afit_pBaseVals[91]
	{0x0F12,0x6E44,0,0},  // #TVAR_afit_pBaseVals[92]
	{0x0F12,0x2864,0,0},  // #TVAR_afit_pBaseVals[93]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[94]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[95]
	{0x0F12,0x1E00,0,0},  // #TVAR_afit_pBaseVals[96]
	{0x0F12,0x0714,0,0},  // #TVAR_afit_pBaseVals[97]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[98]
	{0x0F12,0x1404,0,0},  // #TVAR_afit_pBaseVals[99]
	{0x0F12,0x0F0A,0,0},  // #TVAR_afit_pBaseVals[100]
	{0x0F12,0x400F,0,0},  // #TVAR_afit_pBaseVals[101]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[102]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[103]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[104]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[105]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[106]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[107]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[108]
	{0x0F12,0x00C1,0,0},  // #TVAR_afit_pBaseVals[109]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[110]
	{0x0F12,0x009C,0,0},  // #TVAR_afit_pBaseVals[111]
	{0x0F12,0x0251,0,0},  // #TVAR_afit_pBaseVals[112]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[113]
	{0x0F12,0x000C,0,0},  // #TVAR_afit_pBaseVals[114]
	{0x0F12,0x0010,0,0},  // #TVAR_afit_pBaseVals[115]
	{0x0F12,0x012C,0,0},  // #TVAR_afit_pBaseVals[116]
	{0x0F12,0x03E8,0,0},  // #TVAR_afit_pBaseVals[117]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[118]
	{0x0F12,0x005A,0,0},  // #TVAR_afit_pBaseVals[119]
	{0x0F12,0x0070,0,0},  // #TVAR_afit_pBaseVals[120]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[121]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[122]
	{0x0F12,0x01AE,0,0},  // #TVAR_afit_pBaseVals[123]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[124]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[125]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[126]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[127]
	{0x0F12,0x003E,0,0},  // #TVAR_afit_pBaseVals[128]
	{0x0F12,0x0008,0,0},  // #TVAR_afit_pBaseVals[129]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[130]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[131]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[132]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[133]
	{0x0F12,0x0A24,0,0},  // #TVAR_afit_pBaseVals[134]
	{0x0F12,0x1701,0,0},  // #TVAR_afit_pBaseVals[135]
	{0x0F12,0x0229,0,0},  // #TVAR_afit_pBaseVals[136]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[137]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[138]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[139]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[140]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[141]
	{0x0F12,0x045A,0,0},  // #TVAR_afit_pBaseVals[142]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[143]
	{0x0F12,0x0301,0,0},  // #TVAR_afit_pBaseVals[144]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[145]
	{0x0F12,0x081E,0,0},  // #TVAR_afit_pBaseVals[146]
	{0x0F12,0x0A14,0,0},  // #TVAR_afit_pBaseVals[147]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[148]
	{0x0F12,0x0A00,0,0},  // //0A03 // #TVAR_afit_pBaseVals[149]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[150]
	{0x0F12,0x0018,0,0},  // #TVAR_afit_pBaseVals[151]
	{0x0F12,0x0002,0,0},  // #TVAR_afit_pBaseVals[152]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[153]
	{0x0F12,0x1102,0,0},  // #TVAR_afit_pBaseVals[154]
	{0x0F12,0x001B,0,0},  // #TVAR_afit_pBaseVals[155]
	{0x0F12,0x0900,0,0},  // #TVAR_afit_pBaseVals[156]
	{0x0F12,0x0600,0,0},  // #TVAR_afit_pBaseVals[157]
	{0x0F12,0x0504,0,0},  // #TVAR_afit_pBaseVals[158]
	{0x0F12,0x0306,0,0},  // #TVAR_afit_pBaseVals[159]
	{0x0F12,0x4603,0,0},  // #TVAR_afit_pBaseVals[160]
	{0x0F12,0x0880,0,0},  // #TVAR_afit_pBaseVals[161]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[162]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[163]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[164]
	{0x0F12,0x0707,0,0},  // #TVAR_afit_pBaseVals[165]
	{0x0F12,0x3C01,0,0},  // #TVAR_afit_pBaseVals[166]
	{0x0F12,0xC83C,0,0},  // #TVAR_afit_pBaseVals[167]
	{0x0F12,0x50C8,0,0},  // #TVAR_afit_pBaseVals[168]
	{0x0F12,0x0500,0,0},  // #TVAR_afit_pBaseVals[169]
	{0x0F12,0x0004,0,0},  // #TVAR_afit_pBaseVals[170]
	{0x0F12,0x3C0A,0,0},  // #TVAR_afit_pBaseVals[171]
	{0x0F12,0x0714,0,0},  // #TVAR_afit_pBaseVals[172]
	{0x0F12,0x3214,0,0},  // #TVAR_afit_pBaseVals[173]
	{0x0F12,0x5A03,0,0},  // #TVAR_afit_pBaseVals[174]
	{0x0F12,0x1228,0,0},  // #TVAR_afit_pBaseVals[175]
	{0x0F12,0x4012,0,0},  // #TVAR_afit_pBaseVals[176]
	{0x0F12,0x0604,0,0},  // #TVAR_afit_pBaseVals[177]
	{0x0F12,0x1E06,0,0},  // #TVAR_afit_pBaseVals[178]
	{0x0F12,0x011E,0,0},  // #TVAR_afit_pBaseVals[179]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[180]
	{0x0F12,0x3A3C,0,0},  // #TVAR_afit_pBaseVals[181]
	{0x0F12,0x585A,0,0},  // #TVAR_afit_pBaseVals[182]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[183]
	{0x0F12,0x030A,0,0},  // #TVAR_afit_pBaseVals[184]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[185]
	{0x0F12,0x141E,0,0},  // #TVAR_afit_pBaseVals[186]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[187]
	{0x0F12,0x0432,0,0},  // #TVAR_afit_pBaseVals[188]
	{0x0F12,0x1428,0,0},  // #TVAR_afit_pBaseVals[189]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[190]
	{0x0F12,0x0440,0,0},  // #TVAR_afit_pBaseVals[191]
	{0x0F12,0x0302,0,0},  // #TVAR_afit_pBaseVals[192]
	{0x0F12,0x1E1E,0,0},  // #TVAR_afit_pBaseVals[193]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[194]
	{0x0F12,0x3C01,0,0},  // #TVAR_afit_pBaseVals[195]
	{0x0F12,0x5A3A,0,0},  // #TVAR_afit_pBaseVals[196]
	{0x0F12,0x2858,0,0},  // #TVAR_afit_pBaseVals[197]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[198]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[199]
	{0x0F12,0x1E00,0,0},  // #TVAR_afit_pBaseVals[200]
	{0x0F12,0x0714,0,0},  // #TVAR_afit_pBaseVals[201]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[202]
	{0x0F12,0x2804,0,0},  // #TVAR_afit_pBaseVals[203]
	{0x0F12,0x0F1E,0,0},  // #TVAR_afit_pBaseVals[204]
	{0x0F12,0x400F,0,0},  // #TVAR_afit_pBaseVals[205]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[206]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[207]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[208]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[209]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[210]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[211]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[212]
	{0x0F12,0x00C1,0,0},  // #TVAR_afit_pBaseVals[213]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[214]
	{0x0F12,0x009C,0,0},  // #TVAR_afit_pBaseVals[215]
	{0x0F12,0x0251,0,0},  // #TVAR_afit_pBaseVals[216]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[217]
	{0x0F12,0x000C,0,0},  // #TVAR_afit_pBaseVals[218]
	{0x0F12,0x0010,0,0},  // #TVAR_afit_pBaseVals[219]
	{0x0F12,0x012C,0,0},  // #TVAR_afit_pBaseVals[220]
	{0x0F12,0x03E8,0,0},  // #TVAR_afit_pBaseVals[221]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[222]
	{0x0F12,0x005A,0,0},  // #TVAR_afit_pBaseVals[223]
	{0x0F12,0x0070,0,0},  // #TVAR_afit_pBaseVals[224]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[225]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[226]
	{0x0F12,0x0226,0,0},  // #TVAR_afit_pBaseVals[227]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[228]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[229]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[230]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[231]
	{0x0F12,0x004E,0,0},  // #TVAR_afit_pBaseVals[232]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[233]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[234]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[235]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[236]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[237]
	{0x0F12,0x0A24,0,0},  // #TVAR_afit_pBaseVals[238]
	{0x0F12,0x1701,0,0},  // #TVAR_afit_pBaseVals[239]
	{0x0F12,0x0229,0,0},  // #TVAR_afit_pBaseVals[240]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[241]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[242]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[243]
	{0x0F12,0x0906,0,0},  // #TVAR_afit_pBaseVals[244]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[245]
	{0x0F12,0x045A,0,0},  // #TVAR_afit_pBaseVals[246]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[247]
	{0x0F12,0x0301,0,0},  // #TVAR_afit_pBaseVals[248]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[249]
	{0x0F12,0x081E,0,0},  // #TVAR_afit_pBaseVals[250]
	{0x0F12,0x0A14,0,0},  // #TVAR_afit_pBaseVals[251]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[252]
	{0x0F12,0x0A00,0,0},  // //0A03 // #TVAR_afit_pBaseVals[253]
	{0x0F12,0x009A,0,0},  // #TVAR_afit_pBaseVals[254]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[255]
	{0x0F12,0x0002,0,0},  // #TVAR_afit_pBaseVals[256]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[257]
	{0x0F12,0x1102,0,0},  // #TVAR_afit_pBaseVals[258]
	{0x0F12,0x001B,0,0},  // #TVAR_afit_pBaseVals[259]
	{0x0F12,0x0900,0,0},  // #TVAR_afit_pBaseVals[260]
	{0x0F12,0x0600,0,0},  // #TVAR_afit_pBaseVals[261]
	{0x0F12,0x0504,0,0},  // #TVAR_afit_pBaseVals[262]
	{0x0F12,0x0306,0,0},  // #TVAR_afit_pBaseVals[263] //*<BU5D05950 zhangsheng 20100422 begin*/ 
	{0x0F12,0x4602,0,0},  // //7903 // #TVAR_afit_pBaseVals[264] //*BU5D05950 zhangsheng 20100422 end>*/ 
	{0x0F12,0x1980,0,0},  // #TVAR_afit_pBaseVals[265]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[266]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[267]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[268]
	{0x0F12,0x0707,0,0},  // #TVAR_afit_pBaseVals[269]
	{0x0F12,0x2A01,0,0},  // #TVAR_afit_pBaseVals[270]
	{0x0F12,0x462A,0,0},  // #TVAR_afit_pBaseVals[271]
	{0x0F12,0x5046,0,0},  // #TVAR_afit_pBaseVals[272]
	{0x0F12,0x0500,0,0},  // #TVAR_afit_pBaseVals[273]
	{0x0F12,0x1A04,0,0},  // #TVAR_afit_pBaseVals[274]
	{0x0F12,0x280A,0,0},  // #TVAR_afit_pBaseVals[275]
	{0x0F12,0x080C,0,0},  // #TVAR_afit_pBaseVals[276]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[277]
	{0x0F12,0x7E03,0,0},  // #TVAR_afit_pBaseVals[278]
	{0x0F12,0x124A,0,0},  // #TVAR_afit_pBaseVals[279]
	{0x0F12,0x4012,0,0},  // #TVAR_afit_pBaseVals[280]
	{0x0F12,0x0604,0,0},  // #TVAR_afit_pBaseVals[281]
	{0x0F12,0x2806,0,0},  // #TVAR_afit_pBaseVals[282]
	{0x0F12,0x0128,0,0},  // #TVAR_afit_pBaseVals[283]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[284]
	{0x0F12,0x2224,0,0},  // #TVAR_afit_pBaseVals[285]
	{0x0F12,0x3236,0,0},  // #TVAR_afit_pBaseVals[286]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[287]
	{0x0F12,0x030A,0,0},  // #TVAR_afit_pBaseVals[288]
	{0x0F12,0x0410,0,0},  // #TVAR_afit_pBaseVals[289]
	{0x0F12,0x141E,0,0},  // #TVAR_afit_pBaseVals[290]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[291]
	{0x0F12,0x0432,0,0},  // #TVAR_afit_pBaseVals[292]
	{0x0F12,0x547D,0,0},  // #TVAR_afit_pBaseVals[293]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[294]
	{0x0F12,0x0440,0,0},  // #TVAR_afit_pBaseVals[295]
	{0x0F12,0x0302,0,0},  // #TVAR_afit_pBaseVals[296]
	{0x0F12,0x2828,0,0},  // #TVAR_afit_pBaseVals[297]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[298]
	{0x0F12,0x2401,0,0},  // #TVAR_afit_pBaseVals[299]
	{0x0F12,0x3622,0,0},  // #TVAR_afit_pBaseVals[300]
	{0x0F12,0x2832,0,0},  // #TVAR_afit_pBaseVals[301]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[302]
	{0x0F12,0x1003,0,0},  // #TVAR_afit_pBaseVals[303]
	{0x0F12,0x1E04,0,0},  // #TVAR_afit_pBaseVals[304]
	{0x0F12,0x0714,0,0},  // #TVAR_afit_pBaseVals[305]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[306]
	{0x0F12,0x7D04,0,0},  // #TVAR_afit_pBaseVals[307]
	{0x0F12,0x0F5E,0,0},  // #TVAR_afit_pBaseVals[308]
	{0x0F12,0x400F,0,0},  // #TVAR_afit_pBaseVals[309]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[310]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[311]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[312]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[313]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[314]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[315]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[316]
	{0x0F12,0x00C1,0,0},  // #TVAR_afit_pBaseVals[317]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[318]
	{0x0F12,0x009C,0,0},  // #TVAR_afit_pBaseVals[319]
	{0x0F12,0x0251,0,0},  // #TVAR_afit_pBaseVals[320]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[321]
	{0x0F12,0x000C,0,0},  // #TVAR_afit_pBaseVals[322]
	{0x0F12,0x0010,0,0},  // #TVAR_afit_pBaseVals[323]
	{0x0F12,0x00C8,0,0},  // #TVAR_afit_pBaseVals[324]
	{0x0F12,0x03E8,0,0},  // #TVAR_afit_pBaseVals[325]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[326]
	{0x0F12,0x0050,0,0},  // #TVAR_afit_pBaseVals[327]
	{0x0F12,0x0070,0,0},  // #TVAR_afit_pBaseVals[328]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[329]
	{0x0F12,0x000A,0,0},  // #TVAR_afit_pBaseVals[330]
	{0x0F12,0x0226,0,0},  // #TVAR_afit_pBaseVals[331]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[332]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[333]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[334]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[335]
	{0x0F12,0x004E,0,0},  // #TVAR_afit_pBaseVals[336]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[337]
	{0x0F12,0x002D,0,0},  // #TVAR_afit_pBaseVals[338]
	{0x0F12,0x0019,0,0},  // #TVAR_afit_pBaseVals[339]
	{0x0F12,0x002D,0,0},  // #TVAR_afit_pBaseVals[340]
	{0x0F12,0x0019,0,0},  // #TVAR_afit_pBaseVals[341]
	{0x0F12,0x0A24,0,0},  // #TVAR_afit_pBaseVals[342]
	{0x0F12,0x1701,0,0},  // #TVAR_afit_pBaseVals[343]
	{0x0F12,0x0229,0,0},  // #TVAR_afit_pBaseVals[344]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[345]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[346]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[347]
	{0x0F12,0x0906,0,0},  // #TVAR_afit_pBaseVals[348]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[349]
	{0x0F12,0x045A,0,0},  // #TVAR_afit_pBaseVals[350]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[351]
	{0x0F12,0x0301,0,0},  // #TVAR_afit_pBaseVals[352]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[353]
	{0x0F12,0x081E,0,0},  // #TVAR_afit_pBaseVals[354]
	{0x0F12,0x0A14,0,0},  // #TVAR_afit_pBaseVals[355]
	{0x0F12,0x0F0F,0,0},  // #TVAR_afit_pBaseVals[356]
	{0x0F12,0x0A00,0,0},  // //0A00 // #TVAR_afit_pBaseVals[357]
	{0x0F12,0x009A,0,0},  // #TVAR_afit_pBaseVals[358]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[359]
	{0x0F12,0x0001,0,0},  // #TVAR_afit_pBaseVals[360]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[361]
	{0x0F12,0x1002,0,0},  // #TVAR_afit_pBaseVals[362]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[363]
	{0x0F12,0x0900,0,0},  // #TVAR_afit_pBaseVals[364]
	{0x0F12,0x0600,0,0},  // #TVAR_afit_pBaseVals[365]
	{0x0F12,0x0504,0,0},  // #TVAR_afit_pBaseVals[366]
	{0x0F12,0x0307,0,0},  // #TVAR_afit_pBaseVals[367]//*<BU5D05950 zhangsheng 20100422 begin*/
	{0x0F12,0x6402,0,0},  // //8902 // #TVAR_afit_pBaseVals[368]//*BU5D05950 zhangsheng 20100422 end>*/
	{0x0F12,0x1980,0,0},  // #TVAR_afit_pBaseVals[369]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[370]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[371]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[372]
	{0x0F12,0x0707,0,0},  // #TVAR_afit_pBaseVals[373]
	{0x0F12,0x2A01,0,0},  // #TVAR_afit_pBaseVals[374]
	{0x0F12,0x262A,0,0},  // #TVAR_afit_pBaseVals[375]
	{0x0F12,0x5026,0,0},  // #TVAR_afit_pBaseVals[376]
	{0x0F12,0x0500,0,0},  // #TVAR_afit_pBaseVals[377]
	{0x0F12,0x1A04,0,0},  // #TVAR_afit_pBaseVals[378]
	{0x0F12,0x280A,0,0},  // #TVAR_afit_pBaseVals[379]
	{0x0F12,0x080C,0,0},  // #TVAR_afit_pBaseVals[380]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[381]
	{0x0F12,0x7E03,0,0},  // #TVAR_afit_pBaseVals[382]
	{0x0F12,0x124A,0,0},  // #TVAR_afit_pBaseVals[383]
	{0x0F12,0x4012,0,0},  // #TVAR_afit_pBaseVals[384]
	{0x0F12,0x0604,0,0},  // #TVAR_afit_pBaseVals[385]
	{0x0F12,0x3C06,0,0},  // #TVAR_afit_pBaseVals[386]
	{0x0F12,0x013C,0,0},  // #TVAR_afit_pBaseVals[387]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[388]
	{0x0F12,0x1C1E,0,0},  // #TVAR_afit_pBaseVals[389]
	{0x0F12,0x1E22,0,0},  // #TVAR_afit_pBaseVals[390]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[391]
	{0x0F12,0x030A,0,0},  // #TVAR_afit_pBaseVals[392]
	{0x0F12,0x0214,0,0},  // #TVAR_afit_pBaseVals[393]
	{0x0F12,0x0E14,0,0},  // #TVAR_afit_pBaseVals[394]
	{0x0F12,0xFF06,0,0},  // #TVAR_afit_pBaseVals[395]
	{0x0F12,0x0432,0,0},  // #TVAR_afit_pBaseVals[396]
	{0x0F12,0x547D,0,0},  // #TVAR_afit_pBaseVals[397]
	{0x0F12,0x150C,0,0},  // #TVAR_afit_pBaseVals[398]
	{0x0F12,0x0440,0,0},  // #TVAR_afit_pBaseVals[399]
	{0x0F12,0x0302,0,0},  // #TVAR_afit_pBaseVals[400]
	{0x0F12,0x3C3C,0,0},  // #TVAR_afit_pBaseVals[401]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[402]
	{0x0F12,0x1E01,0,0},  // #TVAR_afit_pBaseVals[403]
	{0x0F12,0x221C,0,0},  // #TVAR_afit_pBaseVals[404]
	{0x0F12,0x281E,0,0},  // #TVAR_afit_pBaseVals[405]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[406]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[407]
	{0x0F12,0x1402,0,0},  // #TVAR_afit_pBaseVals[408]
	{0x0F12,0x060E,0,0},  // #TVAR_afit_pBaseVals[409]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[410]
	{0x0F12,0x7D04,0,0},  // #TVAR_afit_pBaseVals[411]
	{0x0F12,0x0C5E,0,0},  // #TVAR_afit_pBaseVals[412]
	{0x0F12,0x4015,0,0},  // #TVAR_afit_pBaseVals[413]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[414]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[415]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[416]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[417]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[418] //high luma sta
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[419]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[420]
	{0x0F12,0x00C1,0,0},  // #TVAR_afit_pBaseVals[421]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[422]
	{0x0F12,0x009C,0,0},  // #TVAR_afit_pBaseVals[423]
	{0x0F12,0x0251,0,0},  // #TVAR_afit_pBaseVals[424]
	{0x0F12,0x03FF,0,0},  // #TVAR_afit_pBaseVals[425]
	{0x0F12,0x000C,0,0},  // #TVAR_afit_pBaseVals[426]
	{0x0F12,0x0010,0,0},  // #TVAR_afit_pBaseVals[427]
	{0x0F12,0x0032,0,0},  // #TVAR_afit_pBaseVals[428]
	{0x0F12,0x028A,0,0},  // #TVAR_afit_pBaseVals[429]
	{0x0F12,0x0032,0,0},  // #TVAR_afit_pBaseVals[430]
	{0x0F12,0x01F4,0,0},  // #TVAR_afit_pBaseVals[431]
	{0x0F12,0x0070,0,0},  // #TVAR_afit_pBaseVals[432]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[433]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[434]
	{0x0F12,0x01AA,0,0},  // #TVAR_afit_pBaseVals[435]
	{0x0F12,0x003C,0,0},  // #TVAR_afit_pBaseVals[436]
	{0x0F12,0x0050,0,0},  // #TVAR_afit_pBaseVals[437]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[438]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[439]
	{0x0F12,0x0044,0,0},  // #TVAR_afit_pBaseVals[440]
	{0x0F12,0x0014,0,0},  // #TVAR_afit_pBaseVals[441]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[442]
	{0x0F12,0x0019,0,0},  // #TVAR_afit_pBaseVals[443]
	{0x0F12,0x0046,0,0},  // #TVAR_afit_pBaseVals[444]
	{0x0F12,0x0019,0,0},  // #TVAR_afit_pBaseVals[445]
	{0x0F12,0x0A24,0,0},  // #TVAR_afit_pBaseVals[446]
	{0x0F12,0x1701,0,0},  // #TVAR_afit_pBaseVals[447]
	{0x0F12,0x0229,0,0},  // #TVAR_afit_pBaseVals[448]
	{0x0F12,0x0503,0,0},  // #TVAR_afit_pBaseVals[449]
	{0x0F12,0x080F,0,0},  // #TVAR_afit_pBaseVals[450]
	{0x0F12,0x0808,0,0},  // #TVAR_afit_pBaseVals[451]
	{0x0F12,0x0000,0,0},  // #TVAR_afit_pBaseVals[452]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[453]
	{0x0F12,0x012D,0,0},  // #TVAR_afit_pBaseVals[454]
	{0x0F12,0x1414,0,0},  // #TVAR_afit_pBaseVals[455]
	{0x0F12,0x0301,0,0},  // #TVAR_afit_pBaseVals[456]
	{0x0F12,0xFF07,0,0},  // #TVAR_afit_pBaseVals[457]
	{0x0F12,0x061E,0,0},  // #TVAR_afit_pBaseVals[458]
	{0x0F12,0x0A1E,0,0},  // #TVAR_afit_pBaseVals[459]
	{0x0F12,0x0606,0,0},  // #TVAR_afit_pBaseVals[460]
	{0x0F12,0x0A00,0,0},  ////0A00 // #TVAR_afit_pBaseVals[461]
	{0x0F12,0x379A,0,0},  // #TVAR_afit_pBaseVals[462]
	{0x0F12,0x1028,0,0},  // #TVAR_afit_pBaseVals[463]
	{0x0F12,0x0001,0,0},  // #TVAR_afit_pBaseVals[464]
	{0x0F12,0x00FF,0,0},  // #TVAR_afit_pBaseVals[465]
	{0x0F12,0x1002,0,0},  // #TVAR_afit_pBaseVals[466]
	{0x0F12,0x001E,0,0},  // #TVAR_afit_pBaseVals[467]
	{0x0F12,0x0900,0,0},  // #TVAR_afit_pBaseVals[468]
	{0x0F12,0x0600,0,0},  // #TVAR_afit_pBaseVals[469]
	{0x0F12,0x0504,0,0},  // #TVAR_afit_pBaseVals[470]
	{0x0F12,0x0307,0,0},  // #TVAR_afit_pBaseVals[471] //*<BU5D05950 zhangsheng 20100422 begin*/
	{0x0F12,0x6401,0,0},  // #TVAR_afit_pBaseVals[472] //*BU5D05950 zhangsheng 20100422 end>*/ 
	{0x0F12,0x1980,0,0},  // #TVAR_afit_pBaseVals[473]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[474]
	{0x0F12,0x0080,0,0},  // #TVAR_afit_pBaseVals[475]
	{0x0F12,0x5050,0,0},  // #TVAR_afit_pBaseVals[476]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[477]
	{0x0F12,0x1C01,0,0},  // #TVAR_afit_pBaseVals[478]
	{0x0F12,0x191C,0,0},  // #TVAR_afit_pBaseVals[479]
	{0x0F12,0x210C,0,0},  // #TVAR_afit_pBaseVals[480]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[481]
	{0x0F12,0x1E04,0,0},  // #TVAR_afit_pBaseVals[482]
	{0x0F12,0x0A08,0,0},  // #TVAR_afit_pBaseVals[483]
	{0x0F12,0x070C,0,0},  // #TVAR_afit_pBaseVals[484]
	{0x0F12,0x3264,0,0},  // #TVAR_afit_pBaseVals[485]
	{0x0F12,0x7E02,0,0},  // #TVAR_afit_pBaseVals[486]
	{0x0F12,0x104A,0,0},  // #TVAR_afit_pBaseVals[487]
	{0x0F12,0x4012,0,0},  // #TVAR_afit_pBaseVals[488]
	{0x0F12,0x0604,0,0},  // #TVAR_afit_pBaseVals[489]
	{0x0F12,0x4606,0,0},  // #TVAR_afit_pBaseVals[490]
	{0x0F12,0x0146,0,0},  // #TVAR_afit_pBaseVals[491]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[492]
	{0x0F12,0x1C18,0,0},  // #TVAR_afit_pBaseVals[493]
	{0x0F12,0x1819,0,0},  // #TVAR_afit_pBaseVals[494]
	{0x0F12,0x0028,0,0},  // #TVAR_afit_pBaseVals[495]
	{0x0F12,0x030A,0,0},  // #TVAR_afit_pBaseVals[496]
	{0x0F12,0x0514,0,0},  // #TVAR_afit_pBaseVals[497]
	{0x0F12,0x0C14,0,0},  // #TVAR_afit_pBaseVals[498]
	{0x0F12,0xFF05,0,0},  // #TVAR_afit_pBaseVals[499]
	{0x0F12,0x0432,0,0},  // #TVAR_afit_pBaseVals[500]
	{0x0F12,0x547D,0,0},  // #TVAR_afit_pBaseVals[501]
	{0x0F12,0x1514,0,0},  // #TVAR_afit_pBaseVals[502]
	{0x0F12,0x0440,0,0},  // #TVAR_afit_pBaseVals[503]
	{0x0F12,0x0302,0,0},  // #TVAR_afit_pBaseVals[504]
	{0x0F12,0x4646,0,0},  // #TVAR_afit_pBaseVals[505]
	{0x0F12,0x0101,0,0},  // #TVAR_afit_pBaseVals[506]
	{0x0F12,0x1801,0,0},  // #TVAR_afit_pBaseVals[507]
	{0x0F12,0x191C,0,0},  // #TVAR_afit_pBaseVals[508]
	{0x0F12,0x2818,0,0},  // #TVAR_afit_pBaseVals[509]
	{0x0F12,0x0A00,0,0},  // #TVAR_afit_pBaseVals[510]
	{0x0F12,0x1403,0,0},  // #TVAR_afit_pBaseVals[511]
	{0x0F12,0x1405,0,0},  // #TVAR_afit_pBaseVals[512]
	{0x0F12,0x050C,0,0},  // #TVAR_afit_pBaseVals[513]
	{0x0F12,0x32FF,0,0},  // #TVAR_afit_pBaseVals[514]
	{0x0F12,0x7D04,0,0},  // #TVAR_afit_pBaseVals[515]
	{0x0F12,0x145E,0,0},  // #TVAR_afit_pBaseVals[516]
	{0x0F12,0x4015,0,0},  // #TVAR_afit_pBaseVals[517]
	{0x0F12,0x0204,0,0},  // #TVAR_afit_pBaseVals[518]
	{0x0F12,0x0003,0,0},  // #TVAR_afit_pBaseVals[519]
	  // AFIT table (Constants)
	{0x0F12,0x7F7A,0,0}, // #afit_pConstBaseVals[0]
	{0x0F12,0x7F9D,0,0}, // #afit_pConstBaseVals[1]
	{0x0F12,0xBEFC,0,0}, // #afit_pConstBaseVals[2]
	{0x0F12,0xF7BC,0,0}, // #afit_pConstBaseVals[3]
	{0x0F12,0x7E06,0,0}, // #afit_pConstBaseVals[4]
	{0x0F12,0x0053,0,0}, // #afit_pConstBaseVals[5]
	  // Update Changed Registers
	{0x002A,0x0664,0,0},
	{0x0F12,0x013E,0,0}, //seti_uContrastCenter

	{0x002A,0x04D6,0,0},
	{0x0F12,0x0001,0,0}, // #REG_TC_DBG_ReInitCmd
	{0x0028,0xD000,0,0},
	{0x002A,0x1102,0,0},
	{0x0F12,0x00C0,0,0}, // Use T&P index 22 and 23
	{0x002A,0x113C,0,0},
	{0x0F12,0x267C,0,0}, // Trap 22 address 0x71aa
	{0x0F12,0x2680,0,0}, // Trap 23 address 0x71b4
	{0x002A,0x1142,0,0}, 
	{0x0F12,0x00C0,0,0}, // Trap Up Set (trap Addr are > 0x10000) 
	{0x002A,0x117C,0,0}, 
	{0x0F12,0x2CE8,0,0}, // Patch 22 address (TrapAndPatchOpCodes array index 22) 
	{0x0F12,0x2CeC,0,0}, // Patch 23 address (TrapAndPatchOpCodes array index 23) 
	// Fill RAM with alternative op-codes
	{0x0028,0x7000,0,0}, // start add MSW
	{0x002A,0x2CE8,0,0}, // start add LSW
	{0x0F12,0x0007,0,0}, // Modify LSB to control AWBB_YThreshLow
	{0x0F12,0x00e2,0,0}, // 
	{0x0F12,0x0005,0,0}, // Modify LSB to control AWBB_YThreshLowBrLow
	{0x0F12,0x00e2,0,0}, // 
	    // Update T&P tuning parameters
	{0x002A,0x337A,0,0},
	{0x0F12,0x0006,0,0}, // #Tune_TP_atop_dbus_reg // 6 is the default HW value
	//============================================================
	// Frame rate setting 
	//============================================================
	// How to set
	// 1. Exposure value
	// dec2hex((1 / (frame rate you want(ms))) * 100d * 4d)
	// 2. Analog Digital gain
	// dec2hex((Analog gain you want) * 256d)
	//============================================================
	      // Set preview exposure time
	{0x002A,0x0530,0,0},
	{0x002A,0x0530,0,0},//lt_uMaxExp1//   
	{0x0F12,0x3415,0,0},        
	{0x002A,0x0534,0,0},//lt_uMaxExp2//   
	{0x0F12,0x682A,0,0},        
	{0x002A,0x167C,0,0},//lt_uMaxExp3//   
	{0x0F12,0x8235,0,0},        
	{0x002A,0x1680,0,0},//lt_uMaxExp4//   
	{0x0F12,0xc350,0,0},        
	{0x0F12,0x0000,0,0},        
	   
	{0x002A,0x0538,0,0},//It_uCapMaxExp1 // 
	{0x0F12,0x3415,0,0},        
	{0x002A,0x053C,0,0},//It_uCapMaxExp2 // 
	{0x0F12,0x682A,0,0},        
	{0x002A,0x1684,0,0},//It_uCapMaxExp3 // 
	{0x0F12,0x8235,0,0},        
	{0x002A,0x1688,0,0},//It_uCapMaxExp4 // 
	{0x0F12,0xc350,0,0},        
	{0x0F12,0x0000,0,0}, 

	     // Set gain
	{0x002A,0x0540,0,0},                     
	{0x0F12,0x0150,0,0},//lt_uMaxAnGain1_700lux//                
	{0x0F12,0x0190,0,0},//lt_uMaxAnGain2_400lux//          
	{0x002A,0x168C,0,0},                     
	{0x0F12,0x0250,0,0},//MaxAnGain3_200lux//             
	{0x0F12,0x0600,0,0},//MaxAnGain4 //
	   
	{0x002A,0x0544,0,0},        
	{0x0F12,0x0100,0,0},//It_uMaxDigGain // 
	{0x0F12,0x8000,0,0}, //Max Gain 8 // 
	{0x002A,0x1694,0,0},
	{0x0F12,0x0001,0,0}, // #evt1_senHal_bExpandForbid
	{0x002A,0x051A,0,0},
	{0x0F12,0x0111,0,0}, // #lt_uLimitHigh 
	{0x0F12,0x00F0,0,0}, // #lt_uLimitLow

	{0x002A,0x3286,0,0},
	{0x0F12,0x0001,0,0},  //Pre/Post gamma on(ฟ๘บน) 

	{0x002A,0x020E,0,0},                                       
	{0x0F12,0x000F,0,0}, //Contr 
	//================================================================================================
	// How to set
	// 1. MCLK
	// 2. System CLK
	// 3. PCLK
	//================================================================================================
	//clk Settings
	{0x002A,0x01CC,0,0},
	{0x0F12,0x5DC0,0,0},//5FB4//#REG_TC_IPRM_InClockLSBs
	{0x0F12,0x0000,0,0},//#REG_TC_IPRM_InClockMSBs
	{0x002A,0x01EE,0,0},
	{0x0F12,0x0000,0,0},//#REG_TC_IPRM_UseNPviClocks//Number of PLL setting
	{0x0F12,0x0002,0,0},//#REG_TC_IPRM_UseNmipiClocks//Number of PLL setting                     
	{0x0F12,0x0001,0,0},//Number of mipi lanes                                                   
	//Set system CLK//52MHz                                                                      
	{0x002A,0x01F6,0,0},                                                                         
	{0x0F12,0x32C8,0,0},//#REG_TC_IPRM_OpClk4KHz_0                                               
	{0x0F12,0x3A88,0,0},//#REG_TC_IPRM_MinOutRate4KHz_0                                          
	{0x0F12,0x3AA8,0,0},//#REG_TC_IPRM_MaxOutRate4KHz_0                                          
	{0x0F12,0x2904,0,0},//#REG_TC_IPRM_OpClk4KHz_1                                               
	{0x0F12,0x1194,0,0},//#REG_TC_IPRM_MinOutRate4KHz_1                                          
	{0x0F12,0x1194,0,0},//#REG_TC_IPRM_MaxOutRate4KHz_1                                          
	//Update PLL                                                                                 
	{0x002A,0x0208,0,0},                                                                         
	{0x0F12,0x0001,0,0},//#REG_TC_IPRM_InitParamsUpdated   

	   //Auto Flicker 60Hz Start
	{0x0028,0x7000,0,0},
	{0x002A,0x0C18,0,0}, //#AFC_Default60Hz
	{0x0F12,0x0000,0,0}, // #AFC_Default60Hz  1: Auto Flicker 60Hz start 0: Auto Flicker 50Hz start
	{0x002A,0x04D2,0,0}, // #REG_TC_DBG_AutoAlgEnBits
	{0x0F12,0x067F,0,0},

	{0x002A,0x020E,0,0},       
	{0x0F12,0x0015,0,0},            //Contr

	{0x002A,0x0452,0,0},
	{0x0F12,0x0055,0,0},   //REG_TC_BRC_usPrevQuality //Best Quality : 85d; Good Quality : 50d;Poor Quality :20d  
	{0x0F12,0x0055,0,0},   //REG_TC_BRC_usCaptureQuality //Best Quality : 85d; Good Quality : 50d;Poor Quality :20d

        //AWB Speed control
        {0x002A,0x0DCA,0,0},                                                         
        {0x0F12,0x0030,0,0}, // 0030 //AWB Gain
        {0x002A,0x0DCC,0,0},                                                         
        {0x0F12,0x0000,0,0}, //0007 //AWB Speed

	//HS_Prepare
        {0x002A,0xB0C2,0,0},                                                         
        {0x0F12,0x0003,0,0}, 
	//HS_Zero
       //{0x002A,0xB0C0,0,0},                                                         
       //{0x0F12,0x0003,0,0}, 
	
	//================================================================================================
	// SET PREVIEW CONFIGURATION_0
	// # Foramt: YUV422
	// # Size:   640*480
	// # FPS:    7.5-30fps(using normal_mode preview)
	//================================================================================================
	{0x002A,0x026C,0,0},
	{0x0F12,0x0280,0,0},//#REG_0TC_PCFG_usWidth//640
	{0x0F12,0x01E0,0,0},//#REG_0TC_PCFG_usHeight//480
	{0x0F12,0x0005,0,0},//#REG_0TC_PCFG_Format 0270
	{0x0F12,0x3AA8,0,0},//157C//3AA8//#REG_0TC_PCFG_usMaxOut4KHzRate 0272
	{0x0F12,0x3A88,0,0},//157C//3A88//#REG_0TC_PCFG_usMinOut4KHzRate 0274
	{0x0F12,0x0100,0,0},//#REG_0TC_PCFG_OutClkPerPix88 0276
	{0x0F12,0x0800,0,0},//#REG_0TC_PCFG_uMaxBpp88 027
	{0x0F12,0x0042,0,0},//0052//#REG_0TC_PCFG_PVIMask//s0050 = FALSE in MSM6290 : s0052 = TRUE in MSM6800//reg 027A 02
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_OIFMask 4000
	{0x0F12,0x03C0,0,0},//01E0//#REG_0TC_PCFG_usJpegPacketSize
	{0x0F12,0x0000,0,0},//0000//#REG_0TC_PCFG_usJpegTotalPackets
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uClockInd
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_usFrTimeType
	{0x0F12,0x0001,0,0},//1//#REG_0TC_PCFG_FrRateQualityType
	{0x0F12,0x0535,0,0},//03E8//#REG_0TC_PCFG_usMaxFrTimeMsecMult10//10fps
	{0x0F12,0x029A,0,0},//01C6//#REG_0TC_PCFG_usMinFrTimeMsecMult10//15fps
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_bSmearOutput
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sSaturation
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sSharpBlur
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sColorTemp
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uDeviceGammaIndex
	{0x0F12,0x0003,0,0},//#REG_0TC_PCFG_uPrevMirror
	{0x0F12,0x0003,0,0},//#REG_0TC_PCFG_uCaptureMirror
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uRotation
#if 0
	//================================================================================================
	// SET PREVIEW CONFIGURATION_1
	// # Foramt: YUV422
	// # Size:   640*480
	// # FPS:    7.5-30fps(using video preview)
	//================================================================================================
	{0x002A,0x029C,0,0},
	{0x0F12,0x0280,0,0},//#REG_0TC_PCFG_usWidth//640
	{0x0F12,0x01E0,0,0},//#REG_0TC_PCFG_usHeight//480
	{0x0F12,0x0005,0,0},//#REG_0TC_PCFG_Format 0270
	{0x0F12,0x3AA8,0,0},//157C//3AA8//#REG_0TC_PCFG_usMaxOut4KHzRate 0272
	{0x0F12,0x3A88,0,0},//157C//3A88//#REG_0TC_PCFG_usMinOut4KHzRate 0274
	{0x0F12,0x0100,0,0},//#REG_0TC_PCFG_OutClkPerPix88 0276
	{0x0F12,0x0800,0,0},//#REG_0TC_PCFG_uMaxBpp88 027
	{0x0F12,0x0042,0,0},//0052//#REG_0TC_PCFG_PVIMask//s0050 = FALSE in MSM6290 : s0052 = TRUE in MSM6800//reg 027A 02
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_OIFMask 4000
	{0x0F12,0x03C0,0,0},//01E0//#REG_0TC_PCFG_usJpegPacketSize
	{0x0F12,0x0000,0,0},//0000//#REG_0TC_PCFG_usJpegTotalPackets
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uClockInd
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_usFrTimeType
	{0x0F12,0x0001,0,0},//1//#REG_0TC_PCFG_FrRateQualityType
	{0x0F12,0x0535,0,0},//03E8//#REG_0TC_PCFG_usMaxFrTimeMsecMult10//7.5fps
	{0x0F12,0x014D,0,0},//01C6//#REG_0TC_PCFG_usMinFrTimeMsecMult10//30fps
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_bSmearOutput
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sSaturation
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sSharpBlur
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_sColorTemp
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uDeviceGammaIndex
	{0x0F12,0x0003,0,0},//#REG_0TC_PCFG_uPrevMirror
	{0x0F12,0x0003,0,0},//#REG_0TC_PCFG_uCaptureMirror
	{0x0F12,0x0000,0,0},//#REG_0TC_PCFG_uRotation
	
#endif
	//================================================================================================
	//APPLY PREVIEW CONFIGURATION & RUN PREVIEW
	//================================================================================================
	{0x002A,0x023C,0,0},
	{0x0F12,0x0000,0,0},//#REG_TC_GP_ActivePrevConfig//Select preview configuration_0
	{0x002A,0x0240,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_PrevOpenAfterChange
	{0x002A,0x0230,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_NewConfigSync//Update preview configuration
	{0x002A,0x023E,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_PrevConfigChanged
	{0x002A,0x0220,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_EnablePreview//Start preview
	{0x0F12,0x0001,0,0},//#REG_TC_GP_EnablePreviewChanged
	{0x0028,0xD000,0,0},
	{0x002A,0xb0a0,0,0},
	{0x0F12,0x0000,0,0},
	{0x0028,0x7000,0,0},
	{0x002A,0x0222,0,0},
	{0x0F12,0x0001,0,0},//#REG_TC_GP_EnablePreview//Start preview
	   
	//================================================================================================
	//SET CAPTURE CONFIGURATION_0
	//# Foramt :YUV
	//# Size: 2048*1536
	//# FPS : 5fps
	//================================================================================================
	{0x0028,0x7000,0,0},
	{0x002A,0x035C,0,0},
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_uCaptureModeJpEG
	{0x0F12,0x0800,0,0},//0800//#REG_0TC_CCFG_usWidth
	{0x0F12,0x0600,0,0},//0600//#REG_0TC_CCFG_usHeight
	{0x0F12,0x0005,0,0},//#REG_0TC_CCFG_Format//5:YUV9:JPEG
	{0x0F12,0x3AA8,0,0},//157C//3AA8//#REG_0TC_CCFG_usMaxOut4KHzRate
	{0x0F12,0x3A88,0,0},//157C//3A88//#REG_0TC_CCFG_usMinOut4KHzRate
	{0x0F12,0x0100,0,0},//#REG_0TC_CCFG_OutClkPerPix88
	{0x0F12,0x0800,0,0},//#REG_0TC_CCFG_uMaxBpp88
	{0x0F12,0x0042,0,0},//0052//#REG_0TC_PCFG_PVIMask//s0050 = FALSE in MSM6290 : s0052 = TRUE in MSM6800//reg 02
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_OIFMask
	{0x0F12,0x03C0,0,0},//01E0//#REG_0TC_CCFG_usJpegPacketSize
	{0x0F12,0x0000,0,0},//08fc//#REG_0TC_CCFG_usJpegTotalPackets
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_uClockInd
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_usFrTimeType
	{0x0F12,0x0002,0,0},//2//#REG_0TC_CCFG_FrRateQualityType
	{0x0F12,0x07D0,0,0},//029A//0535//#REG_0TC_CCFG_usMaxFrTimeMsecMult10//5fps
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_usMinFrTimeMsecMult10//10fps
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_bSmearOutput
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_sSaturation
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_sSharpBlur
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_sColorTemp
	{0x0F12,0x0000,0,0},//#REG_0TC_CCFG_uDeviceGammaIndex

       {0xFFFF,0x0014,0,100},
       
	{0x002A,0x020C,0,0},
	{0x0F12,0x0000,0,0},
	{0x0028,0xD000,0,0},
	{0x002A,0x1000,0,0},
	{0x0F12,0x0001,0,0},
	{0x0028,0x7000,0,0},
	};

//image quality setting
static struct msm_camera_i2c_reg_conf s5k5ca_init_iq_settings[] = {
};

static struct msm_camera_i2c_conf_array s5k5ca_init_conf[] = {
	{&s5k5ca_init_settings[0],
	ARRAY_SIZE(s5k5ca_init_settings), 20, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_init_iq_settings[0],
	ARRAY_SIZE(s5k5ca_init_iq_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array s5k5ca_confs[] = {
	{&s5k5ca_snap_settings[0],
	ARRAY_SIZE(s5k5ca_snap_settings), 20, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_prev_30fps_settings[0],
	ARRAY_SIZE(s5k5ca_prev_30fps_settings), 20, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_prev_60fps_settings[0],
	ARRAY_SIZE(s5k5ca_prev_60fps_settings),0 , MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_prev_90fps_settings[0],
	ARRAY_SIZE(s5k5ca_prev_90fps_settings),0 , MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_csi_params s5k5ca_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x14,
};

static struct v4l2_subdev_info s5k5ca_subdev_info[] = {
	{
		.code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt	= 1,
		.order	  = 0,
	}
	/* more can be supported, to be added later */
};

static struct msm_sensor_output_info_t s5k5ca_dimensions[] = {
	{ /* For SNAPSHOT 15fps*/
		.x_output = 0x800,  /*2048*/  /*for 3Mp*/
		.y_output = 0x600,  /*1536*/
		.line_length_pclk = 0x880,//0x880,
		.frame_length_lines = 0x640,//0x640,
		.vt_pixel_clk = 52224000,//17408000,//52224000,
		.op_pixel_clk = 52224000,//17408000,//52224000,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW 30fps*/
		.x_output = 0x280,	/*640*/  /*for 3Mp*/
		.y_output = 0x1E0,	/*480*/
		.line_length_pclk = 0x299,
		.frame_length_lines = 0x1F3,
		.vt_pixel_clk = 60000000,//56000000,
		.op_pixel_clk = 60000000,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW 60 fps*/
		.x_output = 0x320,	/*800*/  /*for 3Mp*/
		.y_output = 0x258,	/*600*/
		.line_length_pclk = 0x400,
		.frame_length_lines = 0x298,
		.vt_pixel_clk = 40796160,
		.op_pixel_clk = 40796160,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW 90 fps*/
		.x_output = 0x320,	/*800*/  /*for 3Mp*/
		.y_output = 0x258,	/*600*/
		.line_length_pclk = 0x400,
		.frame_length_lines = 0x298,
		.vt_pixel_clk = 61194240,
		.op_pixel_clk = 61194240,
		.binning_factor = 0x0,
	},
};

static struct msm_sensor_output_reg_addr_t s5k5ca_reg_addr = {
};

static struct msm_camera_csi_params *s5k5ca_csi_params_array[] = {
	&s5k5ca_csi_params,
	&s5k5ca_csi_params,
	&s5k5ca_csi_params, //60fps
	&s5k5ca_csi_params, //90fps
};

static struct msm_sensor_id_info_t s5k5ca_id_info = {
	.sensor_id_reg_addr = 0x0f12,
	.sensor_id = 0x05ca,
};

static struct msm_sensor_exp_gain_info_t s5k5ca_exp_gain_info = {
	.coarse_int_time_addr = 0x0496,
	.global_gain_addr = 0x0494,
	.vert_offset = 4,
};

static int32_t s5k5ca_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
				uint16_t gain, uint32_t line)
{
	CDBG("s5k5ca_write_exp_gain : Not supported\n");
	return 0;
}

int32_t s5k5ca_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
				struct fps_cfg *fps)
{
	CDBG("s5k5ca_sensor_set_fps: Not supported\n");
	return 0;
}

static const struct i2c_device_id s5k5ca_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k5ca_s_ctrl},
	{ }
};

int32_t s5k5ca_sensor_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);
	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;//offset to avoid i2c conflicts

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	usleep_range(5000, 5100);
	printk(KERN_ALERT "%s:the rc = %d",__func__,rc);
	return rc;
}

static struct i2c_driver s5k5ca_i2c_driver = {
	.id_table = s5k5ca_i2c_id,
	.probe  = s5k5ca_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k5ca_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&s5k5ca_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k5ca_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k5ca_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k5ca_subdev_ops = {
	.core = &s5k5ca_subdev_core_ops,
	.video  = &s5k5ca_subdev_video_ops,
};
static void s5k5ca_power_reset(struct  msm_camera_sensor_info *sensordata);
int32_t s5k5ca_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
//	int value = -1;
	struct msm_camera_sensor_info *info = NULL;
	info = s_ctrl->sensordata;
	if(pwdn) {
	msm_sensor_mclk_enable(s_ctrl, 1);
	msleep(100);
	gpio_direction_output(info->sensor_pwd, 0);	
	
	return 0;
	}
	if(info == NULL){
		printk(KERN_ALERT "####s5k5ca_sensor_power_up is running,but the info is NULL####\n");
		return -1;
	}
	CDBG("%s IN PIN_PWD:%d, PIN_RST:%d\r\n", __func__, info->sensor_pwd, info->sensor_reset);

	rc = gpio_request(info->sensor_pwd,"s5k5ca");

	if(0 == rc){
      		rc = gpio_tlmm_config(GPIO_CFG(info->sensor_pwd, 0,GPIO_CFG_OUTPUT,
                            GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if(rc < 0){
			gpio_free(info->sensor_pwd);
			CDBG("s5k5ca gpio_tlmm_config(%d) fail",info->sensor_pwd);
        		return rc;		
		}
	}

	rc = gpio_request(info->sensor_reset,"s5k5ca");
	if(0 == rc){
      		rc = gpio_tlmm_config(GPIO_CFG(info->sensor_reset, 0,GPIO_CFG_OUTPUT,
                            GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if(rc < 0){
			gpio_free(info->sensor_reset);
			CDBG("s5k5ca gpio_tlmm_config(%d) fail",info->sensor_reset);
        		return rc;		
		}
	}
	gpio_direction_output(info->sensor_pwd, 1);
	gpio_direction_output(info->sensor_reset, 0);

	usleep_range(10000, 11000);
	if (info->pmic_gpio_enable) {
	//	lcd_camera_power_onoff(1);
	}
	usleep_range(10000, 11000);

	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}

	usleep_range(1000, 1100);
	/* turn on ldo and vreg */
	gpio_direction_output(info->sensor_pwd, 0);
//	gpio_set_value(info->sensor_pwd, 1);
//	value = gpio_get_value(info->sensor_pwd);
//	printk(KERN_ALERT "The info->sensor_pwd is %d\n",value);
	msleep(20);
	gpio_direction_output(info->sensor_reset, 1);
//	gpio_set_value(info->sensor_reset, 1);
	msleep(1);
//	value = gpio_get_value(info->sensor_reset);
//	printk(KERN_ALERT "The info->sensor_reset is %d\n",value);
//	gpio_set_value(info->sensor_reset, 0);
//	msleep(1);
//	value = gpio_get_value(info->sensor_reset);
//	printk(KERN_ALERT "The info->sensor_reset is %d\n",value);
//	gpio_set_value(info->sensor_reset, 1);
//	msleep(1);
//	value = gpio_get_value(info->sensor_reset);
//	printk(KERN_ALERT "The info->sensor_reset is %d\n",value);
	msleep(25);
//	s5k5ca_power_reset(info);
//	while(1);
	pwdn = 1;
	return rc;
}

int32_t s5k5ca_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	struct msm_camera_sensor_info *info = NULL;
printk(KERN_ALERT "#########ffff##########\n");
	CDBG("%s IN\r\n", __func__);
	info = s_ctrl->sensordata;
	gpio_direction_output(info->sensor_pwd, 1);
	msleep(100);
	msm_sensor_mclk_enable(s_ctrl, 0);

	return rc;
}

/* ***********************match_id start************************** */
static int s5k5ca_i2c_txdata(struct msm_camera_i2c_client * client,
				u16 saddr,
				u8 *txdata,
				int length)
{
	unsigned short loacal_saddr = saddr >> 1;
	struct i2c_msg msg[] = {
		{
			.addr  = loacal_saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(client->client->adapter, msg, 1) < 0)
		return -EIO;
	else
		return 0;
}

static int s5k5ca_i2c_rxdata(struct msm_camera_i2c_client * client,
				unsigned short saddr,
				unsigned char *rxdata,
				int length)
{
	unsigned short loacal_saddr = saddr >> 1;
	struct i2c_msg msgs[] = {
		{
			.addr   = loacal_saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr   = loacal_saddr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxdata,
		},
	};

	if (i2c_transfer(client->client->adapter, msgs, 2) < 0) {
		CDBG("s5k5ca_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t s5k5ca_i2c_read_word(struct msm_camera_i2c_client * client,
				unsigned short saddr,
				unsigned int raddr,
				unsigned int *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4] = {0};

	if (!rdata)
	return -EIO;

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = s5k5ca_i2c_rxdata(client, saddr, buf, 2);
	if (rc < 0) {
		CDBG("s5k5ca_i2c_read_word failed!\n");
		return rc;
	}
	*rdata = buf[0] << 8 | buf[1];

	return rc;
}

static int s5k5ca_i2c_write_word(struct msm_camera_i2c_client * client,
				unsigned short saddr,
				unsigned int waddr,
				unsigned int wdata)
{
	int rc = -EIO;
	int counter = 0;
	unsigned char buf[4] = {0};
	buf[0] = (waddr & 0xFF00)>>8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00)>>8;
	buf[3] = (wdata & 0x00FF);

	while ((counter < 10) &&(rc != 0))
	{
		rc = s5k5ca_i2c_txdata(client, saddr, buf, 4);
		CDBG("s5k5ca_i2c_write_word, Reg(0x%04x) = 0x%04x, rc=%d!\n",waddr, wdata,rc);
		if (rc < 0)
		{
			counter++;
			CDBG("s5k5ca_i2c_txdata failed,Reg(0x%04x) = 0x%04x, rc=%d!\n", waddr, wdata,rc);
		}
	}

	return rc;
}

static int s5k5ca_sensor_match_id(struct msm_sensor_ctrl_t  *s_ctrl)
{
	int rc = 0;
	uint model_id = 0;
	CDBG("=========%s s_ctrl->sensor_i2c_client->client->addr = 0x%x \n",__func__,s_ctrl->sensor_i2c_client->client->addr);
	printk(KERN_ALERT "=========%s s_ctrl->sensor_i2c_client->client->addr = 0x%x \n",__func__,s_ctrl->sensor_i2c_client->client->addr);
	rc = s5k5ca_i2c_write_word(s_ctrl->sensor_i2c_client, s_ctrl->sensor_i2c_client->client->addr,0x0028,0xD000);
//	rc = s5k5ca_i2c_write_word(s_ctrl->sensor_i2c_client, s_ctrl->sensor_i2c_client->client->addr,0x002E,0x0040);
	rc = s5k5ca_i2c_read_word(s_ctrl->sensor_i2c_client, s_ctrl->sensor_i2c_client->client->addr, 0xd0001006, &model_id);
	if ((rc < 0)||(model_id != 0x05ca)){
		CDBG("s5k5ca_read_model_id model_id(0x%04x) mismatch!\n",model_id);
		return -ENODEV;
	}
	CDBG("s5k5ca_read_model_id model_id(0x%04x) match success!\n",model_id);

	return rc;
}
/* ***********************match_id end************************** */

static void s5k5ca_power_reset(struct  msm_camera_sensor_info *sensordata)
{
	CDBG("%s\n",__func__);
	gpio_set_value(sensordata->sensor_reset, 1);
	msleep(1);
	gpio_set_value(sensordata->sensor_reset, 0);
	msleep(1);
	gpio_set_value(sensordata->sensor_reset, 1);
	msleep(1);
}

int32_t s5k5ca_msm_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
				int update_type, int res)
{
	int32_t rc = 0;

	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");

		s5k5ca_power_reset(s_ctrl->sensordata);

		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);

		if (!csi_config) {
			s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
			msleep(20);

			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CSIC_CFG, s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(20);
			csi_config = 1;
			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		}

		msm_sensor_write_conf_array(
				s_ctrl->sensor_i2c_client,
				s_ctrl->msm_sensor_reg->mode_settings, res);
		msleep(10);

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_PCLK_CHANGE,
				&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
		msleep(20);
	}
	return rc;
}

static struct msm_sensor_fn_t s5k5ca_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = s5k5ca_sensor_set_fps,

	.sensor_write_exp_gain = s5k5ca_write_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k5ca_write_exp_gain,

	.sensor_csi_setting = s5k5ca_msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k5ca_sensor_power_up,
	.sensor_power_down = s5k5ca_sensor_power_down,
	.sensor_match_id = s5k5ca_sensor_match_id,
};

static struct msm_sensor_reg_t s5k5ca_regs = {
	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.start_stream_conf = NULL,
	.start_stream_conf_size = 0,
	.stop_stream_conf = NULL,
	.stop_stream_conf_size = 0,
	.group_hold_on_conf = NULL,
	.group_hold_on_conf_size = 0,
	.group_hold_off_conf = NULL,
	.group_hold_off_conf_size = 0,
	.init_settings = &s5k5ca_init_conf[0],
	.init_size = ARRAY_SIZE(s5k5ca_init_conf),
	.mode_settings = &s5k5ca_confs[0],
	.output_settings = &s5k5ca_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k5ca_confs),
};

static struct msm_sensor_ctrl_t s5k5ca_s_ctrl = {
	.msm_sensor_reg = &s5k5ca_regs,
	.sensor_i2c_client = &s5k5ca_sensor_i2c_client,
	.sensor_i2c_addr =  0x5a,
	.sensor_output_reg_addr = &s5k5ca_reg_addr,
	.sensor_id_info = &s5k5ca_id_info,
	.sensor_exp_gain_info = &s5k5ca_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &s5k5ca_csi_params_array[0],
	.msm_sensor_mutex = &s5k5ca_mut,
	.sensor_i2c_driver = &s5k5ca_i2c_driver,
	.sensor_v4l2_subdev_info = s5k5ca_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k5ca_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k5ca_subdev_ops,
	.func_tbl = &s5k5ca_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision WXGA YUV sensor driver");
MODULE_LICENSE("GPL v2");

