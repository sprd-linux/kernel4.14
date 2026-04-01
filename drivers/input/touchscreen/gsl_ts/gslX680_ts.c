/* 
 * drivers/input/touchscreen/gslX680.c
 *
 * Sileadinc gslX680 TouchScreen driver. 
 *
 * Copyright (c) 2012  Sileadinc
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
 *   1.0		 2012-04-18		   zxw
 *
 * note: only support mulititouch	Wenfs 2010-10-01
 */

#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/byteorder/generic.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/firmware.h>
#include <linux/proc_fs.h>
//#include <linux/regulator/consumer.h>
//#include <mach/regulator.h>

#if defined (JZHK_CTP_J106_TAIYI_SP111A)
#include "gsl_ts_fw_j106_taiyi_sp111a_fwvga.h"
#elif defined (JZHK_CTP_J106_TAIYI_SP111B_HD720)
#include "gsl_ts_fw_j106_taiyi_sp111b_hd720.h"
#else
#include "gsl_ts_fw.h"
#endif

#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/notifier.h>
#include <video/adf_notifier.h>

#include <linux/wakelock.h>
#include <soc/sprd/board.h>

//#define USE_TP_PSENSOR

#define TS_DEBUG_MSG
#ifdef TS_DEBUG_MSG
#define GSL168X_DBG(format, ...)	printk(KERN_INFO "GSL168X " format "\n", ## __VA_ARGS__)
#else
#define GSL168X_DBG(format, ...)
#endif
#define I2C_BOARD_INFO_METHOD   1

//#define GSL9XX_IO_CTR
#define MAX_FINGERS		10
#define MAX_CONTACTS	10
#define DMA_TRANS_LEN	0x20
#define GSL_PAGE_REG	0xf0

#define GSL_MONITOR
#if defined(ZCFG_MK_TP_GESTURE)
#define GSL_GESTURE
#endif
#define TOUCH_VIRTUAL_KEYS

struct notifier_block adf_event_block_gsl;

char sprd_tp_name[256];
EXPORT_SYMBOL(sprd_tp_name);
#ifdef GSL_GESTURE
static int gesture_flag_lock = 0;
static struct wake_lock tp_wakelock;
//#define REPORT_KEY_VALUE KEY_POWER//KEY_F1      //report key set
#define READ_DATA_LEN	8
//static void gsl_irq_mode_change(struct i2c_client *client,u32 flag);
typedef enum{
	GE_DISABLE = 0,
	GE_ENABLE = 1,
	GE_WAKEUP = 2,
	GE_NOWORK =3,
}GE_T;
static GE_T gsl_gesture_status = GE_DISABLE;
static unsigned int gsl_gesture_flag = 1;
static struct input_dev *gsl_power_idev;
static char gsl_gesture_c = 0;
static int power_key_status = 0;
extern void gsl_GestureExternInt(unsigned int *model,int len);
//extern int irq_set_irq_type(unsigned int irq, unsigned int type);
#endif
static volatile int gsl_halt_flag = 0;
//static struct i2c_client *gsl_client = NULL;
#define PRESS_MAX    		255
#ifdef FILTER_POINT
#define FILTER_MAX	3
#endif

#define TPD_PROC_DEBUG
#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
//static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif

#ifdef GSL_IDENTY_TP
static int gsl_tp_type = 0;
static unsigned int *gsl_config_data_id = gsl_config_data_id_1691;
#endif

#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
//static char dac_counter = 0;
static char b0_counter = 0;
static char bc_counter = 0;
static char i2c_lock_flag = 0;
#endif 



#ifdef USE_TP_PSENSOR
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
//#include "gslX680_psensor.h"

static int tp_ps_opened = 0;
//static atomic_t ps_flag;
static tp_ps_t *tp_ps = 0;
static int ps_en = 0;
//static int psensor_flag = 0;
static int suspend_entry_flag = 0;
//static u8 gsl_psensor_data[4];
#endif

#ifdef GSL_REPORT_POINT_SLOT
    #include <linux/input/mt.h>
#endif


struct sprd_i2c_setup_data {
	unsigned i2c_bus;  //the same number as i2c->adap.nr in adapter probe function
	unsigned short i2c_address;
	int irq;
	char type[I2C_NAME_SIZE];
};

spinlock_t resume_lock;
//static int *flag;

static void gslx680_hw_reset(void);

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;

static struct i2c_client *this_client = NULL;
//static struct sprd_i2c_setup_data gslX680_ts_setup={0, GSLX680_TS_ADDR, 0, GSLX680_TS_NAME};

struct gslX680_ts_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	u8 touch_data[44];	
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
	struct gslX680_ts_platform_data	*platform_data;
};

struct gslX680_ts_data *g_gslx680_ts;

#ifdef HAVE_TOUCH_KEY
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;	
};

#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))
const u16 key_array[]={
KEY_ARRAY
}; 

struct key_data gsl_key_data[MAX_KEY_NUM] = {
#if 1
  //huangxl 2014.03.07
  GSL_KEY_DATA  
#else  
	{KEY_BACK, 260, 280,490, 530},
	{KEY_HOMEPAGE, 120, 160, 490, 530},
	{KEY_APPSELECT, 40, 60, 490, 530},
#endif    
};
#endif

#ifdef TOUCH_VIRTUAL_KEYS

    #if defined (JZHK_CTP_J106_TAIYI_SP111A)
    #define CTP_BUTTON_KEY_Y   1000
    #define MENU_CTP_BUTTON_X  300
    #define HOME_CTP_BUTTON_X  200
    #define BACK_CTP_BUTTON_X  100 	
#elif defined (JZHK_CTP_J106_TAIYI_SP111B_HD720)
    #define CTP_BUTTON_KEY_Y   1800
    #define MENU_CTP_BUTTON_X  450
    #define HOME_CTP_BUTTON_X  250
    #define BACK_CTP_BUTTON_X  100 	
#else
    #define CTP_BUTTON_KEY_Y   1000
    #define MENU_CTP_BUTTON_X  100
    #define HOME_CTP_BUTTON_X  200
    #define BACK_CTP_BUTTON_X  300 
#endif

#define BUTTON_WIDTH 40
#define BUTTON_HEIGHT 40
#define VIRTUAL_VEBRATOR_FUC 	1

static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{

	return sprintf(buf,
		"0x%02x:%d:%d:%d:%d:%d:0x%02x:%d:%d:%d:%d:%d:0x%02x:%d:%d:%d:%d:%d\n",
		EV_KEY, KEY_APPSELECT, MENU_CTP_BUTTON_X, CTP_BUTTON_KEY_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
		EV_KEY, KEY_HOMEPAGE, HOME_CTP_BUTTON_X, CTP_BUTTON_KEY_Y, BUTTON_WIDTH, BUTTON_HEIGHT,
		EV_KEY, KEY_BACK, BACK_CTP_BUTTON_X, CTP_BUTTON_KEY_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.gslx680_ts",
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

static void gslX680_ts_virtual_keys_init(void)
{
    int ret;
    struct kobject *properties_kobj;	
	
    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");    
}

#endif

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

#if 0
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;
	//xfer_msg[0].timing = 400;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;
	//xfer_msg[1].timing = 400;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}
#endif

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;
	//xfer_msg[0].timing = 400;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		GSL168X_DBG("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
	
	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		GSL168X_DBG("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		GSL168X_DBG("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}

#ifdef GSL_GESTURE
static unsigned int gsl_read_oneframe_data(unsigned int *data,
				unsigned int addr,unsigned int len)
{
	//u8 buf[4], read_len;
	u8 buf[4];
	int i = 0;
	GSL168X_DBG("=======%s addr = %x, len = %d\n",__func__, addr, len);

#if 1
			buf[0] = ((addr+i*4)/0x80)&0xff;
			buf[1] = (((addr+i*4)/0x80)>>8)&0xff;
			buf[2] = (((addr+i*4)/0x80)>>16)&0xff;
			buf[3] = (((addr+i*4)/0x80)>>24)&0xff;
			gsl_ts_write(this_client, 0xf0, buf, 4);
			gsl_ts_read(this_client, (((addr+i*4)%0x80+8)&0x5f), (char *)&data[i], 4);
			gsl_ts_read(this_client, (addr+i*4)%0x80, (char *)&data[i], 4);
			
	for(i=0;i<len;i++)
	{
			gsl_ts_read(this_client, (addr+i*4)%0x80, (char *)&data[i], 4);
			//print_info("data[%d] = 0x%08x\n", i, data[i]);
	}
#else

	//int i = 0;
	u8 reg_a[4]={0x0a,0x00,0x00,0x00};
	u8 buf_a5[4]={0x00,0x00,0x00,0xa5};
	u8 buf_zero[4]={0x00,0x00,0x00,0x00};
	u32 buf32;
	struct timeval start;
	struct timeval end;
	
	printk("tp-gsl-gesture %s\n",__func__);
	printk("gsl_read_oneframe_data:::addr=%x,len=%x\n",addr,len);
	
	while(1)
	{		
		gsl_ts_write(this_client,0xf0,&reg_a[0], 4);
		buf_a5[2] = i;
		gsl_ts_write(this_client,0x08,&buf_a5[0], 4);
		do_gettimeofday(&start);
		while(1)
		{
			gsl_ts_read(this_client,0xbc,(u8 *)&buf32,4);
			if((buf32 & 0xffff) == 0xa500+i)
			{
				int k;
				for(k=0;k<15;k++)
				{
					gsl_ts_read(this_client,0x80+k*4,(u8 *)&buf32,4);
					data[i] = buf32;
					GSL168X_DBG("=======%s data %d = %x\n",__func__, i, data[i]);
					if(++i >= len)
					{
						gsl_ts_write(this_client,0xf0,&reg_a[0], 4);
						gsl_ts_write(this_client,0x08,&buf_zero[0], 4);
						return 1;
					}
				}
				break;
			}
			do_gettimeofday(&end);
			if(((end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000) < 100)
				continue;
			gsl_ts_write(this_client,0xf0,&reg_a[0], 4);
			gsl_ts_write(this_client,0x08,&buf_zero[0], 4);
			return 0;			
		}
	}
	return 1;
#endif

	return len;
}
#endif

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

#ifdef GSL_IDENTY_TP
static void gsl_load_fw(struct i2c_client *client,const struct fw_data *GSL_DOWNLOAD_DATA,int data_len)
#else
static void gsl_load_fw(struct i2c_client *client)
#endif
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	struct fw_data *ptr_fw;

	GSL168X_DBG("=============gsl_load_fw start==============\n");

#ifdef GSL_IDENTY_TP
	{
		ptr_fw = GSL_DOWNLOAD_DATA;
		source_len = data_len;
	}
#else
	ptr_fw = GSLX680_FW;
	source_len = ARRAY_SIZE(GSLX680_FW);
#endif
	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
		{
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	GSL168X_DBG("=============gsl_load_fw end==============\n");

}

static int test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret, rc = 1;
	
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
    		rc --;
	else
		GSL168X_DBG("I read reg 0xf0 is %x\n", read_buf);
	
	msleep(2);
	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	if(ret  >=  0 )
		GSL168X_DBG("I write reg 0xf0 0x12\n");
	
	msleep(2);
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if(ret <  0 )
		rc --;
	else
		GSL168X_DBG("I read reg 0xf0 is 0x%x\n", read_buf);

	return rc;
}

static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
	
#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_config_data_id);
#endif
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
}

#ifdef GSL9XX_IO_CTR
static void gsl_io_control(struct i2c_client *client)
{
	u8 buf[4] = {0};
	int i;
	for(i=0;i<5;i++){
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0xfe;
		buf[3] = 0x1;
		i2c_smbus_write_i2c_block_data(client, 0xf0, 4, buf);
		buf[0] = 0x5;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0x80;
		i2c_smbus_write_i2c_block_data(client, 0x78, 4, buf);	
		msleep(5);
	}
	msleep(50);
}
#endif

static void reset_chip(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};
	
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(20);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
	#ifdef GSL9XX_IO_CTR
	gsl_io_control(client);
	#endif
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x03;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
}   
#ifdef GSL_IDENTY_TP
int gsl_identify_tp(struct i2c_client *client);  
#endif	
static void init_chip(struct i2c_client *client)
{
	int rc;
#ifdef GSL_IDENTY_TP
	u32 tmp;
#endif	
	rc = test_i2c(client);
	if(rc < 0)
	{
		GSL168X_DBG("------gslX680 test_i2c error------\n");	
		return;
	}	
	clr_reg(client);
	reset_chip(client);
#ifdef GSL_IDENTY_TP
	if(0==gsl_tp_type)
		gsl_identify_tp(client);
#endif	

#ifdef GSL_IDENTY_TP
	if(1==gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX680_FW_1691);
		gsl_load_fw(client,GSLX680_FW_1691,tmp);
	}
	else if(2==gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX680_FW_960);
		gsl_load_fw(client,GSLX680_FW_960,tmp);
	}
	/*
	else if(3==gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX680_FW_THREE);
		gsl_load_fw(client,GSLX680_FW_THREE,tmp);
	}
	else if(4==gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX680_FW_FOUR);
		gsl_load_fw(client,GSLX680_FW_FOUR,tmp);
	}*/
#else
	gsl_load_fw(client);
#endif		
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);		
}

static void check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};
	
	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
	GSL168X_DBG("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		GSL168X_DBG("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}
}

#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
    if(ch>='0' && ch<='9')
        return (ch-'0');
    else
        return (ch-'a'+10);
}

//static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
static int gsl_config_read_proc(struct seq_file *m,void *v)
{
	char temp_data[5] = {0};
	unsigned int tmp=0;
	
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_NOID_VERSION
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		seq_printf(m,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_NOID_VERSION 
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			seq_printf(m,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<512)
			{
					seq_printf(m,"%d\n",gsl_config_data_id[tmp]); 
			}
#endif
		}
		else 
		{
			gsl_ts_write(this_client,0Xf0,&gsl_data_proc[4],4);
			if(gsl_data_proc[0] < 0x80)
				gsl_ts_read(this_client,gsl_data_proc[0],temp_data,4);
			gsl_ts_read(this_client,gsl_data_proc[0],temp_data,4);

			seq_printf(m,"offset : {0x%02x,0x",gsl_data_proc[0]);
			seq_printf(m,"%02x",temp_data[3]);
			seq_printf(m,"%02x",temp_data[2]);
			seq_printf(m,"%02x",temp_data[1]);
			seq_printf(m,"%02x};\n",temp_data[0]);
		}
	}
//	*eof = 1;
	return (0);
}
//static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
static  ssize_t gsl_config_write_proc(struct file *file, const char __user  *buffer, size_t  count, loff_t *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
	int tmp = 0;
	int tmp1 = 0;
	GSL168X_DBG("[tp-gsl][%s] \n",__func__);
	if(count > 512)
	{
	//	GSL168X_DBG("size not match [%d:%ld]\n", CONFIG_LEN, count);
	//GSL168X_DBG("size not match [%d:%d]\n", CONFIG_LEN, count);
        return -EFAULT;
	}
	path_buf=kzalloc(count,GFP_KERNEL);
	if(!path_buf)
	{
		GSL168X_DBG("alloc path_buf memory error \n");
		return -1;
	}
	if(copy_from_user(path_buf, buffer, count))
	{
		GSL168X_DBG("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	GSL168X_DBG("[tp-gsl][%s][%s]\n",__func__,temp_buf);
	
	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);	
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);
	
	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		GSL168X_DBG("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
		gsl_proc_flag = 1;
		reset_chip(this_client);
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		reset_chip(this_client);
		startup_chip(this_client);
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		gsl_ts_write(this_client,buf[4],buf,4);
	}
#ifdef GSL_NOID_VERSION
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<512)
		{
			gsl_config_data_id[tmp1] = tmp;
		}
	}
#endif
exit_write_proc_out:
	kfree(path_buf);
	return count;
}
static int gsl_server_list_open(struct inode *inode,struct file *file)
{
	return single_open(file,gsl_config_read_proc,NULL);
}
static const struct file_operations gsl_seq_fops = {
	.open = gsl_server_list_open,
	.read = seq_read,
	.release = single_release,
	.write = gsl_config_write_proc,
	.owner = THIS_MODULE,
};
#endif

#ifdef  USE_TP_PSENSOR
/*customer implement: do something like read data from TP IC*/
static int tp_ps_getdata(char *data)
{
	
	unsigned char read_buf[4];
	gsl_ts_read(this_client, 0xac, read_buf, sizeof(read_buf));
	*data = !(read_buf[0]);
	GSL168X_DBG("read_buf[0]=%d\n\n",read_buf[0]);
	
	return 0;
}

static int tp_ps_enable(void)
{	
		u8 buf[4]={0};
		printk("%s\n", __func__);
		
		ps_en = 1;
		buf[3] = 0x00;
		buf[2] = 0x00;
		buf[1] = 0x00;
		buf[0] = 0x4;
		gsl_ts_write(this_client, 0xf0, buf, 4);
		buf[3] = 0x0;
		buf[2] = 0x0;
		buf[1] = 0x0;
		buf[0] = 0x2;  
		gsl_ts_write(this_client, 0, buf, 4);
		ps_en = 1;
		return 0;
}

static int tp_ps_disable(void)
{
	u8 buf[4]={0};
	printk("%s\n", __func__);
	ps_en = 0;
	buf[3] = 0x00;
	buf[2] = 0x00;
	buf[1] = 0x00;
	buf[0] = 0x4;
	gsl_ts_write(this_client, 0xf0, buf, 4);
	buf[3] = 0; 
	buf[2] = 0; 
	buf[1] = 0; 
	buf[0] = 0; 
	gsl_ts_write(this_client, 0, buf, 4);
	
	return 0;
}

static int tp_ps_open(struct inode *inode, struct file *file)
{
	GSL168X_DBG("%s\n", __func__);
	if (tp_ps_opened)
		return -EBUSY;
	tp_ps_opened = 1;
	return 0;
}

static int tp_ps_release(struct inode *inode, struct file *file)
{
	GSL168X_DBG("%s", __func__);
	tp_ps_opened = 0;
	return tp_ps_disable();
}

static long tp_ps_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
		int flag;
		void __user *argp = (void __user *)arg;
		char *strbuf = "LTR558ALS";		
		printk(KERN_ERR "%s: ps io ctrl cmd %d", __func__, _IOC_NR(cmd));
		//ioctl message handle must define by android sensor library (case by case)
		switch(cmd)
		{
			case LTR_IOCTL_GET_PFLAG:
	
				printk(KERN_ERR "%s: LTR_IOCTL_GET_PFLAG\n", __func__);
				flag = (ps_en) ? (1) : (0);
				if (copy_to_user(argp, &flag, sizeof(flag)))
					return -EFAULT;
	
				printk(KERN_ERR "%s: LTR_IOCTL_GET_PFLAG pflag = %d\n", __func__, flag);
				break;
	
			case LTR_IOCTL_SET_PFLAG:
				printk(KERN_ERR "%s: LTR_IOCTL_SET_PFLAG\n", __func__);

				if (copy_from_user(&flag, argp, sizeof(flag)))
					return -EFAULT;
				if (flag < 0 || flag > 1) {
					return -EINVAL;
				}
	
				//em_sensor_restart_work();
	
				if(flag==1){
					tp_ps_enable();	
				}
				else if(flag==0) {
					tp_ps_disable();	
				}
	
				//elan_sensor_restart_work();
	

				printk(KERN_ERR "%s: LTR_IOCTL_SET_PFLAG pflag = %d\n", __func__, flag);				
				break;
	
			case LTR_IOCTL_GET_DATA:
				printk(KERN_ERR "%s: LTR_IOCTL_GET_DATA\n", __func__);
				
				break;
				
			case LTR_IOCTL_SET_LFLAG:
				printk(KERN_ERR "%s: LTR_IOCTL_SET_LFLAG\n", __func__);			
			break;
			
			case LTR_IOCTL_GET_CHIPINFO: {
				if(copy_to_user(argp, strbuf, strlen(strbuf)+1))
					return -EFAULT;
				}

			break;
			
			default:
				printk(KERN_ERR "%s: invalid cmd\n", __func__);				
				return -EINVAL;
		}
		return 0;

}

static struct file_operations tp_ps_fops = {
	.owner			= THIS_MODULE,
	.open			= tp_ps_open,
	.release		= tp_ps_release,
    .unlocked_ioctl = tp_ps_ioctl,
};

static struct miscdevice tp_ps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = TP_PS_DEVICE,
	.fops = &tp_ps_fops,
};

/* call this function in tp work func*/
static int tp_ps_report_dps(u8 touches, unsigned int y)
{
	unsigned char dps_data = 0;
	
	tp_ps_getdata(&dps_data);
	
	GSL168X_DBG("%s: proximity=%d", __func__, dps_data);
	
	input_report_abs(tp_ps->input, ABS_DISTANCE, dps_data);
	input_sync(tp_ps->input);

	if(0 == dps_data)
		return 1;
	else
		return 0;
}

static int tp_ps_init(struct i2c_client *client)
{
	int err = 0;
	struct input_dev *input_dev;
    
	ps_en = 0;
	
	tp_ps = kzalloc(sizeof(tp_ps_t), GFP_KERNEL);
	if (!tp_ps)
	{
		GSL168X_DBG("%s: request memory failed\n", __func__);
		err= -ENOMEM;
		goto exit_mem_fail;
	}
		
	//register device
	err = misc_register(&tp_ps_device);
	if (err) {
		GSL168X_DBG("%s: tp_ps_device register failed\n", __func__);
		goto exit_misc_reg_fail;
	}

	// register input device 
	input_dev = input_allocate_device();
	if (!input_dev) 
	{
		GSL168X_DBG("%s: input allocate device failed\n", __func__);
		err = -ENOMEM;
		goto exit_input_dev_allocate_failed;
	}

	tp_ps->input = input_dev;

	input_dev->name = TP_PS_INPUT_DEV;
	input_dev->phys  = TP_PS_INPUT_DEV;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0010;

	__set_bit(EV_ABS, input_dev->evbit);	
	//for proximity
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	err = input_register_device(input_dev);
	if (err < 0)
	{
	    GSL168X_DBG("%s: input device regist failed\n", __func__);
	    goto exit_input_register_failed;
	}

	GSL168X_DBG("%s: Probe Success!\n",__func__);
	return 0;

exit_input_register_failed:
	input_free_device(input_dev);
exit_input_dev_allocate_failed:
	misc_deregister(&tp_ps_device);
exit_misc_reg_fail:
	kfree(tp_ps);
exit_mem_fail:
	return err;
}

static int tp_ps_uninit(void)
{
	misc_deregister(&tp_ps_device);
	//free input
	input_unregister_device(tp_ps->input);
	input_free_device(tp_ps->input);
	//free alloc
	kfree(tp_ps);
	tp_ps = 0;
return 0;
}

#endif

#ifdef FILTER_POINT
static void filter_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;
	u16 filter_step_x = 0, filter_step_y = 0;
	
	id_sign[id] = id_sign[id] + 1;
	if(id_sign[id] == 1)
	{
		x_old[id] = x;
		y_old[id] = y;
	}
	
	x_err = x > x_old[id] ? (x -x_old[id]) : (x_old[id] - x);
	y_err = y > y_old[id] ? (y -y_old[id]) : (y_old[id] - y);

	if( (x_err > FILTER_MAX && y_err > FILTER_MAX/3) || (x_err > FILTER_MAX/3 && y_err > FILTER_MAX) )
	{
		filter_step_x = x_err;
		filter_step_y = y_err;
	}
	else
	{
		if(x_err > FILTER_MAX)
			filter_step_x = x_err; 
		if(y_err> FILTER_MAX)
			filter_step_y = y_err;
	}

	if(x_err <= 2*FILTER_MAX && y_err <= 2*FILTER_MAX)
	{
		filter_step_x >>= 2; 
	SCREEN_MAX_Y,	filter_step_y >>= 2;
	}
	else if(x_err <= 3*FILTER_MAX && y_err <= 3*FILTER_MAX)
	{
		filter_step_x >>= 1; 
		filter_step_y >>= 1;
	}	
	else if(x_err <= 4*FILTER_MAX && y_err <= 4*FILTER_MAX)
	{
		filter_step_x = filter_step_x*3/4; 
		filter_step_y = filter_step_y*3/4;
	}
	
	x_new = x > x_old[id] ? (x_old[id] + filter_step_x) : (x_old[id] - filter_step_x);
	y_new = y > y_old[id] ? (y_old[id] + filter_step_y) : (y_old[id] - filter_step_y);

	x_old[id] = x_new;
	y_old[id] = y_new;
}
#else

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}
#endif

#ifdef HAVE_TOUCH_KEY

static void report_key(struct gslX680_ts_data *ts, u16 x, u16 y,u8 pressure,u8 id)
{
	u16 i = 0;

	for(i = 0; i < MAX_KEY_NUM; i++) 
	{
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) && (y < gsl_key_data[i].y_max))
		{
			key = gsl_key_data[i].key;	
			input_report_key(ts->input_dev, key, 1);
			input_sync(ts->input_dev); 		
CREEN_MAX_Y,
			break;
		}
	}
}
#endif
#ifdef GSL_IDENTY_TP
#if defined(GSLX680_WPF_D5022)
#define GSL_C		100
#define GSL_CHIP_1	0xff807801 //hsd
#define GSL_CHIP_2	0xff80f801 //mwd
#elif defined(GSLX680_ALOES_A5086C_FWVGA)
#define GSL_C		100
#define GSL_CHIP_1	0xffc1f801//1691  fff07801 
#define GSL_CHIP_2	0xffeaa500 //960
//#define GSL_CHIP_3	0xff407801//THREE 
//#define GSL_CHIP_4	0xff807803  //FOUR
#else
#define GSL_C		100
#define GSL_CHIP_1	0xfff07801 //g3 
#define GSL_CHIP_2	0xffeaa500 //960
//#define GSL_CHIP_3	0xff407801//THREE 
//#define GSL_CHIP_4	0xff807803  //FOUR
#endif
static unsigned int gsl_count_one(unsigned int flag)
{
	unsigned int tmp=0; 
	int i =0;

	for (i=0 ; i<32 ; i++) {
		if (flag & (0x1 << i)) {
			tmp++;
		}
	}
	return tmp;
}

 int gsl_identify_tp(struct i2c_client *client)
{
	u8 buf[4];
	int i,err=1;
	int flag=0;
	unsigned int tmp,tmp0;
	unsigned int tmp1,tmp2,tmp3,tmp4;
	u32 num;

identify_tp_repeat:
	clr_reg(client);
	reset_chip(client);
	num = ARRAY_SIZE(GSL_TP_CHECK_FW);
	gsl_load_fw(client,GSL_TP_CHECK_FW,num);
	startup_chip(client);
	msleep(200);
	i2c_smbus_read_i2c_block_data(client,0xb4,4,buf);
	GSL168X_DBG("the test 0xb4 = {0x%02x%02x%02x%02x}\n",buf[3],buf[2],buf[1],buf[0]);

	if (((buf[3] << 8) | buf[2]) > 1) {
		GSL168X_DBG("[TP-GSL][%s] is start ok\n",__func__);
		msleep(20);
		i2c_smbus_read_i2c_block_data(client,0xb8,4,buf);
		tmp = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		GSL168X_DBG("the test 0xb8 = {0x%02x%02x%02x%02x}\n",buf[3],buf[2],buf[1],buf[0]);

		tmp1 = gsl_count_one(GSL_CHIP_1^tmp);
		tmp0 = gsl_count_one((tmp&GSL_CHIP_1)^GSL_CHIP_1); 
		tmp1 += tmp0*GSL_C;
		GSL168X_DBG("[TP-GSL] tmp1 = %d\n",tmp1);
		
		tmp2 = gsl_count_one(GSL_CHIP_2^tmp); 
		tmp0 = gsl_count_one((tmp&GSL_CHIP_2)^GSL_CHIP_2);
		tmp2 += tmp0*GSL_C;
		GSL168X_DBG("[TP-GSL] tmp2 = %d\n",tmp2);
/*
		tmp3 = gsl_count_one(GSL_CHIP_3^tmp);
		tmp0 = gsl_count_one((tmp&GSL_CHIP_3)^GSL_CHIP_3); 
		tmp3 += tmp0*GSL_C;
		GSL168X_DBG("[TP-GSL] tmp3 = %d\n",tmp3);
		
		tmp4 = gsl_count_one(GSL_CHIP_4^tmp); 
		tmp0 = gsl_count_one((tmp&GSL_CHIP_4)^GSL_CHIP_4);
		tmp4 += tmp0*GSL_C;
		GSL168X_DBG("[TP-GSL] tmp4 = %d\n",tmp4);
*/
		if (0xffffffff == GSL_CHIP_1) {
			tmp1=0xffff;
		}
		if (0xffffffff == GSL_CHIP_2) {
			tmp2=0xffff;
		}
/*
		if (0xffffffff == GSL_CHIP_3) {
			tmp3=0xffff;
		}
		if (0xffffffff == GSL_CHIP_4) {
			tmp4=0xffff;
		}
		*/
		GSL168X_DBG("[TP-GSL] tmp = %d\n",tmp);
		GSL168X_DBG("[TP-GSL] tmp1 = %d\n",tmp1);
		GSL168X_DBG("[TP-GSL] tmp2 = %d\n",tmp2);
		GSL168X_DBG("[TP-GSL] tmp3 = %d\n",tmp3);
		GSL168X_DBG("[TP-GSL] tmp4 = %d\n",tmp4);
		tmp = tmp1;
		if (tmp1 > tmp2) {
			tmp = tmp2; 
		}
	/*
		if(tmp > tmp3){
			tmp = tmp3;
		}
		if(tmp>tmp4){
			tmp = tmp4;
		}
		*/
		if(tmp == tmp1){
			gsl_config_data_id = gsl_config_data_id_1691;
			gsl_tp_type = 1;
		} else if(tmp == tmp2) {
			gsl_config_data_id = gsl_config_data_id_960;
			gsl_tp_type = 2;
		} // else if(tmp == tmp3) {
		//	gsl_config_data_id = gsl_config_data_id_three;
		//	gsl_tp_type = 3;
		//}  else if(tmp == tmp4) {
		//	gsl_config_data_id = gsl_config_data_id_four;
		//	gsl_tp_type = 4;
		//} 
		err = 1;
	} else {
		flag++;
		if(flag < 3) {
			goto identify_tp_repeat;
		}
		err = 0;
	}
	return err; 
}
#endif

static void report_data(struct gslX680_ts_data *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	GSL168X_DBG("report_data: id %d, x %d, y %d \n",id, x, y);

#ifdef VIRTUAL_VEBRATOR_FUC
			// input_mt_slot(ts->input_dev, id);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
			//input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
			input_report_key(ts->input_dev, BTN_TOUCH, 1);	
			input_mt_sync(ts->input_dev);
#else
#ifdef HAVE_TOUCH_KEY
        if(x > SCREEN_MAX_X ||y > SCREEN_MAX_Y)
        {
                report_key(ts,x,y,pressure,id);
                return;
        }
#endif

	input_mt_slot(ts->input_dev, id);
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
#endif		
}

static void gslX680_ts_worker(struct work_struct *work)
{
	int rc;

	u8 id, touches;
	u16 x, y;
	int i = 0;
#ifdef GSL_NOID_VERSION
	struct gsl_touch_info cinfo={{0},{0},{0},0};
	u32 tmp1=0;
	u8 buf[4]={0};
#endif	
	struct gslX680_ts_data *ts = i2c_get_clientdata(this_client);

	GSL168X_DBG("gslX680_ts_pen_irq_work \n");

#ifdef GSL_MONITOR
	if(i2c_lock_flag != 0)
		goto i2c_lock_schedule;
	else
		i2c_lock_flag = 1;
#endif

#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1)
		goto schedule;
#endif

#ifdef GSL_GESTURE
	if(gsl_gesture_flag==1 && GE_NOWORK == gsl_gesture_status){
		goto schedule;
	}
#endif

#ifdef GSL_GESTURE
				if(gsl_gesture_status==GE_ENABLE&&gsl_gesture_flag==1){
					wake_lock_timeout(&tp_wakelock, msecs_to_jiffies(2000));
					//GSL168X_DBG("gsl-wake_lock_timeout\n");
				}
#endif

	rc = gsl_ts_read(this_client, 0x80, ts->touch_data, sizeof(ts->touch_data));
	GSL168X_DBG("--zxw--0x80--ts->touch_data[3]=%x [2]=%x [1]=%x [0]=%x--\n",ts->touch_data[3],ts->touch_data[2],ts->touch_data[1],ts->touch_data[0]);
	if (rc < 0) 
	{
		dev_err(&this_client->dev, "read failed\n");
		goto schedule;
	}

	touches = ts->touch_data[0];
#ifdef GSL_NOID_VERSION
	cinfo.finger_num = touches;
	GSL168X_DBG("tp-gsl  finger_num = %d\n",cinfo.finger_num);
	for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++)
	{
		cinfo.x[i] = join_bytes( ( ts->touch_data[4 * i + 7] & 0xf),ts->touch_data[4 * i + 6]);
		cinfo.y[i] = join_bytes(ts->touch_data[4 * i + 5],ts->touch_data[4 * i +4]);
		cinfo.id[i] = (ts->touch_data[4*i + 7]&0xf0)>>4;
		GSL168X_DBG("tp-gsl  x = %d y = %d \n",cinfo.x[i],cinfo.y[i]);
	}
	cinfo.finger_num=(ts->touch_data[3]<<24)|(ts->touch_data[2]<<16)
		|(ts->touch_data[1]<<8)|(ts->touch_data[0]);
	gsl_alg_id_main(&cinfo);
	tmp1=gsl_mask_tiaoping();
	GSL168X_DBG("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
		gsl_ts_write(this_client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		GSL168X_DBG("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_ts_write(this_client,0x8,buf,4);
	}
	touches = cinfo.finger_num;
#endif


#ifdef USE_TP_PSENSOR
		if(ps_en)
		{
	#ifdef GSL_ALG_ID
			unsigned int y = 0;
			y =  cinfo->y[0];
	#else
			y = join_bytes(buf[5],buf[4]);
	#endif
	
			if(tp_ps_report_dps(touches, y))
			{
				goto schedule;
			}
		}
#endif


#ifdef GSL_GESTURE
	GSL168X_DBG("--zxw--gsl_gesture_status=%d  gsl_gesture_flag=%d--\n",gsl_gesture_status,gsl_gesture_flag);
	if(GE_ENABLE == gsl_gesture_status && gsl_gesture_flag == 1){
	    unsigned int key_data=0;
		int tmp_c = 0;
//		int flag = 0;	
		tmp_c = gsl_obtain_gesture();
		GSL168X_DBG("zxw---gsl_gesture_[tmp_c] = %d, [Char] = %c\n",tmp_c,tmp_c);
		switch(tmp_c){
		//wxt
	    case 'c':
	    case 'C':
            key_data = KEY_C;			
            break;
        case 'e':
        case 'E':
            key_data = KEY_E;			
            break;
        case 'm':
        case 'M':
            key_data = KEY_M;				
            break;
        case 'o':
        case 'O':
            key_data = KEY_O;
            break;
        case 'v':
        case 'V':
            key_data = KEY_V;
            break;
        case 'w':
        case 'W':
            key_data = KEY_W;
            break;
        case 'z':
        case 'Z':
            key_data = KEY_Z;
            break;
        case 0xa1fa:
            key_data = KEY_RIGHT;
            tmp_c = 'R';
            break;
        case 0xa1fb:
            key_data = KEY_LEFT;
            tmp_c = 'L';
            break;
        case 0xa1fc:
            key_data = KEY_UP;
            tmp_c = 'U';
            break;
        case 0xa1fd:
            key_data = KEY_DOWN;
            tmp_c = 'D';
            break;
        case '*':
            key_data = KEY_U;
            break;
		default:
			GSL168X_DBG("zxw---can't reconition gsl_gesture_[key_data] = %d, [Char] = %c\n",key_data,key_data);
			break;
		}

		GSL168X_DBG("--zxw---tmp_c=%d-key_data=%d--\n",tmp_c,key_data);
		//if((key_data != 0)&& (power_key_status == 0)){	
		if(key_data != 0 && gesture_flag_lock == 0){	
			msleep(10);
		//	gsl_irq_mode_change(this_client,0);
			GSL168X_DBG("tp-gsl gesture %c;\n",(char)(tmp_c & 0xff));
			gsl_gesture_c = (char)(tmp_c & 0xff);
			input_report_key(ts->input_dev,key_data,1); //KEY_POWER
			//input_report_key(gsl_power_idev,KEY_POWER,1); //KEY_POWER
			input_sync(ts->input_dev);
			input_report_key(ts->input_dev,key_data,0);  //KEY_POWER
			//input_report_key(gsl_power_idev,KEY_POWER,0);  //KEY_POWER
			input_sync(ts->input_dev);
			mdelay(200);
			
			wake_lock_timeout(&tp_wakelock, msecs_to_jiffies(200)); 
			
			power_key_status = 1;
			gesture_flag_lock = 1;
			
		}
		goto schedule;
	}
#endif




	for(i = 1; i <= MAX_CONTACTS; i ++)
	{
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i = 0; i < (touches > MAX_FINGERS ? MAX_FINGERS : touches); i ++)
	{
	#ifdef GSL_NOID_VERSION
		id = cinfo.id[i];
		x =  cinfo.x[i];
		y =  cinfo.y[i];	
	#else	
		id = ts->touch_data[4 * i + 7] >> 4;
		x = join_bytes( ( ts->touch_data[4 * i + 7] & 0xf),ts->touch_data[4 * i + 6]);
		y = join_bytes(ts->touch_data[4 * i + 5],ts->touch_data[4 * i +4]);

	#endif

        #if 0
        //huangxl 2014.03.07
        GSL168X_DBG("=====get x=%d 1st====\r\n");
        x = SCREEN_MAX_X>=x?SCREEN_MAX_X-x:0;
        GSL168X_DBG("=====get x=%d 2nd====\r\n");
        #endif
        
		if(1 <= id && id <= MAX_CONTACTS)
		{
		#ifdef FILTER_POINT
			filter_point(x, y ,id);
		#else
			record_point(x, y , id);
		#endif
			report_data(ts, x, y, 10, id);		
			id_state_flag[id] = 1;
		}

	}
	for(i = 1; i <= MAX_CONTACTS; i ++)
	{	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) )
		{
			//if(key_state_flag==0){
#ifndef VIRTUAL_VEBRATOR_FUC			
				input_mt_slot(ts->input_dev, i);
                        	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
                        	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
#endif			//}
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifdef VIRTUAL_VEBRATOR_FUC	
	if(0 == touches)
	{
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_mt_sync(ts->input_dev);
	}
#else	
	if(0 == touches)
	{

	#ifdef HAVE_TOUCH_KEY
		if(key_state_flag)
		{
        		input_report_key(ts->input_dev, key, 0);
			input_sync(ts->input_dev);
			key_state_flag = 0;
		}
		else
	#endif
		{
			input_mt_slot(ts->input_dev, i);
                        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
                        input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
	}	
#endif	
	input_sync(ts->input_dev);

schedule:
#ifdef GSL_MONITOR
	i2c_lock_flag = 0;
i2c_lock_schedule:
#endif
	enable_irq(this_client->irq);
}

#ifdef GSL_MONITOR
//static void gsl_monitor_worker(void)
static void gsl_monitor_worker(struct work_struct *work)
{
	//char write_buf[4] = {0};
	char read_buf[4]  = {0};
	char init_chip_flag = 0;
	
	GSL168X_DBG("----------------gsl_monitor_worker-----------------\n");	

	#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1)
		return;
     #endif
	 
	if(i2c_lock_flag != 0)
			goto queue_monitor_work;
		else
			i2c_lock_flag = 1;
		
	gsl_ts_read(this_client, 0xb0, read_buf, 4);
	if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		b0_counter ++;
	else
		b0_counter = 0;

	if(b0_counter > 1)
	{
		GSL168X_DBG("======read 0xb0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		b0_counter = 0;
		goto queue_monitor_init_chip;
	}
	
	gsl_ts_read(this_client, 0xb4, read_buf, 4);	
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if(int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) 
	{
		GSL168X_DBG("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",int_1st[3], int_1st[2], int_1st[1], int_1st[0], int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		init_chip_flag = 1;
		goto queue_monitor_init_chip;
	}
#if 1 //version 1.4.0 or later than 1.4.0 read 0xbc for esd checking
		gsl_ts_read(this_client, 0xbc, read_buf, 4);	
		if(read_buf[3] != 0 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
			bc_counter++;
		else
			bc_counter = 0;
		if(bc_counter > 1)
		{
			printk("<<<< wanghe ======read 0xbc: %x %x %x %x======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
			init_chip_flag = 1;    // wanghe 2014-03-18 entry here to reset.. mtk_gsl1688_huling2_12_.h 0x74 & 0x7c  !!!!
			bc_counter = 0;
		}
#else

	write_buf[3] = 0x01;
	write_buf[2] = 0xfe;
	write_buf[1] = 0x10;
	write_buf[0] = 0x00;
	gsl_ts_write(this_client, 0xf0, write_buf, 4);
	gsl_ts_read(this_client, 0x10, read_buf, 4);
	gsl_ts_read(this_client, 0x10, read_buf, 4);
	if(read_buf[3] < 10 && read_buf[2] < 10 && read_buf[1] < 10 && read_buf[0] < 10)
		dac_counter ++;
	else
		dac_counter = 0;

	if(dac_counter > 1) 
	{
		GSL168X_DBG("======read DAC1_0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		dac_counter = 0;
	}
#endif
queue_monitor_init_chip:

	

	if(init_chip_flag)
		init_chip(this_client);
	
	i2c_lock_flag = 0;
	
queue_monitor_work:	
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 200);//modify by 50
	
}
#endif

static irqreturn_t gslX680_ts_interrupt(int irq, void *dev_id)
{

	struct gslX680_ts_data *gslX680_ts = (struct gslX680_ts_data *)dev_id;

//	GSL168X_DBG("---zxw-->gslX680_ts_interrupt");

    	disable_irq_nosync(this_client->irq);
	if (!work_pending(&gslX680_ts->pen_event_work)) {
		queue_work(gslX680_ts->ts_workqueue, &gslX680_ts->pen_event_work);
	}

	return IRQ_HANDLED;
}

#if 0
static int mms_setup_power(bool onoff)
{

  int ret=0;
	struct regulator *touch_regulator_3v3 =  NULL;

GSL168X_DBG("[TSP] %s \n", __func__);

	touch_regulator_3v3 = regulator_get(NULL,"vddsim2");

	if (IS_ERR(touch_regulator_3v3)) {
		touch_regulator_3v3 = NULL;
		GSL168X_DBG("get touch_regulator_3v3 regulator error\n");
		return;
	}

	if(onoff) {
		regulator_set_voltage(touch_regulator_3v3, 3000000, 3000000);
		ret = regulator_enable(touch_regulator_3v3);
		if (ret) {
			GSL168X_DBG(KERN_ERR "%s: touch_regulator_3v3 enable failed (%d)\n",__func__, ret);
			}
	}
	else
	{
		ret = regulator_disable(touch_regulator_3v3);
		if (ret) {
		GSL168X_DBG(KERN_ERR "%s: touch_regulator_3v0 disable failed (%d)\n",__func__, ret);
		}
																	     
	}
	return ret;
}
#endif

#ifdef GSL_GESTURE
static void gsl_enter_doze(struct i2c_client *client)
{
	u8 buf[4] = {0};

	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	gsl_ts_write(client,0xf0,buf,4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0x1;
	buf[3] = 0x5a;
	gsl_ts_write(client,0x8,buf,4);
	//gsl_gesture_status = GE_NOWORK;
	msleep(5);
	gsl_gesture_status = GE_ENABLE;

}
static void gsl_quit_doze(struct i2c_client *client)
{
	u8 buf[4] = {0};
//	u32 tmp;

	gsl_gesture_status = GE_DISABLE;

	gslx680_hw_reset();
			
	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	gsl_ts_write(this_client,0xf0,buf,4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0x5a;
	gsl_ts_write(this_client,0x8,buf,4);
	msleep(10);

}


static void gsl_irq_mode_change(struct i2c_client *client,u32 flag)
{
	u8 buf[4]={0};
	buf[0] = 0x6;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	gsl_ts_write(client,0xf0,buf,4);
	if(flag == 1){
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
	}else if(flag == 0){
		buf[0] = 1;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
	}else{
		return;
	}
	gsl_ts_write(client,0x1c,buf,4);
}

static ssize_t gsl_sysfs_tpgesture_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t len=0;
#if 0
	sprintf(&buf[len],"%s\n","tp gesture is on/off:");
	len += (strlen("tp gesture is on/off:")+1);
	if(gsl_gesture_flag == 1){
		sprintf(&buf[len],"%s\n","  on  ");
		len += (strlen("  on  ")+1);
	}else if(gsl_gesture_flag == 0){
		sprintf(&buf[len],"%s\n","  off  ");
		len += (strlen("  off  ")+1);
	}

	sprintf(&buf[len],"%s\n","tp gesture:");
	len += (strlen("tp gesture:")+1);
#endif
	sprintf(&buf[len],"%c\n",gsl_gesture_c);
	len += 2;	
    return len;
}
//wuhao start
static ssize_t gsl_sysfs_tpgesture_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
//	char tmp_buf[16];
	
#if 0
	if(buf[0] == '0'){
		gsl_gesture_flag = 0;  
	}else if(buf[0] == '1'){
		gsl_gesture_flag = 1;
	}
#endif
    return count;
}

#if 1
static void gsl_request_power_idev(void)
{
	int rc;
	struct input_dev *idev;
	idev = input_allocate_device();
	if(!idev){
		return;
	}
	gsl_power_idev = idev;
	idev->name = "gsl_gesture";
	idev->id.bustype = BUS_I2C;
	input_set_capability(idev,EV_KEY,KEY_POWER);
	input_set_capability(idev,EV_KEY,KEY_END);

	rc = input_register_device(idev);
	if(rc){
		input_free_device(idev);
		gsl_power_idev = NULL;
	}
}
#endif

static DEVICE_ATTR(tpgesture, S_IRUGO | S_IWUSR, gsl_sysfs_tpgesture_show, gsl_sysfs_tpgesture_store);

//add by lijin 2014.9.1
static ssize_t gsl_sysfs_tpgesture_func_show(struct device *dev,struct device_attribute *attr, char *buf)
{
#if 1 
	ssize_t len=0;
#if 1
	sprintf(&buf[len],"%s\n","tp gesture is on/off:");
	len += (strlen("tp gesture is on/off:")+1);
	if(gsl_gesture_flag == 1){
		sprintf(&buf[len],"%s\n","  on  ");
		len += (strlen("  on  ")+1);
	}else if(gsl_gesture_flag == 0){
		sprintf(&buf[len],"%s\n","  off  ");
		len += (strlen("  off  ")+1);
	}

	//sprintf(&buf[len],"%s\n","tp gesture:");
	//len += (strlen("tp gesture:")+1);
#endif
	//sprintf(&buf[len],"%c\n",gsl_gesture_c);
	//len += 2;	
    return len;


#else
	//ssize_t len=0;
	//return sprintf(&buf[len],"%c\n",gsl_gesture_flag);
	return sprintf(buf,"%c\n",gsl_gesture_flag);
	//len += 2;	
    //return len;
#endif	
	
}
//wuhao start
static ssize_t gsl_sysfs_tpgesture_func_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
//	char tmp_buf[16];
	
#if 1
	if(buf[0] == '0'){
		//gsl_gesture_flag = 0;  
	}else if(buf[0] == '1'){
		gsl_gesture_flag = 1;
	}
#endif
    return count;
}

static DEVICE_ATTR(tpgesture_func, S_IRUGO | S_IWUSR, gsl_sysfs_tpgesture_func_show, gsl_sysfs_tpgesture_func_store);

//end
#if 0
static struct attribute *gslX680_attributes[] = {
	&dev_attr_tpgesture.attr,
	&dev_attr_tpgesture_func.attr,
	NULL
};

static struct attribute_group gslX680_attribute_group = {
	.attrs = gslX680_attributes
};
#endif

static unsigned int gsl_gesture_init(void)
{
	int ret;
	struct kobject *gsl_debug_kobj;
	gsl_debug_kobj = kobject_create_and_add("sileadinc", NULL) ;
	if (gsl_debug_kobj == NULL)
	{
		GSL168X_DBG("%s: subsystem_register failed\n", __func__);
		return -ENOMEM;
	}
#if 1
	ret = sysfs_create_file(gsl_debug_kobj, &dev_attr_tpgesture.attr);
	//ret = device_create_file(gsl_debug_kobj, &dev_attr_tpgesture);
    if (ret)
    {
        printk("%s: sysfs_create_version_file failed\n", __func__);
        return ret;
    }

	ret = sysfs_create_file(gsl_debug_kobj, &dev_attr_tpgesture_func.attr);
        //ret = device_create_file(gsl_debug_kobj, &dev_attr_tpgesture_fun);
    if (ret)
    {
        GSL168X_DBG("%s: sysfs_create_version_file failed\n", __func__);
        return ret;
    }
#else
//add by lijin 2014.9.1
	ret = sysfs_create_group(gsl_debug_kobj, &gslX680_attribute_group);
	if (ret < 0) 
	{
		GSL168X_DBG("%s: sysfs_create_version_file failed\n", __func__);
		return ret;
	}

//end
#endif
    gsl_request_power_idev();
	return 1;
}

#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gslX680_ts_suspend(struct early_suspend *handler)
#endif
static void gslX680_ts_suspend(void)
{
	struct gslX680_ts_platform_data *pdata = g_gslx680_ts->platform_data;
#if defined(ZCFG_MK_TP_GESTURE)	
	int ret;
#endif
	GSL168X_DBG("==gslX680_ts_suspend----start=\n");
	//tp_ps_enable();
#ifdef USE_TP_PSENSOR
		if (ps_en == 1)
		{
			printk("==gslX680_ts_suspend=USE_TP_PSENSOR,  do nothing.\n");
			return;
		}
#endif
#ifdef GSL_MONITOR
	GSL168X_DBG( "gsl_ts_suspend () : cancel gsl_monitor_work\n");
	cancel_delayed_work_sync(&gsl_monitor_work);
	i2c_lock_flag = 0;
#endif

	gsl_halt_flag=1;
	
#ifdef USE_TP_PSENSOR	
	suspend_entry_flag = 1;
#endif

#ifdef GSL_GESTURE
GSL168X_DBG("gslX680_ts_suspend gsl_gesture_flag = %d,,,gsl_gesture_status = %d\n",gsl_gesture_flag,gsl_gesture_status);
    gesture_flag_lock = 0;
	if(power_key_status == 0){
		gsl_gesture_c = '*';
	}
	power_key_status = 0;
	if(gsl_gesture_flag == 1){
		ret = enable_irq_wake(this_client->irq);
		GSL168X_DBG("set_irq_wake(1) result = %d\n",ret);
		//gsl_gesture_flag = 0;
		gsl_irq_mode_change(this_client,1);
		irq_set_irq_type(this_client->irq, IRQF_TRIGGER_HIGH|IRQF_NO_SUSPEND | IRQF_ONESHOT); //IRQ_TYPE_EDGE_BOTH IRQ_TYPE_LEVEL_LOW |IRQF_ONESHOT|IRQF_NO_SUSPEND
		gsl_enter_doze(this_client);
		return;
	}
#endif
    disable_irq_nosync(this_client->irq);
	gpio_set_value(pdata->reset_gpio_number, 0);
//	gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);

GSL168X_DBG("==gslX680_ts_suspend----end=\n");
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void gslX680_ts_resume(struct early_suspend *handler)
#endif
static void gslX680_ts_resume(void)
{
	struct gslX680_ts_platform_data *pdata = g_gslx680_ts->platform_data;	
#if defined(ZCFG_MK_TP_GESTURE)
	int ret = 0;
#endif
	gsl_halt_flag=0;
	GSL168X_DBG("--zxw--==gslX680_ts_resume start=\n");

#ifdef USE_TP_PSENSOR
	GSL168X_DBG("--zxw-111111111--%s---ps_en=%d  suspend_entry_flag=%d----\n",__func__,ps_en,suspend_entry_flag);
    if (ps_en == 1)
    {
    	GSL168X_DBG("==gslX680_ts_resume=USE_TP_PSENSOR,  do nothing.\n");
		if(suspend_entry_flag == 0)
			    return;
    }
    else{
		suspend_entry_flag = 0;
	}
	GSL168X_DBG("--zxw-2222222222222222--%s---ps_en=%d  suspend_entry_flag=%d----\n",__func__,ps_en,suspend_entry_flag);
#endif

#ifdef GSL_GESTURE
       gesture_flag_lock = 0;
spin_lock(&resume_lock); // add 20141111
	if(gsl_gesture_flag == 1){
		ret =disable_irq_wake(this_client->irq);
		GSL168X_DBG("set_irq_wake(1) result = %d\n",ret);
		irq_set_irq_type(this_client->irq,IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND | IRQF_ONESHOT);
		gsl_quit_doze(this_client);
		gsl_irq_mode_change(this_client,0);
	//gsl_gesture_flag = 0;	
	}
	msleep(2);
	power_key_status = 0;
spin_unlock(&resume_lock);// add 20141111
#endif
	//gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
	gpio_set_value(pdata->reset_gpio_number, 1);

	msleep(20);
	reset_chip(this_client);
	startup_chip(this_client);		
#ifdef USE_TP_PSENSOR
	if(ps_en != 1 )
		{
			check_mem_data(this_client);
		}
	else
	{
			tp_ps_enable();
		}
#else
	check_mem_data(this_client);
	
#endif
	
#ifdef GSL_MONITOR
	GSL168X_DBG( "gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 300);
	i2c_lock_flag = 0;
#endif	

#ifdef GSL_GESTURE
	return;
#else	
	enable_irq(this_client->irq);	
#endif
	GSL168X_DBG("--zxw--==gslX680_ts_resume end=\n");
}

static void gslx680_hw_reset(void)
{
	struct gslX680_ts_platform_data *pdata = g_gslx680_ts->platform_data;

	gpio_direction_output(pdata->reset_gpio_number, 1);
	msleep(1);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(200);
}

static void gslx680_ts_hw_init(struct gslX680_ts_data *gslX680_ts)
{
	//struct regulator *reg_vdd;
	//struct i2c_client *client = gslX680_ts->client;
	struct gslX680_ts_platform_data *pdata = gslX680_ts->platform_data;

	GSL168X_DBG(KERN_INFO "%s [irq=%d];[rst=%d]\n",__func__,pdata->irq_gpio_number,pdata->reset_gpio_number);
	gpio_request(pdata->irq_gpio_number, "ts_irq_pin");
	gpio_request(pdata->reset_gpio_number, "ts_rst_pin");
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);
	//vdd power on
#if 0
#if defined(CONFIG_ARCH_SC8825) || defined (CONFIG_ARCH_SC7710)
	reg_vdd = regulator_get(&client->dev, pdata->vdd_name);
#else
	reg_vdd = regulator_get(&client->dev, REGU_NAME_TP);
#endif
	regulator_set_voltage(reg_vdd, 2800000, 2800000);
	regulator_enable(reg_vdd);
#endif
	msleep(100);
	//reset
	gslx680_hw_reset();
}

#ifdef CONFIG_OF
static struct gslX680_ts_platform_data *gslX680_ts_parse_dt(struct device *dev)
{
	struct gslX680_ts_platform_data *pdata;
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

	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
#endif

#if 0
static int check_ctp_chip(void)
{
	cdc_tp_lock_mutex();
	cdc_tp_device_id(0x1680);
	cdc_tp_unlock_mutex();
	return 0;
}

static int remove_ctp_chip(void)
{
	cdc_tp_lock_mutex();
	cdc_tp_device_id(0xFFFF);
	cdc_tp_unlock_mutex();
	return 0;
}
#endif

//add by glu
static int ts_adf_event_handler(struct notifier_block *nb, unsigned long action, void *data)
{

	struct adf_notifier_event *event = data;
	int adf_event_data;

	if (action != ADF_EVENT_BLANK)
		return NOTIFY_DONE;

	adf_event_data = *(int *)event->data;
	printk("receive adf event with adf_event_data=%d", adf_event_data);

	switch (adf_event_data) {
	case DRM_MODE_DPMS_ON:
		gslX680_ts_resume();     //替换为你现在的resume
		break;
	case DRM_MODE_DPMS_OFF:
		gslX680_ts_suspend();	////替换为你现在的suspend
		break;
	default:
		printk("receive adf event with error data, adf_event_data=%d",
			adf_event_data);
		break;
	}

	return NOTIFY_OK;
}
//add by glu end
static int gslX680_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	
	struct gslX680_ts_data *gslX680_ts;
	struct input_dev *input_dev;
	struct gslX680_ts_platform_data *pdata = client->dev.platform_data;
	int err = 0;
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *np = client->dev.of_node;
#endif

	u8 read_buf[4]  = {0};	
	//u16 i = 0;
	printk("---zxw-->%s\n",__func__);
//	if(cdc_tp_device_id(0)!=0)
//	{
//		printk("CTP(0x%x)Exist!", cdc_tp_device_id(0));
//		return -ENODEV;
//	}

#ifdef CONFIG_OF

	if (np && !pdata){
		pdata = gslX680_ts_parse_dt(&client->dev);
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
		goto exit_alloc_platform_data_failed;
	}

	printk("--gslX680---==kzalloc=ok\n");
	gslX680_ts = kzalloc(sizeof(*gslX680_ts), GFP_KERNEL);
	if (!gslX680_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	err = test_i2c(client);
	if(err < 0)
	{
		printk("------gslX680 test_i2c error !!!------\n");	
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}	
//	check_ctp_chip();



#ifdef TOUCH_VIRTUAL_KEYS
	gslX680_ts_virtual_keys_init();
#endif
	g_gslx680_ts = gslX680_ts;
	

	
	gslX680_ts->platform_data = pdata;
	gslx680_ts_hw_init(gslX680_ts);
	i2c_set_clientdata(client, gslX680_ts);
	client->irq = gpio_to_irq(pdata->irq_gpio_number);
	
	this_client = client;

	printk("I2C addr=%x\n", client->addr);

	INIT_WORK(&gslX680_ts->pen_event_work, gslX680_ts_worker);
	printk("gslX680 INIT_WORK OK\n");
	gslX680_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!gslX680_ts->ts_workqueue) {
		printk("gslX680 ts_workqueue NULL\n");
		err = -ESRCH;
		goto exit_create_singlethread;
	}
	
#ifdef USE_TP_PSENSOR
	tp_ps_init(client);
#endif
	
	GSL168X_DBG("%s: ==request_irq=\n",__func__);
	GSL168X_DBG("%s IRQ number is %d", client->name, client->irq);
	err = request_irq(client->irq, gslX680_ts_interrupt, IRQF_TRIGGER_RISING|IRQF_ONESHOT|IRQF_NO_SUSPEND, client->name, gslX680_ts);
	//err = request_irq(client->irq, gslX680_ts_interrupt, IRQF_TRIGGER_RISING|IRQF_NO_SUSPEND, client->name, gslX680_ts);
	if (err < 0) {
		dev_err(&client->dev, "gslX680_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(client->irq);

	GSL168X_DBG("==input_allocate_device=\n");
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	gslX680_ts->input_dev = input_dev;

	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
#ifdef VIRTUAL_VEBRATOR_FUC	
	__set_bit(KEY_APPSELECT,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOMEPAGE,  input_dev->keybit);
#endif
#ifdef GSL_GESTURE
	//__set_bit(KEY_POWER,  input_dev->keybit);
	__set_bit(KEY_LEFT,  input_dev->keybit);
	__set_bit(KEY_RIGHT,  input_dev->keybit);
	__set_bit(KEY_UP,  input_dev->keybit);
	__set_bit(KEY_DOWN,  input_dev->keybit);
	__set_bit(KEY_U,  input_dev->keybit);
	__set_bit(KEY_O,  input_dev->keybit);
	__set_bit(KEY_W,  input_dev->keybit);
	__set_bit(KEY_M,  input_dev->keybit);
	__set_bit(KEY_E,  input_dev->keybit);
	__set_bit(KEY_C,  input_dev->keybit);
	__set_bit(KEY_S,  input_dev->keybit);
	__set_bit(KEY_V,  input_dev->keybit);
	__set_bit(KEY_Z,  input_dev->keybit);
	__set_bit(KEY_A,  input_dev->keybit);
#endif
	input_set_abs_params(input_dev,
				ABS_MT_TRACKING_ID, 0, 255, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);	
#ifndef VIRTUAL_VEBRATOR_FUC	

	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	input_mt_init_slots(input_dev, 11,0);

#ifdef HAVE_TOUCH_KEY
    for(i = 0; i < MAX_KEY_NUM; i++)
    {
        input_set_capability(input_dev, EV_KEY, key_array[i]);
//	set_bit(key_array[i], input_dev->keybit);
    }
#endif
#endif
#ifdef GSL_GESTURE
	//input_set_capability(input_dev, EV_KEY, KEY_POWER);
//	input_set_capability(input_dev, EV_KEY, REPORT_KEY_VALUE);	
	//wxt
    input_set_capability(input_dev, EV_KEY, KEY_C);
    input_set_capability(input_dev, EV_KEY, KEY_E);
    input_set_capability(input_dev, EV_KEY, KEY_M);
    input_set_capability(input_dev, EV_KEY, KEY_O);
    input_set_capability(input_dev, EV_KEY, KEY_V);
    input_set_capability(input_dev, EV_KEY, KEY_W);
    input_set_capability(input_dev, EV_KEY, KEY_Z);
    input_set_capability(input_dev, EV_KEY, KEY_RIGHT);
    input_set_capability(input_dev, EV_KEY, KEY_LEFT);
    input_set_capability(input_dev, EV_KEY, KEY_UP);
    input_set_capability(input_dev, EV_KEY, KEY_DOWN);
    input_set_capability(input_dev, EV_KEY, KEY_U);
#endif
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_dev->name = GSLX680_TS_NAME;		//dev_name(&client->dev)
	strcpy(sprd_tp_name,GSLX680_TS_NAME); //zxw add
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"gslX680_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef GSL_GESTURE
	gsl_FunIICRead(gsl_read_oneframe_data);
//	gsl_GestureExternInt(gsl_model_extern,sizeof(gsl_model_extern)/sizeof(unsigned int)/18);

#endif
	init_chip(this_client);
	check_mem_data(this_client);
	
//add by glu
adf_event_block_gsl.notifier_call = ts_adf_event_handler;
/* make sure we're the first to suspend/resume */
adf_event_block_gsl.priority = 1000;
ret = adf_register_client(&adf_event_block_gsl);
if (ret < 0)
	printk("register adf notifier fail, cannot sleep when screen off");
else
	printk("register adf notifier succeed");	
//add end by glu
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	GSL168X_DBG("==register_early_suspend =");  
	//gslX680_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	gslX680_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 1;
	gslX680_ts->early_suspend.suspend = gslX680_ts_suspend;
	gslX680_ts->early_suspend.resume	= gslX680_ts_resume;
	register_early_suspend(&gslX680_ts->early_suspend);
#endif

   	enable_irq(client->irq);


#ifdef TPD_PROC_DEBUG
	proc_create(GSL_CONFIG_PROC_FILE,0666,NULL,&gsl_seq_fops);
	gsl_proc_flag = 0;
    
#endif

#ifdef GSL_MONITOR
	GSL168X_DBG( "gsl_ts_probe () : queue gsl_monitor_workqueue\n");

	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 1500);  //1000
#endif
#ifdef GSL_GESTURE
	#if 1
	gsl_gesture_init();
	#else
	ret= sysfs_create_group(&client->dev.kobj, &gslX680_attribute_group);
	if (ret < 0) {		
		printk("--zxw--sysfs_create_group fail--\n");
		return -ENOMEM;													
	}
	#endif
	wake_lock_init(&tp_wakelock,WAKE_LOCK_SUSPEND, "tp_input_wakelock");//add by xieyuehong 20181115
#endif


	spin_lock_init(&resume_lock);  // add 20141030

		msleep(30);	
if(gsl_ts_read(client, 0xbc, read_buf, sizeof(read_buf)) <0 ){
	printk("%s,zxw i2c  err ::%d ----------------------------->\r\n",__func__,err);
}
		printk("---zxw--->%s: ==probe over =\n",__func__);
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, gslX680_ts);
exit_irq_request_failed:
	cancel_work_sync(&gslX680_ts->pen_event_work);
	destroy_workqueue(gslX680_ts->ts_workqueue);
exit_create_singlethread:
	GSL168X_DBG("==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	kfree(gslX680_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	//sprd_free_gpio_irq(gslX680_ts_setup.irq);
	free_irq(client->irq, gslX680_ts);
exit_alloc_platform_data_failed:
//	remove_ctp_chip();
	return err;
}

static int gslX680_ts_remove(struct i2c_client *client)
{

	struct gslX680_ts_data *gslX680_ts = i2c_get_clientdata(client);

	GSL168X_DBG("==gslX680_ts_remove=\n");
#ifdef USE_TP_PSENSOR
	tp_ps_uninit();
#endif

#ifdef GSL_MONITOR
	cancel_delayed_work_sync(&gsl_monitor_work);
	destroy_workqueue(gsl_monitor_workqueue);
#endif
//add by glu
adf_unregister_client(&adf_event_block_gsl);
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&gslX680_ts->early_suspend);
#endif
	free_irq(client->irq, gslX680_ts);
	//sprd_free_gpio_irq(gslX680_ts_setup.irq);
	input_unregister_device(gslX680_ts->input_dev);
	kfree(gslX680_ts);
	cancel_work_sync(&gslX680_ts->pen_event_work);
	destroy_workqueue(gslX680_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
#ifdef GSL_GESTURE	
	wake_lock_destroy(&tp_wakelock);//add by xieyuehong 20181115
#endif
	return 0;
}




static const struct i2c_device_id gslX680_ts_id[] = {
	{ GSLX680_TS_NAME, 0 },{ }
};
MODULE_DEVICE_TABLE(i2c, gslX680_ts_id);


static const struct of_device_id gslX680_of_match[] = {
       { .compatible = "gslX680,gslX680_ts", },
       { }
};

MODULE_DEVICE_TABLE(of, gslX680_of_match);

static struct i2c_driver gslX680_ts_driver = {
	.probe		= gslX680_ts_probe,
	.remove		= gslX680_ts_remove,
	.id_table	= gslX680_ts_id,
	.driver	= {
		.name	= GSLX680_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = gslX680_of_match,
	},
};

static int __init gslX680_ts_init(void)
{

	printk("--zxw-->==gslX680_ts_init==\n");	
	return  i2c_add_driver(&gslX680_ts_driver);

}

static void __exit gslX680_ts_exit(void)
{
	i2c_del_driver(&gslX680_ts_driver);	
}

module_init(gslX680_ts_init);
module_exit(gslX680_ts_exit);

MODULE_AUTHOR("zxw");
MODULE_DESCRIPTION("GSLX680 TouchScreen Driver");
MODULE_LICENSE("GPL");


