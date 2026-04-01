#ifndef __UDC_TP_H__
#define __UDC_TP_H__

#define  TP_DEBUG

#ifdef TP_DEBUG
#define TP_ERR(format, ...)	printk(KERN_ERR "TP_TRACE " format "\n", ## __VA_ARGS__)
#define TP_INFO(format, ...)	printk(KERN_INFO "TP_TRACE " format "\n", ## __VA_ARGS__)
#define TP_WARM(format, ...)	printk(KERN_WARNING "TP_TRACE " format "\n", ## __VA_ARGS__)
#define TP_NOTICE(format, ...)	printk(KERN_NOTICE "TP_TRACE " format "\n", ## __VA_ARGS__)

#else
#define TP_ERR(format, ...)	
#define TP_INFO(format, ...)
#define TP_WARM(format, ...)	
#define TP_NOTICE(format, ...)
#endif

#define UDC_TP_NAME 			"udc_tp"
#define UDC_TP_BOARD_INFO_NAME 	"board_properties"
#define MULTI_TP_VIRUALKEY_FILE_NAME 	"virtualkeys"

#define UDC_TP_VIRTUAL_KEY_NAME_SIZE		100 
#define UDC_TP_VIRTAUL_KEY_FILE_SIZE		200
#define UDC_TP_MAX_FINGER_NUM			5
#define UDC_TP_MAX_KEY_NUM            10

#define	UDC_TP_IOCTL_BASE 88
/** The following define the IOCTL command values via the ioctl macros */
#define	UDC_TP_IOCTL_I2C_WRITE_DATA		_IOW(UDC_TP_IOCTL_BASE, 0, int)
#define	UDC_TP_IOCTL_I2C_READ_DATA		_IOW(UDC_TP_IOCTL_BASE, 1, int)
#define	UDC_TP_IOCTL_I2C_CALIBRATE		_IOW(UDC_TP_IOCTL_BASE, 2, int)
#define	UDC_TP_IOCTL_I2C_UPDATE		    _IOW(UDC_TP_IOCTL_BASE, 3, int)

typedef enum _udc_tp_key_id{
	TP_FIRMWARE_NAME    = 0  ,
	TP_ADDR              ,
	TP_PANEL_RAW_W_H     ,
	TP_PANEL_REL_W_H    ,
	TP_ID                ,
	TP_IRQFLAG           ,
	TP_TIME_UP           ,
	TP_FINGERNUM         ,
	TP_KEYTYPE           ,
	TP_KEYNUM            ,
	TP_KEYCODE           ,
	//for virtal key
	TP_KEY_MID_X         ,
	TP_KEY_MID_Y         ,
	TP_KEY_WIDTH         ,
	TP_KEY_HEIGHT        ,
	TP_FINGER_NUM_INDEX  ,
	TP_FINGER_NUM_OPR    ,
	TP_KEY_VALUE_INDEX    ,
	TP_KEY_VALUE_OPR      ,
	TP_X_OFFSET          ,
	TP_Y_OFFSET          ,
	TP_OFFSET_CACULATE_TYPE,//0：+，1：-
	TP_X_OPR_INDEX       ,//两个值。第一个为第一个操作数的索引，第二个为第二个操作数的索引
	TP_X_FIR_OPR         ,//三个值。第一个为相与的数，第二个为左移位数，第三个为右移的位数
	TP_X_SEC_OPR         ,//三个值。第一个为相与的数，第二个为左移位数，第三个为右移的位数
	TP_Y_OPR_INDEX       ,//两个值。第一个为第一个操作数的索引，第二个为第二个操作数的索引
	TP_Y_FIR_OPR         ,//三个值。第一个为相与的数，第二个为左移位数，第三个为右移的位数
	TP_Y_SEC_OPR         ,//三个值。第一个为相与的数，第二个为左移位数，第三个为右移的位数
	TP_NAGETA            ,//非0表示需要取反
	TP_DIS_THRESHOLD     ,

	TP_NEXT_POINT_OFFSET ,
	TP_PRESSURE          ,
	TP_WIDTH             ,
	TP_RESET             ,
	TP_READ_CHIP_ID      ,
	TP_READ_FINGER_NUM   ,
	TP_READ_KEY_VALUE    ,
	TP_READ_POINT        ,
	TP_OPEN              ,
	TP_SUSPEND           ,
	TP_RESUME            ,
	TP_CALIBRATE         ,
	TP_END_CMD           ,
#ifdef CONFIG_USE_TP_PSENSOR
	TP_PSENSOR_ENABLE    ,
	TP_PSENSOR_DISABLE   ,
	TP_PSENSOR_GET_DATA  ,
#endif
	
	TP_INVALID = 0XFFFF  ,
      
}udc_tp_key_id;


typedef enum _udc_tp_event_type{
	TP_EVENT_TYPE_ABS,
	TP_EVENT_TYPE_KEY,
	TP_EVENT_TYPE_MAX,

}udc_tp_event_type;


struct tp_event 
{	int type;//0:ABS_event, 1:KEY_event
	int x[UDC_TP_MAX_FINGER_NUM];
	int y[UDC_TP_MAX_FINGER_NUM];
       int point_count;
	int key_count;   
	int key_event[UDC_TP_MAX_KEY_NUM];	
};

typedef struct  _udc_tp_abs{
    unsigned short max_point;
    unsigned short point_index;
    unsigned short point_gap;
	unsigned short point_magic[3];
	unsigned short offset_x;
	unsigned short offset_y;
	unsigned short offset_direction;
	unsigned short x_index[2];
	unsigned short x1_magic[3];
	unsigned short x2_magic[3];
	unsigned short y_index[2];
	unsigned short y1_magic[3];
	unsigned short y2_magic[3];
	unsigned short negate;
	unsigned short point_threshold;
	udc_t pressure;
	udc_t width;
}udc_tp_abs;

typedef struct _udc_tp_virtual_key{
	udc_t x[UDC_TP_MAX_KEY_NUM];
	udc_t y[UDC_TP_MAX_KEY_NUM];
	udc_t w[UDC_TP_MAX_KEY_NUM];
	udc_t h[UDC_TP_MAX_KEY_NUM];
}udc_tp_virtual_key;


typedef struct _udc_tp_key{
	udc_t type;
	udc_t count;
	udc_t code[UDC_TP_MAX_KEY_NUM];
	udc_tp_virtual_key virtual;
       udc_t status_index;
	udc_t status_mask;
	
}udc_tp_key;

	
 
typedef struct _udc_tp_config
{
	udc_t  				i2c_bus_num; 
	char 				*firmware_name;		// check firmware file
	unsigned long	    irq_flag;			 
	unsigned short 		addr;          //slave address
       udc_t         panel_raw_w;   
	udc_t         panel_raw_h; 
	udc_t         panel_rel_w;
	udc_t         panel_rel_h;
	unsigned short		time_up;	   //unit ms ....some devices need to report tp up after sample
	unsigned short 		time_hz;	
	unsigned char 	    device_id;           // tp device_id
	int				     is_init;
	udc_tp_abs abs;
	udc_tp_key key;
}udc_tp_config;

struct key_attr
{
	struct attribute attr;
	ssize_t (*show) (struct key_file_info	*p_key_file_info, char *buf);
};

struct  key_file_info
{
	char file_name[UDC_TP_VIRTUAL_KEY_NAME_SIZE];
	char file_buf[UDC_TP_VIRTAUL_KEY_FILE_SIZE];
	struct key_attr		file_attr;
	struct kobject      file_obj;
	int					is_file_create;
};

typedef struct udc_tp_prv_data
{
	struct i2c_client 		*i2c_client;	
	struct input_dev 		*input_dev;
	struct work_struct 		event_work;
	struct workqueue_struct 	*work_queue;
	struct timer_list		timer;
	struct early_suspend		*early_suspend;
	struct tp_event			event;
	struct key_file_info		key_file_info;
}prv_data;

typedef struct _udc_tp{
	udc_s* udc;
	udc_item_s* item;
	udc_tp_config config;
	struct udc_tp_prv_data *prv_data;
	void (*early_init)(struct _udc_tp* tp);
	int (*late_init)(struct _udc_tp* tp);
	void (*config_operation)(struct _udc_tp* tp,unsigned char device_id);
	int (*identify)(struct _udc_tp* tp,udc_t section_id);
	int (*add_driver)(struct _udc_tp* tp);
	void (*del_driver)(struct _udc_tp* tp);
	void (*open)(struct _udc_tp* tp);
	void (*reset)(struct _udc_tp* tp);
	void (*suspend)(struct early_suspend *handler);
	void (*resume)(struct early_suspend *handler);
	int (*caculate_tp_data)(struct _udc_tp* tp,struct tp_event *tp_event,u8 *point_count_buf,u8 *key_value_buf,u8 *abs_buffer);

#ifdef CONFIG_USE_TP_PSENSOR
	int(*caculate_psensor_data)(struct _udc_tp* tp,u8 *buf,u8 *dps_data);
#endif
}udc_tp;

typedef void (*config_tp_op)(struct _udc_tp* tp,unsigned char device_id);
extern void tp_operation_bind(struct _udc_tp* tp,config_tp_op operation);

#endif 

extern void udc_tp_pwron(struct _udc_tp* tp);
extern void udc_tp_pwroff(struct _udc_tp* tp);
extern udc_tp* udc_tp_create(void);
extern void udc_tp_destroy(udc_tp* tp);
extern udc_tp* udc_tp_get_tp(void);
extern int udc_tp_do(udc_tp *tp , udc_t key_id, udc_t * reg_values, udc_t  reg_values_count);
extern void udc_tp_default_op(struct _udc_tp* tp,unsigned char device_id);




