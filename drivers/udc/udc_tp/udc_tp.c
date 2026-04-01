/*
 *				         Udc_Tp
 *
 *              Copyright (C) 2012 Spreadtrum
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/udc.h>


int default_key_num = 4;
int default_key_code[4] = {KEY_HOME,KEY_MENU,KEY_BACK,KEY_SEARCH};
int default_reset[6] = {1,5,0,20,1,5};

static ssize_t udc_tp_set_calibrate(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len);
static int udc_tp_update_fw_bin(struct _udc_tp_config *config);
static ssize_t i2c_read_data_test(struct device* cd,struct device_attribute *attr, char* buf,size_t len);
static ssize_t udc_tp_show_suspend(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t udc_tp_store_suspend(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static DEVICE_ATTR(calibrate, S_IRUGO | S_IWUSR, NULL, udc_tp_set_calibrate);
static DEVICE_ATTR(suspend, S_IRUGO | S_IWUSR, udc_tp_show_suspend, udc_tp_store_suspend);
static DEVICE_ATTR(i2c_test, S_IWUSR,  NULL,i2c_read_data_test);
udc_t udc_tp_get_keyid(char* key);
int udc_tp_add_driver(struct _udc_tp* tp);
void udc_tp_del_driver(struct _udc_tp* tp);
int add_tp_i2c_device(struct _udc_tp* tp);
int udc_tp_identify(struct _udc_tp* tp,udc_t section_id);
int udc_tp_late_init(struct _udc_tp* tp);
	
udc_tp g_udc_tp ;

#define	MAX_FINGER_NUM	5
#define   MAX_DATA_COUNT   50


struct point_node_t{
	int	posx;
	int	posy;
};
static struct point_node_t point_slot_back[MAX_FINGER_NUM*2];
static int distance[5]={0};
static int touch_flage[5]={0};
udc_item_s g_udc_tp_item[] = {
	{TP_FIRMWARE_NAME     ,   0,   0},
	{TP_ADDR              ,   0,   0},
	{TP_PANEL_RAW_W_H     ,   0,   0},
	{TP_PANEL_REL_W_H     ,   0,   0},
	{TP_ID                ,   0,   0},
	{TP_IRQFLAG           ,   0,   0},
	{TP_TIME_UP           ,   0,   0},
	{TP_FINGERNUM         ,   0,   0},
	{TP_KEYTYPE           ,   0,   0},
	{TP_KEYNUM            ,   0,   0},
	{TP_KEYCODE           ,   0,   0},
	{TP_KEY_MID_X         ,   0,   0},
	{TP_KEY_MID_Y         ,   0,   0},
	{TP_KEY_WIDTH         ,   0,   0},
	{TP_KEY_HEIGHT        ,   0,   0},
	{TP_FINGER_NUM_INDEX  ,   0,   0},
	{TP_FINGER_NUM_OPR    ,   0,   0},
	{TP_KEY_VALUE_INDEX   ,   0,   0},
	{TP_KEY_VALUE_OPR     ,   0,   0},
	{TP_X_OFFSET          ,   0,   0},
	{TP_Y_OFFSET          ,   0,   0},
	{TP_OFFSET_CACULATE_TYPE ,   0,   0},
	
	{TP_X_OPR_INDEX       ,   0,   0},
	{TP_X_FIR_OPR         ,   0,   0},
	{TP_X_SEC_OPR         ,   0,   0},
	{TP_Y_OPR_INDEX       ,   0,   0},
	{TP_Y_FIR_OPR         ,   0,   0},
	{TP_Y_SEC_OPR         ,   0,   0},
	{TP_NAGETA            ,   0,   0},
	{TP_DIS_THRESHOLD        ,   0,   0},
	
	{TP_NEXT_POINT_OFFSET ,   0,   0},
	{TP_PRESSURE          ,   0,   0},
	{TP_WIDTH             ,   0,   0},
	{TP_RESET             ,   0,   0},	
	{TP_READ_CHIP_ID      ,   0,   0},
	{TP_READ_FINGER_NUM   ,   0,   0},
	{TP_READ_KEY_VALUE    ,   0,   0},	
	{TP_READ_POINT        ,   0,   0},
	{TP_OPEN          ,   0,   0},
	{TP_SUSPEND       ,   0,   0},
	{TP_RESUME        ,   0,   0},
	{TP_CALIBRATE     ,   0,   0},
	{TP_END_CMD       ,   0,   0},
	
#ifdef CONFIG_USE_TP_PSENSOR	
	{TP_PSENSOR_ENABLE        ,   0,   0},
	{TP_PSENSOR_DISABLE     ,   0,   0},
	{TP_PSENSOR_GET_DATA       ,   0,   0},
#endif
	
	{0XFFFF               ,   0,   0}
	
};

#ifdef CONFIG_USE_TP_PSENSOR

typedef  struct _tp_ps_t{
	struct input_dev *input_dev;
}tp_ps_t;

#define PS_DEBUG

#ifdef PS_DEBUG
#define PS_DBG(format, ...)	\
		printk(KERN_INFO "TP_PSENSOR " format "\n", ## __VA_ARGS__)
#else
#define PS_DBG(format, ...)
#endif

static int tp_ps_opened = 0;
static atomic_t ps_flag;
static tp_ps_t *tp_ps = NULL;

static int psensor_enable(struct _udc_tp* tp)
{
    PS_DBG("==%s==\n",__func__);
    u8 buf[3];
    int ret = -1;	
	ret = udc_tp_do(tp, TP_PSENSOR_ENABLE, NULL, 0);
    PS_DBG("%s: ret=%d", __func__, ret);
    return ret;
}


static int psensor_disable(struct _udc_tp* tp)
{
    PS_DBG("==%s==\n",__func__);
    u8 buf[3];
    int ret = -1;	
	ret = udc_tp_do(tp, TP_PSENSOR_DISABLE, NULL, 0);
    PS_DBG("%s: ret=%d", __func__, ret);
    return ret;
}

static int tp_ps_open(struct inode *inode, struct file *file)
{
	PS_DBG("%s\n", __func__);
	if (tp_ps_opened)
		return -EBUSY;
	tp_ps_opened = 1;
	return 0;
}

static int tp_ps_release(struct inode *inode, struct file *file)
{
    PS_DBG("%s", __func__);
	tp_ps_opened = 0;
	udc_tp* tp = udc_tp_get_tp();
	return psensor_disable(tp);
	
}

static long tp_ps_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
   void __user *argp = (void __user *)arg;
    int flag;
	udc_t data;
	
	PS_DBG("%s: cmd %d", __func__, _IOC_NR(cmd));
	udc_tp* tp = udc_tp_get_tp();

	//get ioctl parameter
	switch (cmd) {
	case LTR_IOCTL_SET_PFLAG:
		if (copy_from_user(&flag, argp, sizeof(flag))) {
			return -EFAULT;
		}
		if (flag < 0 || flag > 1) {
			return -EINVAL;
		}
		PS_DBG("%s: set flag=%d", __func__, flag);
		atomic_set(&ps_flag, flag);	
		if(flag==1){
			psensor_enable(tp);
		}
		else if(flag==0) {
			psensor_disable(tp);
		}		
		break;
		
	case LTR_IOCTL_GET_PFLAG:
		flag = atomic_read(&ps_flag);
		if (copy_to_user(argp, &flag, sizeof(flag))) 
		{
				return -EFAULT;
		}
		PS_DBG("%s: get flag=%d", __func__, flag);		
		break;

	case LTR_IOCTL_GET_DATA:
		udc_tp_do(tp, TP_PSENSOR_GET_DATA, &data, 1);
		if (copy_to_user(argp, &data, sizeof(udc_t))) {
			return -EFAULT;
		}
		PS_DBG("%s: get data=%d", __func__, flag);
		break;	
		
	default:
	    PS_DBG("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
		return -EINVAL;
	} 

	return 0;	
}

static struct file_operations tp_ps_fops = {
	.owner				= THIS_MODULE,
	.open				= tp_ps_open,
	.release			= tp_ps_release,
	.unlocked_ioctl				= tp_ps_ioctl,
};

static struct miscdevice tp_ps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = UDC_PS_NAME,
	.fops = &tp_ps_fops,
};


/* call this function in tp work func*/
static void tp_ps_report_dps(struct _udc_tp* tp)
{
	PS_DBG("%s\n", __func__);
	unsigned char dps_data = 0;
	int   ret;
	u8  buf[32];
	udc_tp_do(tp, TP_PSENSOR_GET_DATA, buf, 16);
	if(NULL !=tp->caculate_psensor_data)
		tp->caculate_psensor_data(tp,buf,&dps_data);
    
	input_report_abs(tp_ps->input_dev, ABS_DISTANCE, dps_data);      
	input_sync(tp_ps->input_dev);   

}

static int tp_ps_init(struct i2c_client *client)
{
	int err = 0;
	struct input_dev *input_dev;

	tp_ps = kzalloc(sizeof(tp_ps_t), GFP_KERNEL);
	if (!tp_ps)
	{
		PS_DBG("%s: request memory failed\n", __func__);
		err= -ENOMEM;
		goto exit_mem_fail;
	}
		
	//register device
	err = misc_register(&tp_ps_device);
	if (err) {
		PS_DBG("%s: tp_ps_device register failed\n", __func__);
		goto exit_misc_reg_fail;
	}

	// register input device 
	input_dev = input_allocate_device();
	if (!input_dev) 
	{
		PS_DBG("%s: input allocate device failed\n", __func__);
		err = -ENOMEM;
		goto exit_input_dev_allocate_failed;
	}

	tp_ps->input_dev = input_dev;

	input_dev->name = UDC_PS_INPUT_NAME;
	input_dev->phys  = UDC_PS_INPUT_NAME;
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
	    PS_DBG("%s: input device regist failed\n", __func__);
	    goto exit_input_register_failed;
	}

	PS_DBG("%s: Probe Success!\n",__func__);
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

static int tp_ps_uninit()
{
	misc_deregister(&tp_ps_device);
	//free input
	input_unregister_device(tp_ps->input_dev);
	input_free_device(tp_ps->input_dev);
	//free alloc
	kfree(tp_ps);
	tp_ps = NULL;
}

#endif //CONFIG_USE_TP_PSENSOR

static struct i2c_client * i2c_static_add_device(udc_tp* tp,struct i2c_board_info *info)
{
	TP_INFO("==%s==", __func__);
	struct i2c_client 	*i2c_client 	= NULL;
	struct i2c_adapter *i2c_adapter		= NULL;
	struct _udc_tp_config *config = &tp->config;
	
	i2c_adapter = i2c_get_adapter(config->i2c_bus_num);
	if (!i2c_adapter)
	{
		TP_ERR(" Can't get i2c adapter");
		goto i2c_err;
	}
	i2c_client = i2c_new_device(i2c_adapter, info);
	if (!i2c_client) 
	{
		TP_ERR("%s:  Can't add i2c device at 0x%x",__func__, config->addr);
		goto i2c_err;
	}
	
	i2c_put_adapter(i2c_adapter);
	return i2c_client;
i2c_err:
	return NULL;
}

/* default kobject attribute operations */

static ssize_t udc_tp_key_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *buf)
{
	struct key_attr *file_attr;
	struct key_file_info	*p_key_file_info;
	ssize_t ret = -EIO;

	file_attr = container_of(attr, struct key_attr, attr);
	p_key_file_info = container_of(file_attr,  struct key_file_info,  file_attr);
	if (file_attr->show)
		ret = file_attr->show(p_key_file_info, buf);
	return ret;
}


const struct sysfs_ops udc_tp_key_sysfs_ops =
{
	.show	= udc_tp_key_attr_show,
};

static struct kobj_type udc_tp_key_ktype = 
{
	.sysfs_ops	= &udc_tp_key_sysfs_ops,
};


static ssize_t udc_tp_virtual_keys_show(struct key_file_info	*p_key_file_info, char *buf)
{	
	return sprintf (buf, "%s", p_key_file_info->file_buf); 
}


static void  udc_tp_create_virtal_key(udc_tp* tp,struct udc_tp_prv_data *prv_data, struct input_dev *input_dev)
{
	int   i;
	int	ret;
	char tempStr[20];
	struct key_file_info	*p_key_file_info = &prv_data->key_file_info;
	struct _udc_tp_config 	*config 	= &tp->config;
	udc_tp_key* key = &config->key;

      TP_INFO("%s: key->count = %d", __func__, key->count);	
	
	if ( 0 == key->count )
	{
		TP_NOTICE("%s hasn't not virtual keys");
		return;
	}

	sprintf (p_key_file_info->file_name, "%s.%s",  MULTI_TP_VIRUALKEY_FILE_NAME, UDC_TP_NAME);

	for (i = 0; i < key->count; i++)
	{
	   	
               __set_bit(key->code[i],  input_dev->keybit);

              if ( i == key->count -1)
              {
			sprintf(tempStr, "%s:%d:%d:%d:%d:%d",__stringify(EV_KEY),
			key->code[i], key->virtual.x[i], key->virtual.y[i], key->virtual.w[i], key->virtual.h[i]);
              }
		else
		{
			sprintf(tempStr, "%s:%d:%d:%d:%d:%d:",__stringify(EV_KEY),
			key->code[i], key->virtual.x[i], key->virtual.y[i], key->virtual.w[i], key->virtual.h[i]);
		}
			  
		strcat(p_key_file_info->file_buf, tempStr);
             
     }
 
	ret = kobject_init_and_add(&p_key_file_info->file_obj, &udc_tp_key_ktype, NULL, UDC_TP_BOARD_INFO_NAME);
	if (ret)
	{
		TP_ERR("Error to init obj in fun %s return %d", __func__, ret);
		return;
	}
	p_key_file_info->file_attr.attr.name = p_key_file_info->file_name;
	p_key_file_info->file_attr.attr.mode = S_IRUGO;
	p_key_file_info->file_attr.show		= &udc_tp_virtual_keys_show;
	
    ret = sysfs_create_file(&p_key_file_info->file_obj,  &p_key_file_info->file_attr.attr);
    	
   	if ( ret)
   	{			
       		TP_ERR("failed to create board_properties return %d", ret);    
		return;
   	}		
	p_key_file_info->is_file_create = 1;
	return;
}

static struct input_dev *udc_tp_input_device_init(udc_tp* tp,struct udc_tp_prv_data *prv_data)
{
	TP_INFO("==%s==", __func__);	
	int i,error;
	struct input_dev *input_dev = NULL;
	struct _udc_tp_config *config = &tp->config;
	udc_tp_abs* abs = &config->abs;
	udc_tp_key* key = &config->key;
	
			
	input_dev = input_allocate_device();
	if (!input_dev) 
	{
		TP_ERR( "Failed to allocate input!");
		return NULL;
	}
	
	input_dev->name		= UDC_TP_NAME;	

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
#if defined(CONFIG_UDC_VER_4_0_1) || defined(CONFIG_UDC_VER_5_1_3)
  	__set_bit(BTN_TOUCH, input_dev->keybit);
#endif		
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	input_set_abs_params(input_dev,ABS_MT_POSITION_X, 0, config->panel_rel_w - 1, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_POSITION_Y, 0, config->panel_rel_h - 1, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
				 	
	    //if need virtual key,create it
	if ( key->type == 0 )
		udc_tp_create_virtal_key(tp,prv_data,  input_dev);

	//if need report key_event,set keycode
	if ( key->type == 1  )
	{
		for ( i = 0; i < key->count; i++)
		{
			__set_bit(key->code[i], input_dev->keybit);
		}
	}

	error = input_register_device(input_dev);
	if (error)
	{
		input_put_device(input_dev);
		TP_ERR("Unable to register device %d!", error);
		return NULL;
	}

	return input_dev;	
}
     					
int udc_caculate_tp_data(struct _udc_tp* tp, struct tp_event *event,u8 *point_count_buf,u8 *key_value_buf,u8 *abs_buffer)
{
	unsigned char touch = 0,pix_id,slot_id;
	int i,temp;
	int x,y;
	struct _udc_tp_config *config = &tp->config;
	udc_tp_abs* abs =  &config->abs;
	unsigned short *point_magic = abs->point_magic;
	unsigned short *x1_magic = config->abs.x1_magic;
	unsigned short *x2_magic = config->abs.x2_magic;
	unsigned short *y1_magic = config->abs.y1_magic;
	unsigned short *y2_magic = config->abs.y2_magic;
	unsigned short x_index[2];
	unsigned short y_index[2];
	
	x_index[0] = config->abs.x_index[0];
	x_index[1] = config->abs.x_index[1];
	y_index[0] = config->abs.y_index[0];
	y_index[1] = config->abs.y_index[1];
		
    TP_INFO("==%s==", __func__);
	touch = ( (point_count_buf[abs->point_index] & point_magic[0]) << point_magic[1] ) >> (point_magic[2]);
	TP_INFO("touch = 0x%x\n",touch);
    //Ignore the false points
	if ( touch > abs->max_point)
    {
        event->point_count = 0;	
		TP_INFO("Invalid touch = 0x%x\n",touch);
		return -1;
    }
		
	if(touch) 
	{	
		//for pixcir FT5X0X,cy8ctst242，it7250,gt818x
		for ( i = 0;   i< touch; i++ )	
		{
			event->x[i] = (((abs_buffer[x_index[0]]&(*x1_magic))<<(*(x1_magic+1))>>(*(x1_magic+2))) 
				            |((abs_buffer[x_index[1]] &(*x2_magic))<<(*(x2_magic+1))>>(*(x2_magic+2))));
								
			event->y[i] = (((abs_buffer[y_index[0]]&(*y1_magic))<<(*(y1_magic+1))>>(*(y1_magic+2))) 
				            |((abs_buffer[y_index[1]] &(*y2_magic))<<(*(y2_magic+1))>>(*(y2_magic+2))));

                    //TP_INFO("udc_caculate_tp_data, line = %d, config->panel_rel_w = %d, config->panel_raw_w = %d, x = %d,y = %d\n", __LINE__, config->panel_rel_w, config->panel_raw_w, event->x[i],event->y[i]);
		       event->x[i] = event->x[i] * config->panel_rel_w / config->panel_raw_w;
		       event->y[i] = event->y[i] * config->panel_rel_h / config->panel_raw_h;
			//TP_INFO("udc_caculate_tp_data, line = %d, config->panel_rel_h = %d, config->panel_raw_h = %d, x = %d,y = %d\n", __LINE__, config->panel_rel_h, config->panel_raw_h, event->x[i],event->y[i]);
		      
			if (config->abs.negate )
			{
				if (event->x[i] < 0)
					event->x[i] = 0;
				else if (event->x[i] > config->panel_rel_w)
					event->x[i] = config->panel_rel_w;

				if (event->y[i] < 0)
					event->y[i] = 0;
				else if (event->y[i] > config->panel_rel_h)
					event->y[i] = config->panel_rel_h;	
				event->x[i] = -event->x[i];
				event->y[i] = -event->y[i];
			}
				
			if(config->abs.offset_direction)
			{
				event->x[i] = event->x[i] - config->abs.offset_x;;
				event->y[i] = event->y[i] - config->abs.offset_y;;
			}
			else
			{
				event->x[i] = event->x[i] + config->abs.offset_x;;
				event->y[i] = event->y[i] + config->abs.offset_y;;
			}
			if(event->x[i] > config->panel_rel_w || event->y[i] > (config->panel_rel_h + 200) )
			{
				TP_INFO("get dirty data,x = %d,y = %d\n",event->x[i],event->y[i]);
				event->point_count = 0;
//				return -1;
			}
			if(distance[i]==0)
			{
				point_slot_back[i].posx = event->x[i];
				point_slot_back[i].posy = event->y[i];			
			}
			x_index[0] += abs->point_gap;
			x_index[1] += abs->point_gap;
			y_index[0] += abs->point_gap;
			y_index[1] += abs->point_gap;				
			event->point_count++;
		}
		
		for(i=0;i<touch;i++)
		{
			x = point_slot_back[i].posx-event->x[i];
			x = (x>0)?x:-x;
			y = point_slot_back[i].posy-event->y[i];
			y =(y>0)?y:-y;
			temp=x+y;
			if(distance[i])
			{
				if ( (temp < abs->point_threshold ) && (touch_flage[i] == 0 ) )
				{
					event->x[i] = point_slot_back[i].posx;
					event->y[i] = point_slot_back[i].posy;
					TP_INFO("report back\n");
				}
				else 
					touch_flage[i]=1;
			} 
			else 
				distance[i]=1;
		}
		
	}
	else 
	{
		memset(distance,0,sizeof(distance));
		memset(touch_flage,0,sizeof(touch_flage));		
	}
exit_work_func:
        return 0;	
}



int udc_tp_get_key_event(struct _udc_tp* tp, struct tp_event *event, udc8_t *buffer)
{

	int i;
	udc8_t  key_status = 0;
	struct _udc_tp_config *config = &tp->config;
	udc_tp_key *key = &config->key;
	   
	key_status = buffer[key->status_index] & key->status_mask;

	//Key Pressed
	if(key_status)
	{
	      
		TP_INFO("key_value = %d",key_status);
              event->key_count = 0;
		for (i = 0; i < key->count; i++)
		{
			if(key_status & (0x01<<i))
			{
				event->type = TP_EVENT_TYPE_KEY;
				event->key_event[i] = 1;
				event->key_count ++;	
				return 1;
			}
		}		
	}

	return 0;
		
}


static void udc_tp_irq_work(struct work_struct *work)
{
	int i,j,ret;
	int isEnableIrq = 1;
	struct _udc_tp_config *config;
	struct udc_tp_prv_data *prv_data;
	struct tp_event *event;
	struct input_dev*input_dev;
       unsigned char point_count_buf[MAX_DATA_COUNT] = {0};
	unsigned char key_value_buf[MAX_DATA_COUNT] = {0};
	unsigned char abs_buffer[MAX_DATA_COUNT] = {0};
	udc_tp* tp = udc_tp_get_tp();
	udc_tp_key* key = &tp->config.key;
       udc_tp_abs* abs = &tp->config.abs;
	
	TP_INFO("==%s==", __func__);
	   
#ifdef CONFIG_USE_TP_PSENSOR
    int  flag;
	flag = atomic_read(&ps_flag);
	if(flag)
    {
		tp_ps_report_dps(tp);
	}
#endif	
	
	
	
	prv_data 	=tp->prv_data;
	config 	= &tp->config;
	input_dev 	= prv_data->input_dev;
	event		= &prv_data->event;
	
		
	if ((config->time_up)&&(config->time_hz))
	{  
		del_timer(&prv_data->timer);
	}	

	event->point_count = 0;
	event->type = TP_EVENT_TYPE_ABS;

	udc_tp_do(tp, TP_READ_FINGER_NUM,&point_count_buf[0], 25);
	 //Handle Physical Key
	udc_tp_do(tp, TP_READ_KEY_VALUE,&key_value_buf[0], 25);
	udc_tp_do(tp, TP_READ_POINT, &abs_buffer[0], 25);

        if ( udc_tp_get_key_event(tp, event, key_value_buf) )
	 {
		//Should be key status
              if ( event->key_count  > 0 && event->type == TP_EVENT_TYPE_KEY)
             	{
#if defined(CONFIG_UDC_VER_4_0_1) || defined(CONFIG_UDC_VER_5_1_3)
	       	input_report_key(input_dev, BTN_TOUCH, 1);
#endif	      
			for( j = 0; j < key->count; j++)
			{
				if(event->key_event[j]==1)
				{
					input_report_key(input_dev, key->code[j], 1);	
					TP_INFO("input_report_key,down:key->code = %d\n",key->code[j]);
				}
			}
#ifdef CONFIG_UDC_VER_2_3_5					
			input_report_key(input_dev, BTN_TOUCH, 1);
#endif				
			input_sync(input_dev);
		}
	 }
	 else
	 {

		       // Report KeyUP
			if ( event->key_count  > 0)
			{
				for ( i = 0; i < key->count; i ++ )
				{
					if(event->key_event[i] == 1)
					{
						input_report_key(input_dev, key->code[i], 0);
						event->key_event[i]=0;
						TP_INFO("input_report_key,release\n");	
					}
				}
#ifdef CONFIG_UDC_VER_2_3_5			
			  input_report_key(input_dev, BTN_TOUCH, 0);
#endif			
#if defined(CONFIG_UDC_VER_4_0_1) || defined(CONFIG_UDC_VER_5_1_3)
		          input_report_key(input_dev, BTN_TOUCH, 0);
			   input_mt_sync(input_dev);
#endif	
			   input_sync(input_dev);

			   event->key_count = 0;		

			}

	 }
	
	  
	ret = tp->caculate_tp_data(tp,event,point_count_buf,key_value_buf,abs_buffer);
	if(ret < 0)
		goto exit_work_func;

	
	 if (event->point_count > UDC_TP_MAX_FINGER_NUM)
	{
		TP_ERR("point_count(%d) too much", event->point_count);
	} 	
	else if (0 == event->point_count)
	{
		if (event->key_count  == 0)
		{
#ifdef CONFIG_UDC_VER_2_3_5		
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 0);
#endif			
#if defined(CONFIG_UDC_VER_4_0_1) || defined(CONFIG_UDC_VER_5_1_3)
            input_report_key(input_dev, BTN_TOUCH, 0);
	     	input_mt_sync(input_dev);
#endif	
			input_sync(input_dev);
			TP_INFO("input_report_abs,release\n");	

		}

	}
	else
	{
		if (event->type == TP_EVENT_TYPE_ABS)
		{
#if defined(CONFIG_UDC_VER_4_0_1) || defined(CONFIG_UDC_VER_5_1_3)
       		input_report_key(input_dev, BTN_TOUCH, 1);
#endif	  	
			for ( i = 0; i < event->point_count; i++ )
			{
#ifdef CONFIG_UDC_VER_2_3_5				
				input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, abs->pressure);
#endif				
				input_report_abs(input_dev, ABS_MT_POSITION_X, event->x[i]);
				input_report_abs(input_dev, ABS_MT_POSITION_Y, event->y[i]);

#ifdef CONFIG_UDC_VER_2_3_5		
				input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, abs->width);
#endif				
				input_mt_sync(input_dev);	
				TP_INFO("x = (%d), y = (%d)\n",event->x[i], event->y[i]);
				TP_INFO("input_report_abs,down\n");
			}
			input_sync(input_dev);
		
		}
		
		if ((config->time_up)&&(config->time_hz))
		{	
			prv_data->timer.expires = config->time_hz + jiffies;
			add_timer(&prv_data->timer);
			isEnableIrq = 0;			
		}	

		
	}

exit_work_func:
	if (isEnableIrq )
	{
		enable_irq(prv_data->i2c_client->irq);	
	}

}

static void udc_tp_timer_handle(unsigned long data)
{
	TP_INFO("==%s==", __func__);
	struct udc_tp_prv_data *prv_data = (struct udc_tp_prv_data *)data;
	
	if (!work_pending(&prv_data->event_work))
	{
		queue_work(prv_data->work_queue, &prv_data->event_work);
	}

}
static irqreturn_t udc_tp_irq_handler(int irq, void *dev_id)
{
	TP_INFO("==%s==", __func__);
	struct udc_tp_prv_data *prv_data = (struct udc_tp_prv_data *)dev_id;

	disable_irq_nosync(irq);
	if (!work_pending(&prv_data->event_work))
	{
		queue_work(prv_data->work_queue, &prv_data->event_work);
	}
	return IRQ_HANDLED;
}

/*
description: ioctl function for character device driver
prarmeters:  inode   file node
             filp    file pointer
             cmd     command
             arg     arguments
return:      status
*/
static int udc_tp_misc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned char buffer[32]={0};
	int len=0;
	int ret;
	udc_tp* tp = udc_tp_get_tp();
	struct _udc_tp_config *config;
	struct udc_tp_prv_data *prv_data;
	prv_data 	=tp->prv_data;
	config 	= &tp->config;
	struct i2c_client	*i2c_client;
	i2c_client = prv_data->i2c_client;	
	struct i2c_msg msgs[] = {
		{.addr = config->addr, .flags = 0, .len = len, .buf = buffer,}
    };
	
 // parsing ioctl command
	switch(cmd){
		case UDC_TP_IOCTL_I2C_WRITE_DATA:
			ret = copy_from_user(buffer, (unsigned char*)arg, len);
			if(ret < 0)
			{
						 TP_ERR("%s, copy data from user space, failed\n", __func__);
						 return -1;
			}
			ret = i2c_transfer(i2c_client->adapter, msgs, 1);
			if(ret < 0)
			{
				TP_ERR("%s, i2c write, failed\n", __func__);
				return -1;
			}
		break;
		
		case UDC_TP_IOCTL_I2C_READ_DATA:
			msgs[0].flags = I2C_M_RD;
			ret = i2c_transfer(i2c_client->adapter, msgs, 1);
			if(ret < 0)
			{
				TP_ERR("%s, i2c read, failed\n", __func__);
				return -1;
			}
			ret = copy_to_user((unsigned char*)arg, buffer, len);
			if(ret < 0)
			{
				TP_ERR("%s, copy data to user space, failed\n", __func__);
				return -1;
			}
		break;
		
		case UDC_TP_IOCTL_I2C_CALIBRATE:
//			if(config->op.tp_calibrate)
//				config->op.tp_calibrate();
		break;
				
		case UDC_TP_IOCTL_I2C_UPDATE:
//			if(config->op.update_fw)
				udc_tp_update_fw_bin(config);		
		break;
		
        default:
			return -1;
	}
    return 0;
}

static int udc_tp_update_fw_bin(struct _udc_tp_config *config)
{
 	int ret = 0;
    const struct firmware *fw;
    unsigned char *fw_buf = NULL;
	struct platform_device *pdev= NULL;
	
	pdev = platform_device_register_simple((char*)UDC_TP_NAME , 0, NULL, 0);
	if (IS_ERR(pdev))
	{
		TP_ERR("Failed to register firmware");
		return -1;
	}

	ret = request_firmware(&fw, config->firmware_name, &pdev->dev);
    if (ret) 
	{
		TP_ERR(" request_firmware error");
		platform_device_unregister(pdev);
       	return -1;
    }

	platform_device_unregister(pdev);
 	TP_INFO("%s:fw size=%d\n", __func__,fw->size);
	
    fw_buf = kzalloc(fw->size, GFP_KERNEL | GFP_DMA);
    memcpy(fw_buf, fw->data, fw->size);
/*	if (config->op.update_fw)
	{
   		config->op.update_fw(fw_buf, fw->size);
	}
*/	
	TP_INFO("%s: update finish\n", __func__);
    	release_firmware(fw);
  	kfree(fw_buf);

	return 0;    	
}

int udc_tp_do(udc_tp *tp , udc_t key_id, udc_t * reg_values, udc_t  reg_values_count)
{
	 int ret = -EINVAL;   
	udc_t* value;
	udc_t flag;
	udc_t value_count;
	udc_s* udc = tp->udc;
	udc_i2c_info i2c_info;
	i2c_info.i2c_client = tp->prv_data->i2c_client;
	value_count = tp->item[key_id].value_count;
	value = tp->item[key_id].value;	
	
	if( NULL == value)
	    return 0;
	if (value_count <= 2 )
		return 0;
	
	flag = value[0];
	if (UDC_BLOCK_FLAG != flag )
		return 0;
	i2c_info.mask_bits = value[1];
	i2c_info.value = value+2;
	i2c_info.value_count = value_count-2;
	ret  = udc_i2c_do(udc,&i2c_info,reg_values,reg_values_count);
	return ret;
}

void udc_tp_pwron(struct _udc_tp* tp)
{
	TP_INFO("%s\n",__func__);
	
#if defined CONFIG_UDC_VER_5_1_3
	struct regulator *reg_vdd;
	reg_vdd = regulator_get(NULL, "vdd28");
	regulator_set_voltage(reg_vdd, 2800000, 2800000);
	regulator_enable(reg_vdd);

#else
       udc_regulator_on(SEC_TP);
#endif
	msleep(40);
}

static void udc_tp_configpins(void)
{
	gpio_direction_output(udc_gpio_tp_rst, 1);
	gpio_direction_input(udc_gpio_tp_irq);
	gpio_set_value(udc_gpio_tp_rst, 1);
	
}

static void udc_tp_reset(struct _udc_tp* tp)
{
	TP_INFO("%s\n",__func__);
	udc_t value_count;
	udc_t *value;
	value_count = tp->item[TP_RESET].value_count;
	if(value_count<6)
	{
		value = default_reset;
	}
	else
		value = tp->item[TP_RESET].value;	
	
	gpio_set_value(udc_gpio_tp_rst, value[0]);
	msleep(value[1]);
	gpio_set_value(udc_gpio_tp_rst,value[2]);
	msleep(value[3]); 
	gpio_set_value(udc_gpio_tp_rst, value[4]);
	msleep(value[5]);
}

void udc_tp_open(struct _udc_tp* tp)
{
	udc_tp_configpins();
	udc_tp_pwron(tp);	
	tp->reset(tp);
	udc_tp_do(tp, TP_OPEN, NULL, 0);
}

void udc_tp_pwroff(struct _udc_tp* tp)
{
	TP_INFO("==%s==\n", __func__);
#if defined CONFIG_UDC_VER_5_1_3
	
	struct regulator *regu;
	regu = regulator_get(NULL, "vdd28");
	regulator_disable(regu);
	regulator_put(regu);

#else
	udc_regulator_off(SEC_TP);
#endif
}

static  void udc_tp_suspend(struct early_suspend *h)
{
	TP_INFO("==%s==\n", __func__);
	
#ifdef CONFIG_USE_TP_PSENSOR
    int  flag;
	flag = atomic_read(&ps_flag);
	if(flag)
		return;
#endif	
	
	udc_tp* tp = udc_tp_get_tp();
	struct udc_tp_prv_data *prv_data 	=tp->prv_data;
	struct _udc_tp_config 	*config = &tp->config;

	udc_tp_do(tp, TP_SUSPEND, NULL, 0);	
	
	if ((config->time_up)&&(config->time_hz))
       {
			del_timer(&prv_data->timer);

	}
	
	disable_irq_nosync(prv_data->i2c_client->irq);
}

static  void udc_tp_resume(struct early_suspend *h)
{
	TP_INFO("==%s==\n", __func__);
	
#ifdef CONFIG_USE_TP_PSENSOR
    int  flag;
	flag = atomic_read(&ps_flag);
	if(flag)
		return;
#endif	
	
	udc_tp* tp = udc_tp_get_tp();

	tp->reset(tp);
	msleep(100);
	udc_tp_do(tp, TP_RESUME, NULL, 0);
	enable_irq(tp->prv_data->i2c_client->irq);
}

/* udc_tp_show_suspend --  for suspend/resume debug
 *                         show current status
 * params:	no care
 * @return: len
 */
int suspend_flag = 0;
static ssize_t udc_tp_show_suspend(struct device* cd,
				     struct device_attribute *attr, char* buf)
{
	ssize_t ret = 0;

	if(suspend_flag==1)
		sprintf(buf, "udc_tp Suspend\n");
	else
		sprintf(buf, "udc_tp Resume\n");
	
	ret = strlen(buf) + 1;

	return ret;
}


/* udc_tp_store_suspend -- for suspend/resume debug
 *                         set suspend/resume
 * params:	no care
 * @return: len
 */
static ssize_t udc_tp_store_suspend(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	suspend_flag= on_off;
	udc_tp* tp = udc_tp_get_tp();
	
	if(on_off==1)
	{
		printk(KERN_INFO "udc_tp Entry Suspend\n");
		tp->suspend(NULL);
	}
	else
	{
		printk(KERN_INFO "udc_tp Entry Resume\n");
		tp->resume(NULL);
	}
	
	return len;
}

static ssize_t i2c_read_data_test(struct device* cd,struct device_attribute *attr, char* buf,size_t len)
{
	udc_t device_id;
	udc_tp* tp = udc_tp_get_tp();

	unsigned long on_off = simple_strtoul(buf, NULL, 10);	
	if(on_off==0)
	{
		udc_tp_do(tp, TP_READ_CHIP_ID, &device_id, 1);
		TP_INFO("device_id id(%d)\n",device_id); 
	}

	return len;
}

/* udc_tp_calibrate --  enable tp chip calibration fucntion
 * @buf: 1 = start calibrate , first input variable 
 * @return: len
 */
static ssize_t udc_tp_set_calibrate(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	udc_tp* tp = udc_tp_get_tp();
	if(on_off==1)
	{
		TP_INFO("%s: udc_tp calibrate\n",__func__);
		udc_tp_do(tp, TP_CALIBRATE, NULL, 0);
	}
	
	return len;
}

/* udc_tp_create_sysfs --  create sysfs attribute
 * client:	i2c client
 * @return: len
 */
static int udc_tp_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);
	TP_INFO("%s", __func__);
	
	err = device_create_file(dev, &dev_attr_suspend);
	err = device_create_file(dev, &dev_attr_i2c_test);
	err = device_create_file(dev, &dev_attr_calibrate);
	return err;
}

static const struct file_operations udc_tp_misc_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = udc_tp_misc_ioctl,
};

static struct miscdevice udc_tp_misc_device = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = UDC_TP_NAME,
		.fops = &udc_tp_misc_fops,
};

static  void udc_tp_free(struct udc_tp_prv_data *prv_data)
{
	struct i2c_client 	*i2c_client = NULL;
	struct _udc_tp_config 	*config = NULL;

	if (NULL == prv_data)
		goto exit;

	udc_tp* tp = udc_tp_get_tp();
	i2c_client 	= prv_data->i2c_client;
	config	= &tp->config;

	if (i2c_client)
	{
 		i2c_set_clientdata(i2c_client, NULL);
		if ( i2c_client->irq)
		{
			free_irq(i2c_client->irq, prv_data);
			udc_put_irq(i2c_client->irq, 0xFFFF);
		}
	}
	
	if (prv_data->work_queue)
	{
		cancel_work_sync(&prv_data->event_work);
		destroy_workqueue(prv_data->work_queue);
	}

	if ((config->time_up)&&(config->time_hz))
	{
		del_timer_sync(&prv_data->timer);
	}	

	if (prv_data->input_dev)
	{
		input_unregister_device(prv_data->input_dev);	
	}

	misc_deregister(&udc_tp_misc_device);

exit:
	return;

}

udc_t udc_tp_get_value(udc_t key_id, udc_t* value, udc_t value_count)
{
    udc_tp* tp = udc_tp_get_tp();
	return udc_get_item_value(tp->item, key_id, value, value_count);
}

udc_t* udc_tp_get_string(udc_t key_id, udc_t *value_count)
{
	udc_t* value; 
	udc_tp* tp = udc_tp_get_tp();
	
	TP_INFO("%s",__func__);
    
    *value_count = tp->item[key_id].value_count;
	TP_INFO("count = %d", *value_count);
	if(*value_count)
	{
		value = tp->item[key_id].value;
		return value;
	}
    else 
	   return 0;   
}

void udc_tp_default_op(struct _udc_tp* tp,unsigned char device_id)
{
	tp->open = udc_tp_open;
	tp->reset = udc_tp_reset;
	tp->suspend = udc_tp_suspend;
	tp->resume = udc_tp_resume;
	tp->caculate_tp_data = udc_caculate_tp_data;

#ifdef CONFIG_USE_TP_PSENSOR	
	tp->caculate_psensor_data = NULL;
#endif
}

void udc_tp_early_init(struct _udc_tp* tp)
{
	TP_INFO("%s",__func__);
	int ret = 0;
	udc_t param;
	udc_s* udc;
	struct _udc_tp_config* config = &tp->config;
	udc_board_config* board_config = &tp->udc->board->config;
	config->i2c_bus_num = board_config->tp_i2c_bus_num;
	config->addr = 0x5c;
	config->panel_raw_w = 480;   
	config->panel_raw_h = 800; 
	config->panel_rel_w = 480;
	config->panel_rel_h = 800;
	config->device_id = 200;
	tp->late_init = udc_tp_late_init;
	tp->add_driver = udc_tp_add_driver;
	tp->del_driver = udc_tp_del_driver;
	tp->identify = udc_tp_identify;
	tp->config_operation = udc_tp_default_op;


	add_tp_i2c_device(tp);		
}

int udc_tp_late_init(struct _udc_tp* tp)
{
	TP_INFO("%s",__func__);
	char* name;
	udc_t value_count,param,i;	
	udc_tp_config *config = &tp->config;
	udc_tp_key *key = &config->key;
	
	unsigned short data_opr[10];
	udc_t panel_w_h[2];

       config->panel_raw_w = 480;   
	config->panel_raw_h = 800; 
	config->panel_rel_w = 480;
	config->panel_rel_h = 800;
	config->firmware_name = (char*)UDC_TP_NAME;
	config->irq_flag = 0x2;
	config->time_up = 10;
	config->abs.point_index = 0;
	config->abs.offset_x = 30;
	config->abs.offset_y = 40;
	config->abs.offset_direction = 1;
	config->abs.max_point = 1;
	name = (unsigned long*)udc_tp_get_string(TP_FIRMWARE_NAME , &value_count);
	if(value_count)
		config->firmware_name = (char*)name;
    TP_INFO("TP_FIRMWARE_NAME %s\n", config->firmware_name);

	if ( udc_tp_get_value(TP_PANEL_RAW_W_H, panel_w_h, 2) )
	{
		config->panel_raw_w = panel_w_h[0];
		config->panel_raw_h = panel_w_h[1];
	
	}
	TP_INFO("TP_PANEL_RAW_W_H:%d, %d\n", config->panel_raw_w, config->panel_raw_h);

	if ( udc_tp_get_value(TP_PANEL_REL_W_H, panel_w_h, 2) )
	{
		config->panel_rel_w = panel_w_h[0];
		config->panel_rel_h = panel_w_h[1];
	}	
	TP_INFO("TP_PANEL_REL_W_H:%d, %d\n", config->panel_rel_w, config->panel_rel_h);
	
	if (udc_tp_get_value(TP_IRQFLAG, &param, 1) )
		config->irq_flag = (unsigned long)param;
	TP_INFO("TP_IRQFLAG 0x%x\n", config->irq_flag);
	

      if ( udc_tp_get_value(TP_KEY_VALUE_INDEX, &param, 1) )
		key->status_index = param;
       else	
	   	key->status_index = 0;
	TP_INFO("TP_KEY_VALUE_INDEX 0x%x\n", key->status_index);
	
	if ( udc_tp_get_value(TP_KEY_VALUE_OPR, &param, 1) )
		key->status_mask = param;
	else 
		key->status_mask = 0; 
	TP_INFO("TP_KEY_VALUE_OPR 0x%x\n", key->status_mask);
	
	if ( udc_tp_get_value(TP_KEYTYPE, &param, 1) )
		key->type = param;
	else
		key->type = 0;
	TP_INFO("TP_KEYTYPE 0x%x\n", key->type);

	if ( udc_tp_get_value(TP_KEYNUM, &param, 1) )
		key->count = param;
	else
		key->count = 4;
	TP_INFO("TP_KEYNUM 0x%x\n", key->count);

       udc_tp_get_value(TP_KEYCODE, key->code, key->count);

	udc_tp_get_value(TP_KEY_MID_X, key->virtual.x, key->count);

	udc_tp_get_value(TP_KEY_MID_Y, key->virtual.y, key->count);

	udc_tp_get_value(TP_KEY_WIDTH, key->virtual.w, key->count);

	udc_tp_get_value(TP_KEY_HEIGHT, key->virtual.h, key->count);
	
       TP_INFO("TP_KEYCODE:", key->count);
	for( i = 0; i< key->count; i++ )
       {
		TP_INFO("code: %d, ", key->code[i]);
		TP_INFO("x: %d, ", key->virtual.x[i]);
		TP_INFO("y: %d, ", key->virtual.y[i]);
		TP_INFO("w: %d, ", key->virtual.w[i]);
		TP_INFO("h: %d, ", key->virtual.h[i]);
		TP_INFO("\n");
		
	}
	

	if (udc_tp_get_value(TP_TIME_UP , &param, 1) )
		config->time_up = param;
	TP_INFO("TP_TIME_UP 0x%x\n", config->time_up);


	if (udc_tp_get_value(TP_FINGERNUM , &param, 1) )
		config->abs.max_point = param;
	TP_INFO("TP_FINGERNUM 0x%x\n", config->abs.max_point);

    if ( udc_tp_get_value(TP_FINGER_NUM_INDEX, &param, 1) )
		config->abs.point_index = param;
	TP_INFO("TP_FINGER_NUM_INDEX 0x%x\n", config->abs.point_index);
	
	if ( ! udc_tp_get_value(TP_FINGER_NUM_OPR, data_opr, 3) )
	{	data_opr[0] = 0x07; data_opr[1] = 0 ;data_opr[2] = 0;
	}
	for(i = 0;i< 3; i++)
	{
		config->abs.point_magic[i] = data_opr[i];
	    TP_INFO("TP_FINGER_NUM_OPR 0x%x\n", config->abs.point_magic[i]);
	}
	
	
		
	if (udc_tp_get_value(TP_X_OFFSET, &param, 1) )
		config->abs.offset_x = param;
	TP_INFO("TP_X_OFFSET 0x%x\n", config->abs.offset_x);

	if (udc_tp_get_value(TP_Y_OFFSET, &param, 1) )
		config->abs.offset_y = param;
	TP_INFO("TP_Y_OFFSET 0x%x\n", config->abs.offset_y);
	
	if (udc_tp_get_value(TP_OFFSET_CACULATE_TYPE, &param, 1) )
		config->abs.offset_direction = param;
	TP_INFO("TP_OFFSET_CACULATE_TYPE 0x%x\n", config->abs.offset_direction);
	
	if ( ! udc_tp_get_value(TP_X_OPR_INDEX, data_opr, 2) )
	{	data_opr[0] = 3; data_opr[1] = 2;}
	for(i = 0;i< 2; i++)
	{
		config->abs.x_index[i] = data_opr[i];
	    TP_INFO("TP_X_OPR_INDEX 0x%x\n", config->abs.x_index[i]);
	}
	
	if ( ! udc_tp_get_value(TP_X_FIR_OPR, data_opr, 3) )
	{	
	       data_opr[0] = 0xff; 
	       data_opr[1] = 8;
		data_opr[2] = 0;
	}
	for(i = 0;i< 3; i++)
	{
		config->abs.x1_magic[i] = data_opr[i];
	    TP_INFO("TP_X_FIR_OPR_INDEX 0x%x\n", config->abs.x1_magic[i]);
	}
	
	if ( ! udc_tp_get_value(TP_X_SEC_OPR, data_opr, 3) )
	{	
	        data_opr[0] = 0xff; 
		 data_opr[1] = 0;
		 data_opr[2] = 0;
	}
	for(i = 0;i< 3; i++)
	{
		config->abs.x2_magic[i] = data_opr[i];
	    TP_INFO("TP_X_SEC_OPR_INDEX 0x%x\n", config->abs.x2_magic[i]);
	}

	if ( ! udc_tp_get_value(TP_Y_OPR_INDEX, data_opr, 2) )
	{	data_opr[0] = 5; data_opr[1] = 4;}
	for(i = 0;i< 2; i++)
	{
		config->abs.y_index[i] = data_opr[i];
	    TP_INFO("TP_Y_OPR_INDEX 0x%x\n", config->abs.y_index[i]);
	}
	
	if ( ! udc_tp_get_value(TP_Y_FIR_OPR, data_opr, 3) )
	{	data_opr[0] = 0xff; data_opr[1] = 8;data_opr[2] = 0;}
	for(i = 0;i< 3; i++)
	{
		config->abs.y1_magic[i] = data_opr[i];
	    TP_INFO("TP_Y_FIR_OPR_INDEX 0x%x\n", config->abs.y1_magic[i]);
	}
	
	if ( ! udc_tp_get_value(TP_Y_SEC_OPR, data_opr, 3) )
	{	data_opr[0] = 0xff; data_opr[1] = 0;data_opr[2] = 0;}
	for(i = 0;i< 3; i++)
	{
		config->abs.y2_magic[i] = data_opr[i];
	    TP_INFO("TP_Y_SEC_OPR_INDEX 0x%x\n", config->abs.y2_magic[i]);
	}
	
	if ( ! udc_tp_get_value(TP_NAGETA, &param, 1) )
		param = 40;
	config->abs.negate = param;
	TP_INFO("TP_NAGETA 0x%x\n", config->abs.negate);

	if ( ! udc_tp_get_value(TP_NEXT_POINT_OFFSET, &param, 1) )
		param = 5;
	config->abs.point_gap = param;
	TP_INFO("TP_NEXT_POINT_OFFSET 0x%x\n", config->abs.point_gap);
	
	if ( ! udc_tp_get_value(TP_DIS_THRESHOLD, &param, 1) )
		param = 30;
	config->abs.point_threshold = param;
	TP_INFO("TP_DIS_THRESHOLD 0x%x\n", config->abs.point_threshold);

	if (udc_tp_get_value(TP_PRESSURE, &param, 1) )
		config->abs.pressure = param;
	else 
	       config->abs.pressure = 200;
	TP_INFO("TP_PRESSURE 0x%x\n", config->abs.pressure);

	if (udc_tp_get_value(TP_WIDTH, &param, 1) )
		config->abs.width= param;
	else 
	       config->abs.width = 1;
	TP_INFO("TP_WIDTH 0x%x\n", config->abs.width);
	
	TP_INFO("%s ok!",__func__);
	return 1;
}

udc_tp* udc_tp_create(void)
{
   	udc_t ret;
    udc_s* udc;
	udc_tp* tp = &g_udc_tp;	

	udc = udc_get_udc();
	tp->udc = udc;
	tp->item = g_udc_tp_item;
	tp->early_init = udc_tp_early_init;
	tp->prv_data = kzalloc(sizeof*tp->prv_data, GFP_KERNEL);
	if (!tp->prv_data)
	{
		TP_ERR("unable to alloc tp->prv_data");
		ret = -ENOMEM;
		goto error_exit;
	}

	TP_INFO("udc_tp_create ok!\n");	
    return 	tp; 
error_exit:
    return ret;
}

void udc_tp_destroy(udc_tp* tp)
{
	struct udc_tp_prv_data *prv_data = tp->prv_data;
	if (prv_data)
	{
		if (prv_data->early_suspend)
		{
			unregister_early_suspend(prv_data->early_suspend);
			kfree(prv_data->early_suspend);
			prv_data->early_suspend = NULL;
		}

		if (prv_data->key_file_info.is_file_create)
		{		
			sysfs_remove_file(&prv_data->key_file_info.file_obj, &prv_data->key_file_info.file_attr.attr);
			kobject_del(&prv_data->key_file_info.file_obj);
		}
		kfree(tp->prv_data);
		tp->prv_data = NULL;
	}
}

int udc_tp_identify(struct _udc_tp* tp,udc_t section_id)
{
	TP_INFO("==%s==", __func__);
	udc_t device_id = 0;
	int i; 
	udc_t param;
	udc_t ret;
	udc_tp_config *config = &tp->config;
	udc_section section;
	udc_section next;
	
	ret  = udc_search_first_section(tp->udc, section_id, &section); 
    if(ret)	
		udc_match_item(tp->udc, &section, tp->item);
	else
		{
		printk("test 1111111  ret ==%d \n",ret);
	   return -1;
		}
	while(1)
	{
		
	      if(udc_tp_get_value(TP_ADDR, &param, 1));
			config->addr = param; 
		TP_INFO("TP_ADDR 0x%x\n", config->addr);
	
		if ( udc_tp_get_value(TP_ID, &param, 1) )
			config->device_id = param;
		TP_INFO("TP_ID 0x%x\n", config->device_id);

		tp->config_operation(tp,config->device_id);
              tp->prv_data->i2c_client->addr = config->addr;
		
		tp->open(tp);
		udc_tp_do(tp, TP_READ_CHIP_ID, &device_id, 1);
		TP_INFO("device_id id(%d)",device_id); 
		if(device_id == config->device_id)
		{
			TP_INFO("indentify OK id = %d\n",device_id); 
			return 1;
		}
		else
		{
			ret = udc_search_next_section(tp->udc,&section,&next);
			if(ret)
			{
				udc_match_item(tp->udc, &next, tp->item);
				section = next;
			}
			else
				return 0;
		}
	}
}

static int  udc_tp_probe(struct i2c_client *i2c_client, const struct i2c_device_id *id)
{
	int error = 0;
	
	int ret,i,flag=0;
	unsigned char device_id;
	struct input_dev *input_dev= NULL;
	udc_tp* tp = udc_tp_get_tp();
	struct udc_tp_prv_data *prv_data 	=tp->prv_data;
	struct _udc_tp_config 	*config = &tp->config;    
	prv_data = i2c_get_clientdata(i2c_client);	
	tp->prv_data->i2c_client 	= i2c_client;
	config->is_init 	=	1;

	TP_INFO("==%s==", __func__);

	if (!i2c_check_functionality(i2c_client->adapter, I2C_FUNC_I2C))
		goto error_exit;

	// setup work struct to handle read data function
	INIT_WORK(&prv_data->event_work,  udc_tp_irq_work);
	prv_data->work_queue = create_singlethread_workqueue(dev_name(&i2c_client->dev));
	if (!prv_data->work_queue)
	{
		error = -ESRCH;
		goto  error_exit;
	}	

	//setup timer to check tp up event
	if (config->time_up)
	{
		init_timer(&prv_data->timer);
		prv_data->timer.function = udc_tp_timer_handle;
		prv_data->timer.data= (unsigned long)prv_data;
		{
			config->time_hz = config->time_up*HZ/1000;
		}
	}

	//setup input device
	input_dev = udc_tp_input_device_init(tp,prv_data);
	if (!input_dev)
	{
		TP_ERR("Failed to allocate input dev!");
		error = -ENOMEM;
		goto error_exit;
	}

	prv_data->input_dev = input_dev;	
	input_set_drvdata(input_dev, prv_data);
	
	error = misc_register(&udc_tp_misc_device);
	if(error<0)
	{
		TP_ERR("udc_tp_misc_device register failed\n");
		error = -ENODEV;
		goto error_exit;
	}
	
#if CONFIG_USE_TP_PSENSOR
    tp_ps_init(i2c_client);
#endif	

	//setup irq 
	i2c_client->irq = udc_get_irq(SEC_TP, 0xffff);
	
	error = request_irq(i2c_client->irq, udc_tp_irq_handler, config->irq_flag, i2c_client->name, prv_data);
	if (error< 0) 
	{
		TP_ERR("alloc irq error!");
		error = -EAGAIN;
		goto error_exit;
	}
	disable_irq_nosync(i2c_client->irq);
	
	//set power operation 
	prv_data->early_suspend			= kzalloc(sizeof*prv_data->early_suspend, GFP_KERNEL);
	prv_data->early_suspend->level 	= EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	prv_data->early_suspend->suspend 	= tp->suspend;
	prv_data->early_suspend->resume	=tp->resume;
	register_early_suspend(prv_data->early_suspend);

	udc_tp_create_sysfs(i2c_client); 

    enable_irq(i2c_client->irq);
	TP_INFO("probe ok");
		
	return 0;
error_exit:
	udc_tp_free(prv_data);
	return error;

}

static int  udc_tp_remove(struct i2c_client *i2c_client)
{
	struct udc_tp_prv_data *prv_data = i2c_get_clientdata(i2c_client);

	TP_INFO("udc_tp_remove.");
	
#ifdef CONFIG_USE_TP_PSENSOR
	tp_ps_uninit();
#endif	
	
	udc_tp_free(prv_data);	
	return 0;
}

/*i2c addr will be change after real device probing*/
static struct i2c_board_info  tp_i2c_boardinfo = 
{
        I2C_BOARD_INFO(UDC_TP_NAME, 0x02),
};

static const struct i2c_device_id udc_tp_id[] = 
{
	{ UDC_TP_NAME, 0x55},
	{ }
};
MODULE_DEVICE_TABLE(i2c,  udc_tp_id);

static struct i2c_driver udc_tp_driver = {
	.driver =
	{
		.owner	= THIS_MODULE,
		.name 	= UDC_TP_NAME,
	},
	.probe = udc_tp_probe,
	.remove = udc_tp_remove,
	.id_table = udc_tp_id,	
};

int add_tp_i2c_device(struct _udc_tp* tp)
{
	TP_INFO("==%s==", __func__);
	udc_t ret = 0;
	struct i2c_client *i2c_client = NULL;
	i2c_client = i2c_static_add_device(tp,&tp_i2c_boardinfo);
	if (NULL == i2c_client)
	{
        TP_ERR("add i2c device error(%d)", ret);
	    ret = -ENODEV;
        return ret;
    }
	tp->prv_data->i2c_client = i2c_client;	
	i2c_set_clientdata(i2c_client, tp->prv_data);
	return 0;
}

int udc_tp_add_driver(struct _udc_tp* tp)
{
	TP_INFO("==%s==", __func__);
	return i2c_add_driver(&udc_tp_driver);
}

void udc_tp_del_driver(struct _udc_tp* tp)
{
	TP_INFO("udc tp remove");
	if (tp)
	{
		if (tp->prv_data->i2c_client)
		{
			i2c_unregister_device(tp->prv_data->i2c_client);
		}

	}
	i2c_del_driver(&udc_tp_driver);
}

udc_tp* udc_tp_get_tp(void)
{
	udc_tp* udc_tp = &g_udc_tp;
	return udc_tp; 
}

void tp_operation_bind(struct _udc_tp* tp,config_tp_op operation)
{
	tp->config_operation = operation;
}

EXPORT_SYMBOL(tp_operation_bind); 
EXPORT_SYMBOL(udc_tp_create);
EXPORT_SYMBOL(udc_tp_destroy);
EXPORT_SYMBOL(udc_tp_do);
EXPORT_SYMBOL(udc_tp_get_tp);
EXPORT_SYMBOL(udc_tp_pwron);
EXPORT_SYMBOL(udc_tp_pwroff);
EXPORT_SYMBOL(udc_tp_default_op);

MODULE_AUTHOR("Huosheng.Wang <huosheng.wang@spreadtrum.com>");
MODULE_DESCRIPTION("Udc_Tp");
MODULE_LICENSE("GPL"); 
