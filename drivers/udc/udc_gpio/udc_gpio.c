/* drivers/video/sc8800/udc_gpio.c
 *
 *
 * Copyright (C) 2010 Spreadtrum
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/udc.h>

#define GPIO_DEFAUT_HIGH		(1 << 31)
#define GPIO_DEFAUT_LOW			(0 << 31)
#define GPIO_DIRECTION_OUTPUT	(1 << 30)
#define GPIO_DIRECTION_INPUT		(1 << 29)
#define GPIO_LOGIC_TRUE			(1<< 28)
#define GPIO_LOGIC_FALSE			(0 << 28)


#define GPIO_INDEX_MAX			(0xffff)

int udc_gpio_wifi_power;
int udc_gpio_wifi_reset;
int udc_gpio_wifi_pwd;
int udc_gpio_wifi_irq;

int udc_gpio_bt_power;
int udc_gpio_bt_reset;
int udc_gpio_bt_rts;
int udc_gpio_bt_hostwake;
int udc_gpio_bt_extwake;

int udc_gpio_cmmb_power;
int udc_gpio_cmmb_reset;
int udc_gpio_cmmb_irq;

int udc_gpio_tp_rst;
int udc_gpio_tp_irq;

int udc_gpio_pls_irq;
int udc_gpio_mint_irq;

int udc_gpio_gps_pwr;
int udc_gpio_gps_rst;
int udc_gpio_gps_onoff;

int udc_gpio_main_camera_pwd;
int udc_gpio_sub_camera_pwd;
int udc_gpio_hp_detect;

int udc_gpio_reserved0;
int udc_gpio_reserved1;
int udc_gpio_reserved2;
int udc_gpio_reserved3;
int udc_gpio_reserved4;
int udc_gpio_reserved5;
int udc_gpio_reserved6;
int udc_gpio_reserved7;
int udc_gpio_reserved8;
int udc_gpio_reserved9;




struct udc_gpio_initdata udc_gpio_func_cfg[] = {
	
	{&udc_gpio_wifi_power,			  -1, 		UDC_GPIO_WIFI_POWER, 				0, 		0},
	{&udc_gpio_wifi_reset,			  -1, 		UDC_GPIO_WIFI_RESET, 				0, 		0},
	{&udc_gpio_wifi_pwd,				  -1, 	UDC_GPIO_WIFI_PWD, 				0, 		0},
	{&udc_gpio_wifi_irq,				  -1, 		UDC_GPIO_WIFI_IRQ, 		      			0, 		0},
	{&udc_gpio_bt_power,				  -1, 	UDC_GPIO_BT_POWER, 				0, 		0},
	{&udc_gpio_bt_reset,				  -1, 	UDC_GPIO_BT_RESET, 		      			0, 		0},
	{&udc_gpio_bt_rts,					  -1, 	UDC_GPIO_BT_RTS, 		       		0, 		0},
       {&udc_gpio_bt_hostwake,	        -1,  			UDC_GPIO_BT_WAKE_HOST, 				0, 		0},
	{&udc_gpio_bt_extwake,	        -1,  			UDC_GPIO_BT_WAKE_BT, 				0, 		0},
	
	{&udc_gpio_cmmb_power,			  -1, 	UDC_GPIO_CMMB_POWER, 				0, 		0},
	{&udc_gpio_cmmb_reset,			  -1, 	UDC_GPIO_CMMB_RESET, 				0, 		0},
	{&udc_gpio_cmmb_irq,				  -1, 	UDC_GPIO_CMMB_IRQ, 				0, 		0},	
	{&udc_gpio_tp_rst,					  -1,   	UDC_GPIO_TP_RST, 		       		0, 		0},
	{&udc_gpio_tp_irq,					  -1, 	UDC_GPIO_TP_IRQ, 		       		0, 		0},
	{&udc_gpio_pls_irq,					  -1, 	UDC_GPIO_PLS_IRQ , 		      			0, 		0},
	{&udc_gpio_mint_irq,				  -1, 	UDC_GPIO_MINT_IRQ, 				0, 		0},
	{&udc_gpio_gps_pwr,					  -1, UDC_GPIO_GPS_PWR, 		       		0, 		0},
	{&udc_gpio_gps_rst,					  -1, UDC_GPIO_GPS_RST, 		       		0, 		0},
	{&udc_gpio_gps_onoff,				  -1, 	UDC_GPIO_GPS_ONOFF, 		      		0, 		0},
	{&udc_gpio_main_camera_pwd,	  -1, 		UDC_GPIO_MAIN_CAMERA_PWD, 		0, 		0},
	{&udc_gpio_sub_camera_pwd,	  -1, 		UDC_GPIO_SUB_CAMERA_PWD, 			0, 		0},
       {&udc_gpio_hp_detect,	        -1,  			UDC_GPIO_DETECT_HEADSET, 				0, 		0},

       {&udc_gpio_reserved0,	        -1,  			UDC_GPIO_RESERVED0, 				0, 		0},
	{&udc_gpio_reserved1,	        -1,   			UDC_GPIO_RESERVED1, 				0, 		0},
	{&udc_gpio_reserved2,	        -1,  			UDC_GPIO_RESERVED2, 				0, 		0},
	{&udc_gpio_reserved3,	        -1,   			UDC_GPIO_RESERVED3, 				0, 		0},
	{&udc_gpio_reserved4,	        -1,  			UDC_GPIO_RESERVED4, 				0, 		0},
	{&udc_gpio_reserved5,	        -1,   			UDC_GPIO_RESERVED5, 				0, 		0},
	{&udc_gpio_reserved6,	        -1,  			UDC_GPIO_RESERVED6, 				0, 		0},
	{&udc_gpio_reserved7,	        -1,  			UDC_GPIO_RESERVED7, 				0, 		0},
	{&udc_gpio_reserved8,	        -1,  			UDC_GPIO_RESERVED8, 				0, 		0},
	{&udc_gpio_reserved9,	        -1,  			UDC_GPIO_RESERVED9, 				0, 		0},
	{NULL						,	  	   	  -1, 		0xffff, 	0xffff, 		0},

};


void  udc_get_gpio_cfg(struct udc_gpio_initdata** desc, int* size)
{
 
    UDC_TRACE("%s\n", __func__);
    *desc = udc_gpio_func_cfg;
    *size = ARRAY_SIZE(udc_gpio_func_cfg);

    udc_gpio_create(SEC_GPIO, udc_gpio_func_cfg,ARRAY_SIZE(udc_gpio_func_cfg));

}


EXPORT_SYMBOL_GPL(udc_gpio_wifi_power);
EXPORT_SYMBOL_GPL(udc_gpio_wifi_reset);
EXPORT_SYMBOL_GPL(udc_gpio_wifi_pwd);
EXPORT_SYMBOL_GPL(udc_gpio_wifi_irq);
EXPORT_SYMBOL_GPL(udc_gpio_bt_power);
EXPORT_SYMBOL_GPL(udc_gpio_bt_reset);
EXPORT_SYMBOL_GPL(udc_gpio_bt_rts);
EXPORT_SYMBOL_GPL(udc_gpio_bt_hostwake);
EXPORT_SYMBOL_GPL(udc_gpio_bt_extwake);
EXPORT_SYMBOL_GPL(udc_gpio_tp_rst);
EXPORT_SYMBOL_GPL(udc_gpio_tp_irq);
EXPORT_SYMBOL_GPL(udc_gpio_cmmb_power);
EXPORT_SYMBOL_GPL(udc_gpio_cmmb_reset);
EXPORT_SYMBOL_GPL(udc_gpio_cmmb_irq);
EXPORT_SYMBOL_GPL(udc_gpio_pls_irq);
EXPORT_SYMBOL_GPL(udc_gpio_mint_irq);
EXPORT_SYMBOL_GPL(udc_gpio_gps_pwr);
EXPORT_SYMBOL_GPL(udc_gpio_gps_rst);
EXPORT_SYMBOL_GPL(udc_gpio_gps_onoff);
EXPORT_SYMBOL_GPL(udc_gpio_main_camera_pwd);
EXPORT_SYMBOL_GPL(udc_gpio_sub_camera_pwd);
EXPORT_SYMBOL_GPL(udc_gpio_hp_detect);
EXPORT_SYMBOL_GPL(udc_gpio_reserved0);
EXPORT_SYMBOL_GPL(udc_gpio_reserved1);
EXPORT_SYMBOL_GPL(udc_gpio_reserved2);
EXPORT_SYMBOL_GPL(udc_gpio_reserved4);
EXPORT_SYMBOL_GPL(udc_gpio_reserved5);
EXPORT_SYMBOL_GPL(udc_gpio_reserved6);
EXPORT_SYMBOL_GPL(udc_gpio_reserved7);
EXPORT_SYMBOL_GPL(udc_gpio_reserved8);
EXPORT_SYMBOL_GPL(udc_gpio_reserved9);





udc_gpio g_udc_gpio;



__init void udc_gpio_init(void)
{
     struct udc_gpio_initdata *udc_gpio_cfg, *udc_gd;
	 int udc_size;
	 int    gpio_io;
	 int i,size;
	 int gpio, value,logic;
	 
	 
	udc_get_gpio_cfg(&udc_gpio_cfg,&udc_size);
	
	for (i = 0; (i < udc_size)&&(udc_gpio_cfg[i].key_id!=0xffff); i++) {

		udc_gd = &udc_gpio_cfg[i]; 
     		gpio_io = udc_gd->io;

	      //UDC_GPIO_TRACE("%s : udc_gpio_cfg[%d], gpio num = %d \n", __func__, i, gpio_io);

		 gpio = gpio_io & GPIO_INDEX_MAX;

		 if(gpio_io == -1) {
		 	continue;
		 }

		*(udc_gd->gpio) = gpio;
		value = !!(gpio_io & GPIO_DEFAUT_HIGH);
		logic =  !!(gpio_io & GPIO_LOGIC_TRUE);
		
		UDC_GPIO_TRACE("%s : udc_gpio_cfg[%d], gpio num = %d , gpio level 0x%x   , gpio output 0x%x , gpio input 0x%x   ,  gpio logic 0x%x   \n", __func__,i,gpio,value,gpio_io & GPIO_DIRECTION_OUTPUT,gpio_io & GPIO_DIRECTION_INPUT,logic);



		gpio_request(gpio, 	NULL);
			
		
		/*GPIO's direction no longer depends on pin's sleep status.*/
		if (gpio_io & GPIO_DIRECTION_OUTPUT) {
		   gpio_direction_output(gpio, value);
		   
		} else if (gpio_io & GPIO_DIRECTION_INPUT) {
		    gpio_direction_input(gpio);
		} else {
		   printk(KERN_WARNING "%s : not supported gpio direction!\n", __func__);
		}
		gpio_free(gpio_io);
		//sprd_gpio_logic[gpio] = (u8)logic;
	}


}



int udc_gpio_config_function(struct _udc_gpio* udc_gpio)
{
	udc_t* value;
	udc_t i,ret;
	struct _udc_gpio_info udc_gpio_info;
	int    gpio_io;

	UDC_TRACE("%s\n", __func__);

	   for ( i = 0 ; i <  udc_gpio->size; i++ )
	   {
               if ( UDC_GPIO_INVALID  !=  udc_gpio->gpio_cfg[i].key_id )
               {

			ret = udc_gpio_get_value(udc_gpio->gpio_cfg[i].key_id,  &udc_gpio_info, 4);
					
                     if  ( 0 == ret )
			{
                           gpio_io = -1;
		      	       continue;
			}
			else
			{

				  UDC_TRACE("udc_gpio_cfg[%x], gpio num 0x%x , gpio level 0x%x   , gpio direction 0x%x   ,  gpio logic 0x%x   \n", i,udc_gpio_info.gpio_num,udc_gpio_info.gpio_level,udc_gpio_info.gpio_direction,udc_gpio_info.gpio_logic);

				  gpio_io = (int)udc_gpio_info.gpio_num;

				  if ( udc_gpio_info.gpio_level == 1 )
				       gpio_io |= GPIO_DEFAUT_HIGH;
				  else
				       gpio_io |=  GPIO_DEFAUT_LOW;

				  if  ( udc_gpio_info.gpio_direction == 1 )
				        gpio_io |=GPIO_DIRECTION_OUTPUT;
				  else
				        gpio_io |=GPIO_DIRECTION_INPUT;

				  if ( udc_gpio_info.gpio_logic == 1 )
				         gpio_io |= GPIO_LOGIC_TRUE;
				  else
				         gpio_io |=GPIO_LOGIC_FALSE;	   				  
                             				  
			}
			
                    udc_gpio->gpio_cfg[i].io= gpio_io;
					
		 }	  
    
	   }
        
	return 1;

gpio_config_load_err:
	return 0;
}

udc_t udc_gpio_get_value(udc_t key_id, udc_t* value, udc_t value_count)
{
	//udc_gpio* gpio = &g_udc_gpio;
	
       //return udc_get_item_value(gpio->item, key_id, value, value_count);


	udc_t i = 0;
	udc_t min_count;
	udc_gpio* udc_io = &g_udc_gpio;
	
       udc_t count = udc_io->gpio_cfg[key_id].value_count;

       min_count = MIN(value_count, count);
		   
       if ( min_count <= 0 )
	{   	
		return 0;
       }
	   
       for ( i = 0; i < min_count; i++ )
       {
              value[i] = udc_io->gpio_cfg[key_id].value[i];
       } 
	   
	return min_count;

	
	
}


udc_gpio* udc_gpio_create( udc_t section_id ,struct udc_gpio_initdata* gpio_config, int size)
{
       udc_t ret = 0;
	udc_section section;   
       udc_s* udc;
       udc_item_s find_item;
	struct udc_gpio_initdata* item;
	udc_gpio* udc_io = &g_udc_gpio;
	
	UDC_TRACE("%s\n", __func__);
	
      	udc = udc_get_udc();   
       udc_search_first_section(udc, section_id, &section);
	   
	udc_io->udc = udc;
	udc_io->gpio_cfg = gpio_config;
	udc_io->size = size;
       item = udc_io->gpio_cfg;
	  
     	while( item && 0xffff != item->key_id )
       {

	       ret = udc_search_value(udc, &section,  item->key_id, &find_item);

		if ( ret > 0 ) 
		{
		      item->value = find_item.value;
			  
			item->value_count = find_item.value_count;
       	}
		item++;
		
	}
	udc_gpio_config_function(udc_io);
	return 	udc_io;
}



