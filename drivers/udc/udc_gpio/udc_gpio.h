#ifndef __UDC_GPIO_H__
#define __UDC_GPIO_H__


#ifdef UDC_GPIO_DEBUG
#define UDC_GPIO_TRACE printk
#else
#define UDC_GPIO_TRACE(...)
#endif


/* sprd gpio */
typedef struct _udc_gpio_info{
	udc_t gpio_num;
	udc_t gpio_level;
	udc_t gpio_direction;
	udc_t gpio_logic;
};

typedef enum _udc_gpio_keyid{
       UDC_GPIO_WIFI_POWER = 0,
	UDC_GPIO_WIFI_RESET         ,
	UDC_GPIO_WIFI_PWD          ,
	UDC_GPIO_WIFI_IRQ            ,

	UDC_GPIO_BT_POWER         ,
	UDC_GPIO_BT_RESET           ,
	UDC_GPIO_BT_RTS              ,
       UDC_GPIO_BT_WAKE_HOST  ,
	UDC_GPIO_BT_WAKE_BT   ,
	
	UDC_GPIO_CMMB_POWER     ,
	UDC_GPIO_CMMB_RESET        ,
	UDC_GPIO_CMMB_IRQ          ,

	UDC_GPIO_TP_RST              ,
	UDC_GPIO_TP_IRQ              ,
	UDC_GPIO_PLS_IRQ             ,
	UDC_GPIO_MINT_IRQ           ,
	UDC_GPIO_GPS_PWR          ,
	UDC_GPIO_GPS_RST            ,
	UDC_GPIO_GPS_ONOFF        ,

	UDC_GPIO_MAIN_CAMERA_PWD,
	UDC_GPIO_SUB_CAMERA_PWD,
	UDC_GPIO_DETECT_HEADSET,
       UDC_GPIO_RESERVED0           ,
	UDC_GPIO_RESERVED1		  ,
	UDC_GPIO_RESERVED2		  ,
	UDC_GPIO_RESERVED3		  ,
	UDC_GPIO_RESERVED4		  ,
	UDC_GPIO_RESERVED5		  ,
	UDC_GPIO_RESERVED6		  ,
	UDC_GPIO_RESERVED7		  ,
	UDC_GPIO_RESERVED8		  ,
	UDC_GPIO_RESERVED9		  ,
	UDC_GPIO_INVALID = 0XFFFF ,
      
}udc_gpio_keyid;


struct udc_gpio_initdata {
    int* gpio;
    int io;
    udc_t key_id;
    udc_t  value_count;
    udc_t*  value;		
	
};

typedef struct _udc_gpio{
	udc_s* udc;
	struct _udc_gpio_info info;
	struct udc_gpio_initdata* gpio_cfg;
	int size;
}udc_gpio;


extern int udc_gpio_wifi_power;
extern int udc_gpio_wifi_reset;
extern int udc_gpio_wifi_pwd;
extern int udc_gpio_wifi_irq;
extern int udc_gpio_bt_power;
extern int udc_gpio_bt_reset;
extern int udc_gpio_bt_rts;
extern int udc_gpio_bt_hostwake;
extern int udc_gpio_bt_extwake;
extern int udc_gpio_cmmb_power;
extern int udc_gpio_cmmb_reset;
extern int udc_gpio_cmmb_irq;
extern int udc_gpio_tp_rst;
extern int udc_gpio_tp_irq;
extern int udc_gpio_pls_irq;
extern int udc_gpio_mint_irq;
extern int udc_gpio_gps_pwr;
extern int udc_gpio_gps_rst;
extern int udc_gpio_gps_onoff;
extern int udc_gpio_main_camera_pwd;
extern int udc_gpio_sub_camera_pwd;
extern int udc_gpio_hp_detect;

extern int udc_gpio_reserved0;
extern int udc_gpio_reserved1;
extern int udc_gpio_reserved2;
extern int udc_gpio_reserved3;
extern int udc_gpio_reserved4;
extern int udc_gpio_reserved5;
extern int udc_gpio_reserved6;
extern int udc_gpio_reserved7;
extern int udc_gpio_reserved8;
extern int udc_gpio_reserved9;


#ifdef CONFIG_UDC_VER_2_3_5
#define sprd_3rdparty_gpio_wifi_power      udc_gpio_wifi_power
#define sprd_3rdparty_gpio_wifi_reset      udc_gpio_wifi_reset
#define sprd_3rdparty_gpio_wifi_pwd        udc_gpio_wifi_pwd
#define sprd_3rdparty_gpio_wifi_irq        udc_gpio_wifi_irq
#define sprd_3rdparty_gpio_bt_power        udc_gpio_bt_power
#define sprd_3rdparty_gpio_bt_reset        udc_gpio_bt_reset
#define sprd_3rdparty_gpio_bt_rts          udc_gpio_bt_rts
#define sprd_3rdparty_gpio_bt_hostwake     udc_gpio_bt_hostwake
#define sprd_3rdparty_gpio_bt_extwake      udc_gpio_bt_extwake
#define sprd_3rdparty_gpio_cmmb_power      udc_gpio_cmmb_power
#define sprd_3rdparty_gpio_cmmb_reset      udc_gpio_cmmb_reset
#define sprd_3rdparty_gpio_cmmb_irq        udc_gpio_cmmb_irq
#define sprd_3rdparty_gpio_tp_rst          udc_gpio_tp_rst
#define sprd_3rdparty_gpio_tp_irq          udc_gpio_tp_irq
#define sprd_3rdparty_gpio_pls_irq         udc_gpio_pls_irq
#define sprd_3rdparty_gpio_mint_irq        udc_gpio_mint_irq
#define sprd_3rdparty_gpio_gps_pwr         udc_gpio_gps_pwr
#define sprd_3rdparty_gpio_gps_rst         udc_gpio_gps_rst
#define sprd_3rdparty_gpio_gps_onoff       udc_gpio_gps_onoff
#define sprd_3rdparty_gpio_main_camera_pwd udc_gpio_main_camera_pwd
#define sprd_3rdparty_gpio_sub_camera_pwd  udc_gpio_sub_camera_pwd
#define sprd_3rdparty_gpio_hp_detect       udc_gpio_hp_detect                      
#define sprd_3rdparty_gpio_reserved0       udc_gpio_reserved0
#define sprd_3rdparty_gpio_reserved1       udc_gpio_reserved1
#define sprd_3rdparty_gpio_reserved2       udc_gpio_reserved2
#define sprd_3rdparty_gpio_reserved3       udc_gpio_reserved3
#define sprd_3rdparty_gpio_reserved4       udc_gpio_reserved4
#define sprd_3rdparty_gpio_reserved5       udc_gpio_reserved5
#define sprd_3rdparty_gpio_reserved6       udc_gpio_reserved6
#define sprd_3rdparty_gpio_reserved7       udc_gpio_reserved7
#define sprd_3rdparty_gpio_reserved8       udc_gpio_reserved8
#define sprd_3rdparty_gpio_reserved9       udc_gpio_reserved9

#endif


udc_gpio* udc_gpio_create( udc_t section_id ,struct udc_gpio_initdata* gpio_cfg, int size);
udc_t udc_gpio_get_value(udc_t key_id, udc_t* value, udc_t value_count);
int udc_gpio_config_function(struct _udc_gpio* udc_gpio);
void udc_get_gpio_cfg(struct udc_gpio_initdata** desc, int* size);
void udc_gpio_init(void);

#endif


