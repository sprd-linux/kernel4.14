#ifndef __UDC_LCD_H__
#define __UDC_LCD_H__

//#include <../udc_base.h>

#define UDC_LCD_DEBUG 1



#ifdef UDC_LCD_DEBUG
#define UDC_LCD_TRACE printk
#else
#define UDC_LCD_TRACE(...)
#endif

#define UDC_LCD_SEND_CMD 0x8000
#define UDC_LCD_SEND_DAT  0x9000
#define UDC_LCD_DELAY_MS 0xa000
#define UDC_LCD_READ_DAT 0xb000
#define UDC_LCD_RETURN_ID 0xc000
#define UDC_LCD_POWER_ON 0xd000
#define UDC_LCD_POWER_OFF 0xe000

#define UDC_LCD_DIR_NONE   0x0000
#define UDC_LCD_DIR_LEFT   0x0100
#define UDC_LCD_DIR_TOP   0x0200
#define UDC_LCD_DIR_RIGHT   0x0300
#define UDC_LCD_DIR_BOTTOM   0x0400


#define UDC_LCD_MIPI_FORCE_WRITE  0x9100
#define UDC_LCD_MIPI_GEN_WRITE  0x9200
#define UDC_LCD_MIPI_FORCE_READ 0xb100
#define UDC_LCD_MIPI_GEN_READ 0xb200
#define UDC_LCD_MIPI_EOTP_SET 0xc100
#define UDC_LCD_MIPI_SET_LP_MODE 0xc200
#define UDC_LCD_MIPI_SET_DATA_LP_MODE 0xc300
#define UDC_LCD_MIPI_SET_HS_MODE 0xc400
#define UDC_LCD_MIPI_SET_DATA_HS_MODE 0xc500
#define UDC_LCD_MIPI_SET_CMD_MODE 0xc600



typedef enum _udc_lcd_keyid{
	LCD_FMARK         ,
	LCD_NAME         ,
	LCD_ID                  ,
	LCD_WIDTH            ,
	LCD_HIGHT            ,
	LCD_MODE                ,
	LCD_DIRECTION        ,
	LCD_BUS_MODE         ,
	LCD_BUS_WIDTH        ,
	LCD_TIMING0          ,
	LCD_TIMING1          ,
	LCD_READ_ID          ,
	LCD_INIT_PARA        ,
	LCD_SET_WINDOW       ,
	LCD_INVALIDATE_RECT  ,
	LCD_DIR_NORMAL       ,
	LCD_DIR_ROT90        ,
	LCD_DIR_ROT180       ,
	LCD_DIR_ROT270       ,
	LCD_DIR_MIRH         ,
	LCD_DIR_MIRV         ,
	LCD_DIR_MIRHV        ,
	LCD_ENTER_SLEEP      ,
	LCD_EXIST_SLEEP      ,
	
	LCD_WORK_MODE        ,
	LCD_LAN_NUM          ,
	LCD_PHY_FEQ          ,
	LCD_H_SYNC_POL       ,
	LCD_V_SYNC_POL       ,
	LCD_DE_POL           ,
	LCD_TE_POL           ,
	LCD_COLOR_MODE_POL   ,
	LCD_SHUT_DOWN_POL    ,
	LCD_POWER_MODE       ,
	LCD_READ_POWERMODE   ,
	LCD_SPEED_MODE   ,
	LCD_FPS          ,
	LCD_SUSPEND_MODE,
	LCD_DSC_COMPRESSION ,
	LCD_POWER_ON_SEQUENCE ,
	LCD_POWER_OFF_SEQUENCE ,
	LCD_WIDTH_MM,
	LCD_HEIGHT_MM,
	LCD_SIMU_WIDTH,
	LCD_SIMU_HEIGHT,
	LCD_BURST_MODE,
	LCD_PIXEL_CLK,
	LCD_SUPPLY_VOLTAGE,
	LCD_PHY_ESCAPE_CLOCK,
	CONFIG_MAX_ID,
	LCD_INVALID = 0XFFFF ,
      
}udc_lcd_keyid;

typedef struct sprd_panel    udc_sprd_panel;
typedef struct panel_info  udc_panel_info;
//typedef struct panel_ops 	 udc_panel_ops;


typedef struct _udc_lcd{
	udc_s* udc;
	udc_item_s* item;
	udc_t lcd_fmark;
	udc_t lcd_cs;
	udc_t reserved0;
	udc_t current_lcd_offset;
	udc_section current_section;
	uint32_t power_mode;
	udc_sprd_panel *panel;

}udc_lcd;

#define UDC_LCM_ID 0x5555

#define udc_lcd_writel lcdc_write
#define udc_lcd_readl  lcdc_read

extern udc_lcd* udc_lcd_create(udc_t section_id, udc_sprd_panel  *panel);
extern udc_t udc_lcd_get_value(udc_t key_id, udc_t* value, udc_t value_count);
extern udc_lcd* udc_get_lcd(udc_t section_id);
extern int udc_lcd_config_panel(udc_lcd* lcd, udc_t section_id);


#endif


