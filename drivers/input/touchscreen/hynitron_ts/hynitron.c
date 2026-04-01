/*
 * drivers/input/touchscreen/cst8xx_ts.c
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
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/fs.h>

#if(defined(CONFIG_I2C_SPRD) ||defined(CONFIG_I2C_SPRD_V1))
//#include <soc/sprd/i2c-sprd.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <soc/sprd/board.h>

#include "hynitron.h"
#include "hynitron_ex_fun.h"
#include "hynitron_ctl.h"





#define HYNITRON_TP_PROC_SELF_TEST  // factory_test online enable

#if defined(ZCFG_MK_TP_GESTURE)
#define HYN_GESTRUE
#endif

#if defined(ZCFG_MK_TP_PROXIMITY)
#define TP_PROXIMITY_SENSOR
#endif

#ifdef CONFIG_PM_SLEEP
#include <video/adf_notifier.h>
#endif

#define HYN_DRIVER_VERSION      "HYN_CY_Spreadtrum7731E_8.1_V1.0_20190219"
#define HYN_PROJECT_ID          "CPH_1901007_CST836U_HYN_Y550006A2_R_LS036_财富 "


#if HYN_EN_AUTO_UPDATE
unsigned char *p_cst836u_upgrade_firmware=NULL;
unsigned char  apk_upgrade_flag=0;
extern unsigned char app_bin[];
#endif

#ifdef HYNITRON_TP_PROC_SELF_TEST
#include "hynitron_rawtest_config.h"
#endif


#ifdef TP_PROXIMITY_SENSOR
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#define PROXIMITY_DEVICE		"ctp_proximity"

//ioctl cmd
#define HYN_IOCTL_MAGIC			0X5D
#define HYN_IOCTL_PROX_ON		_IO(HYN_IOCTL_MAGIC, 7)
#define HYN_IOCTL_PROX_OFF		_IO(HYN_IOCTL_MAGIC, 8)

#define HYN_REGS_PS_CTL			0xB0

#define TPD_PROXIMITY_DEVICE			"CST8XX"

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

//#define CST836U_DBG
#ifdef CST836U_DBG
#define ENTER printk(KERN_INFO "[CST836U_DBG] func: %s  line: %04d\n", __func__, __LINE__);
#define PRINT_DBG(x...)  printk(KERN_INFO "[CST836U_DBG] " x)
#define PRINT_INFO(x...)  printk(KERN_INFO "[CST836U_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[CST836U_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[CST836U_WARN] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#else
#define ENTER
#define PRINT_DBG(x...)
#define PRINT_INFO(x...)  printk(KERN_INFO "[CST836U_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[CST836U_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[CST836U_WARN] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#endif

extern int tp_device_id(int id);
extern void ctp_lock_mutex(void);
extern void ctp_unlock_mutex(void);
extern int  cst8xx_proc_fs_init(void);




#ifdef CONFIG_PM_SLEEP
static struct notifier_block adf_event_block;
#endif

struct mutex enable_irq_mutex_hyn;  // 增加一个互斥信号量，修改连续快速按power键进行休眠唤醒操作会导致tp无响应的问题。
//int irq_num=10;

#define APK_DEBUG
#if defined(ZCFG_TP_FOCALTECH_AUTO_UPGRADE)
#define SPRD_AUTO_UPGRADE
#endif
#define SYSFS_DEBUG
#define HYN_CTL_IIC


#define	USE_WAIT_QUEUE	1
#define	USE_THREADED_IRQ	0
#define	USE_WORK_QUEUE	0

#define	TOUCH_VIRTUAL_KEYS
#define	MULTI_PROTOCOL_TYPE_B	0
#ifdef CONFIG_ARCH_SCX35LT8
#define	TS_MAX_FINGER		2
#else
#define	TS_MAX_FINGER		5
#define	FTS_PACKET_LENGTH	128
#endif


#ifdef HYN_GESTRUE
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

#define HYN_GESTRUE_POINTS 255
#define HYN_GESTRUE_POINTS_ONETIME	62
#define HYN_GESTRUE_POINTS_HEADER 8
#define HYN_GESTURE_OUTPUT_ADRESS 0xD3
#define HYN_GESTURE_OUTPUT_UNIT_LENGTH 4

short pointnum = 0;
unsigned short coordinate_x[260] = {0};
unsigned short coordinate_y[260] = {0};


//add wake lock for gestrue
#define GESTURE_WAKE_LOCK 1
#if GESTURE_WAKE_LOCK
struct wake_lock suspend_gestrue_lock;
#endif

#endif

/***************************************************************/

#if USE_WAIT_QUEUE
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag = 0;
#endif

//static int fw_size;

static struct cst8xx_ts_data *g_cst8xx_ts;
static struct i2c_client *this_client;
u8 tp_fm_ver = 0;

//static unsigned char suspend_flag = 0;


 struct Upgrade_Info hyn_updateinfo[] =
{
	{0x13,"HYN_CST836U",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000},
    {0x14,"HYN_CST836A",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 1, 2000}
};
				
struct Upgrade_Info hyn_updateinfo_curr;
#ifdef SPRD_AUTO_UPGRADE
extern int fts_ctpm_auto_upgrade(struct i2c_client *client);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cst8xx_ts_suspend(struct early_suspend *handler);
static void cst8xx_ts_resume(struct early_suspend *handler);
#endif


struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	pressure;
    u8  touch_point;
};

struct cst8xx_ts_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	struct ts_event	event;
#if USE_WORK_QUEUE
	struct work_struct	pen_event_work;
	struct workqueue_struct	*ts_workqueue;
#endif
#if defined(CONFIG_PM_SLEEP)
struct work_struct		 resume_work;
struct workqueue_struct *ts_resume_workqueue;

#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct work_struct       resume_work;
	struct workqueue_struct *ts_resume_workqueue;
	struct early_suspend	early_suspend;
#endif
	struct cst8xx_ts_platform_data	*platform_data;
};


static int cst8xx_i2c_rxdata(char *rxdata, int length)
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

static int cst8xxi2c_txdata(char *txdata, int length)
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
Name	:	 cst8xx_write_reg

Input	:	addr -- address
                     para -- parameter

Output	:

function	:	write register of ft5x0x

***********************************************************************************************/
static int cst8xx_write_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = cst8xxi2c_txdata(buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}

/***********************************************************************************************
Name	:	cst8xx_read_reg

Input	:	addr
                     pdata

Output	:

function	:	read register of ft5x0x

***********************************************************************************************/
static int cst8xx_read_reg(u8 addr, u8 *pdata)
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
	struct cst8xx_ts_data *data = i2c_get_clientdata(this_client);
	struct cst8xx_ts_platform_data *pdata = data->platform_data;
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

static void cst8xx_ts_virtual_keys_init(void)
{
    int ret = 0;

    pr_info("[HYN] %s\n",__func__);

    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");
}

static void cst8xx_ts_virtual_keys_destroy(void)
{
	if (properties_kobj)
	{
		sysfs_remove_group(properties_kobj, &properties_attr_group);
		kobject_del(properties_kobj);
	}
}
#endif


#ifdef TP_PROXIMITY_SENSOR

static int CST8XX_enable_ps(int enable)
{
	TPD_PROXIMITY_INFO("CST8XX_enable_ps %d!\n", enable);
	if(enable==1)
	{
		tpd_proximity_flag =1;
		cst8xx_write_reg(HYN_REGS_PS_CTL, 0x1);
		if(first_call==1)
		{
			first_call=0;
			first_proximity_faraway=1;
		}
	}
	else
	{	
		tpd_proximity_flag =0;
		cst8xx_write_reg(HYN_REGS_PS_CTL, 0x0);
	}

	return 1;
}

static long CST8XX_ioctl_operate(struct file *file, unsigned int cmd, unsigned long arg)
{
	TPD_PROXIMITY_INFO("CST8XX_ioctl_operate %d!\n", cmd);

	switch(cmd)
	{
	case HYN_IOCTL_PROX_ON:
		CST8XX_enable_ps(1);   
		break;

	case HYN_IOCTL_PROX_OFF:
		CST8XX_enable_ps(0);
		break;

	default:
		pr_err("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
		return -EINVAL;
	}
	return 0;
}

static struct file_operations CST8XX_proximity_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.release = NULL,
	.unlocked_ioctl = CST8XX_ioctl_operate
};

struct miscdevice CST8XX_proximity_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PROXIMITY_DEVICE,		//match the hal's name 
	.fops = &CST8XX_proximity_fops,
};
#endif

static void cst8xx_clear_report_data(struct cst8xx_ts_data *cst8xx_ts)
{
	int i;

	for(i = 0; i < TS_MAX_FINGER; i++) {
	#if MULTI_PROTOCOL_TYPE_B
		input_mt_slot(cst8xx_ts->input_dev, i);
		input_mt_report_slot_state(cst8xx_ts->input_dev, MT_TOOL_FINGER, false);
	#endif
	}
	input_report_key(cst8xx_ts->input_dev, BTN_TOUCH, 0);
	#if !MULTI_PROTOCOL_TYPE_B
		input_mt_sync(cst8xx_ts->input_dev);
	#endif
	input_sync(cst8xx_ts->input_dev);
}
#ifdef HYN_GESTRUE
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

int Ges_trans_key(struct cst8xx_ts_data *info,unsigned int code)
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
	struct cst8xx_ts_data *info = i2c_get_clientdata(this_client);
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

static int cst8xx_read_Gestruedata(void)
{
	unsigned char buf[HYN_GESTRUE_POINTS * 3] = { 0 };
	int ret = -1;
	int i = 0;
	int gestrue_id = 0;
	buf[0] = 0xd3;


	pointnum = 0;

	ret = hyn_i2c_Read(this_client, buf, 1, buf, HYN_GESTRUE_POINTS_HEADER);
	TPD_GES_DEBUG( "tpd read HYN_GESTRUE_POINTS_HEADER.\n");

	if (ret < 0)
	{
		pr_info( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	/* FW */
	 if (hyn_updateinfo_curr.CHIP_ID==0x54 || hyn_updateinfo_curr.CHIP_ID == 0x64 )
	 {
		gestrue_id = buf[0];
		pointnum = (short)(buf[1]) & 0xff;
		buf[0] = 0xd3;

		if((pointnum * 4 + 8)<255)
		{
			ret = hyn_i2c_Read(this_client, buf, 1, buf, (pointnum * 4 + 8));
		}
		else
		{
			ret = hyn_i2c_Read(this_client, buf, 1, buf, 255);
			ret = hyn_i2c_Read(this_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
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
		 ret = hyn_i2c_Read(this_client, buf, 1, buf, (pointnum * 4 + 8));
	}
	else
	{
		 ret = hyn_i2c_Read(this_client, buf, 1, buf, 255);
		 ret = hyn_i2c_Read(this_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
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
static int cst8xx_update_data(void)
{
	struct cst8xx_ts_data *data = i2c_get_clientdata(this_client);
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
				cst8xx_write_reg(0x8b, 0x1);
			}
		else
		{
			printk("[chenqx]Not charging\n");
			cst8xx_write_reg(0x8b, 0x0);
		}
	}
#endif
/*********************add by chenqx for TP shake when charging***************************/

	ret = cst8xx_i2c_rxdata(buf, 33);

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

		cst8xx_read_reg(FT_REGS_PS_CTL, &FT_REGS_PS_CTL_VALIUE);
		if(FT_REGS_PS_CTL_VALIUE ==0)
		{
		  TPD_PROXIMITY_DEBUG("proximity_flag FT6X36 Read 0XB0 Reg Error,Rewrite it");
		  cst8xx_write_reg(FT_REGS_PS_CTL, 0x1);
		  cst8xx_read_reg(0x01, &proximity_flag);
		}
		TPD_PROXIMITY_DEBUG("proximity_flag FT_REGS_PS_CTL_VALIUE=0x%x\n", FT_REGS_PS_CTL_VALIUE);
	#endif
		//  0x01-->C0 ê????ü  E0 ê???à?
		cst8xx_read_reg(0x01, &proximity_flag);
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
		if((x>720)||(y>1440)){
			continue;
		}
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
#if defined(HYN_GESTRUE)
	u8 state;
#endif
	struct sched_param param = { .sched_priority = 5 };
	sched_setscheduler(current, SCHED_RR, &param);

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, (0 != tpd_flag));
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
#if defined(HYN_GESTRUE)
		i2c_smbus_read_i2c_block_data(this_client, 0xd0, 1, &state);
		TPD_GES_DEBUG("====touch_event_handler== state = %d\n",state);
		if (state == 1)
		{
#if GESTURE_WAKE_LOCK
			wake_lock_timeout(&suspend_gestrue_lock, msecs_to_jiffies(2500));
#endif
            cst8xx_read_Gestruedata();
            continue;
		}
#endif
		cst8xx_update_data();

	} while (!kthread_should_stop());

	return 0;
}
#endif

#if USE_WORK_QUEUE
static void cst8xx_ts_pen_irq_work(struct work_struct *work)
{
	cst8xx_update_data();
	enable_irq(this_client->irq);
}
#endif

static irqreturn_t cst8xx_ts_interrupt(int irq, void *dev_id)
{
#if defined(HYN_GESTRUE)
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
	struct cst8xx_ts_data *cst8xx_ts = (struct cst8xx_ts_data *)dev_id;

	if (!work_pending(&cst8xx_ts->pen_event_work)) {
		queue_work(cst8xx_ts->ts_workqueue, &cst8xx_ts->pen_event_work);
	}
	return IRQ_HANDLED;
#endif

#if USE_THREADED_IRQ
	cst8xx_update_data();
	return IRQ_HANDLED;
#endif

}

 void cst8xx_ts_reset(void)
{
	struct cst8xx_ts_platform_data *pdata = g_cst8xx_ts->platform_data;

	gpio_direction_output(pdata->reset_gpio_number, 1);
	msleep(1);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(10);
}

static void cst8xx_suspend(void)
{
	int ret = -1;
	#if defined(HYN_GESTRUE)
	struct file *fp; 
	#endif
	pr_info("==%s==\n", __FUNCTION__);
	mutex_lock(&enable_irq_mutex_hyn);
#ifdef TP_PROXIMITY_SENSOR
	TPD_PROXIMITY_DEBUG("cst8xx_ts_suspend: tpd_proximity_flag=%d, tpd_halt=%d\n", tpd_proximity_flag, tpd_halt);
	if(tpd_proximity_flag == 1)
	{
		tpd_halt=1;
		mutex_unlock(&enable_irq_mutex_hyn);
		return;
	}

	tpd_halt = 0;
#endif

#if defined(HYN_GESTRUE)
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
		
		mutex_unlock(&enable_irq_mutex_hyn);
		hyn_write_reg(this_client, 0xd0, 0x01);
		pr_info("=====HYN_GESTRUE====>cst8xx_ts_suspend, CHIP_ID = 0x%x. \n", hyn_updateinfo_curr.CHIP_ID);
		if (hyn_updateinfo_curr.CHIP_ID==0x54 || hyn_updateinfo_curr.CHIP_ID == 0x64 )
		{
			hyn_write_reg(this_client, 0xd1, 0xff);
			hyn_write_reg(this_client, 0xd2, 0xff);
			hyn_write_reg(this_client, 0xd5, 0xff);
			hyn_write_reg(this_client, 0xd6, 0xff);
			hyn_write_reg(this_client, 0xd7, 0xff);
			hyn_write_reg(this_client, 0xd8, 0xff);
		}
		return;
	}
#endif
	ret = cst8xx_write_reg(CST8XX_REG_PMODE, PMODE_HIBERNATE);
	if(ret){
		PRINT_ERR("==cst8xx_ts_suspend==  cst8xx_write_reg fail\n");
	}
	disable_irq(this_client->irq);
	
	mutex_unlock(&enable_irq_mutex_hyn);
	//irq_num--;

	//TPD_PROXIMITY_DEBUG("cst8xx_ts_suspend: tpd_proximity_flag=%d,irq_num=%d\n", tpd_proximity_flag, irq_num);
	cst8xx_clear_report_data(g_cst8xx_ts);
	//gpio_set_value(g_cst8xx_ts->platform_data->reset_gpio_number, 0);//for future use
}


static void cst8xx_resume(void)
{
		struct cst8xx_ts_data  *cst8xx_ts = (struct cst8xx_ts_data *)i2c_get_clientdata(this_client);
		pr_info("==%s==\n", __FUNCTION__);
#ifdef TP_PROXIMITY_SENSOR
		TPD_PROXIMITY_DEBUG("cst8xx_ts_resume: tpd_proximity_flag=%d, tpd_halt=%d\n", tpd_proximity_flag, tpd_halt);
		//Alex.shi on android7.0,if HIC_ENABLE_DEFAULT = 1,then :disable_ps->suspend->resume->enable_ps.But sometime suspend->disable_ps->resume->enable_ps,
		//	then we didnpt exec disable_irq in suspend,but exec enable_irq in resume,and after this ,tp can't work
		if (/*(1 == tpd_proximity_flag) &&*/ (1 == tpd_halt))
		{
			return;
		}
#endif
	
		queue_work(cst8xx_ts->ts_resume_workqueue, &cst8xx_ts->resume_work);
}




#ifdef HYNITRON_TP_PROC_SELF_TEST
#define TEST_PASS	                0x0000
#define TEST_BEYOND_MAX_LIMIT		0x0001
#define TEST_BEYOND_MIN_LIMIT		0x0002

static u8  testItem=0;
static int testResult=-1;
static unsigned short m_os_test_buf[50];
static unsigned short g_allch_num = 0;
static unsigned char  g_testdata_buf[100];

int hynitron_chip_self_test_sub(void)
{
	int i,ret;
	int min_raw,max_raw;

	ret = 0;

	if(g_allch_num>44) g_allch_num = 44;
	
	for(i=0;i<5;i++)
	{
		cst8xx_ts_reset();
		
		msleep(100);
		
		ret = cst8xx_write_reg(0x00,0x04);

		msleep(800);

		if(ret<0) continue;

		g_testdata_buf[0]=0;
		ret = cst8xx_i2c_rxdata((char*)g_testdata_buf,g_allch_num*2+1);

		if(ret<0) continue;

		if(g_testdata_buf[0]==0x04)		//read the factory test mode
		{
			break;
		}
		else
		{
			ret = -2;
		}
	}

#if (ENABLE_DEBUG_PRINT==1)	
	printk("hynitron cst8xx test buf(i=%d):\r\n",i);
	for(i=0;i<g_allch_num*2+1;i++)
	{
		printk("%d ",g_testdata_buf[i]);
	}
	printk("hynitron cst8xx test buf end\r\n");
#endif

		
	printk("hynitron cst8xx test data(i=%d):\r\n",i);
	for(i=0;i<g_allch_num;i++)
	{
		m_os_test_buf[i] = (g_testdata_buf[i*2+1]<<8) + (g_testdata_buf[i*2+2]);
		printk("%d ",m_os_test_buf[i]);
	}
	printk("hynitron cst8xx test data end\r\n");


	if(ret>=0)
	{
		ret = 0;
		
		for(i=0;i<g_allch_num;i++)
		{
			min_raw = cst8xx_test_info.rawdata_ref[i] - cst8xx_test_info.diff_allow[i];
			max_raw = cst8xx_test_info.rawdata_ref[i] + cst8xx_test_info.diff_allow[i];

			if(m_os_test_buf[i]<min_raw)
			{
				ret |= 1;
#if (ENABLE_DEBUG_PRINT==1)	
				printk("cst8xx min err:%d,%d,%d\r\n",m_os_test_buf[i],min_raw,i);
#endif
			}
			else if(m_os_test_buf[i]>max_raw)
			{
				ret |= 2;
#if (ENABLE_DEBUG_PRINT==1)	
				printk("cst8xx max err:%d,%d,%d\r\n",m_os_test_buf[i],max_raw,i);
#endif
			}
		}	
	}

	cst8xx_ts_reset();
	msleep(100);
	
	return ret;
}

static ssize_t hynitron_self_test_read(struct file *f, char __user *page,
    size_t size, loff_t *ppos)
{
    int ret = 0;
    u8 out_buf[16];
    u8 len;

    if (*ppos != 0)
    {
        return 0;
    }
   
    switch (testItem)
    {
    case 0:
    	len = sprintf(out_buf, "null\n");
        break;
    case '1':
        len = sprintf(out_buf, "%d\n",testResult);
        break;
    default:
    	len = sprintf(out_buf, "null\n");
        break;
    }

    ret = copy_to_user(page, out_buf, len);
    if (ret < 0 ) {
        return ret;
    }
     *ppos += len;
    return len;
}

static void hynitron_self_test_fun(void)
{
    int ret = 0;

    testResult = 0xAA; // Now is running test
    disable_irq(this_client->irq);
	
    msleep(200);

    ret = hynitron_chip_self_test_sub();
	if (ret != 0) {
		ret = hynitron_chip_self_test_sub();
	}
    if (ret == 0x00) {
        testResult = TEST_PASS;

    } else if (ret >= 0x01 && ret <= 0x03) {
        testResult = TEST_BEYOND_MAX_LIMIT | TEST_BEYOND_MIN_LIMIT;

    } else {
        testResult = -EIO;

    }
    enable_irq(this_client->irq);
}

static ssize_t hynitron_self_test_write(struct file *f, const char __user *buffer,
    size_t size, loff_t *ppos)
{
    int buflen = size;
    int ret = 0;
    u8 *local_buf;
    if (buflen == 0)
    {
        return 0;
    }

    local_buf = kmalloc(size, GFP_KERNEL);
    if (local_buf == NULL)
    {
        return -EFAULT;
    }
    if (copy_from_user(local_buf, buffer, buflen))
    {

        return -EFAULT;
    }

    testItem = local_buf[0];
    switch (testItem)
    {
	    case 0:

	        break;
	    case '1':
	        hynitron_self_test_fun();
	        break;
	    default:
	        break;
    }
    kfree(local_buf);
    if (ret < 0)
    {
        return ret;
    }
    return size;

}

static struct proc_dir_entry *hynitron_self_test_entry = NULL;

static const struct file_operations hynitron_self_test_fops = {
    .read  = hynitron_self_test_read,
    .write = hynitron_self_test_write,
};

static ssize_t show_hynitron_test_result(struct device *dev,
    struct device_attribute *attr,char *buf)
{
    int i;
    char *ptr = buf;

	
    hynitron_self_test_fun(); //

    ptr += sprintf(ptr, "The result is %0d.\n", testResult);
    ptr += sprintf(ptr, "The ch num is %d, the m_os_test_buf = ",g_allch_num);
    for (i = 0; i < g_allch_num; i++)
    {
        ptr += sprintf(ptr, "%d(%d,%d),", m_os_test_buf[i],cst8xx_test_info.rawdata_ref[i]-cst8xx_test_info.diff_allow[i],
			           cst8xx_test_info.rawdata_ref[i]+cst8xx_test_info.diff_allow[i]);
    }
    ptr += sprintf(ptr, "\n");
    ptr += sprintf(ptr, "end.\n");

    return (ptr - buf);
}

static ssize_t store_hynitron_test_result(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static DEVICE_ATTR(hynitron_test, 0664, show_hynitron_test_result, store_hynitron_test_result);

static struct attribute *hynitron_test_attrs[] = {
    &dev_attr_hynitron_test.attr,
    NULL, // Can not delete this line!!! The phone will reset.
};

static struct attribute_group hynitron_test_attr_group = {
    .attrs = hynitron_test_attrs,
};
#endif






#ifdef CONFIG_PM_SLEEP
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
		cst8xx_resume();
		break;
	case DRM_MODE_DPMS_OFF:
		cst8xx_suspend();
		break;
	default:
		//TS_WARN("receive adf event with error data, adf_event_data=%d",adf_event_data);
		break;
	}

	return NOTIFY_OK;
}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void cst8xx_ts_suspend(struct early_suspend *handler)
{
	cst8xx_suspend();
}

static void cst8xx_ts_resume(struct early_suspend *handler)
{
	cst8xx_resume();
}
#endif

static void cst8xx_ts_resume_work(struct work_struct *work)
{
	pr_info("==%s==\n", __FUNCTION__);
	mutex_lock(&enable_irq_mutex_hyn);
#ifdef HYN_GESTRUE
	if (1==s_gesture_switch) 
	{
		disable_irq_wake(this_client->irq);
		irq_set_irq_type(this_client->irq,IRQF_TRIGGER_FALLING);
		hyn_write_reg(this_client,0xd0,0x00);
	}
#endif
	cst8xx_ts_reset();
	//cst8xx_write_reg(FT5X0X_REG_PERIODACTIVE, 7);
#if !defined(TP_PROXIMITY_SENSOR) 
	#if defined(HYN_GESTRUE)
	if (1 != s_gesture_switch)
	#endif
		enable_irq(this_client->irq);
#endif

	msleep(2);
	cst8xx_clear_report_data(g_cst8xx_ts);
#ifdef TP_PROXIMITY_SENSOR
	msleep(100);

	if((tpd_halt ==0)
	#if defined(HYN_GESTRUE)
		&&  (1 != s_gesture_switch)
	#endif
	)
	enable_irq(this_client->irq);
	//irq_num++;

	//TPD_PROXIMITY_DEBUG("cst8xx_ts_resume_work: tpd_proximity_flag=%d,irq_num=%d\n", tpd_proximity_flag, irq_num);
	if(tpd_proximity_flag == 1)
	{
		CST8XX_enable_ps(1);	
	}
#endif
#if defined(HYN_GESTRUE)
	s_gesture_switch = 0;
#endif
	mutex_unlock(&enable_irq_mutex_hyn);
}


static void cst8xx_ts_hw_init(struct cst8xx_ts_data *cst8xx_ts)
{
	struct regulator *reg_vdd;
	struct i2c_client *client = cst8xx_ts->client;
	struct cst8xx_ts_platform_data *pdata = cst8xx_ts->platform_data;
	int ret = 0;

	pr_info("[HYN] %s [irq=%d];[rst=%d]\n",__func__,
		pdata->irq_gpio_number,pdata->reset_gpio_number);
	gpio_request(pdata->irq_gpio_number, "ts_irq_pin");
	gpio_request(pdata->reset_gpio_number, "ts_rst_pin");
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);

	reg_vdd = regulator_get(&client->dev, pdata->vdd_name);
	if (!WARN(IS_ERR(reg_vdd), "[HYN] cst8xx_ts_hw_init regulator: failed to get %s.\n", pdata->vdd_name)) {
		if(!strcmp(pdata->vdd_name,"vdd18"))
			regulator_set_voltage(reg_vdd,1800000,1800000);
		if(!strcmp(pdata->vdd_name,"vdd28"))
			regulator_set_voltage(reg_vdd, 2800000, 2800000);
		ret=regulator_enable(reg_vdd);
			if (ret) {
			printk("cst8xx_ts_hw_init:regulator_enable fail\n");
		}
	}
	msleep(100);
	cst8xx_ts_reset();
}

void hynitron_get_upgrade_array(struct i2c_client *client)
{

	u8 chip_id;
	u32 i;

	i2c_smbus_read_i2c_block_data(client,CST8XX_REG_CHIP_ID,1,&chip_id);

	printk("%s chip_id = 0x%x\n", __func__, chip_id);

	for(i=0;i<sizeof(hyn_updateinfo)/sizeof(struct Upgrade_Info);i++)
	{
		if(chip_id==hyn_updateinfo[i].CHIP_ID)
		{
			memcpy(&hyn_updateinfo_curr, &hyn_updateinfo[i], sizeof(struct Upgrade_Info));
			break;
		}
	}

	if(i >= sizeof(hyn_updateinfo)/sizeof(struct Upgrade_Info))
	{
		memcpy(&hyn_updateinfo_curr, &hyn_updateinfo[0], sizeof(struct Upgrade_Info));
	}
}

static int check_ctp_chip(void)
{
	ctp_lock_mutex();
	tp_device_id(0x836);
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
static struct cst8xx_ts_platform_data *cst8xx_ts_parse_dt(struct device *dev)
{
	struct cst8xx_ts_platform_data *pdata;
	struct device_node *np = dev->of_node;
	int ret;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Could not allocate struct cst8xx_ts_platform_data");
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


static int cst8xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cst8xx_ts_data	*cst8xx_ts;
	struct input_dev		*input_dev;
	struct cst8xx_ts_platform_data	*pdata = client->dev.platform_data;
	int						err = 0;
	unsigned char			uc_reg_value;

#if ANDROID_TOOL_SURPORT
	int ret    = -1;
#endif
	
#ifdef CONFIG_OF
	struct device_node		*np = client->dev.of_node;
#endif

	pr_info("[HYN] %s: probe\n",__func__);

	if(tp_device_id(0)!=0)
	{
		printk("CTP(0x%x)Exist!", tp_device_id(0));
		return -ENODEV;
	}

#ifdef CONFIG_OF
	if (np && !pdata){
		pdata = cst8xx_ts_parse_dt(&client->dev);
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

	cst8xx_ts = kzalloc(sizeof(*cst8xx_ts), GFP_KERNEL);
	if (!cst8xx_ts)	{
		err = -ENOMEM;
		goto exit_check_functionality_failed;
	}

	g_cst8xx_ts = cst8xx_ts;
	cst8xx_ts->platform_data = pdata;
	this_client = client;
	cst8xx_ts->client = client;
	cst8xx_ts_hw_init(cst8xx_ts);
	msleep(200);
	i2c_set_clientdata(client, cst8xx_ts);
	client->irq = gpio_to_irq(pdata->irq_gpio_number);

	#if(defined(CONFIG_I2C_SPRD) || defined(CONFIG_I2C_SPRD_V1))
	//sprd_i2c_ctl_chg_clk(client->adapter->nr, 400000);
	#endif

	err = cst8xx_read_reg(CST8XX_REG_CIPHER, &uc_reg_value);
	if (err < 0)
	{
		pr_err("[HYN] read chip id error %x\n", uc_reg_value);

#if HYN_EN_AUTO_UPDATE
	p_cst836u_upgrade_firmware=(unsigned char *)app_bin;
	apk_upgrade_flag=1;
    ctp_hynitron_update(client);	
	msleep(200);
#endif 
	if(cst8xx_read_reg(CST8XX_REG_CIPHER, &uc_reg_value)<0)
	{
		pr_info("[HYN] ctp_hynitron_update redo update fail. \n");
		err = -ENODEV;
		goto exit_chip_check_failed;
			
	}else{
			pr_info("[HYN] ctp_hynitron_update redo update success. \n");
	}

	}
	
	
	check_ctp_chip();
	/* set report rate, about 70HZ */
	//cst8xx_write_reg(FT5X0X_REG_PERIODACTIVE, 7);
#if USE_WORK_QUEUE
	INIT_WORK(&cst8xx_ts->pen_event_work, cst8xx_ts_pen_irq_work);

	cst8xx_ts->ts_workqueue = create_singlethread_workqueue("focal-work-queue");
	if (!cst8xx_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_check_ctp_chip;
	}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)||defined(CONFIG_PM_SLEEP)
	INIT_WORK(&cst8xx_ts->resume_work, cst8xx_ts_resume_work);
	cst8xx_ts->ts_resume_workqueue = create_singlethread_workqueue("cst8xx_ts_resume_work");
	if (!cst8xx_ts->ts_resume_workqueue) {
		err = -ESRCH;
		goto exit_create_resume_workqueue;
	}
#endif

#ifdef TP_PROXIMITY_SENSOR
	err = misc_register(&CST8XX_proximity_misc);
	if (err < 0)
	{
		pr_err("%s: could not register misc device\n", __func__);
		goto err_mis_reg;
	}
#endif

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "[HYN] failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
#ifdef TOUCH_VIRTUAL_KEYS
	cst8xx_ts_virtual_keys_init();
#endif
	cst8xx_ts->input_dev = input_dev;

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

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
#if defined(HYN_GESTRUE)
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
	wake_lock_init(&suspend_gestrue_lock, WAKE_LOCK_SUSPEND, "suspend_gestrue");
#endif	

#endif

	input_dev->name = HYNITRON_TS_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"[HYN] cst8xx_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#if USE_THREADED_IRQ
	err = request_threaded_irq(client->irq, NULL, cst8xx_ts_interrupt, 
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, cst8xx_ts);
#elif defined(HYN_GESTRUE) || defined(TP_PROXIMITY_SENSOR)
    err = request_irq(client->irq, cst8xx_ts_interrupt,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, cst8xx_ts);
#else
	err = request_irq(client->irq, cst8xx_ts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, cst8xx_ts);
#endif
	if (err < 0) {
		dev_err(&client->dev, "[HYN] ft5x0x_probe: request irq failed %d\n",err);
		goto exit_irq_request_failed;
	}
#ifdef CONFIG_PM_SLEEP
	adf_event_block.notifier_call = ts_adf_event_handler;
	adf_register_client(&adf_event_block);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	cst8xx_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	cst8xx_ts->early_suspend.suspend = cst8xx_ts_suspend;
	cst8xx_ts->early_suspend.resume	= cst8xx_ts_resume;
	register_early_suspend(&cst8xx_ts->early_suspend);
#endif

hynitron_get_upgrade_array(client);

#ifdef SYSFS_DEBUG	
	hyn_create_sysfs(client);
#endif

#ifdef HYN_CTL_IIC	
if (hyn_rw_iic_drv_init(client) < 0)	
{
	dev_err(&client->dev, "%s:[HYN] create fts control iic driver failed\n",	__func__);
		goto exit_iic_drv_init_failed;
}
#endif


#if HYN_EN_AUTO_UPDATE
	p_cst836u_upgrade_firmware=(unsigned char *)app_bin;
	apk_upgrade_flag=0;
    ctp_hynitron_update(client);
	msleep(200);
#endif  


#if ANDROID_TOOL_SURPORT
	ret = cst8xx_proc_fs_init();
	if (ret < 0) {
		printk("[HYN]create cst2xx proc fs failed.\n");
	}
#endif

#ifdef HYNITRON_TP_PROC_SELF_TEST
	g_allch_num = cst8xx_test_info.chan_num;
	hynitron_self_test_entry = proc_create("hynitron_tp_test", 0666, NULL,&hynitron_self_test_fops);
    err=sysfs_create_group(&client->dev.kobj, &hynitron_test_attr_group);
    if (err < 0) {
        printk("Can not create sysfs hynitron_attr_group!\n");
		goto exit_iic_drv_init_failed;
    }

#endif



#if USE_WAIT_QUEUE
	thread = kthread_run(touch_event_handler, 0, "focal-wait-queue");
	if (IS_ERR(thread))
	{
		err = PTR_ERR(thread);
		PRINT_ERR("failed to create kernel thread: %d\n", err);
		goto exit_kthread_run_failed;
	}
#endif
#ifdef HYN_GESTRUE
	pr_info("HYN_GESTRUE init!\n");
	//init_para(720,1280,100,0,0);
#endif
	mutex_init(&enable_irq_mutex_hyn);
	{
		
		
		extern void zyt_info_s2(char* s1,char* s2);
		extern void zyt_info_sx(char* c1,int x);

		hyn_read_reg(client, CST8XX_REG_FW_VER, &tp_fm_ver);
		zyt_info_s2("[TP] : ",hyn_updateinfo_curr.HYN_NAME);
		zyt_info_sx("[CST8XX] : FW Version:",tp_fm_ver);
	}
	pr_info("%s: probe Success!\n", __func__);
	return 0;

exit_kthread_run_failed:

#ifdef HYN_CTL_IIC	
	hyn_rw_iic_drv_exit();
#endif
exit_iic_drv_init_failed:
#ifdef SYSFS_DEBUG
	hyn_release_sysfs(client);
#endif
#ifdef HYNITRON_TP_PROC_SELF_TEST
    if(hynitron_self_test_entry){
		remove_proc_entry("tlsc_test", NULL);
    }
#endif	

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&cst8xx_ts->early_suspend);
#endif
	free_irq(client->irq, cst8xx_ts);
exit_irq_request_failed:
	input_unregister_device(input_dev);
exit_input_register_device_failed:
	cst8xx_ts_virtual_keys_destroy();
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
#ifdef TP_PROXIMITY_SENSOR	
	misc_deregister(&CST8XX_proximity_misc);
	err_mis_reg:
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (cst8xx_ts->ts_resume_workqueue) {
		destroy_workqueue(cst8xx_ts->ts_resume_workqueue);
	}
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) ||defined(CONFIG_PM_SLEEP)
exit_create_resume_workqueue:
#endif
#if USE_WORK_QUEUE
	if (cst8xx_ts->ts_workqueue)
	{
		destroy_workqueue(cst8xx_ts->ts_workqueue);
	}
#endif
#if USE_WORK_QUEUE
exit_check_ctp_chip:
#endif
	remove_ctp_chip();
exit_chip_check_failed:
	gpio_free(pdata->irq_gpio_number);
	gpio_free(pdata->reset_gpio_number);
	kfree(cst8xx_ts);
	cst8xx_ts = NULL;
	i2c_set_clientdata(client, NULL);

exit_check_functionality_failed:
	if (client->dev.platform_data) {
		kfree(client->dev.platform_data);
		client->dev.platform_data = NULL;
	}
exit_alloc_platform_data_failed:
	return err;
}

static int cst8xx_ts_remove(struct i2c_client *client)
{
	struct cst8xx_ts_data *cst8xx_ts = i2c_get_clientdata(client);

	pr_info("==cst8xx_ts_remove=\n");
	
#ifdef SYSFS_DEBUG
	hyn_release_sysfs(client);
#endif
#ifdef HYN_CTL_IIC	
	hyn_rw_iic_drv_exit();
#endif
#ifdef APK_DEBUG
	//cst8xx_release_apk_debug_channel();
#endif
	
#ifdef HYNITRON_TP_PROC_SELF_TEST
    if(hynitron_self_test_entry){
		remove_proc_entry("hynitron_tp_test", NULL);
    }
#endif	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&cst8xx_ts->early_suspend);
#endif
	free_irq(client->irq, cst8xx_ts);
	input_unregister_device(cst8xx_ts->input_dev);
	cst8xx_ts_virtual_keys_destroy();
	input_free_device(cst8xx_ts->input_dev);
#ifdef TP_PROXIMITY_SENSOR	
	misc_deregister(&CST8XX_proximity_misc);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (cst8xx_ts->ts_resume_workqueue) {
		destroy_workqueue(cst8xx_ts->ts_resume_workqueue);
	}
#endif
#if USE_WORK_QUEUE
	if (cst8xx_ts->ts_workqueue)
	{
		destroy_workqueue(cst8xx_ts->ts_workqueue);
	}
#endif
	remove_ctp_chip();
	if (cst8xx_ts->platform_data->reset_gpio_number>0)
		gpio_free(cst8xx_ts->platform_data->reset_gpio_number);
	if (cst8xx_ts->platform_data->irq_gpio_number>0)
		gpio_free(cst8xx_ts->platform_data->irq_gpio_number);
	if (cst8xx_ts) {
		kfree(cst8xx_ts);
		cst8xx_ts = NULL;
	}
	i2c_set_clientdata(client, NULL);
	if (client->dev.platform_data) {
		kfree(client->dev.platform_data);
		client->dev.platform_data = NULL;
	}

	return 0;
}

static const struct i2c_device_id cst8xx_ts_id[] = {
	{ HYNITRON_TS_NAME, 0 },{ }
};


MODULE_DEVICE_TABLE(i2c, cst8xx_ts_id);

static const struct of_device_id hynitron_of_match[] = {
       { .compatible = "hynitron,hynitron_ts", },
       { }
};

MODULE_DEVICE_TABLE(of,hynitron_of_match);
static struct i2c_driver cst8xx_ts_driver = {
	.probe		= cst8xx_ts_probe,
	.remove		= cst8xx_ts_remove,
	.id_table	= cst8xx_ts_id,
	.driver	= {
		.name	= HYNITRON_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = hynitron_of_match,
	},
	
//	.suspend = cst8xx_suspend,
//	.resume = cst8xx_resume,
};

static int __init cst8xx_ts_init(void)
{
	pr_info("==cst8xx_ts_init==\n");

	if(tp_device_id(0)!=0)
	{
		printk("CTP(0x%x)Exist!", tp_device_id(0));
		return -ENODEV;
	}
	return i2c_add_driver(&cst8xx_ts_driver);
}

static void __exit cst8xx_ts_exit(void)
{
	i2c_del_driver(&cst8xx_ts_driver);
}

module_init(cst8xx_ts_init);
module_exit(cst8xx_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
