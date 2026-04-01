/*
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * VERSION      	DATE			AUTHOR 
 *    1.0		  2010-01-05			WenFS
 *
 * note: only support mulititouch	Wenfs 2010-10-01
 */

#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
//#include <linux/i2c/ft53x6_ts.h>
//#include <soc/sprd/regulator.h>
#include <linux/input/mt.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/err.h>

#if(defined(CONFIG_I2C_SPRD) ||defined(CONFIG_I2C_SPRD_V1))
//#include <soc/sprd/i2c-sprd.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/irq.h>
//#include <linux/wakelock.h>
#include <soc/sprd/board.h>

#if defined(ZCFG_MK_TP_GESTURE)
#define FTS_GESTRUE
#endif

#if defined(ZCFG_MK_TP_PROXIMITY)
#define TP_PROXIMITY_SENSOR
#endif

#ifdef TS_USE_ADF_NOTIFIER
#include <video/adf_notifier.h>
#endif


#ifdef TP_PROXIMITY_SENSOR
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#define PROXIMITY_DEVICE		"ctp_proximity"

//ioctl cmd
#define FT_IOCTL_MAGIC			0X5D
#define FT_IOCTL_PROX_ON		_IO(FT_IOCTL_MAGIC, 7)
#define FT_IOCTL_PROX_OFF		_IO(FT_IOCTL_MAGIC, 8)

#define FT_REGS_PS_CTL			0xB0

#define TPD_PROXIMITY_DEVICE			"FT6206"

#undef TPD_PROXIMITY_DEBUG_ON
#define TPD_PROXIMITY_INFO(a,arg...)	printk(TPD_PROXIMITY_DEVICE ": " a,##arg)
#if defined(TPD_PROXIMITY_DEBUG_ON)
#define TPD_PROXIMITY_DEBUG(a,arg...)	printk(TPD_PROXIMITY_DEVICE ": " a,##arg)
#else
#define TPD_PROXIMITY_DEBUG(arg...)
#endif
static u8 tpd_proximity_flag = 0;
static int tpd_halt = 0;

static u8 tpd_proximity_dir_faraway = 0;//0-->close ; 1--> far away


//add by jinq for first call no response
static u8 first_call=1;
static u8 first_proximity_faraway=0;
//add  by jinq end
#endif
//------------------------------proximity    end----------------------------------

//#define FT53X6_DBG
#ifdef FT53X6_DBG
#define ENTER printk(KERN_INFO "[FT53X6_DBG] func: %s  line: %04d\n", __func__, __LINE__);
#define PRINT_DBG(x...)  printk(KERN_INFO "[FT53X6_DBG] " x)
#define PRINT_INFO(x...)  printk(KERN_INFO "[FT53X6_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FT53X6_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FT53X6_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#else
#define ENTER
#define PRINT_DBG(x...)
#define PRINT_INFO(x...)  printk(KERN_INFO "[FT53X6_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FT53X6_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FT53X6_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#endif

extern int tp_device_id(int id);
extern void ctp_lock_mutex(void);
extern void ctp_unlock_mutex(void);




#ifdef TS_USE_ADF_NOTIFIER
static struct notifier_block adf_event_block;
#endif

struct mutex enable_irq_mutex;  // 增加一个互斥信号量，修改连续快速按power键进行休眠唤醒操作会导致tp无响应的问题。
//int irq_num=10;

#define APK_DEBUG
#if defined(ZCFG_TP_FOCALTECH_AUTO_UPGRADE)
#define SPRD_AUTO_UPGRADE
#endif
#define SYSFS_DEBUG
#define FTS_CTL_IIC
#include "focaltech.h"
#include "focaltech_ex_fun.h"
#include "focaltech_ctl.h"

#define	TOUCH_VIRTUAL_KEYS
#define	MULTI_PROTOCOL_TYPE_B	0
#ifdef CONFIG_ARCH_SCX35LT8
#define	TS_MAX_FINGER		2
#else
#define	TS_MAX_FINGER		5
#define	FTS_PACKET_LENGTH	128
#endif


#ifdef FTS_GESTRUE
//#include "ft_gesture_lib.h"
#include <linux/fs.h>

#undef TPD_GES_DEBUG_ON
#define TPD_GES_INFO(a,arg...)	printk(a,##arg)
#if defined(TPD_GES_DEBUG_ON)
#define TPD_GES_DEBUG(a,arg...)	printk(a,##arg)
#else
#define TPD_GES_DEBUG(arg...)
#endif

#define GESTURE_FUNCTION_SWITCH 	"/mnt/vendor/gesture/gesture_switch"
static int s_gesture_switch = 1;	// Defaultly, the macro is open

#define GESTURE_LEFT		0x20
#define GESTURE_RIGHT		0x21
#define GESTURE_UP			0x22
#define GESTURE_DOWN		0x23
#define GESTURE_DOUBLECLICK 0x24
#define GESTURE_O			0x30
#define GESTURE_W			0x31
#define GESTURE_M			0x32
#define GESTURE_E			0x33
#define GESTURE_C			0x34
#define GESTURE_S					0x46
#define GESTURE_V					0x54
#define GESTURE_Z					0x41
#define GESTURE_Z_FT6336U			0x65

#define FTS_GESTRUE_POINTS 255
#define FTS_GESTRUE_POINTS_ONETIME	62
#define FTS_GESTRUE_POINTS_HEADER 8
#define FTS_GESTURE_OUTPUT_ADRESS 0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 4

short pointnum = 0;
unsigned short coordinate_x[260] = {0};
unsigned short coordinate_y[260] = {0};

//add wake lock for gestrue
#define GESTURE_WAKE_LOCK 1
#if GESTURE_WAKE_LOCK
struct wakeup_source suspend_gestrue_lock;
#endif

//extern int fetch_object_sample(unsigned char *buf,short pointnum);
//extern void init_para(int x_pixel,int y_pixel,int time_slot,int cut_x_pixel,int cut_y_pixel);
//suspend_state_t get_suspend_state(void);
#endif

/***************************************************************/

#if USE_WAIT_QUEUE
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag = 0;
#endif

#if 0
static unsigned char FT5316_FW[]=
{
#include "ft5316_720p.i"
};

static unsigned char FT5306_FW[]=
{
#include "ft5306_qHD.i"
};

static unsigned char *CTPM_FW = FT5306_FW;
#endif
//static int fw_size;

struct ft5x0x_ts_data *g_ft5x0x_ts;
static struct i2c_client *this_client;

//static unsigned char suspend_flag = 0;


 struct Upgrade_Info fts_updateinfo[] =
{
#if defined(ZCFG_SET_FT5436_TP_NAME)
	{0x54,"FT54x6",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000},
#else
	{0x54,"FT3x27",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000},
#endif
	{0x55,"FT5x06",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000},
	{0x08,"FT5606",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x06, 100, 2000},
	{0x0a,"FT5x16",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x07, 1, 1500},
	{0x05,"FT6208",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,60, 30, 0x79, 0x05, 10, 2000},
	{0x06,"FT6x06",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,100, 30, 0x79, 0x08, 10, 2000},
	{0x36,"FT6x36",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000},
    {0x64,"FT6x36U",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x1c, 10, 2000}, //0x18 changed to 0x1c
	{0x55,"FT5x06i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000},
	{0x14,"FT5336",TPD_MAX_POINTS_5,AUTO_CLB_NEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x13,"FT3316",TPD_MAX_POINTS_5,AUTO_CLB_NEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x12,"FT5436i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x11,"FT5336i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,30, 30, 0x79, 0x11, 10, 2000},
};
				
struct Upgrade_Info fts_updateinfo_curr;
#ifdef SPRD_AUTO_UPGRADE
extern int fts_ctpm_auto_upgrade(struct i2c_client *client);
#endif
#if 0//dennis
/* Attribute */
static ssize_t ft5x0x_show_suspend(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t ft5x0x_store_suspend(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t ft5x0x_show_version(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t ft5x0x_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
#endif
//static unsigned char ft5x0x_read_fw_ver(void);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x0x_ts_suspend(struct early_suspend *handler);
static void ft5x0x_ts_resume(struct early_suspend *handler);
#endif
//static int fts_ctpm_fw_update(void);
//static int fts_ctpm_fw_upgrade_with_i_file(void);


#if 0//dennis
static DEVICE_ATTR(suspend, S_IRUGO | S_IWUSR, ft5x0x_show_suspend, ft5x0x_store_suspend);
static DEVICE_ATTR(update, S_IRUGO | S_IWUSR, ft5x0x_show_version, ft5x0x_update);

static ssize_t ft5x0x_show_suspend(struct device* cd,
				     struct device_attribute *attr, char* buf)
{
	ssize_t ret = 0;

	if(suspend_flag==1)
		sprintf(buf, "FT5x0x Suspend\n");
	else
		sprintf(buf, "FT5x0x Resume\n");

	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t ft5x0x_store_suspend(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	suspend_flag = on_off;

	if(on_off==1)
	{
		pr_info("FT5x0x Entry Suspend\n");
	#ifdef CONFIG_HAS_EARLYSUSPEND
		ft5x0x_ts_suspend(NULL);
	#endif
	}
	else
	{
		pr_info("FT5x0x Entry Resume\n");
	#ifdef CONFIG_HAS_EARLYSUSPEND
		ft5x0x_ts_resume(NULL);
	#endif
	}

	return len;
}

static ssize_t ft5x0x_show_version(struct device* cd,
				     struct device_attribute *attr, char* buf)
{
	ssize_t ret = 0;
	unsigned char uc_reg_value; 

	uc_reg_value = ft5x0x_read_fw_ver();

	sprintf(buf, "ft5x0x firmware id is V%x\n", uc_reg_value);

	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t ft5x0x_update(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	unsigned char uc_reg_value;

	uc_reg_value = ft5x0x_read_fw_ver();

	if(on_off==1)
	{
		pr_info("ft5x0x update, current firmware id is V%x\n", uc_reg_value);
		//fts_ctpm_fw_update();
		fts_ctpm_fw_upgrade_with_i_file();
	}

	return len;
}

static int ft5x0x_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_suspend);
	err = device_create_file(dev, &dev_attr_update);

	return err;
}
#endif


static int ft5x0x_i2c_rxdata(char *rxdata, int length)
{
	int ret = 0;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

	if (i2c_transfer(this_client->adapter, msgs, 2) != 2) {
		ret = -EIO;
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	}

	return ret;
}


static int ft5x0x_i2c_txdata(char *txdata, int length)
{
	int ret = 0;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

	if (i2c_transfer(this_client->adapter, msg, 1) != 1) {
		ret = -EIO;
		pr_err("%s i2c write error: %d\n", __func__, ret);
	}

	return ret;
}

/***********************************************************************************************
Name	:	 ft5x0x_write_reg

Input	:	addr -- address
                     para -- parameter

Output	:

function	:	write register of ft5x0x

***********************************************************************************************/


static int ft5x0x_write_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = ft5x0x_i2c_txdata(buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}


/***********************************************************************************************
Name	:	ft5x0x_read_reg

Input	:	addr
                     pdata

Output	:

function	:	read register of ft5x0x

***********************************************************************************************/
static int ft5x0x_read_reg(u8 addr, u8 *pdata)
{
	int ret = 0;
	u8 buf[2] = {addr, 0};

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= buf+1,
		},
	};

	if (i2c_transfer(this_client->adapter, msgs, 2) != 2) {
		ret = -EIO;
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	}

	*pdata = buf[1];
	return ret;
}


#ifdef TOUCH_VIRTUAL_KEYS

static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{

#if defined(ZCFG_TP_FOCALTECH_KEY_MAP)
	u32 key_map_data[12]=ZCFG_TP_FOCALTECH_KEY_MAP;
	return sprintf(buf,"%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d\n"
		,__stringify(EV_KEY), __stringify(KEY_APPSELECT),key_map_data[0],key_map_data[1],key_map_data[2],key_map_data[3]
		,__stringify(EV_KEY), __stringify(KEY_HOMEPAGE),key_map_data[4],key_map_data[5],key_map_data[6],key_map_data[7]
		,__stringify(EV_KEY), __stringify(KEY_BACK),key_map_data[8],key_map_data[9],key_map_data[10],key_map_data[11]);
#else
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ft5x0x_ts_platform_data *pdata = data->platform_data;
	return sprintf(buf,"%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d\n"
		,__stringify(EV_KEY), __stringify(KEY_APPSELECT),pdata ->virtualkeys[0],pdata ->virtualkeys[1],pdata ->virtualkeys[2],pdata ->virtualkeys[3]
		,__stringify(EV_KEY), __stringify(KEY_HOMEPAGE),pdata ->virtualkeys[4],pdata ->virtualkeys[5],pdata ->virtualkeys[6],pdata ->virtualkeys[7]
		,__stringify(EV_KEY), __stringify(KEY_BACK),pdata ->virtualkeys[8],pdata ->virtualkeys[9],pdata ->virtualkeys[10],pdata ->virtualkeys[11]);
#endif
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.focaltech_ts",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static struct kobject *properties_kobj;

static void ft5x0x_ts_virtual_keys_init(void)
{
    int ret = 0;

    pr_info("[FST] %s\n",__func__);

    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");
}

static void ft5x0x_ts_virtual_keys_destroy(void)
{
	if (properties_kobj)
	{
		sysfs_remove_group(properties_kobj, &properties_attr_group);
		kobject_del(properties_kobj);
	}
}
#endif

/***********************************************************************************************
Name	:	 ft5x0x_read_fw_ver

Input	:	 void

Output	:	 firmware version

function	:	 read TP firmware version

***********************************************************************************************/
/*static unsigned char ft5x0x_read_fw_ver(void)
{
	unsigned char ver;
	ft5x0x_read_reg(FT5X0X_REG_FIRMID, &ver);
	return(ver);
}
*/
#ifdef TP_PROXIMITY_SENSOR
/*static int FT6206_get_ps_value(void)
{
	TPD_PROXIMITY_INFO("FT6206_get_ps_value %d!\n", tpd_proximity_dir_faraway);
	return tpd_proximity_dir_faraway;
}*/

static int FT6206_enable_ps(int enable)
{
	TPD_PROXIMITY_INFO("FT6206_enable_ps %d!\n", enable);
	if(enable==1)
	{
		tpd_proximity_flag =1;
		ft5x0x_write_reg(FT_REGS_PS_CTL, 0x1);
		if(first_call==1)
		{
			first_call=0;
			first_proximity_faraway=1;
		}
	}
	else
	{	
		tpd_proximity_flag =0;
		ft5x0x_write_reg(FT_REGS_PS_CTL, 0x0);
	}

	return 1;
}

static long FT6206_ioctl_operate(struct file *file, unsigned int cmd, unsigned long arg)
{
	TPD_PROXIMITY_INFO("FT6206_ioctl_operate %d!\n", cmd);

	switch(cmd)
	{
	case FT_IOCTL_PROX_ON:
		FT6206_enable_ps(1);   
		break;

	case FT_IOCTL_PROX_OFF:
		FT6206_enable_ps(0);
		break;

	default:
		pr_err("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
		return -EINVAL;
	}
	return 0;
}

static struct file_operations FT6206_proximity_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.release = NULL,
	.unlocked_ioctl = FT6206_ioctl_operate
};

struct miscdevice FT6206_proximity_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PROXIMITY_DEVICE,		//match the hal's name 
	.fops = &FT6206_proximity_fops,
};
#endif

static void ft5x0x_clear_report_data(struct ft5x0x_ts_data *ft5x0x_ts)
{
	int i;

	for(i = 0; i < TS_MAX_FINGER; i++) {
	#if MULTI_PROTOCOL_TYPE_B
		input_mt_slot(ft5x0x_ts->input_dev, i);
		input_mt_report_slot_state(ft5x0x_ts->input_dev, MT_TOOL_FINGER, false);
	#endif
	}
	input_report_key(ft5x0x_ts->input_dev, BTN_TOUCH, 0);
	#if !MULTI_PROTOCOL_TYPE_B
		input_mt_sync(ft5x0x_ts->input_dev);
	#endif
	input_sync(ft5x0x_ts->input_dev);
}
#ifdef FTS_GESTRUE
////=================Customer ges code Start===============================////
#define GESTURE_FUNCTION_KEY_C_NEXT 	"/mnt/vendor/gesture/c_next"
#define GESTURE_FUNCTION_KEY_C_PRE 	"/mnt/vendor/gesture/c_pre"
#define GESTURE_FUNCTION_KEY_C_PLAY_PAUSE	"/mnt/vendor/gesture/c_play_pause"

#define GESTURE_FUNCTION_KEY_W_NEXT 	"/mnt/vendor/gesture/w_next"
#define GESTURE_FUNCTION_KEY_W_PRE 	"/mnt/vendor/gesture/w_pre"
#define GESTURE_FUNCTION_KEY_W_PLAY_PAUSE	"/mnt/vendor/gesture/w_play_pause"

#define GESTURE_FUNCTION_KEY_V_NEXT 	"/mnt/vendor/gesture/v_next"
#define GESTURE_FUNCTION_KEY_V_PRE 	"/mnt/vendor/gesture/v_pre"
#define GESTURE_FUNCTION_KEY_V_PLAY_PAUSE	"/mnt/vendor/gesture/v_play_pause"

#define GESTURE_FUNCTION_KEY_M_NEXT 	"/mnt/vendor/gesture/m_next"
#define GESTURE_FUNCTION_KEY_M_PRE 	"/mnt/vendor/gesture/m_pre"
#define GESTURE_FUNCTION_KEY_M_PLAY_PAUSE	"/mnt/vendor/gesture/m_play_pause"

#define GESTURE_FUNCTION_KEY_S_NEXT 	"/mnt/vendor/gesture/s_next"
#define GESTURE_FUNCTION_KEY_S_PRE 	"/mnt/vendor/gesture/s_pre"
#define GESTURE_FUNCTION_KEY_S_PLAY_PAUSE	"/mnt/vendor/gesture/s_play_pause"

#define GESTURE_FUNCTION_KEY_Z_NEXT 	"/mnt/vendor/gesture/z_next"
#define GESTURE_FUNCTION_KEY_Z_PRE 	"/mnt/vendor/gesture/z_pre"
#define GESTURE_FUNCTION_KEY_Z_PLAY_PAUSE	"/mnt/vendor/gesture/z_play_pause"

#define GESTURE_FUNCTION_KEY_O_NEXT 	"/mnt/vendor/gesture/o_next"
#define GESTURE_FUNCTION_KEY_O_PRE 	"/mnt/vendor/gesture/o_pre"
#define GESTURE_FUNCTION_KEY_O_PLAY_PAUSE	"/mnt/vendor/gesture/o_play_pause"

#define GESTURE_FUNCTION_KEY_E_NEXT 	"/mnt/vendor/gesture/e_next"
#define GESTURE_FUNCTION_KEY_E_PRE 	"/mnt/vendor/gesture/e_pre"
#define GESTURE_FUNCTION_KEY_E_PLAY_PAUSE	"/mnt/vendor/gesture/e_play_pause"

#if defined(ZCFG_TP_FOCALTECH_GESTRUE_DOUBLECLICK)
#define GESTURE_FUNCTION_KEY_U_NEXT 	"/mnt/vendor/gesture/u_next"
#define GESTURE_FUNCTION_KEY_U_PRE	"/mnt/vendor/gesture/u_pre"
#define GESTURE_FUNCTION_KEY_U_PLAY_PAUSE	"/mnt/vendor/gesture/u_play_pause"
#endif

#if !defined(ZCFG_TP_FOCALTECH_GESTRUE_NO_DIRECTION)
#define GESTURE_FUNCTION_KEY_RIGHT_NEXT 	"/mnt/vendor/gesture/right_next"
#define GESTURE_FUNCTION_KEY_RIGHT_PRE 	"/mnt/vendor/gesture/right_pre"
#define GESTURE_FUNCTION_KEY_RIGHT_PLAY_PAUSE	"/mnt/vendor/gesture/right_play_pause"

#define GESTURE_FUNCTION_KEY_DOWN_NEXT 	"/mnt/vendor/gesture/down_next"
#define GESTURE_FUNCTION_KEY_DOWN_PRE 	"/mnt/vendor/gesture/down_pre"
#define GESTURE_FUNCTION_KEY_DOWN_PLAY_PAUSE	"/mnt/vendor/gesture/down_play_pause"

#define GESTURE_FUNCTION_KEY_LEFT_NEXT 	"/mnt/vendor/gesture/left_next"
#define GESTURE_FUNCTION_KEY_LEFT_PRE 	"/mnt/vendor/gesture/left_pre"
#define GESTURE_FUNCTION_KEY_LEFT_PLAY_PAUSE	"/mnt/vendor/gesture/left_play_pause"

#define GESTURE_FUNCTION_KEY_UP_NEXT 	"/mnt/vendor/gesture/up_next"
#define GESTURE_FUNCTION_KEY_UP_PRE 	"/mnt/vendor/gesture/up_pre"
#define GESTURE_FUNCTION_KEY_UP_PLAY_PAUSE	"/mnt/vendor/gesture/up_play_pause"
#endif

int Ges_trans_key(struct ft5x0x_ts_data *info,unsigned int code)
{
	struct file *fp; 

	switch (code)
	{
		case KEY_C:
			fp = filp_open(GESTURE_FUNCTION_KEY_C_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_C_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_C_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_C_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_C_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_C_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_C_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_C_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_C_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}				
			break;
		case KEY_W:
			fp = filp_open(GESTURE_FUNCTION_KEY_W_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_W_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_W_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_W_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_W_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_W_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_W_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_W_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_W_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_V:
			fp = filp_open(GESTURE_FUNCTION_KEY_V_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_V_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_V_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_V_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_V_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_V_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_V_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_V_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_V_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_M:
			fp = filp_open(GESTURE_FUNCTION_KEY_M_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_M_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_M_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_M_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_M_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_M_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_M_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_M_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_M_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_S:
			fp = filp_open(GESTURE_FUNCTION_KEY_S_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_S_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_S_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_S_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_S_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_S_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_S_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_S_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_S_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_Z:
			fp = filp_open(GESTURE_FUNCTION_KEY_Z_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_Z_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_Z_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_Z_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_Z_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_Z_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_Z_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_Z_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_Z_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_O:
			fp = filp_open(GESTURE_FUNCTION_KEY_O_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_O_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_O_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_O_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_O_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_O_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_O_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_O_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_O_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_E:
			fp = filp_open(GESTURE_FUNCTION_KEY_E_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_E_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_E_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_E_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_E_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_E_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_E_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_E_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_E_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
#if defined(ZCFG_TP_FOCALTECH_GESTRUE_DOUBLECLICK)
		case KEY_U:
			fp = filp_open(GESTURE_FUNCTION_KEY_U_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_U_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_U_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_U_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_U_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_U_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_U_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_U_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_U_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
#endif
#if !defined(ZCFG_TP_FOCALTECH_GESTRUE_NO_DIRECTION)
		case KEY_RIGHT:
			fp = filp_open(GESTURE_FUNCTION_KEY_RIGHT_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_RIGHT_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_RIGHT_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_RIGHT_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_RIGHT_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_RIGHT_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_RIGHT_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_RIGHT_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_RIGHT_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_DOWN:
			fp = filp_open(GESTURE_FUNCTION_KEY_DOWN_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_DOWN_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_DOWN_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_DOWN_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_DOWN_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_DOWN_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_DOWN_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_DOWN_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_DOWN_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_LEFT:
			fp = filp_open(GESTURE_FUNCTION_KEY_LEFT_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_LEFT_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_LEFT_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			
			fp = filp_open(GESTURE_FUNCTION_KEY_LEFT_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_LEFT_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_LEFT_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_LEFT_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_LEFT_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_LEFT_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
		case KEY_UP:
			fp = filp_open(GESTURE_FUNCTION_KEY_UP_NEXT, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_UP_NEXT);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_UP_NEXT); 
				filp_close(fp, NULL);
				code = KEY_NEXTSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_UP_PRE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_UP_PRE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_UP_PRE); 
				filp_close(fp, NULL);
				code = KEY_PREVIOUSSONG;
				break;
			}
			fp = filp_open(GESTURE_FUNCTION_KEY_UP_PLAY_PAUSE, O_RDONLY , 0);
			if (IS_ERR(fp)) 
			{ 
				pr_info("open file %s error!\n", GESTURE_FUNCTION_KEY_UP_PLAY_PAUSE);
			} 
			else 
			{
				TPD_GES_DEBUG("open file %s success!\n", GESTURE_FUNCTION_KEY_UP_PLAY_PAUSE); 
				filp_close(fp, NULL);
				code = KEY_PLAYPAUSE;
				break;
			}	
			break;
#endif
	}
	input_report_key(info->input_dev, code, 1);
	input_sync(info->input_dev);
	input_report_key(info->input_dev, code, 0);
	input_sync(info->input_dev);
	return 0;
}
////=================Customer ges code End===============================////
static void check_gesture(int gesture_id)
{
	struct ft5x0x_ts_data *info = i2c_get_clientdata(this_client);
	TPD_GES_DEBUG("kaka gesture_id==0x%x\n ",gesture_id);
	switch(gesture_id)
	{
#if !defined(ZCFG_TP_FOCALTECH_GESTRUE_NO_DIRECTION)
		case GESTURE_LEFT:
			Ges_trans_key(info, KEY_LEFT);
			break;
		case GESTURE_RIGHT:
			Ges_trans_key(info, KEY_RIGHT);
			break;
		case GESTURE_UP:
			Ges_trans_key(info, KEY_UP);
			break;
		case GESTURE_DOWN:
			Ges_trans_key(info, KEY_DOWN);
			break;
#endif
#if defined(ZCFG_TP_FOCALTECH_GESTRUE_DOUBLECLICK)
		case GESTURE_DOUBLECLICK:
			Ges_trans_key(info, KEY_U);
			break;
#endif
		case GESTURE_O:
			Ges_trans_key(info, KEY_O);
			break;
		case GESTURE_W:
			Ges_trans_key(info, KEY_W);
			break;
		case GESTURE_M:
			Ges_trans_key(info, KEY_M);
			break;
		case GESTURE_E:
			Ges_trans_key(info, KEY_E);
			break;
		case GESTURE_C:
			Ges_trans_key(info, KEY_C);
			break;
			
		case GESTURE_S:
			Ges_trans_key(info, KEY_S);
			break;

		case GESTURE_V:
			Ges_trans_key(info, KEY_V);
			break;

		case GESTURE_Z_FT6336U:
		case GESTURE_Z:
			Ges_trans_key(info, KEY_Z);
			break;

		default:
			break;
	}
}

static int fts_read_Gestruedata(void)
{
	unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
	int ret = -1;
	int i = 0;
	int gestrue_id = 0;
	buf[0] = 0xd3;


	pointnum = 0;

	ret = fts_i2c_Read(this_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
	TPD_GES_DEBUG( "tpd read FTS_GESTRUE_POINTS_HEADER.\n");

	if (ret < 0)
	{
		pr_info( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	/* FW */
	 if (fts_updateinfo_curr.CHIP_ID==0x54 || fts_updateinfo_curr.CHIP_ID == 0x64 )
	 {
		gestrue_id = buf[0];
		pointnum = (short)(buf[1]) & 0xff;
		buf[0] = 0xd3;

		if((pointnum * 4 + 8)<255)
		{
			ret = fts_i2c_Read(this_client, buf, 1, buf, (pointnum * 4 + 8));
		}
		else
		{
			ret = fts_i2c_Read(this_client, buf, 1, buf, 255);
			ret = fts_i2c_Read(this_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
		}
		if (ret < 0)
		{
			printk( "%s read touchdata failed.\n", __func__);
			return ret;
		}
		check_gesture(gestrue_id);
		for(i = 0;i < pointnum;i++)
		{
			coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
					8 | (((s16) buf[1 + (4 * i)])& 0xFF);
			coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
					8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
		}
		for(i = 0; i < pointnum; i++)
		{
			TPD_GES_DEBUG("coordinate_x[%d]  = %d",i,coordinate_x[i] );
		}
		return -1;
	}

	if (0x24 == buf[0])
	{
		gestrue_id = 0x24;
		check_gesture(gestrue_id);
		return -1;
	}

	pointnum = (short)(buf[1]) & 0xff;
	buf[0] = 0xd3;

	if((pointnum * 4 + 8)<255)
	{
		 ret = fts_i2c_Read(this_client, buf, 1, buf, (pointnum * 4 + 8));
	}
	else
	{
		 ret = fts_i2c_Read(this_client, buf, 1, buf, 255);
		 ret = fts_i2c_Read(this_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
	}
	if (ret < 0)
	{
		printk( "%s read touchdata failed.\n", __func__);
		return ret;
	}

//	gestrue_id = fetch_object_sample(buf, pointnum);
	check_gesture(gestrue_id);

	for(i = 0;i < pointnum;i++)
	{
		coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
			8 | (((s16) buf[1 + (4 * i)])& 0xFF);
		coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
			8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
	}
	return -1;
}
#endif

#define BATTERY_STATUS_PATH "/sys/class/power_supply/battery/status"
static int ft5x0x_update_data(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	u8 buf[33] = {0};
	int ret = -1;
	int i;
	u16 x , y;
	u8 ft_pressure , ft_size;
	
/*********************add by chenqx for TP shake when charging***************************/
#if defined(ZCFG_TP_FOCALTECH_CHARGE_SHAKE)
	struct file *pfile = NULL;
	loff_t pos;
	mm_segment_t old_fs;
	char firmware_buf[8]={0};

	pfile = filp_open(BATTERY_STATUS_PATH, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		printk("[chenqx]can not open BATTERY_STATUS_PATH\n");
	}
	else
	{
		memset(firmware_buf,0x0,8);
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		pos = 0;
		vfs_read(pfile, firmware_buf, 8, &pos);
		filp_close(pfile, NULL);
		set_fs(old_fs);
		
		if (strncmp(firmware_buf, "Charging",8) == 0){
				printk("[chenqx]Charging\n");
				ft5x0x_write_reg(0x8b, 0x1);
			}
		else
		{
			printk("[chenqx]Not charging\n");
			ft5x0x_write_reg(0x8b, 0x0);
		}
	}
#endif
/*********************add by chenqx for TP shake when charging***************************/

	ret = ft5x0x_i2c_rxdata(buf, 33);

	if (ret < 0) {
		pr_err("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07;

#if defined(TP_PROXIMITY_SENSOR)
	if (tpd_proximity_flag) {
		u8 proximity_flag=0;
	#if defined(ZCFG_FT6X36_PS_REG_FAIL_PATCH)
		u8 FT_REGS_PS_CTL_VALIUE=0;

		ft5x0x_read_reg(FT_REGS_PS_CTL, &FT_REGS_PS_CTL_VALIUE);
		if(FT_REGS_PS_CTL_VALIUE ==0)
		{
		  TPD_PROXIMITY_DEBUG("proximity_flag FT6X36 Read 0XB0 Reg Error,Rewrite it");
		  ft5x0x_write_reg(FT_REGS_PS_CTL, 0x1);
		  ft5x0x_read_reg(0x01, &proximity_flag);
		}
		TPD_PROXIMITY_DEBUG("proximity_flag FT_REGS_PS_CTL_VALIUE=0x%x\n", FT_REGS_PS_CTL_VALIUE);
	#endif
		//  0x01-->C0 ê????ü  E0 ê???à?
		ft5x0x_read_reg(0x01, &proximity_flag);
		TPD_PROXIMITY_DEBUG("proximity_flag =0x%x\n", proximity_flag);
		if (0xE0 == proximity_flag)// leave
		{
			if(!tpd_proximity_dir_faraway)
			{
				tpd_proximity_dir_faraway = 1;
				TPD_PROXIMITY_DEBUG("tpd_proximity_dir_faraway=%d\n", tpd_proximity_dir_faraway);
				input_report_abs(data->input_dev, ABS_DISTANCE, tpd_proximity_dir_faraway);
				input_mt_sync(data->input_dev);
				input_sync(data->input_dev);
				return 0;
			}
			else
			{
				TPD_PROXIMITY_DEBUG("11111111 dirty faraway received\n");
			}
		}
		else if (0xC0 == proximity_flag)// close to
		{
			tpd_proximity_dir_faraway = 0;
			TPD_PROXIMITY_DEBUG("tpd_proximity_dir_faraway=%d\n", tpd_proximity_dir_faraway);
			input_report_abs(data->input_dev, ABS_DISTANCE, tpd_proximity_dir_faraway);
			input_mt_sync(data->input_dev);
			input_sync(data->input_dev);
			return 0;
		}
		else if(first_proximity_faraway==1)
		{
				first_proximity_faraway=0;
				tpd_proximity_dir_faraway = 1;
				TPD_PROXIMITY_DEBUG("first_proximity_faraway=%d\n", tpd_proximity_dir_faraway);
				input_report_abs(data->input_dev, ABS_DISTANCE, tpd_proximity_dir_faraway);
				input_mt_sync(data->input_dev);
				input_sync(data->input_dev);
				return 0;
		}
	}
#endif
	for(i = 0; i < TS_MAX_FINGER; i++) {
		if((buf[6*i+3] & 0xc0) == 0xc0)
			continue;
		x = (s16)(buf[6*i+3] & 0x0F)<<8 | (s16)buf[6*i+4];	
		y = (s16)(buf[6*i+5] & 0x0F)<<8 | (s16)buf[6*i+6];
		ft_pressure = buf[6*i+7];
		if(ft_pressure > 127)
			ft_pressure = 127;
		ft_size = (buf[6*i+8]>>4) & 0x0F;
		if((buf[6*i+3] & 0x40) == 0x0) {
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);
		#endif
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE, ft_pressure);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, ft_size);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
		#if !MULTI_PROTOCOL_TYPE_B
			input_mt_sync(data->input_dev);
		#endif
			pr_debug("===x%d = %d,y%d = %d ====",i, x, i, y);
		}
		else {
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		#endif
		}
	}
	if(0 == event->touch_point) {
		for(i = 0; i < TS_MAX_FINGER; i ++) {
			#if MULTI_PROTOCOL_TYPE_B
                            input_mt_slot(data->input_dev, i);
                            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
			#endif
		}
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		#if !MULTI_PROTOCOL_TYPE_B
			input_mt_sync(data->input_dev);
		#endif

	}
	input_sync(data->input_dev);

	return 0;
}

#if USE_WAIT_QUEUE
static int touch_event_handler(void *unused)
{
#if defined(FTS_GESTRUE)
	u8 state;
#endif
	//struct sched_param param = { .sched_priority = 5 };
	//sched_setscheduler(current, SCHED_RR, &param);

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, (0 != tpd_flag));
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
#if defined(FTS_GESTRUE)
		i2c_smbus_read_i2c_block_data(this_client, 0xd0, 1, &state);
		TPD_GES_DEBUG("====touch_event_handler== state = %d\n",state);
		if (state == 1)
		{
#if GESTURE_WAKE_LOCK
			__pm_stay_awake(&suspend_gestrue_lock);
#endif
            fts_read_Gestruedata();
            continue;
		}
#endif
		ft5x0x_update_data();

	} while (!kthread_should_stop());

	return 0;
}
#endif

#if USE_WORK_QUEUE
static void ft5x0x_ts_pen_irq_work(struct work_struct *work)
{
	ft5x0x_update_data();
	enable_irq(this_client->irq);
}
#endif

static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
#if defined(FTS_GESTRUE)
	if (1==s_gesture_switch)
	{
		irq_set_irq_type(this_client->irq,IRQF_TRIGGER_FALLING);
	}
#endif

#if USE_WAIT_QUEUE
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
#endif

#if USE_WORK_QUEUE
	struct ft5x0x_ts_data *ft5x0x_ts = (struct ft5x0x_ts_data *)dev_id;

	if (!work_pending(&ft5x0x_ts->pen_event_work)) {
		queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
	}
	return IRQ_HANDLED;
#endif

#if USE_THREADED_IRQ
	ft5x0x_update_data();
	return IRQ_HANDLED;
#endif

}

static void ft5x0x_ts_reset(void)
{
	struct ft5x0x_ts_platform_data *pdata = g_ft5x0x_ts->platform_data;

	gpio_direction_output(pdata->reset_gpio_number, 1);
	msleep(1);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(200);
}

#if 0
//for future use

struct regulator *vdd28 = NULL;

static void ft53x6_power_off(void)
{
	if(vdd28 != NULL)
		regulator_force_disable(vdd28);
	PRINT_INFO("power off\n");
}

static void ft53x6_power_on(void)
{
	int err = 0;

	if(vdd28 == NULL) {
		vdd28 = regulator_get(NULL, "vdd28");
		if (IS_ERR(vdd28)) {
			PRINT_ERR("regulator_get failed\n");
			return;
		}
		err = regulator_set_voltage(vdd28,2800000,2800000);
		if (err)
			PRINT_ERR("regulator_set_voltage failed\n");
	}
	regulator_enable(vdd28);

	PRINT_INFO("power on\n");
}
#endif




void ft5x0x_suspend(void)
{
	int ret = -1;
	#if defined(FTS_GESTRUE)
	struct file *fp; 
	#endif
	struct ft5x0x_ts_data  *ft5x0x_ts = (struct ft5x0x_ts_data *)i2c_get_clientdata(this_client);
	pr_info("==%s==\n", __FUNCTION__);
	mutex_lock(&enable_irq_mutex);
	ft5x0x_ts->platform_data->suspend = true;
#ifdef TP_PROXIMITY_SENSOR
	TPD_PROXIMITY_DEBUG("ft5x0x_ts_suspend: tpd_proximity_flag=%d, tpd_halt=%d\n", tpd_proximity_flag, tpd_halt);
	if(tpd_proximity_flag == 1)
	{
		tpd_halt=1;
		mutex_unlock(&enable_irq_mutex);
		return;
	}

	tpd_halt = 0;
#endif

#if defined(FTS_GESTRUE)
	fp = filp_open(GESTURE_FUNCTION_SWITCH, O_RDONLY , 0); 
	if (IS_ERR(fp)) 
	{ 
		pr_info("open file %s error!\n", GESTURE_FUNCTION_SWITCH);
		s_gesture_switch = 1;	
	} 
	else 
	{
		printk("open file %s success!\n", GESTURE_FUNCTION_SWITCH);
		s_gesture_switch = 0;	
		filp_close(fp, NULL); 
	}

	if (1==s_gesture_switch)
	{
		enable_irq_wake(this_client->irq);
		irq_set_irq_type(this_client->irq,IRQF_TRIGGER_LOW|IRQF_NO_SUSPEND);
		
		mutex_unlock(&enable_irq_mutex);
		fts_write_reg(this_client, 0xd0, 0x01);
		pr_info("=====FTS_GESTRUE====>ft5x0x_ts_suspend, CHIP_ID = 0x%x. \n", fts_updateinfo_curr.CHIP_ID);
		if (fts_updateinfo_curr.CHIP_ID==0x54 || fts_updateinfo_curr.CHIP_ID == 0x64 )
		{
			fts_write_reg(this_client, 0xd1, 0xff);
			fts_write_reg(this_client, 0xd2, 0xff);
			fts_write_reg(this_client, 0xd5, 0xff);
			fts_write_reg(this_client, 0xd6, 0xff);
			fts_write_reg(this_client, 0xd7, 0xff);
			fts_write_reg(this_client, 0xd8, 0xff);
		}
		return;
	}
#endif
	ret = ft5x0x_write_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	if(ret){
		PRINT_ERR("==ft5x0x_ts_suspend==  ft5x0x_write_reg fail\n");
	}
	disable_irq(this_client->irq);
	
	mutex_unlock(&enable_irq_mutex);
	//irq_num--;

	//TPD_PROXIMITY_DEBUG("ft5x0x_ts_suspend: tpd_proximity_flag=%d,irq_num=%d\n", tpd_proximity_flag, irq_num);
	ft5x0x_clear_report_data(g_ft5x0x_ts);
	//gpio_set_value(g_ft5x0x_ts->platform_data->reset_gpio_number, 0);//for future use
}


void ft5x0x_resume(void)
{
		struct ft5x0x_ts_data  *ft5x0x_ts = (struct ft5x0x_ts_data *)i2c_get_clientdata(this_client);
		ft5x0x_ts->platform_data->suspend = false;
#ifdef TP_PROXIMITY_SENSOR
		TPD_PROXIMITY_DEBUG("ft5x0x_ts_resume: tpd_proximity_flag=%d, tpd_halt=%d\n", tpd_proximity_flag, tpd_halt);
		//Alex.shi on android7.0,if HIC_ENABLE_DEFAULT = 1,then :disable_ps->suspend->resume->enable_ps.But sometime suspend->disable_ps->resume->enable_ps,
		//	then we didnpt exec disable_irq in suspend,but exec enable_irq in resume,and after this ,tp can't work
		if (/*(1 == tpd_proximity_flag) &&*/ (1 == tpd_halt))
		{	
			return;
		}
#endif

		queue_work(ft5x0x_ts->ts_resume_workqueue, &ft5x0x_ts->resume_work);
}

#if defined(TS_USE_ADF_NOTIFIER) 

static int ts_adf_event_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	struct adf_notifier_event *event = data;
	int adf_event_data;


	if (action != ADF_EVENT_BLANK)
		return NOTIFY_DONE;

	adf_event_data = *(int *)event->data;
	//PRINT_INFO("receive adf event with adf_event_data=%d", adf_event_data);

	switch (adf_event_data) {
	case DRM_MODE_DPMS_ON:
		ft5x0x_resume();
		break;
	case DRM_MODE_DPMS_OFF:
		ft5x0x_suspend();
		break;
	default:
		//TS_WARN("receive adf event with error data, adf_event_data=%d",adf_event_data);
		break;
	}

	return NOTIFY_OK;
}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	ft5x0x_suspend();
}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{
	ft5x0x_resume();
}
#endif

static void ft5x0x_ts_resume_work(struct work_struct *work)
{
	pr_info("==%s==\n", __FUNCTION__);
	mutex_lock(&enable_irq_mutex);
#ifdef FTS_GESTRUE
	if (1==s_gesture_switch) 
	{
		disable_irq_wake(this_client->irq);
		irq_set_irq_type(this_client->irq,IRQF_TRIGGER_FALLING);
		fts_write_reg(this_client,0xd0,0x00);
	}
#endif
	ft5x0x_ts_reset();
	//ft5x0x_write_reg(FT5X0X_REG_PERIODACTIVE, 7);
#if !defined(TP_PROXIMITY_SENSOR) 
	#if defined(FTS_GESTRUE)
	if (1 != s_gesture_switch)
	#endif
		enable_irq(this_client->irq);
#endif

	msleep(2);
	ft5x0x_clear_report_data(g_ft5x0x_ts);
#ifdef TP_PROXIMITY_SENSOR
	msleep(100);

	if((tpd_halt ==0)
	#if defined(FTS_GESTRUE)
		&&  (1 != s_gesture_switch)
	#endif
	)
	enable_irq(this_client->irq);
	//irq_num++;

	//TPD_PROXIMITY_DEBUG("ft5x0x_ts_resume_work: tpd_proximity_flag=%d,irq_num=%d\n", tpd_proximity_flag, irq_num);
	if(tpd_proximity_flag == 1)
	{
		FT6206_enable_ps(1);	
	}
#endif
#if defined(FTS_GESTRUE)
	s_gesture_switch = 0;
#endif
	mutex_unlock(&enable_irq_mutex);
}


static void ft5x0x_ts_hw_init(struct ft5x0x_ts_data *ft5x0x_ts)
{
	struct regulator *reg_vdd;
	struct i2c_client *client = ft5x0x_ts->client;
	struct ft5x0x_ts_platform_data *pdata = ft5x0x_ts->platform_data;
	int ret = 0;

	pr_info("[FST] %s [irq=%d];[rst=%d]\n",__func__,
		pdata->irq_gpio_number,pdata->reset_gpio_number);
	gpio_request(pdata->irq_gpio_number, "ts_irq_pin");
	gpio_request(pdata->reset_gpio_number, "ts_rst_pin");
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);

	reg_vdd = regulator_get(&client->dev, pdata->vdd_name);
	if (!WARN(IS_ERR(reg_vdd), "[FST] ft5x0x_ts_hw_init regulator: failed to get %s.\n", pdata->vdd_name)) {
		if(!strcmp(pdata->vdd_name,"vdd18"))
			regulator_set_voltage(reg_vdd,1800000,1800000);
		if(!strcmp(pdata->vdd_name,"vdd28"))
			regulator_set_voltage(reg_vdd, 2800000, 2800000);
		ret=regulator_enable(reg_vdd);
			if (ret) {
			printk("ft5x0x_ts_hw_init:regulator_enable fail\n");
		}
	}
	msleep(100);
	ft5x0x_ts_reset();
}

void focaltech_get_upgrade_array(struct i2c_client *client)
{

	u8 chip_id;
	u32 i;

	i2c_smbus_read_i2c_block_data(client,FT_REG_CHIP_ID,1,&chip_id);

	printk("%s chip_id = 0x%x\n", __func__, chip_id);

	for(i=0;i<sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info);i++)
	{
		if(chip_id==fts_updateinfo[i].CHIP_ID)
		{
			memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct Upgrade_Info));
			break;
		}
	}

	if(i >= sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info))
	{
		memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct Upgrade_Info));
	}
}

static int check_ctp_chip(void)
{
	ctp_lock_mutex();
	tp_device_id(0x5206);
	ctp_unlock_mutex();
	return 0;
}

static int remove_ctp_chip(void)
{
	ctp_lock_mutex();
	tp_device_id(0xFFFF);
	ctp_unlock_mutex();
	return 0;
}

#ifdef CONFIG_OF
static struct ft5x0x_ts_platform_data *ft5x0x_ts_parse_dt(struct device *dev)
{
	struct ft5x0x_ts_platform_data *pdata;
	struct device_node *np = dev->of_node;
	int ret;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Could not allocate struct ft5x0x_ts_platform_data");
		return NULL;
	}
	pdata->reset_gpio_number = of_get_gpio(np, 0);
	if(pdata->reset_gpio_number < 0){
		dev_err(dev, "fail to get reset_gpio_number\n");
		goto fail;
	}
	pdata->irq_gpio_number = of_get_gpio(np, 1);
	if(pdata->reset_gpio_number < 0){
		dev_err(dev, "fail to get reset_gpio_number\n");
		goto fail;
	}
	ret = of_property_read_string(np, "vdd_name", &pdata->vdd_name);
	if(ret){
		dev_err(dev, "fail to get vdd_name\n");
		goto fail;
	}
	ret = of_property_read_u32_array(np, "virtualkeys", pdata->virtualkeys,12);
	if(ret){
		dev_err(dev, "fail to get virtualkeys\n");
		goto fail;
	}
#ifdef ZCFG_LCM_WIDTH
	pdata->TP_MAX_X = ZCFG_LCM_WIDTH;
#else
	ret = of_property_read_u32(np, "TP_MAX_X", &pdata->TP_MAX_X);
	if(ret){
		dev_err(dev, "fail to get TP_MAX_X\n");
		goto fail;
	}
#endif
#ifdef ZCFG_LCM_HEIGHT
	pdata->TP_MAX_Y = ZCFG_LCM_HEIGHT;
#else
	ret = of_property_read_u32(np, "TP_MAX_Y", &pdata->TP_MAX_Y);
	if(ret){
		dev_err(dev, "fail to get TP_MAX_Y\n");
		goto fail;
	}
#endif

	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
#endif


static int ft5x0x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x0x_ts_data	*ft5x0x_ts;
	struct input_dev		*input_dev;
	struct ft5x0x_ts_platform_data	*pdata = client->dev.platform_data;
	int						err = 0;
	unsigned char			uc_reg_value;
#ifdef CONFIG_OF
	struct device_node		*np = client->dev.of_node;
#endif

	pr_info("[FST] %s: probe\n",__func__);

	if(tp_device_id(0)!=0)
	{
		printk("CTP(0x%x)Exist!", tp_device_id(0));
		return -ENODEV;
	}

#ifdef CONFIG_OF
	if (np && !pdata){
		pdata = ft5x0x_ts_parse_dt(&client->dev);
		if(pdata){
			client->dev.platform_data = pdata;
		}
		else{
			err = -ENOMEM;
			goto exit_alloc_platform_data_failed;
		}
	}
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
	if (!ft5x0x_ts)	{
		err = -ENOMEM;
		goto exit_check_functionality_failed;
	}

	g_ft5x0x_ts = ft5x0x_ts;
	ft5x0x_ts->platform_data = pdata;
	this_client = client;
	ft5x0x_ts->client = client;
	ft5x0x_ts_hw_init(ft5x0x_ts);
	i2c_set_clientdata(client, ft5x0x_ts);
	client->irq = gpio_to_irq(pdata->irq_gpio_number);

	#if(defined(CONFIG_I2C_SPRD) || defined(CONFIG_I2C_SPRD_V1))
	//sprd_i2c_ctl_chg_clk(client->adapter->nr, 400000);
	#endif

	err = ft5x0x_read_reg(FT5X0X_REG_CIPHER, &uc_reg_value);
	if (err < 0)
	{
		pr_err("[FST] read chip id error %x\n", uc_reg_value);
		err = -ENODEV;
		goto exit_chip_check_failed;
	}
	check_ctp_chip();
	/* set report rate, about 70HZ */
	//ft5x0x_write_reg(FT5X0X_REG_PERIODACTIVE, 7);
#if USE_WORK_QUEUE
	INIT_WORK(&ft5x0x_ts->pen_event_work, ft5x0x_ts_pen_irq_work);

	ft5x0x_ts->ts_workqueue = create_singlethread_workqueue("focal-work-queue");
	if (!ft5x0x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_check_ctp_chip;
	}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)||defined(CONFIG_PM_SLEEP)
	INIT_WORK(&ft5x0x_ts->resume_work, ft5x0x_ts_resume_work);
	ft5x0x_ts->ts_resume_workqueue = create_singlethread_workqueue("ft5x0x_ts_resume_work");
	if (!ft5x0x_ts->ts_resume_workqueue) {
		err = -ESRCH;
		goto exit_create_resume_workqueue;
	}
#endif

#ifdef TP_PROXIMITY_SENSOR
	err = misc_register(&FT6206_proximity_misc);
	if (err < 0)
	{
		pr_err("%s: could not register misc device\n", __func__);
		goto err_mis_reg;
	}
#endif

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "[FST] failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
#ifdef TOUCH_VIRTUAL_KEYS
	ft5x0x_ts_virtual_keys_init();
#endif
	ft5x0x_ts->input_dev = input_dev;

#ifdef TP_PROXIMITY_SENSOR
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);
#endif
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	__set_bit(KEY_APPSELECT,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOMEPAGE,  input_dev->keybit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

#if MULTI_PROTOCOL_TYPE_B
	input_mt_init_slots(input_dev, TS_MAX_FINGER,0);
#endif
	input_set_abs_params(input_dev,ABS_MT_POSITION_X, 0, pdata->TP_MAX_X, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_POSITION_Y, 0, pdata->TP_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#if defined(ZCFG_TP_FOCALTECH_FIVE_POINTS_TOUCH)
	input_set_abs_params(input_dev,ABS_MT_TRACKING_ID, 0, 10, 0, 0);
#endif
	#if 0
	/*ft5306's firmware is qhd, ft5316's firmware is 720p*/
	if (uc_reg_value == 0x0a || uc_reg_value == 0x0) {
		input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, 720, 0, 0);
		input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, 1280, 0, 0);
	} else {
		input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, 540, 0, 0);
		input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, 960, 0, 0);
	}
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

	#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
#if defined(FTS_GESTRUE)
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_LEFT);
	input_set_capability(input_dev, EV_KEY, KEY_RIGHT);
	input_set_capability(input_dev, EV_KEY, KEY_UP);
	input_set_capability(input_dev, EV_KEY, KEY_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_U);
	input_set_capability(input_dev, EV_KEY, KEY_O);
	input_set_capability(input_dev, EV_KEY, KEY_W);
	input_set_capability(input_dev, EV_KEY, KEY_M);
	input_set_capability(input_dev, EV_KEY, KEY_E);
	input_set_capability(input_dev, EV_KEY, KEY_C);
	input_set_capability(input_dev, EV_KEY, KEY_V);
	input_set_capability(input_dev, EV_KEY, KEY_Z);
	input_set_capability(input_dev, EV_KEY, KEY_S);
	input_set_capability(input_dev, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input_dev, EV_KEY, KEY_NEXTSONG);
	input_set_capability(input_dev, EV_KEY, KEY_PLAYPAUSE);

#if GESTURE_WAKE_LOCK
	wakeup_source_init(&suspend_gestrue_lock, "suspend_gestrue");
#endif	

#endif

	input_dev->name = FOCALTECH_TS_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"[FST] ft5x0x_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#if USE_THREADED_IRQ
	err = request_threaded_irq(client->irq, NULL, ft5x0x_ts_interrupt, 
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ft5x0x_ts);
#elif defined(FTS_GESTRUE) || defined(TP_PROXIMITY_SENSOR)
    err = request_irq(client->irq, ft5x0x_ts_interrupt,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, ft5x0x_ts);
#else
	err = request_irq(client->irq, ft5x0x_ts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ft5x0x_ts);
#endif
	if (err < 0) {
		dev_err(&client->dev, "[FST] ft5x0x_probe: request irq failed %d\n",err);
		goto exit_irq_request_failed;
	}
#ifdef TS_USE_ADF_NOTIFIER
	adf_event_block.notifier_call = ts_adf_event_handler;
	adf_register_client(&adf_event_block);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x_ts->early_suspend.suspend = ft5x0x_ts_suspend;
	ft5x0x_ts->early_suspend.resume	= ft5x0x_ts_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

focaltech_get_upgrade_array(client);

#ifdef SYSFS_DEBUG	
fts_create_sysfs(client);
err = sysfs_create_link(NULL, &client->dev.kobj, "touchscreen");
if (err < 0) {
	//PRINT_ERR("Failed to create link!",err);
	return -ENOMEM;
}

#endif

#ifdef FTS_CTL_IIC	
if (ft_rw_iic_drv_init(client) < 0)	
{
	dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",	__func__);
		goto exit_iic_drv_init_failed;
}
#endif

#ifdef SPRD_AUTO_UPGRADE
	printk("********************Enter CTP Auto Upgrade********************\n");
	fts_ctpm_auto_upgrade(client);
#endif   

#ifdef APK_DEBUG
	ft5x0x_create_apk_debug_channel(client);
#endif

#if USE_WAIT_QUEUE
	thread = kthread_run(touch_event_handler, 0, "focal-wait-queue");
	if (IS_ERR(thread))
	{
		err = PTR_ERR(thread);
		//PRINT_ERR("failed to create kernel thread: %d\n", err);
		goto exit_kthread_run_failed;
	}
#endif
#ifdef FTS_GESTRUE
	pr_info("FTS_GESTRUE init!\n");
	//init_para(720,1280,100,0,0);
#endif
	mutex_init(&enable_irq_mutex);
	{
		u8 tp_fm_ver = 0;
		
		extern void zyt_info_s2(char* s1,char* s2);
		extern void zyt_info_sx(char* c1,int x);

		fts_read_reg(client, FT_REG_FW_VER, &tp_fm_ver);
		zyt_info_s2("[TP] : ",fts_updateinfo_curr.FTS_NAME);
		zyt_info_sx("[FT63XX] : FW Version:",tp_fm_ver);
	}
	pr_info("%s: probe Success!\n", __func__);
	return 0;

exit_kthread_run_failed:
#ifdef APK_DEBUG
	ft5x0x_release_apk_debug_channel();
#endif
#ifdef FTS_CTL_IIC	
	ft_rw_iic_drv_exit();
#endif
exit_iic_drv_init_failed:
#ifdef SYSFS_DEBUG
	sysfs_remove_link(NULL, "touchscreen");
	fts_release_sysfs(client);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x0x_ts->early_suspend);
#endif
	free_irq(client->irq, ft5x0x_ts);
exit_irq_request_failed:
	input_unregister_device(input_dev);
exit_input_register_device_failed:
	ft5x0x_ts_virtual_keys_destroy();
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
#ifdef TP_PROXIMITY_SENSOR	
	misc_deregister(&FT6206_proximity_misc);
	err_mis_reg:
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (ft5x0x_ts->ts_resume_workqueue) {
		destroy_workqueue(ft5x0x_ts->ts_resume_workqueue);
	}
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) ||defined(CONFIG_PM_SLEEP)
exit_create_resume_workqueue:
#endif
#if USE_WORK_QUEUE
	if (ft5x0x_ts->ts_workqueue)
	{
		destroy_workqueue(ft5x0x_ts->ts_workqueue);
	}
#endif
#if USE_WORK_QUEUE
exit_check_ctp_chip:
#endif
	remove_ctp_chip();
exit_chip_check_failed:
	gpio_free(pdata->irq_gpio_number);
	gpio_free(pdata->reset_gpio_number);
	kfree(ft5x0x_ts);
	ft5x0x_ts = NULL;
	i2c_set_clientdata(client, NULL);

exit_check_functionality_failed:
	if (client->dev.platform_data) {
		kfree(client->dev.platform_data);
		client->dev.platform_data = NULL;
	}
exit_alloc_platform_data_failed:
	return err;
}

static int ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts = i2c_get_clientdata(client);

	pr_info("==ft5x0x_ts_remove=\n");
	
	#ifdef SYSFS_DEBUG
	fts_release_sysfs(client);
	#endif
	#ifdef FTS_CTL_IIC	
	ft_rw_iic_drv_exit();
	#endif
	#ifdef APK_DEBUG
	ft5x0x_release_apk_debug_channel();
	#endif
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x0x_ts->early_suspend);
#endif
	free_irq(client->irq, ft5x0x_ts);
	input_unregister_device(ft5x0x_ts->input_dev);
	ft5x0x_ts_virtual_keys_destroy();
	input_free_device(ft5x0x_ts->input_dev);
#ifdef TP_PROXIMITY_SENSOR	
	misc_deregister(&FT6206_proximity_misc);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (ft5x0x_ts->ts_resume_workqueue) {
		destroy_workqueue(ft5x0x_ts->ts_resume_workqueue);
	}
#endif
#if USE_WORK_QUEUE
	if (ft5x0x_ts->ts_workqueue)
	{
		destroy_workqueue(ft5x0x_ts->ts_workqueue);
	}
#endif
	remove_ctp_chip();
	if (ft5x0x_ts->platform_data->reset_gpio_number>0)
		gpio_free(ft5x0x_ts->platform_data->reset_gpio_number);
	if (ft5x0x_ts->platform_data->irq_gpio_number>0)
		gpio_free(ft5x0x_ts->platform_data->irq_gpio_number);
	if (ft5x0x_ts) {
		kfree(ft5x0x_ts);
		ft5x0x_ts = NULL;
	}
	i2c_set_clientdata(client, NULL);
	if (client->dev.platform_data) {
		kfree(client->dev.platform_data);
		client->dev.platform_data = NULL;
	}

	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{ FOCALTECH_TS_NAME, 0 },{ }
};

#if 0
static int ft5x0x_suspend(struct i2c_client *client, pm_message_t mesg)
{ 
	printk("ft5x0x_suspend\n");
#if defined(FTS_GESTRUE)
	if (1==s_gesture_switch)
	{
    	irq_set_irq_type(this_client->irq,IRQF_TRIGGER_LOW|IRQF_NO_SUSPEND); 
	}
#endif
    return 0; 
} 
static int ft5x0x_resume(struct i2c_client *client)
{
	printk("ft5x0x_resume\n");
	return 0;
}
#endif

MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static const struct of_device_id focaltech_of_match[] = {
       { .compatible = "focaltech,focaltech_ts", },
       { }
};

MODULE_DEVICE_TABLE(of, focaltech_of_match);
static struct i2c_driver ft5x0x_ts_driver = {
	.probe		= ft5x0x_ts_probe,
	.remove		= ft5x0x_ts_remove,
	.id_table	= ft5x0x_ts_id,
	.driver	= {
		.name	= FOCALTECH_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = focaltech_of_match,
	},
	
//	.suspend = ft5x0x_suspend,
//	.resume = ft5x0x_resume,
};

static int __init ft5x0x_ts_init(void)
{
	pr_info("==ft5x0x_ts_init==\n");

	if(tp_device_id(0)!=0)
	{
		printk("CTP(0x%x)Exist!", tp_device_id(0));
		return -ENODEV;
	}
	return i2c_add_driver(&ft5x0x_ts_driver);
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
