#ifndef __UDC_BASE_H__
#define __UDC_BASE_H__




#include <sprd_panel.h>
#include <linux/backlight.h>
#include <drm/drm_atomic_helper.h>
#include <linux/pm_runtime.h>
#include <video/mipi_display.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include "sprd_dpu.h"
#include "sprd_panel.h"
#include "dsi/sprd_dsi_api.h"
#include "sysfs/sysfs_display.h"


#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>




typedef unsigned char           udc8_t;  /* 8 bits */
typedef unsigned short int      udc_t;  /* 16 bits */
typedef unsigned long int       udc32_t;  /* 32 bits */

#define  UDC_MEM_SIZE			0x00010000 /* max 64k */
//#define UDC_MEM_ADDR	 		0x04BF0000

#ifdef CONFIG_UDC_VER_2_3_5
#define UDC_MEM_ADDR	 		0x0f000000
#elif defined CONFIG_UDC_VER_5_1_3
#define UDC_MEM_ADDR	 		0x8e000000 // 512M - 16M  0x8e000000
#else
#define UDC_MEM_ADDR	 		0x0e000000 // 256M - 16M  0x0e000000
#endif

#define UDC_BLOCK_FLAG                 0x1111
#define UDC_FUNC_FLAG                  0x2222
#define UDC_PAGE_FLAG                  0x3333
#define UDC_WAIT_FLAG                  0x5555
#define UDC_READ_STATUS_FLAG   0x6666
#define UDC_READ_FLAG                  0x7777
#define UDC_MULTI_READ_FLAG      0x8888
#define UDC_MULTI_WRITE_FLAG    0x9999
#define UDC_DELAY_FLAG                0xffff
#define UDC_UNDEFINED                  0xffff


#define UDC_I2C_REG_16BIT           16
#define UDC_I2C_REG_8BIT             8
#define UDC_I2C_VALUE_16BIT       16
#define UDC_I2C_VALUE_8BIT         8
#define UDC_I2C_VALUE_NULL         0Xffff


#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif



typedef enum _udc_section_id_e{
	SEC_PINMAP      = 0x0100,
	SEC_GPIO		    = 0x0200,
	SEC_BOARD		    = 0x0300,
	SEC_BACKLIGHT   = 0x0400,
	SEC_LCD0		    = 0x0500,
	SEC_LCD1		    = 0x0600,
	SEC_TP			    = 0x0700,
	SEC_GSENSOR     = 0x0800,
	SEC_MSENSOR     = 0x0900,
	SEC_LSENSOR     = 0x0a00,
	SEC_PSENSOR     = 0x0b00,
	SEC_FM			    = 0x0c00,
	SEC_MAIN_CAMERA = 0x0d00,
	SEC_SUB_CAMERA  = 0x0e00,
	SEC_ATV         = 0x0f00,

SEC_INVALID = 0XFFFF ,
      
}udc_section_id_e;

typedef struct _udc_section{
	udc_t id;
	udc_t size;
	udc_t*buffer;
}udc_section;


typedef struct _udc_item_s{
	udc_t key_id;
	udc_t value_count;
	udc_t* value;
}udc_item_s;

typedef struct _udc_i2c_info{
	struct i2c_client 		*i2c_client;
	udc_t      reg_addr_bits;   
	udc_t      reg_value_bits; 
	udc_t* 	   value;
	udc_t      value_count;
	udc_t      mask_bits;	
}udc_i2c_info;

typedef struct _udc_s{
	udc_t	*buffer;               
	udc32_t			 length; 
    unsigned char	*data_buffer;               
	udc32_t	data_buffer_length; 
	struct _udc_board* board;
}udc_s;


extern udc_s* udc_create ( void );
extern udc_s* udc_get_udc (void );
extern udc_t udc_get_item_value(udc_item_s*item, udc_t key_id, udc_t* value, udc_t max_count);
extern udc_t udc_search_first_section(udc_s*udc,  udc_t section_id,  udc_section* section);
extern udc_t udc_search_next_section(udc_s*udc,  udc_section* current_section , udc_section* next);
extern udc_t udc_search_value(udc_s*udc, udc_section* section,  udc_t key_id, udc_item_s* item );
extern udc_t udc_match_item(udc_s*udc, udc_section* section,  udc_item_s* item );
//extern int udc_adi_write_mask(udc32_t reg, udc32_t value, udc32_t mask);
#ifdef CONFIG_UDC_GPIO
extern int udc_get_irq(udc_t section_id, udc_t eic_no);
extern int udc_put_irq(int irq, udc_t eic_no);
#endif
extern udc32_t udc_get_lcd_offset(void);

typedef enum _udc_board_keyid{
	BOARD_FM_LDO             ,
	BOARD_FM_LDO_LEVEL       ,
	BOARD_FM_I2C_BUS_NUM 	 ,
	BOARD_GSENSOR_LDO        ,
	BOARD_GSENSOR_LDO_LEVEL  ,
	BOARD_GSENSOR_I2C_BUS_NUM,
	BOARD_MSENSOR_LDO        ,
	BOARD_MSENSOR_LDO_LEVEL  ,
	BOARD_MSENSOR_I2C_BUS_NUM,
	BOARD_LSENSOR_LDO        ,
	BOARD_LSENSOR_LDO_LEVEL  ,
	BOARD_LSENSOR_I2C_BUS_NUM,
	BOARD_PSENSOR_LDO        ,
	BOARD_PSENSOR_LDO_LEVEL  ,
	BOARD_PSENSOR_I2C_BUS_NUM,
	BOARD_TP_LDO             ,
	BOARD_TP_LDO_LEVEL       ,
	BOARD_TP_I2C_BUS_NUM 	 ,
	BOARD_LCD_CS				,
	BOARD_BACKLIGHT_IC_GPIO   ,
	BOARD_CAMERA_IDENTIFY_FLAG ,
	BOARD_INVALID = 0xffff		,
      
}udc_board_keyid;

typedef struct _udc_board_config {
	udc_t fm_ldo               ;	
	udc_t fm_ldo_level         ;	
	udc_t fm_i2c_bus_num  ;
	udc_t gsensor_ldo          ; 
	udc_t gsensor_ldo_level    ; 
	udc_t gsensor_i2c_bus_num  ; 
	udc_t msensor_ldo          ; 
	udc_t msensor_ldo_level    ; 
	udc_t msensor_i2c_bus_num  ; 
	udc_t lsensor_ldo          ; 
	udc_t lsensor_ldo_level    ; 
	udc_t lsensor_i2c_bus_num  ;  
	udc_t psensor_ldo          ; 
	udc_t psensor_ldo_level    ; 
	udc_t psensor_i2c_bus_num  ; 
	udc_t tp_ldo               ;	
	udc_t tp_ldo_level         ;	
	udc_t tp_i2c_bus_num       ;
	udc_t lcd_cs			;
	udc_t backlight_ic_gpio	;
	udc_t camera_identify_flag;
}udc_board_config;



typedef struct _udc_board{
	udc_s* udc;
	udc_item_s* item;
	udc_board_config config;
}udc_board;


//#define  UDC_DEBUG 1


#ifdef UDC_DEBUG
#define UDC_TRACE printk
#else
#define UDC_TRACE(...)
#endif


#endif

 
