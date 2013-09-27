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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_ili9486.h"
#include <mach/gpio.h>

static struct msm_panel_common_pdata *mipi_ili9486_pdata;
static struct dsi_buf ili9486_tx_buf;
static struct dsi_buf ili9486_rx_buf;

static int mipi_ili9486_bl_ctrl;
static char LCD_in_sleep = 0; //add by dupeng for lcd sleep mode 2013-04-20
#define ILI9486_SLEEP_OFF_DELAY 150
#define ILI9486_DISPLAY_ON_DELAY 150

//#define GAMMA_ZHOU
//#define GAMMA_DU_130325
//#define GAMMA_DU_130327
#define GAMMA_DU_130401


/* common setting */
#ifdef GAMMA_ZHOU
	//static char flag_for_define = 1;
	static char exit_sleep[2] = {0x11, 0x00};
	static char display_on[2] = {0x29, 0x00};
	static char display_off[2] = {0x28, 0x00};
	static char enter_sleep[2] = {0x10, 0x00};
	static char deep_sleep[2] = {0xb7, 0x08};
	static char write_ram[2] = {0x2c, 0x00}; /* write ram */
	static char tear_effect_on[2] = {0x35, 0x00}; /* tear effect */

	
	static char cmd0[9] = {
		0xF1, 0x36, 0x04, 0x00,
		0x3c, 0x0f,0x0f,0xa4,0x02,
	};

	static char cmd1[10] = {
		0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
	};

	static char cmd2[7] = {
		0xf7, 0xa9, 0x91, 0x2d,0x8a,0x4c,0x00,    /////
	};

	static char cmd3[3] = {
		0xf8, 0x21, 0x04,
	};
	static char cmd4[3] = {
		0xf9, 0x00, 0x08,
	};

	static char cmd5[6] = {
		0xf4, 0x00, 0x00, 0x08, 0x91, 0x04,
	};

	//display inversion
	static char cmd6[2] = {
		0xb4, 0x02
	};

	static char cmd7[3] = {
		0xb6, 0x22, 0x02,
	};

	static char cmd8[2] = {
		0xb7, 0x86, 
	};

	static char cmd9[3] = {
		0xc0, 0x0f, 0x0f,
	};

	static char cmd10[2] = {
		0xc1, 0x41, 
	};
	static char cmd11[2] = {
		0xc2, 0x22, 
	};

	static char cmd12[3] = {
		0xc5, 0x00, 0x47,
	};

	static char cmd13[16] = {
		0xE0, 
	    0x0F,//VP63[3:0]//white ++ 
	    0x34,//VP62[5:0]// 
	    0x33,//VP61[5:0]// 
	    0x06,//VP59[3:0]// 
	    0x03,//VP57[4:0] 
	    0x03, //VP50[3:0] 
	    0x4b,//VP43[6:0]//23 
	    0x66,//VP27[3:0];VP36[3:0] 
	    0x2c,//VP20[6:0] 
	    0x0e,//VP13[3:0] 
	    0x08,//VP6[4:0] 
	    0x03,//VP4[3:0] 
	    0x01,//VP2[5:0] 
	    0x00,//VP1[5:0] 
	    0x00, //VP0[3:0]//black
	};

	static char cmd14[16] = {
	    0xE1,
	    0x0F,//VN0[3:0]//black -- 
	    0x3F, //VN1[5:0] 
	    0x3A,//VN2[5:0]
	    0x09,//VN4[3:0] 
	    0x07,//VN6[4:0]
	    0x01,//VN13[3:0] 
	    0x3d,//VN20[6:0] 
	    0x69,//VN36[3:0];VN27[3:0] 
	    0x38,//VN43[6:0] 
	    0x06,//VN50[3:0] 
	    0x0D,//VN57[4:0] 
	    0x06,//VN59[3:0] 
	    0x25,//VN61[5:0] 
	    0x1e,//VN62[5:0] 
	    0x00,//VN63[3:0]//white --
	};

	static char cmd15[5] = {
		0x2A,0x00,0x00 ,0x01,0x3F,
	};
	static char cmd16[5] = {
		0x2b,0x00,0x00 ,0x01,0xDF,
	};

	//gamma disable
	static char cmd17[2] = {
		0x3a, 0x66, 
	};

	//display inversion
	static char cmd18[1] = {
		0x20, 
	};

	static char cmd19_rotate[3] = {
		0xB1, 0xb0, 0x11,
	};

	static char config_MADCTL[2] = {0x36, 0x48}; //40


	static struct dsi_cmd_desc ili9486_cmd_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd1), cmd1},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd2), cmd2},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd4), cmd4},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd5), cmd5},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(config_MADCTL), config_MADCTL},        
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd6), cmd6},
	   	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(cmd19_rotate), cmd19_rotate},        
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd7), cmd7},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd12), cmd12},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd13), cmd13},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17}, 
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},  
	    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(tear_effect_on), tear_effect_on},
	    
	   	{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(write_ram), write_ram},
	};
		
#elif defined GAMMA_DU_130325
	//static char flag_for_define = 2;
	static char exit_sleep[1] = {0x11};
	static char display_on[1] = {0x29};
	static char display_off[2] = {0x28, 0x00};
	static char enter_sleep[2] = {0x10, 0x00};
	static char write_ram[1] = {0x2c}; /* write ram */
	static char tear_effect_on[2] = {0x35, 0x00}; /* tear effect */

	static char cmd0[7] = {
		0xF1, 0x36, 0x04, 0x00,
		0x3c, 0x0f,0x8f,
	};

	static char cmd1[10] = {
		0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
	};

	static char cmd2[5] = {
		0xf7, 0xa9, 0x91, 0x2d,0x8a,	  /////
	};

	static char cmd3[3] = {
		0xf8, 0x21, 0x04,
	};
	static char cmd4[3] = {
		0xf9, 0x00, 0x08,
	};

	//display inversion
	static char cmd6[2] = {
		0xb4, 0x02
	};

	static char cmd7[4] = {
		0xb6, 0x02, 0x02, 0x3b,
	};

	static char cmd8[2] = {
		0xb7, 0xc6, 
	};

	static char cmd9[3] = {
		0xc0, 0x10, 0x10,
	};

	static char cmd10[2] = {
		0xc1, 0x41, 
	};
	static char cmd11[2] = {
		0xc2, 0x22, 
	};

	static char cmd12[3] = {
		0xc5, 0x00, 0x41,
	};

	static char cmd13[16] = {
		0xE0, 
		0x0F,//VP63[3:0]//white ++ 
		0x22,//VP62[5:0]// 
		0x1d,//VP61[5:0]// 
		0x0c,//VP59[3:0]// 
		0x10,//VP57[4:0] 
		0x08, //VP50[3:0] 
		0x53,//VP43[6:0]//23 
		0x67,//VP27[3:0];VP36[3:0] 
		0x46,//VP20[6:0] 
		0x0a,//VP13[3:0] 
		0x17,//VP6[4:0] 
		0x0c,//VP4[3:0] 
		0x13,//VP2[5:0] 
		0x0a,//VP1[5:0] 
		0x00, //VP0[3:0]//black
	};

	static char cmd14[16] = {
		0xE1,
		0x0F,//VN0[3:0]//black -- 
		0x35, //VN1[5:0] 
		0x2c,//VN2[5:0]
		0x0d,//VN4[3:0] 
		0x0c,//VN6[4:0]
		0x03,//VN13[3:0] 
		0x49,//VN20[6:0] 
		0x43,//VN36[3:0];VN27[3:0] 
		0x37,//VN43[6:0] 
		0x07,//VN50[3:0] 
		0x0f,//VN57[4:0] 
		0x03,//VN59[3:0] 
		0x21,//VN61[5:0] 
		0x1e,//VN62[5:0] 
		0x00,//VN63[3:0]//white --
	};

	static char cmd15[5] = {
		0x2A,0x00,0x00 ,0x01,0x3F,
	};
	static char cmd16[5] = {
		0x2b,0x00,0x00 ,0x01,0xDF,
	};

	//gamma disable
	static char cmd17[2] = {
		0x3a, 0x66, 
	};

	//display inversion
	static char cmd18[1] = {
		0x20, 
	};

	static char cmd19_rotate[3] = {
		0xB1, 0xa0, 0x11,
	};

	static char config_MADCTL[2] = {0x36, 0x48}; //40

	static struct dsi_cmd_desc ili9486_cmd_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd2), cmd2},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(cmd19_rotate), cmd19_rotate},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd4), cmd4},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(config_MADCTL), config_MADCTL}, 
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd13), cmd13},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd12), cmd12},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd7), cmd7},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd1), cmd1},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(tear_effect_on), tear_effect_on},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd6), cmd6},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
		
		{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(write_ram), write_ram},
			
	};
	
#elif defined GAMMA_DU_130327
	//static char flag_for_define = 3;
	static char exit_sleep[2] = {0x11, 0x00};
	static char display_on[2] = {0x29, 0x00};
	static char display_off[2] = {0x28, 0x00};
	static char enter_sleep[2] = {0x10, 0x00};
	static char write_ram[2] = {0x2c, 0x00}; /* write ram */
	static char tear_effect_on[2] = {0x35, 0x00}; /* tear effect */

	static char cmd0[9] = {
		0xF1, 0x36, 0x04, 0x00,
		0x3c, 0x0f, 0x0f, 0xa4, 0x02
	};

	static char cmd1[10] = {
		0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
	};

	static char cmd2[7] = {
		0xf7, 0xa9, 0x91, 0x2d, 0x8a, 0x4c, 0x00,	  /////
	};

	static char cmd3[3] = {
		0xf8, 0x21, 0x04,
	};
	static char cmd4[3] = {
		0xf9, 0x00, 0x08,
	};

	static char cmd5[6] = {
		0xf4, 0x00, 0x00, 0x08, 0x91, 0x04,
	};

	static char config_MADCTL[2] = {0x36, 0x48};
	
	//display inversion
	static char cmd6[2] = {
		0xb4, 0x02
	};

	static char cmd19_rotate[3] = {
		0xB1, 0xa0, 0x11,
	};

	static char cmd7[3] = {
		0xb6, 0x22, 0x02, 
	};

	static char cmd8[2] = {
		0xb7, 0x86, 
	};

	static char cmd9[3] = {
		0xc0, 0x0f, 0x0f,
	};

	static char cmd10[2] = {
		0xc1, 0x41, 
	};

	static char cmd11[2] = {
		0xc2, 0x22, 
	};
	
	static char cmd12[3] = {
		0xc5, 0x00, 0x41,
	};

	static char cmd13[16] = {
		0xE0, 
		0x0F,//VP63[3:0]//white ++ 
		0x34,//VP62[5:0]// 
		0x33,//VP61[5:0]// 
		0x06,//VP59[3:0]// 
		0x03,//VP57[4:0] 
		0x03, //VP50[3:0] 
		0x4B,//VP43[6:0]//23 
		0x66,//VP27[3:0];VP36[3:0] 
		0x2C,//VP20[6:0] 
		0x0E,//VP13[3:0] 
		0x08,//VP6[4:0] 
		0x03,//VP4[3:0] 
		0x01,//VP2[5:0] 
		0x00,//VP1[5:0] 
		0x00, //VP0[3:0]//black
	};

	static char cmd14[16] = {
		0xE1,
		0x0F,//VN0[3:0]//black -- 
		0x3F, //VN1[5:0] 
		0x3A,//VN2[5:0]
		0x09,//VN4[3:0] 
		0x07,//VN6[4:0]
		0x01,//VN13[3:0] 
		0x3D,//VN20[6:0] 
		0x69,//VN36[3:0];VN27[3:0] 
		0x38,//VN43[6:0] 
		0x06,//VN50[3:0] 
		0x0D,//VN57[4:0] 
		0x06,//VN59[3:0] 
		0x25,//VN61[5:0] 
		0x1E,//VN62[5:0] 
		0x00,//VN63[3:0]//white --
	};

	static char cmd15[5] = {
		0x2A,0x00,0x00 ,0x01,0x3F,
	};

	static char cmd16[5] = {
		0x2b,0x00,0x00 ,0x01,0xDF,
	};

	//gamma disable
	static char cmd17[2] = {
		0x3a, 0x66, 
	};

	//display inversion
	static char cmd18[2] = {
		0x20, 0x00,
	};

	static struct dsi_cmd_desc ili9486_cmd_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd0), cmd0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd1), cmd1},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd2), cmd2},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd3), cmd3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd4), cmd4},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd5), cmd5},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(config_MADCTL), config_MADCTL},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd6), cmd6},
		{DTYPE_DCS_WRITE, 1, 0, 0, 10,	sizeof(cmd19_rotate), cmd19_rotate},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd7), cmd7},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd8), cmd8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd9), cmd9},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd10), cmd10},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd11), cmd11},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd12), cmd12},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd13), cmd13},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd14), cmd14},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd15), cmd15},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd16), cmd16},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd17), cmd17},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(tear_effect_on), tear_effect_on},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
		{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(write_ram), write_ram},
	};
#elif defined GAMMA_DU_130401
#if 0
	//static char flag_for_define = 4;
	static char display_off[2] = {0x28, 0x00};
	static char enter_sleep[2] = {0x10, 0x00};
	
	static char cmd0[7] = {
		0xF1, 0x36, 0x04, 0x00,
		0x3c, 0x0f,0x8f,
	};

	static char cmd2[5] = {
		0xf7, 0xa9, 0x91, 0x2d,0x8a,	  /////
	};

	static char cmd19_rotate[3] = {
		0xB1, 0xB0, 0x11,
	};

	static char cmd9[3] = {
		0xc0, 0x10, 0x10,
	};

	static char cmd4[3] = {
		0xf9, 0x00, 0x08,
	};

	static char config_MADCTL[2] = {0x36, 0x48}; //40

	static char cmd13[16] = {
		0xE0, 
		0x0F,//VP63[3:0]//white ++ 
		0x22,//VP62[5:0]// 
		0x1d,//VP61[5:0]// 
		0x0c,//VP59[3:0]// 
		0x10,//VP57[4:0] 
		0x08, //VP50[3:0] 
		0x53,//VP43[6:0]//23 
		0x67,//VP27[3:0];VP36[3:0] 
		0x46,//VP20[6:0] 
		0x0a,//VP13[3:0] 
		0x17,//VP6[4:0] 
		0x0c,//VP4[3:0] 
		0x13,//VP2[5:0] 
		0x0a,//VP1[5:0] 
		0x00, //VP0[3:0]//black
	};

	static char cmd14[16] = {
		0xE1,
		0x0F,//VN0[3:0]//black -- 
		0x35, //VN1[5:0] 
		0x2c,//VN2[5:0]
		0x0d,//VN4[3:0] 
		0x0c,//VN6[4:0]
		0x03,//VN13[3:0] 
		0x49,//VN20[6:0] 
		0x43,//VN36[3:0];VN27[3:0] 
		0x37,//VN43[6:0] 
		0x07,//VN50[3:0] 
		0x0f,//VN57[4:0] 
		0x03,//VN59[3:0] 
		0x21,//VN61[5:0] 
		0x1e,//VN62[5:0] 
		0x00,//VN63[3:0]//white --
	};

	static char cmd15[5] = {
		0x2A,0x00,0x00 ,0x01,0x3F,
	};
	
		static char cmd16[5] = {
		0x2b,0x00,0x00 ,0x01,0xDF,
	};

	//gamma disable
	static char cmd17[2] = {
		0x3a, 0x66, 
	};

	static char cmd10[2] = {
		0xc1, 0x41, 
	};

	static char cmd11[2] = {
		0xc2, 0x22, 
	};

	static char cmd3[3] = {
			0xf8, 0x21, 0x04,
	};

	static char cmd12[3] = {
		0xc5, 0x00, 0x41,
	};
	
	static char cmd7[4] = {
		0xb6, 0x02, 0x02, 0x3b,
	};

	static char cmd1[10] = {
		0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
	};
		
	static char tear_effect_on[2] = {0x35, 0x00}; /* tear effect */

	static char cmd8[2] = {
		0xb7, 0xc6, 
	};

	//display inversion
	static char cmd6[2] = {
		0xb4, 0x02
	};
	
	//display inversion
	static char cmd18[2] = {
		0x20, 0x00,
	};

	static char exit_sleep[2] = {0x11, 0x00};
	static char display_on[2] = {0x29, 0x00};
	static char write_ram[2] = {0x2c, 0x00}; /* write ram */

	static struct dsi_cmd_desc ili9486_cmd_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd0), cmd0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd2), cmd2},
		{DTYPE_DCS_WRITE, 1, 0, 0, 10,	sizeof(cmd19_rotate), cmd19_rotate},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd9), cmd9},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd4), cmd4},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(config_MADCTL), config_MADCTL}, 
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd13), cmd13},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd14), cmd14},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd15), cmd15},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd16), cmd16},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd17), cmd17},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd10), cmd10},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd11), cmd11},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd12), cmd12},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd7), cmd7},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd1), cmd1},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(tear_effect_on), tear_effect_on},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd8), cmd8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd6), cmd6},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
		{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(write_ram), write_ram},
			
	};
#endif 
//static char flag_for_define = 4;
	static char display_off[2] = {0x28, 0x00};
	static char enter_sleep[2] = {0x10, 0x00};

	static char cmd0[7] = {
		0xF1, 0x36, 0x04, 0x00,
		0x3c, 0x0f,0x8f,
	};

	static char cmd2[5] = {
		0xf7, 0xa9, 0x91, 0x2d,0x8a,	  /////
	};

	static char cmd19_rotate[3] = {
		0xB1, 0xB0, 0x11,
	};

	static char cmd9[3] = {
		0xc0, 0x10, 0x10,
	};

	static char cmd4[3] = {
		0xf9, 0x00, 0x08,
	};

	static char config_MADCTL[2] = {0x36, 0x08}; //40

	static char cmd13[16] = {
		0xE0,
		0x0F,//VP63[3:0]//white ++
		0x22,//VP62[5:0]//
		0x1d,//VP61[5:0]//
		0x0c,//VP59[3:0]//
		0x10,//VP57[4:0]
		0x08, //VP50[3:0]
		0x53,//VP43[6:0]//23
		0x67,//VP27[3:0];VP36[3:0]
		0x46,//VP20[6:0]
		0x0a,//VP13[3:0]
		0x17,//VP6[4:0]
		0x0c,//VP4[3:0]
		0x13,//VP2[5:0]
		0x0a,//VP1[5:0]
		0x00, //VP0[3:0]//black
	};

	static char cmd14[16] = {
		0xE1,
		0x0F,//VN0[3:0]//black --
		0x35, //VN1[5:0]
		0x2c,//VN2[5:0]
		0x0d,//VN4[3:0]
		0x0c,//VN6[4:0]
		0x03,//VN13[3:0]
		0x49,//VN20[6:0]
		0x43,//VN36[3:0];VN27[3:0]
		0x37,//VN43[6:0]
		0x07,//VN50[3:0]
		0x0f,//VN57[4:0]
		0x03,//VN59[3:0]
		0x21,//VN61[5:0]
		0x1e,//VN62[5:0]
		0x00,//VN63[3:0]//white --
	};

	static char cmd15[5] = {
		0x2A,0x00,0x00 ,0x01,0x3F,
	};

		static char cmd16[5] = {
		0x2b,0x00,0x00 ,0x01,0xDF,
	};

	//gamma disable
	static char cmd17[2] = {
		0x3a, 0x66,
	};

	static char cmd10[2] = {
		0xc1, 0x41,
	};

	static char cmd11[2] = {
		0xc2, 0x22,
	};

	static char cmd3[3] = {
			0xf8, 0x21, 0x04,
	};

	static char cmd12[3] = {
		0xc5, 0x00, 0x41,
	};

	static char cmd7[4] = {
		0xb6, 0x02, 0x22, 0x3b,
	};

	static char cmd1[10] = {
		0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
	};

	static char tear_effect_on[2] = {0x35, 0x00}; /* tear effect */
	static char tear_effect_modify[3] = {0x44, 0x00, 0x3f};

	static char cmd8[2] = {
		0xb7, 0xc6,
	};

	//display inversion
	static char cmd6[2] = {
		0xb4, 0x02
	};

	//display inversion
	static char cmd18[2] = {
		0x20, 0x00,
	};

	static char exit_sleep[2] = {0x11, 0x00};
	static char display_on[2] = {0x29, 0x00};
	static char write_ram[2] = {0x2c, 0x00}; /* write ram */

	static struct dsi_cmd_desc ili9486_cmd_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd0), cmd0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd2), cmd2},
		{DTYPE_DCS_WRITE, 1, 0, 0, 10,	sizeof(cmd19_rotate), cmd19_rotate},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd9), cmd9},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd4), cmd4},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(config_MADCTL), config_MADCTL},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd13), cmd13},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd14), cmd14},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd15), cmd15},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd16), cmd16},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd17), cmd17},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd10), cmd10},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd11), cmd11},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd12), cmd12},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd7), cmd7},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd1), cmd1},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(tear_effect_on), tear_effect_on},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(tear_effect_modify), tear_effect_modify},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd8), cmd8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(cmd6), cmd6},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
		{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
		{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(write_ram), write_ram},

	};	
#endif

static struct dsi_cmd_desc ili9486_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
    /* zhaiqingbo modify  enter_sleep from DTYPE_DCS_WRITE to DTYPE_GEN_WRITE, DTYPE_DCS_WRITE is not valid*/
	//{DTYPE_GEN_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep},
    //{DTYPE_GEN_WRITE, 1, 0, 0, 120, sizeof(deep_sleep), deep_sleep},  
};

static struct dsi_cmd_desc ili9486_cmd_display_on_cmds_rotate[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50,	sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(write_ram), write_ram},
};




static char video0[9] = {
	0xF1, 0x36, 0x04, 0x00,
	0x3c, 0x0f,0x0f,0xa4,0x02,
};
static char video1[10] = {
	0xf2, 0x18, 0xa3, 0x12,0x02,0xb2,0x12,0xff,0x10,0x00,
};
static char video2[7] = {
	0xf7, 0xa9, 0x91, 0x2d,0x8a,0x4c,0x00,
};
static char video3[3] = {
	0xf8, 0x21, 0x04,
};
static char video4[3] = {
	0xf9, 0x00, 0x08,
};

static char video5[6] = {
	0xf4, 0x00, 0x00,0x08,0x91,0x04,
};
static char video6[3] = {
	0xb6, 0x22, 0x02,
};
static char video7[2] = {
	0xb7, 0x86, 
};
static char video8[3] = {
	0xc0, 0x0f, 0x0f,
};
static char video9[2] = {
	0xc1, 0x41, 
};
static char video10[2] = {
	0xc2, 0x22, 
};
static char video11[3] = {
	0xc5, 0x00, 0x47,
};
//gamma disable
static char video12[2] = {
	0x3a, 0x77, 
};

static char config_video_MADCTL[2] = {0x36, 0xe0};
//static char config_video_MADCTL[2] = {0x36, 0xC0};
static struct dsi_cmd_desc ili9486_video_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video0), video0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video1), video1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video2), video2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video3), video3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video4), video4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video5), video5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video6), video6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video7), video7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video8), video8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video9), video9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video10), video10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video11), video11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video12), video12},

    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(tear_effect_on), tear_effect_on},
        
	{DTYPE_DCS_WRITE, 1, 0, 0, ILI9486_SLEEP_OFF_DELAY, sizeof(exit_sleep),
			exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, ILI9486_DISPLAY_ON_DELAY, sizeof(display_on),
			display_on},
};

static struct dsi_cmd_desc ili9486_video_display_on_cmds_rotate[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 150,
		sizeof(config_video_MADCTL), config_video_MADCTL},
};
#define ILI9486_PANEL_RESET 85    //add by malei for beetle lite at 2013.1.16

void __gpio_set_value(unsigned gpio, int value);
static int mipi_ili9486_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	static int rotate;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;
    //printk("mipi_ili9486_lcd_on Enter----------------*\n");
	mipi  = &mfd->panel_info.mipi;
    
	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}
	
	//dupeng add for lcd sleep 2013/04/19
    if(LCD_in_sleep) {
		//mipi_dsi1_ulps_mode(0);
		mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf,
				ili9486_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(ili9486_cmd_display_on_cmds_rotate));
		
		LCD_in_sleep = 0;
		//printk("mipi_ili9486_lcd_on ----------------\n");
		return 0;
		
	}
	//dupeng add end
    //add by dupeng@tcl.com for bug 438313 (display a flick screen after alcatel one touch screen when power off charge) 
    if (mipi_ili9486_pdata && mipi_ili9486_pdata->pmic_backlight){
		//printk("dupeng set backlight 0.\n");
		mipi_ili9486_pdata->pmic_backlight(0);
    	}
    //add by dupeng end
    //reset	
	gpio_set_value(ILI9486_PANEL_RESET, 1);
	msleep(10);
	gpio_set_value(ILI9486_PANEL_RESET, 0);
	msleep(20);
	gpio_set_value(ILI9486_PANEL_RESET, 1);
	msleep(100);

	if (mipi_ili9486_pdata && mipi_ili9486_pdata->rotate_panel)
		rotate = mipi_ili9486_pdata->rotate_panel();

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf,
			ili9486_video_display_on_cmds,
			ARRAY_SIZE(ili9486_video_display_on_cmds));

		if (rotate) {
			mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf,
				ili9486_video_display_on_cmds_rotate,
			ARRAY_SIZE(ili9486_video_display_on_cmds_rotate));
		}
	} else if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf,
			ili9486_cmd_display_on_cmds,
			ARRAY_SIZE(ili9486_cmd_display_on_cmds));

		//printk("dp test flag_for_define:%d******************----------------\n", flag_for_define);

		if (rotate) {
			mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf,
				ili9486_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(ili9486_cmd_display_on_cmds_rotate));
			//printk("mipi_ili9486_lcd_on is rotate----------------\n");
		}
	}
    //printk("mipi_ili9486_lcd_on Exit----------------\n");
	return 0;
}

static int mipi_ili9486_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	//printk("mipi_ili9486_lcd_off Enter\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &ili9486_tx_buf, ili9486_display_off_cmds,
			ARRAY_SIZE(ili9486_display_off_cmds));

	//dupeng add for lcd sleep 2013/04/19
	
	if(!LCD_in_sleep)
	{
		//mipi_dsi1_ulps_mode(1);
		LCD_in_sleep = 1;
	}
	//dupeng add end
	
	//printk("mipi_ili9486_lcd_off Exit\n");
	return 0;
}

static ssize_t mipi_ili9486_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_ili9486_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_ili9486_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_ili9486_wta_bl_ctrl);

static struct attribute *mipi_ili9486_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_ili9486_fs_attr_group = {
	.attrs = mipi_ili9486_fs_attrs,
};

static int mipi_ili9486_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_ili9486_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}

static int __devinit mipi_ili9486_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	printk("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_ili9486_pdata = pdev->dev.platform_data;
		if (mipi_ili9486_pdata->bl_lock)
			spin_lock_init(&mipi_ili9486_pdata->bl_spinlock);
		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);
	mipi_ili9486_create_sysfs(pthisdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_ili9486_lcd_probe,
	.driver = {
		.name   = "mipi_ili9486",
	},
};

static void mipi_ili9486_set_backlight(struct msm_fb_data_type *mfd)
{
	int ret = -EPERM;
	int bl_level;

	bl_level = mfd->bl_level;
	//printk("mipi_ili9486_set_backlight:%x.\n", bl_level);
	if (mipi_ili9486_pdata && mipi_ili9486_pdata->pmic_backlight)
		ret = mipi_ili9486_pdata->pmic_backlight(bl_level);
	else
		pr_err("%s(): Backlight level set failed", __func__);
}
static struct msm_fb_panel_data ili9486_panel_data = {
	.on	= mipi_ili9486_lcd_on,
	.off = mipi_ili9486_lcd_off,
	.set_backlight = mipi_ili9486_set_backlight,
};

static int ch_used[3];

static int mipi_ili9486_lcd_init(void)
{
	mipi_dsi_buf_alloc(&ili9486_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&ili9486_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_ili9486_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_ili9486_lcd_init();
	if (ret) {
		pr_err("mipi_ili9486_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_ili9486", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	ili9486_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &ili9486_panel_data,
				sizeof(ili9486_panel_data));
	if (ret) {
		printk("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
