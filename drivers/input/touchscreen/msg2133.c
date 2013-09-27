
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>


//-------------------------------------------------------------------
//-------------------------------------------------------------------
#define u8         unsigned char
#define u32        unsigned int
#define s32        signed int

#define REPORT_PACKET_LENGTH    (8)
#define MSG21XX_INT_GPIO       (48)
#define MSG21XX_RESET_GPIO     (35)//(92) modified by yangshujuan for beetle at 2013-1-18
#define MS_TS_MSG21XX_X_MAX   (320)
#define MS_TS_MSG21XX_Y_MAX   (480)



static int msg21xx_irq = 0;
static struct i2c_client     *msg21xx_i2c_client;
static struct work_struct    msg21xx_wq;
static struct early_suspend  early_suspend;
static struct input_dev     *input = NULL;

struct msg21xx_ts_data
{
	uint16_t            addr;
	struct i2c_client  *client;
	struct input_dev   *input_dev;
	int                 use_irq;
	struct work_struct  work;
	int (*power)(int on);
	int (*get_int_status)(void);
	void (*reset_ic)(void);
};


#define MAX_TOUCH_FINGER 2
typedef struct
{
    u16 X;
    u16 Y;
} TouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    TouchPoint_t Point[MAX_TOUCH_FINGER];
} TouchScreenInfo_t;

//---------------------------------------------------------------------------
//#define	__FIRMWARE_UPDATE__
#ifdef __FIRMWARE_UPDATE__

#define FW_ADDR_MSG21XX   (0xC4>>1)
#define FW_ADDR_MSG21XX_TP   (0x4C>>1)
#define FW_UPDATE_ADDR_MSG21XX   (0x92>>1)
#define TP_DEBUG	printk
static  char *fw_version;
static u8 temp[94][1024];
static int FwDataCnt;
struct class *firmware_class;
struct device *firmware_cmd_dev;


static int HalTscrCReadI2CSeq(u8 addr, u8* read_data, u8 size)
{
   //according to your platform.
   	int rc;

	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = size,
			.buf = read_data,
		},
	};

	rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCReadI2CSeq error %d\n", rc);
	}
    return rc;
}

static void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    //according to your platform.
   	int rc;

	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = 0,
			.len = size,
			.buf = data,
		},
	};
	rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", rc,addr);
	}
}

static void dbbusDWIICEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
}

static void dbbusDWIICStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);

    // Delay some interval to guard the next transaction
    //udelay ( 200 );        // delay about 0.2ms
}

static void drvISP_EntryIspMode(void)
{
    u8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
}

static u8 drvISP_Read(u8 n, u8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    u8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
    msctpc_LoopDelay ( 1 );        // delay about 100us*****
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[1];   //poro:2012-04-27
    }
    else
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
    }

    return 0;
}

static void drvISP_WriteEnable(void)
{
    u8 bWriteData[2] =
    {
        0x10, 0x06
    };
    u8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}


static void drvISP_ExitIspMode(void)
{
    u8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
}

static u8 drvISP_ReadStatus(void)
{
    u8 bReadData = 0;
    u8 bWriteData[2] =
    {
        0x10, 0x05
    };
    u8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    msctpc_LoopDelay ( 1 );        // delay about 100us*****
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    return bReadData;
}


static void drvISP_ChipErase()
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
		u32 timeOutCount=0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		msctpc_LoopDelay ( 1 );        // delay about 100us*****
    timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;

  	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		msctpc_LoopDelay ( 1 );        // delay about 100us*****
		timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
	  }
}

static void drvISP_Program(u16 k, u8* pDataToWrite)
{
    u16 i = 0;
    u16 j = 0;
    //u16 n = 0;
    u8 TX_data[133];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
		u32 timeOutCount=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        msctpc_LoopDelay ( 1 );        // delay about 100us*****
       
        timeOutCount=0;
				while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
				{
		   			timeOutCount++;
	     			if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  		}
        
        
        
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    }
}


static void drvISP_Verify(u16 k, u8* pDataToVerify)
{
    u16 i = 0, j = 0;
    u8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
    u8 index=0;
    u32 timeOutCount;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        bWriteData[2] = (u8)((addr + j * 128) >> 16);
        bWriteData[3] = (u8)((addr + j * 128) >> 8);
        bWriteData[4] = (u8)(addr + j * 128);
        msctpc_LoopDelay ( 1 );        // delay about 100us*****
        
        
        timeOutCount=0;
				while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
				{
		   		timeOutCount++;
	     		if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  		}
        
        
        
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
        msctpc_LoopDelay ( 1 );        // delay about 100us*****
        drvISP_Read(128, RX_data);
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
        for (i = 0; i < 128; i++)   //log out if verify error
        {
        if((RX_data[i]!=0)&&index<10)
		{
        //TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
        index++;
		}
            if (RX_data[i] != pDataToVerify[128 * j + i])
            {
                TP_DEBUG("k=%d,j=%d,i=%d===============Update Firmware Error================",k,j,i);
            }
        }
    }
}

static ssize_t firmware_update_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", fw_version);
}

static void _HalTscrHWReset(void)
{
	  gpio_direction_output(MSG21XX_RESET_GPIO, 1);
	  gpio_set_value(MSG21XX_RESET_GPIO, 1);
	  gpio_set_value(MSG21XX_RESET_GPIO, 0);
    mdelay(10);  /* Note that the RST must be in LOW 10ms at least */
	  gpio_set_value(MSG21XX_RESET_GPIO, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    mdelay(50);
}
static ssize_t firmware_update_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};


		_HalTscrHWReset();
		//1.Erase TP Flash first
	  dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();
		mdelay(300);
		
				
		// Disable the Watchdog
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	  //Stop MCU
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);



		//set MCU clock,SPI clock =FRO
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x22;
		dbbus_tx_data[3] = 0x00;     
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x23;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		
	  // Enable slave's ISP ECO mode
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	  //Enable SPI Pad
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //WP overwrite
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set pin high
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbusDWIICIICNotUseBus();
		dbbusDWIICNotStopMCU();
		dbbusDWIICExitSerialDebugMode();
		
		
		
    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay(300);
    
    //2.Program and Verify
    dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();


		
		// Disable the Watchdog
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	  //Stop MCU
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);



		//set MCU clock,SPI clock =FRO
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x22;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x23;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	  // Enable slave's ISP ECO mode
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		
	  //Enable SPI Pad
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //WP overwrite
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set pin high
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbusDWIICIICNotUseBus();
		dbbusDWIICNotStopMCU();
		dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for (i = 0; i < 94; i++)   // total  94 KB : 1 byte per R/W
    {
        drvISP_Program(i, temp[i]);    // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
		TP_DEBUG("update OK\n");
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    return size;
}

static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);

/*test=================*/
static ssize_t firmware_clear_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    u16 k=0,i = 0, j = 0;
    u8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
	u32 addr = 0;
		u32 timeOutCount=0;
	for (k = 0; k < 94; i++)   // total  94 KB : 1 byte per R/W
   {
      addr = k * 1024;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        bWriteData[2] = (u8)((addr + j * 128) >> 16);
        bWriteData[3] = (u8)((addr + j * 128) >> 8);
        bWriteData[4] = (u8)(addr + j * 128);
        msctpc_LoopDelay ( 1 );        // delay about 100us*****

        timeOutCount=0;
				while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
				{
		   		timeOutCount++;
	     		if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  		}
        
        
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
        msctpc_LoopDelay ( 1 );        // delay about 100us*****
        drvISP_Read(128, RX_data);
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
        for (i = 0; i < 128; i++)   //log out if verify error
        {
            if (RX_data[i] != 0xFF)
            {
                TP_DEBUG("k=%d,j=%d,i=%d===============erase not clean================",k,j,i);
            }
        }
    }
	}
   TP_DEBUG("read finish\n");
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_clear_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{

		u8 dbbus_tx_data[4];
		unsigned char dbbus_rx_data[2] = {0};

		_HalTscrHWReset();
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();


		
		// Disable the Watchdog
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	  //Stop MCU
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);



		//set MCU clock,SPI clock =FRO
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x22;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x23;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		
	  // Enable slave's ISP ECO mode
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	  //Enable SPI Pad
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //WP overwrite
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	  //set pin high
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbusDWIICIICNotUseBus();
		dbbusDWIICNotStopMCU();
		dbbusDWIICExitSerialDebugMode();


    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
	  TP_DEBUG("chip erase+\n");
    drvISP_ChipErase();
	  TP_DEBUG("chip erase-\n");
    drvISP_ExitIspMode();
    return size;
}

static DEVICE_ATTR(clear, 0777, firmware_clear_show, firmware_clear_store);

/*test=================*/
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/

static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;

    fw_version = kzalloc(sizeof(char), GFP_KERNEL);
		//SM-BUS GET FW VERSION
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x74;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    TP_DEBUG("***major = %d ***\n", major);
    TP_DEBUG("***minor = %d ***\n", minor);
    sprintf(fw_version,"%03d%03d", major, minor);
    TP_DEBUG("***fw_version = %s ***\n", fw_version);


    return size;
}
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
{

    int i;
	TP_DEBUG("***FwDataCnt = %d ***\n", FwDataCnt);
    for (i = 0; i < 1024; i++)
    {
        memcpy(temp[FwDataCnt], buf, 1024);
    }
    FwDataCnt++;
    return size;
}
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);
#endif
//---------------------------------------------------------------------------
static void msg21xx_suspend(struct early_suspend *h)
{
    gpio_set_value(MSG21XX_RESET_GPIO, 0);//zhaiqingbo 2012.11.12 modify for disabling touch panel
}

static void msg21xx_resume(struct early_suspend *h)
{
    gpio_set_value(MSG21XX_RESET_GPIO, 1);//zhaiqingbo 2012.11.12 modify for enabling touch panel
}

//yangshjuan modified 20130226 for compatibility with FT6306
static int msg21xx_chip_init(void)
{
    int err = 0;
    /* After the LCD is on, power on the TP controller */
    if(gpio_is_valid(MSG21XX_RESET_GPIO))
    {
        err = gpio_request(MSG21XX_RESET_GPIO, "msg21xx_reset");
        printk(KERN_WARNING "gpio_request reset %d", err);
        if(err)
        {
            return err;
        }
        else
        {
        	gpio_direction_output(MSG21XX_RESET_GPIO, 1);
        	gpio_set_value(MSG21XX_RESET_GPIO, 1);
        	gpio_set_value(MSG21XX_RESET_GPIO, 0);
            mdelay(200);  /* Note that the RST must be in LOW 10ms at least */
        	gpio_set_value(MSG21XX_RESET_GPIO, 1);
            /* Enable the interrupt service thread/routine for INT after 50ms */
            mdelay(50);
        }
    }
    return err;
}
//yangshjuan modified 20130226 end


static u8 Calculate_8BitsChecksum( u8 *msg, s32 s32Length )
{
    s32 s32Checksum = 0;
    s32 i;

    for ( i = 0 ; i < s32Length; i++ )
    {
        s32Checksum += msg[i];
    }

    return (u8)( ( -s32Checksum ) & 0xFF );
}


static void msg21xx_data_disposal(struct work_struct *work)
{
    u8 val[8] = {0};
    u8 Checksum = 0;
	u8 i;
    u32 delta_x = 0, delta_y = 0;
    u32 u32X = 0;
    u32 u32Y = 0;
	u8 touchkeycode = 0;
	TouchScreenInfo_t touchData;
	static u32 preKeyStatus=0;
	//static u32 preFingerNum=0;
#define SWAP_X_Y   (1)
//#define REVERSE_X  (1)
//#define REVERSE_Y  (1)
#ifdef SWAP_X_Y
    int tempx;
    int tempy;
#endif

    //_TRACE((TSCR_LEVEL_C_TYP, "DrvTscrCGetData in"));
    i2c_master_recv(msg21xx_i2c_client,&val[0],REPORT_PACKET_LENGTH);
    Checksum = Calculate_8BitsChecksum(&val[0], 7); //calculate checksum
    if ((Checksum == val[7]) && (val[0] == 0x52))   //check the checksum  of packet
    {
        u32X = (((val[1] & 0xF0) << 4) | val[2]);         //parse the packet to coordinates
        u32Y = (((val[1] & 0x0F) << 8) | val[3]);

        delta_x = (((val[4] & 0xF0) << 4) | val[5]);
        delta_y = (((val[4] & 0x0F) << 8) | val[6]);

    #ifdef SWAP_X_Y
		tempy = u32X;
		tempx = u32Y;
        u32X = tempx;
        u32Y = tempy;

		tempy = delta_x;
		tempx = delta_y;
        delta_x = tempx;
        delta_y = tempy;
	#endif
	  #ifdef REVERSE_X
	  u32X = 2047 - u32X;
	  delta_x = 4095 - delta_x;
	  #endif
	  #ifdef REVERSE_Y
	  u32Y = 2047 - u32Y;
	  delta_y = 4095 - delta_y;
	  #endif
        //printk("[HAL] u32X = %d, u32Y = %d \n\t", u32X, u32Y);
        //DBG("[HAL] delta_x = %x, delta_y = %x", delta_x, delta_y);

        if ((val[1] == 0xFF) && (val[2] == 0xFF) && (val[3] == 0xFF) && (val[4] == 0xFF) && (val[6] == 0xFF))
        {
            touchData.Point[0].X = 0; // final X coordinate
            touchData.Point[0].Y = 0; // final Y coordinate

           if((val[5]==0x0)||(val[5]==0xFF))
            {
                touchData.nFingerNum = 0; //touch end
                touchData.nTouchKeyCode = 0; //TouchKeyMode
                touchData.nTouchKeyMode = 0; //TouchKeyMode
            }
            else
            {
                touchData.nTouchKeyMode = 1; //TouchKeyMode
				        touchData.nTouchKeyCode = val[5]; //TouchKeyCode
                touchData.nFingerNum = 1;
            }
        }
		else
		{
		    touchData.nTouchKeyMode = 0; //Touch on screen...

			if(
			   #ifdef REVERSE_X
			       (delta_x == 4095)
			   #else
			       (delta_x == 0) 
			   #endif
			   && 
			   #ifdef REVERSE_Y
			       (delta_y == 4095)
			   #else
			       (delta_y == 0)
			   #endif
			  )
			{
				touchData.nFingerNum = 1; //one touch
				touchData.Point[0].X = (u32X * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[0].Y = (u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;
                printk("tp x:%d, y:%d\n",touchData.Point[0].X,touchData.Point[0].Y);
			}
			else
			{
				u32 x2, y2;

				touchData.nFingerNum = 2; //two touch

				/* Finger 1 */
				touchData.Point[0].X = (u32X * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[0].Y = (u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;

				/* Finger 2 */
				if (delta_x > 2048)     //transform the unsigh value to sign value
				{
					delta_x -= 4096;
				}
				if (delta_y > 2048)
				{
					delta_y -= 4096;
				}

				x2 = (u32)(u32X + delta_x);
				y2 = (u32)(u32Y + delta_y);

				touchData.Point[1].X = (x2 * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[1].Y = (y2 * MS_TS_MSG21XX_Y_MAX) / 2048;
			}
		}

		//report...
		if(touchData.nTouchKeyMode)
		{
			if (touchData.nTouchKeyCode == 1)
				//touchkeycode = KEY_HOME;
                touchkeycode = KEY_MENU;    //modify by zhaiqingbo 2013.1.22
			if (touchData.nTouchKeyCode == 2)
				//touchkeycode = KEY_MENU;
                touchkeycode = KEY_HOME;    //modify by zhaiqingbo 2013.1.22
			if (touchData.nTouchKeyCode == 4)
				touchkeycode = KEY_BACK;
			if (touchData.nTouchKeyCode == 8)
				touchkeycode = KEY_SEARCH;

			if(preKeyStatus!=touchkeycode)
			{
				preKeyStatus=touchkeycode;
				input_report_key(input, touchkeycode, 1);
               	input_report_key(input, BTN_TOUCH, 1);
				//printk("&&&&&&&&useful key code report touch key code = %d\n",touchkeycode);
			}
			input_sync(input);
		}
        else
        {
		    preKeyStatus=0; //clear key status..

            if((touchData.nFingerNum) == 0)   //touch end
            {
				//preFingerNum=0;
				/* modify by zhaiqingbo 2013.1.22 */
				input_report_key(input, KEY_MENU, 0);
				input_report_key(input, KEY_HOME, 0);
				input_report_key(input, KEY_BACK, 0);
				//input_report_key(input, KEY_SEARCH, 0);
				/* modify end */

				input_report_abs(input, ABS_MT_PRESSURE, 0);
				input_report_key(input, BTN_TOUCH, 0);
				input_mt_sync(input);
				input_sync(input);
            }
            else //touch on screen
            {    
			    /*
				if(preFingerNum!=touchData.nFingerNum)   //for one touch <--> two touch issue
				{
					printk("langwenlong number has changed\n");
					preFingerNum=touchData.nFingerNum;
					input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				    input_mt_sync(input);
				    input_sync(input);
				}*/

				for(i = 0;i < (touchData.nFingerNum);i++)
				{
					input_report_abs(input, ABS_MT_PRESSURE, 255);
              		//input_report_abs(input, ABS_MT_TRACKING_ID,	touchData.nFingerNum);		
                    input_report_abs(input, ABS_MT_TOUCH_MAJOR,		1);
					input_report_abs(input, ABS_MT_POSITION_X, touchData.Point[i].X);
					input_report_abs(input, ABS_MT_POSITION_Y, touchData.Point[i].Y);
					input_mt_sync(input);
				}
				input_report_key(input, BTN_TOUCH, 1);
				input_sync(input);
			}
		}
    }
    else
    {
        //DBG("Packet error 0x%x, 0x%x, 0x%x", val[0], val[1], val[2]);
        //DBG("             0x%x, 0x%x, 0x%x", val[3], val[4], val[5]);
        //DBG("             0x%x, 0x%x, 0x%x", val[6], val[7], Checksum);
		printk(KERN_ERR "err status in tp\n");
    }

    enable_irq(msg21xx_irq);
}


static int msg21xx_ts_open(struct input_dev *dev)
{
	return 0;
}

static void msg21xx_ts_close(struct input_dev *dev)
{
	printk("msg21xx_ts_close\n");
}


static int msg21xx_init_input(void)
{
	int err;

	input = input_allocate_device();
	input->name = msg21xx_i2c_client->name;
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &msg21xx_i2c_client->dev;
	input->open = msg21xx_ts_open;
	input->close = msg21xx_ts_close;

	set_bit(EV_ABS, input->evbit);
	//set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_TOUCH, input->keybit);
    /* zhaiqingbo modify 2013.1.22 */
	set_bit(KEY_BACK, input->keybit);
	set_bit(KEY_MENU, input->keybit);
	set_bit(KEY_HOME, input->keybit);
    /* zhaiqingbo modify end*/

    input_set_abs_params(input, ABS_PRESSURE, 0, 255, 0, 0);
	//input_set_abs_params(input, ABS_MT_TRACKING_ID, 0,	2, 0, 0);	
    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 0x7F, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, MS_TS_MSG21XX_X_MAX, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, MS_TS_MSG21XX_Y_MAX, 0, 0);

	err = input_register_device(input);
	if (err)
		goto fail_alloc_input;

fail_alloc_input:
	return 0;
}
static irqreturn_t msg21xx_interrupt(int irq, void *dev_id)
{
	disable_irq_nosync(msg21xx_irq);
	schedule_work(&msg21xx_wq);
	return IRQ_HANDLED;
}


static int __devinit msg21xx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int  err = 0;
    unsigned char val[2] = {0};

	msg21xx_i2c_client = client;

	gpio_request(MSG21XX_INT_GPIO, "interrupt");
//	gpio_request(MSG21XX_RESET_GPIO, "reset");
	gpio_direction_input(MSG21XX_INT_GPIO);
//	gpio_set_value(MSG21XX_INT_GPIO, 1);

	err = msg21xx_chip_init();
    if(err)
    {
        goto exit;
    }
    //yangshjuan modified 20130226 for compatibility with FT6306
    mdelay(200);
    err = i2c_master_recv(msg21xx_i2c_client,&val[0],2);
    printk("i2c_master_recv check i2c addr %d", err);
    if (err < 0)
    {
        goto exit;
    } 
    //yangshjuan modified 20130226 end
    
	INIT_WORK(&msg21xx_wq, msg21xx_data_disposal);
	msg21xx_init_input();
	msg21xx_irq = MSM_GPIO_TO_INT(MSG21XX_INT_GPIO);
	//err = request_irq(msg21xx_irq, msg21xx_interrupt,
	//						IRQF_TRIGGER_RISING, "msg21xx", NULL);
    err = request_irq(msg21xx_irq, msg21xx_interrupt,
							IRQF_TRIGGER_RISING, "msg2133", NULL);
	if (err != 0) {
		printk("%s: cannot register irq\n", __func__);
		goto exit;
	}

	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	early_suspend.suspend = msg21xx_suspend;
	early_suspend.resume = msg21xx_resume;
	register_early_suspend(&early_suspend);

	/*frameware upgrade*/
#ifdef __FIRMWARE_UPDATE__
	firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
    if (IS_ERR(firmware_class))
        pr_err("Failed to create class(firmware)!\n");
    firmware_cmd_dev = device_create(firmware_class,
                                     NULL, 0, NULL, "device");
    if (IS_ERR(firmware_cmd_dev))
        pr_err("Failed to create device(firmware_cmd_dev)!\n");

    // version
    if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
    // update
    if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
    // data
    if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);
	// clear
    if (device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);

	dev_set_drvdata(firmware_cmd_dev, NULL);
#endif

	return 0;

exit: 
    //yangshjuan modified 20130226 for compatibility with FT6306
    gpio_free(MSG21XX_RESET_GPIO);
	return err;
}

static int __devexit msg21xx_remove(struct i2c_client *client)
{
	return 0;
}



static const struct i2c_device_id msg21xx_id[] = {
	//{ "ms-msg21xx", 0x26 },
	{ "msg2133", 0x26 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, msg21xx_id);


static struct i2c_driver msg21xx_driver = {
	.driver = {
		   .name = "msg2133",
		   .owner = THIS_MODULE,
	},
	.probe = msg21xx_probe,
	.remove = __devexit_p(msg21xx_remove),
	.id_table = msg21xx_id,
};





static int __init msg21xx_init(void)
{
	int err;
	err = i2c_add_driver(&msg21xx_driver);
	if (err) {
		//printk(KERN_WARNING "msg21xx  driver failed "
		       //"(errno = %d)\n", err);
        printk(KERN_WARNING "msg2133  driver failed "
		       "(errno = %d)\n", err);
	} else {
		printk( "Successfully added driver %s\n",
		          msg21xx_driver.driver.name);
	}
	return err;
}

static void __exit msg21xx_cleanup(void)
{
	i2c_del_driver(&msg21xx_driver);
}


module_init(msg21xx_init);
module_exit(msg21xx_cleanup);

MODULE_AUTHOR("Mstar semiconductor");
//MODULE_DESCRIPTION("Driver for msg21xx Touchscreen Controller");
MODULE_DESCRIPTION("Driver for msg2133 Touchscreen Controller");
MODULE_LICENSE("GPL");

