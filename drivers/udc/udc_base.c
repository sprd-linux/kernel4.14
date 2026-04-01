/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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

#include "udc_base.h"


udc_s g_udc;

#ifdef CONFIG_UDC_GPIO
static int udc_get_irq_gpio(udc_t section_id)
{ 
	int gpio_num = 0;
	
	switch(section_id)
       {
       case SEC_TP:
	   	gpio_num = udc_gpio_tp_irq;

	       break;

	case SEC_GSENSOR:
	   	break;
	   
      case SEC_MSENSOR:
	  	gpio_num = udc_gpio_mint_irq;
	   	break;

      case SEC_PSENSOR:
	  	gpio_num = udc_gpio_pls_irq;
	   	break;

      case SEC_LSENSOR:
               gpio_num = udc_gpio_pls_irq;

	    break;	

	 default:
	       break;	
	
	}
	
	return gpio_num;


}

int udc_get_irq(udc_t section_id, udc_t eic_no)
{
	int irq_no = 0;
	int irq_gpio = 0;
	
	irq_gpio = udc_get_irq_gpio(section_id);
	
#ifdef CONFIG_UDC_VER_2_3_5

	if (0xFFFF == eic_no)
	{
	      gpio_direction_input(irq_gpio);
       	irq_no = sprd_alloc_gpio_irq(irq_gpio);
	}
	else
	{
       	irq_no = sprd_alloc_eic_irq(eic_no);
	}

#else

        gpio_direction_input(irq_gpio);
        irq_no = gpio_to_irq(irq_gpio);

#endif

      
      UDC_TRACE("%s: section_id = 0x%x, irq_gpio = %d, eic_no = %d\r\n", __func__, section_id, irq_gpio, eic_no);
      
      return irq_no;

}


int udc_put_irq(int irq, udc_t eic_no)
{

#ifdef CONFIG_UDC_VER_2_3_5
	if (eic_no == 0xFFFF)
	{
		sprd_free_gpio_irq(irq);
	}
	else
	{
       	sprd_free_eic_irq(eic_no);
	}
#endif
}
#endif
/*
int udc_adi_write_mask(udc32_t reg, udc32_t value, udc32_t mask)
{
#ifdef CONFIG_UDC_VER_2_3_5
     ANA_REG_AND (reg, ~ mask);
     ANA_REG_OR (reg, value);
     ANA_REG_MSK_OR (reg, value, mask);      	
     return 0;
#else
     return sci_adi_write(reg, value, mask);
#endif

}*/

udc_t udc_get_item_value(udc_item_s*item, udc_t key_id, udc_t* value, udc_t max_count)
{
    udc_t i = 0;
	udc_t count;  
	udc_t value_count = item[key_id].value_count;

       count = MIN(value_count, max_count);
	   
       if ( count <= 0 ) 	
		return 0;

	for ( i = 0; i < count; i++ )
     
              value[i] = item[key_id].value[i];
	return count;
}



/****************************************************************
 * udc_get_file_size: 	caculate the udc data size (the size of udc->buffer),
 *					save the length to udc->length
 * 	in:
 *		@udc		: pointer to udc_s struct
 *	return:
 *		the length of the udc data
*****************************************************************/
udc32_t udc_get_file_size(udc_s*udc)
{
      udc32_t length = 0; 
     
     udc_t*section = udc->buffer;
     udc_t section_id, section_size;
    	 
     
      UDC_TRACE("%s: buffer = 0x%x\n", __func__,  udc->buffer);
    
      if ( NULL == section )
	  	return 0;

	//TODO:  check time stamp
	section += 2;
       length = 4;
 
       
       while(1)
       {
		 //step2 get sec id and size
		section_id = *section++;
		section_size = *section++;
		//end of file
		if ( 0xffff == section_id && 0xffff == section_size ) 
			break;
	       section += ( section_size / ( sizeof(udc_t) ) ) ;
			  
	       length += (4 + section_size);

		udc->data_buffer_length  = MAX(udc->data_buffer_length, section_size); 	 		   
	}

       length += 4; //end flag
	udc->length = length;

       UDC_TRACE("%s: udc->length = 0x%x\n", __func__,  udc->length);

	return length;

}


/****************************************************************
 * udc_search_first_section: search next section by section id
 * 	in:
 *		@udc		: pointer to udc_s struct
 *		@section_id	: section id to search
 *	out:
 *		@section	       : section
 *	return:
 *		1	:            find the section
 *		0      :           no section
*****************************************************************/
udc_t udc_search_first_section(udc_s*udc,  udc_t section_id,  udc_section* section)
{

     udc_t*buffer = udc->buffer;
     udc_t udc_section_id, udc_section_size;
    	 
      section->size  = 0;

      UDC_TRACE("%s: section_id = 0x%x\n", __func__, section_id);
    
      if ( NULL == buffer )
	  	return 0;
	//stetp1: skip time stemp 2B
       buffer += 2;



       while ( 1 )
       {
		 //step2 get sec id and size
		udc_section_id =  *buffer++;
		udc_section_size = *buffer++;

	       UDC_TRACE("%s: udc_section_id = 0x%x, udc_section_size = 0x%x\n", __func__,  udc_section_id, udc_section_size);
	     
		   
	      //end of udc img
		if ( 0xffff == udc_section_id && 0xffff == udc_section_size) 
		        goto SECTION_OUT;

		//step3:major section id matched
              if  ( (udc_section_id & 0xff00) == section_id)
	       {
		       buffer -=2;
               section->id = udc_section_id;
			   section->size = udc_section_size;
			   section->buffer = buffer;
			
		
			UDC_TRACE("%s: find section = 0x%x, udc_section_size = 0x%x\n", __func__,  buffer, udc_section_size);
			return 1;
			
	       }	
		else
		{		    
			buffer += ( udc_section_size/sizeof(udc_t) );
			 
		}

       }

SECTION_OUT:
	UDC_TRACE("%s: section_id = 0x%x not found\n", __func__,  section_id);
	
      section->size  = 0;
      return 0;


}

/****************************************************************
 * udc_search_next_section: search next section by section id
 * 	in:
 *		@udc		: pointer to udc_s struct
 *		@current	       : current section
 *	out:
 *		@next	       : next section
 *	return:
 *		1	:            find the section
 *		0      :           no section
*****************************************************************/
udc_t udc_search_next_section(udc_s*udc,  udc_section* current_section ,  udc_section* next)
{

       udc_t udc_section_id, udc_section_size;
    udc_t*buffer ;

	udc_section_id =  current_section->id;
	udc_section_size = current_section->size;
	buffer = current_section->buffer;	
    UDC_TRACE("%s: udc_section_id = 0x%x,udc_section_size = 0x%x \n",__func__,current_section->id, current_section->size); 
	 
      UDC_TRACE("%s: buffer = 0x%x\n", __func__,  buffer);
    
      if ( NULL == buffer)
	  	return 0;

	//stetp1: skip current section
       while(1)
      	{
		buffer += 2;
	       buffer += ( udc_section_size/sizeof(udc_t) );
	      
		if ( buffer - udc->buffer >  udc->length )
		{
	             UDC_TRACE("%s: buffer = 0x%x out of file size %d \n", __func__,   buffer - udc->buffer, udc->length);
		      return 0;
		}
        
       	 //step2 get sec id and size
		udc_section_id =  buffer[0];
		udc_section_size = buffer[1];

	       UDC_TRACE("%s: udc_section_id = 0x%x, udc_section_size = 0x%x\n", __func__,  udc_section_id, udc_section_size);
	     
		   
	      //end of udc img
		if ( 0xffff == udc_section_id && 0xffff == udc_section_size) 
		{
	             goto SECTION_OUT;

		}
		
		//step3:major section id matched
              if  ( (udc_section_id & 0xff00) == (current_section->id & 0xff00) )
	       {
		       next->id = udc_section_id;
			next->size = udc_section_size;
			next->buffer = buffer;
			UDC_TRACE("%s: find section = 0x%x, udc_section_size = 0x%x\n", __func__,  buffer, udc_section_size);
			return 1;
			
	       }	
	}

SECTION_OUT:
	UDC_TRACE("%s: NO section :0x%x \n", __func__,  current_section->id);
	
       return 0;


}

udc_t  udc_search_value(udc_s*udc, udc_section* section,  udc_t key_id, udc_item_s* item )
{
     udc_t*pValue = NULL;
     udc_t udc_section_size, count;
     udc_t udc_key_id, udc_keySize;
	 
 
      item->value_count = 0;
      udc_section_size = section->size;
      pValue = section->buffer;
	  
      UDC_TRACE("%s:section->id = 0x%x, buffer = 0x%x\n", __func__, (udc_t)section->id, (unsigned int)pValue);
  
      if ( NULL == pValue || section->size == 0)
	  	return 0;

   
       //stetp1: skip section id and size: 2B
       pValue += 2;
	count = 0;

	while( count < udc_section_size)
	{

      		udc_key_id =  *pValue ++;
	       udc_keySize = *pValue ++;
		 //end of udc img
		if ( 0xffff == udc_key_id && 0xffff == udc_keySize) 
		{
	             goto VALUE_OUT;
		}  
   
     
	       if  (  key_id == udc_key_id )
	       {
                     item->key_id = udc_key_id;
			item->value_count =  udc_keySize/sizeof(udc_t);
			item->value = pValue;
			return 1;
		}
		  
	     	count +=  ( 4 + udc_keySize );
		pValue += (udc_keySize/sizeof(udc_t));

	}

		
VALUE_OUT:
	UDC_TRACE("%s: section_id:0x%x , no key :0x%x\n", __func__,  section->id, key_id);
  	
       item->value_count = 0;
       return 0;

}

udc_t  udc_match_item(udc_s*udc, udc_section* section,  udc_item_s* item )
{     

   	udc_item_s find_item;
	udc_t ret = 0;

	if ( item == NULL )
	    return 0;      	
	   
	while( item && 0xffff != item->key_id )
       {
	       ret = udc_search_value(udc, section, item->key_id, &find_item);

		if ( ret > 0 ) 
		{
		      item->value = find_item.value;
		      item->value_count = find_item.value_count;
		
       	}
		else
		{
			item->value = NULL;
			item->value_count = 0;
		}
		item++;
	}
		
	
	return 1;

}

udc_s* udc_get_udc (void )
{
	return &g_udc;
}


static uint32_t udc_base = 0;
static uint32_t udc_size = 0;
#ifdef CONFIG_UDC_LCD
static uint32_t udc_lcd_offset = 0;
udc32_t udc_get_lcd_offset(void)
{
	return udc_lcd_offset;
}
#endif

static int __init udc_start(char *str)
{ 
       uint32_t i = 0; 
	UDC_TRACE("%s: line= %d\n", __func__, __LINE__); 
	if ((str != NULL) && (str[0] == '0') && (str[1] == 'x')) {

		sscanf(&str[2], "%x", &udc_base);
              	  
		while ( ',' != str[i] )
		{
			i++;
		}
		
		sscanf(&str[ i + 4], "%x", &udc_size);

#ifdef CONFIG_UDC_LCD
              i++;
		while ( ',' != str[i] )
		{
			i++;
		}
		
		sscanf(&str[ i + 4], "%x", &udc_lcd_offset);
#endif		
	}
#ifdef CONFIG_UDC_LCD	
	UDC_TRACE("udc_base = 0x%x, udc_size = 0x%x, udc_lcd_offset = 0x%x\n", udc_base, udc_size, udc_lcd_offset);
#else
      UDC_TRACE("udc_base = 0x%x, udc_size = 0x%x\n", udc_base, udc_size);
#endif
	return 1;
}
__setup("udc=", udc_start);





udc_item_s g_udc_board_item[] = {
  {	BOARD_FM_LDO             , 0, 0},
  {	BOARD_FM_LDO_LEVEL       , 0, 0},
  {	BOARD_FM_I2C_BUS_NUM 	   , 0, 0},
  {	BOARD_GSENSOR_LDO        , 0, 0},
  {	BOARD_GSENSOR_LDO_LEVEL  , 0, 0},
  {	BOARD_GSENSOR_I2C_BUS_NUM, 0, 0},
  {	BOARD_MSENSOR_LDO        , 0, 0},
  {	BOARD_MSENSOR_LDO_LEVEL  , 0, 0},
  {	BOARD_MSENSOR_I2C_BUS_NUM, 0, 0},
  {	BOARD_LSENSOR_LDO        , 0, 0},
  {	BOARD_LSENSOR_LDO_LEVEL  , 0, 0},
  {	BOARD_LSENSOR_I2C_BUS_NUM, 0, 0},
  {	BOARD_PSENSOR_LDO        , 0, 0},
  {	BOARD_PSENSOR_LDO_LEVEL  , 0, 0}, 
  {	BOARD_PSENSOR_I2C_BUS_NUM, 0, 0},
  {	BOARD_TP_LDO             , 0, 0},
  {	BOARD_TP_LDO_LEVEL       , 0, 0},
  {	BOARD_TP_I2C_BUS_NUM 	   , 0, 0},
  {	BOARD_LCD_CS 	   		, 0, 0},
  {	BOARD_BACKLIGHT_IC_GPIO, 0, 0},
  {	BOARD_CAMERA_IDENTIFY_FLAG, 0, 0},
  {   BOARD_INVALID ,            0, 0}      
};  
udc_board g_udc_board;


static udc_t udc_board_get_value(udc_t key_id, udc_t* value, udc_t value_count)
{
       udc_board* board = &g_udc_board;
	return udc_get_item_value(board->item, key_id, value, value_count);
}

static void udc_board_init (udc_board* board)
{
       udc_t param;
	  
	udc_board_config* config = &board->config;

	
	config->fm_ldo                        = 42;	//LDO_LDO_SIM2
	config->fm_ldo_level                = 0;	//LDO_VOLT_LEVEL0
	config->fm_i2c_bus_num          = 2;
	config->gsensor_ldo                 = 0; //LDO_LDO_NULL
	config->gsensor_ldo_level        =  4; //LDO_VOLT_LEVEL_FAULT_MAX 
	config->gsensor_i2c_bus_num  = 0; 
	config->msensor_ldo                = 0; //LDO_LDO_NULL
	config->msensor_ldo_level       =  4; //LDO_VOLT_LEVEL_FAULT_MAX 
	config->msensor_i2c_bus_num = 0 ; 
	config->lsensor_ldo                  = 44; //LDO_LDO_WIF1
	config->lsensor_ldo_level          = 2; //LDO_VOLT_LEVEL2
	config->lsensor_i2c_bus_num    = 0;  
	config->psensor_ldo                 = 44; //LDO_LDO_WIF1
	config->psensor_ldo_level         = 2; //LDO_VOLT_LEVEL2 
	config->psensor_i2c_bus_num  = 0;
	config->tp_ldo                        = 42;	//LDO_LDO_SIM2
	config->tp_ldo_level                = 0;	//LDO_VOLT_LEVEL0
	config->tp_i2c_bus_num          = 1;
	config->lcd_cs			   = 0;   //CS0
       config->backlight_ic_gpio        = 143; 
	config->camera_identify_flag  = 1;	//identify camera everytime
	
       //fm
	 if ( udc_board_get_value(BOARD_FM_LDO, &param, 1) )
	 {
		config->fm_ldo = param;

		UDC_TRACE("%s: line = %d, fm_ldo 0x%x\n", __func__, __LINE__, config->fm_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_FM_LDO_LEVEL, &param, 1) )
	{
		config->fm_ldo_level = param;
		UDC_TRACE("%s: line = %d, fm_ldo_level 0x%x\n", __func__, __LINE__, config->fm_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_FM_I2C_BUS_NUM, &param, 1) )
	{
		config->fm_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, fm_i2c_bus_num 0x%x\n", __func__, __LINE__, config->fm_i2c_bus_num);
    
	}

       //gsensor
	 if ( udc_board_get_value(BOARD_GSENSOR_LDO, &param, 1) )
	 {
		config->gsensor_ldo = param;

		UDC_TRACE("%s: line = %d, gsensor_ldo 0x%x\n", __func__, __LINE__, config->gsensor_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_GSENSOR_LDO_LEVEL, &param, 1) )
	{
		config->gsensor_ldo_level = param;
		UDC_TRACE("%s: line = %d, gsensor_ldo_level 0x%x\n", __func__, __LINE__, config->gsensor_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_GSENSOR_I2C_BUS_NUM, &param, 1) )
	{
		config->gsensor_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, gsensor_i2c_bus_num 0x%x\n", __func__, __LINE__, config->gsensor_i2c_bus_num);
    
	}
       //msesnor
	 if ( udc_board_get_value(BOARD_MSENSOR_LDO, &param, 1) )
	 {
		config->msensor_ldo = param;

		UDC_TRACE("%s: line = %d, msensor_ldo 0x%x\n", __func__, __LINE__, config->msensor_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_MSENSOR_LDO_LEVEL, &param, 1) )
	{
		config->msensor_ldo_level = param;
		UDC_TRACE("%s: line = %d, msensor_ldo_level 0x%x\n", __func__, __LINE__, config->msensor_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_MSENSOR_I2C_BUS_NUM, &param, 1) )
	{
		config->msensor_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, msensor_i2c_bus_num 0x%x\n", __func__, __LINE__, config->msensor_i2c_bus_num);
    
	}

	//lsensor

	 if ( udc_board_get_value(BOARD_LSENSOR_LDO, &param, 1) )
	 {
		config->lsensor_ldo = param;

		UDC_TRACE("%s: line = %d, lsensor_ldo 0x%x\n", __func__, __LINE__, config->lsensor_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_LSENSOR_LDO_LEVEL, &param, 1) )
	{
		config->lsensor_ldo_level = param;
		UDC_TRACE("%s: line = %d, lsensor_ldo_level 0x%x\n", __func__, __LINE__, config->lsensor_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_LSENSOR_I2C_BUS_NUM, &param, 1) )
	{
		config->lsensor_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, lsensor_i2c_bus_num 0x%x\n", __func__, __LINE__, config->lsensor_i2c_bus_num);
    
	}

	 //psensor

	 if ( udc_board_get_value(BOARD_PSENSOR_LDO, &param, 1) )
	 {
		config->psensor_ldo = param;

		UDC_TRACE("%s: line = %d, psensor_ldo 0x%x\n", __func__, __LINE__, config->psensor_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_PSENSOR_LDO_LEVEL, &param, 1) )
	{
		config->psensor_ldo_level = param;
		UDC_TRACE("%s: line = %d, psensor_ldo_level 0x%x\n", __func__, __LINE__, config->psensor_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_PSENSOR_I2C_BUS_NUM, &param, 1) )
	{
		config->psensor_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, psensor_i2c_bus_num 0x%x\n", __func__, __LINE__, config->psensor_i2c_bus_num);
    
	}
	
	 //tp
	 if ( udc_board_get_value(BOARD_TP_LDO, &param, 1) )
	 {
		config->tp_ldo = param;

		UDC_TRACE("%s: line = %d, tp_ldo 0x%x\n", __func__, __LINE__, config->tp_ldo);
	 }	
		
	if ( udc_board_get_value(BOARD_TP_LDO_LEVEL, &param, 1) )
	{
		config->tp_ldo_level = param;
		UDC_TRACE("%s: line = %d, tp_ldo_level 0x%x\n", __func__, __LINE__, config->tp_ldo_level);
    
	}

	if ( udc_board_get_value(BOARD_TP_I2C_BUS_NUM, &param, 1) )
	{
		config->tp_i2c_bus_num = param;
		UDC_TRACE("%s: line = %d, tp_i2c_bus_num 0x%x\n", __func__, __LINE__, config->tp_i2c_bus_num);
	}
	
	if ( udc_board_get_value(BOARD_LCD_CS, &param, 1) )
	{
		config->lcd_cs= param;
		UDC_TRACE("%s: line = %d, lcd_cs %d\n", __func__, __LINE__, config->lcd_cs);
    
	}


	if ( udc_board_get_value(BOARD_BACKLIGHT_IC_GPIO, &param, 1) )
	{
		config->backlight_ic_gpio = param;
		UDC_TRACE("%s: line = %d, backlight_ic_gpio %d\n", __func__, __LINE__, config->backlight_ic_gpio);
    
	} 

	if ( udc_board_get_value(BOARD_CAMERA_IDENTIFY_FLAG, &param, 1) )
	{
		config->camera_identify_flag= param;
		UDC_TRACE("%s: line = %d, camera_identify_flag %d\n", __func__, __LINE__, config->camera_identify_flag);
    
	} 
}


static udc_board* udc_board_create(udc_t section_id)
{
    udc_s* udc;
	udc_board* board = &g_udc_board;
	udc_section section;
	udc_t ret;
       
	
       udc = udc_get_udc();

	board->udc = udc;
	board->item = g_udc_board_item;
	
      
	ret = udc_search_first_section(udc, section_id, &section);
	   
	
       ret = udc_match_item(udc, &section, board->item);
	   
		
	udc_board_init(board);	
	
	return board;

}



static void udc_init (udc_s*udc, uint32_t* buffer )
{
    udc_t* udc_data_from_uboot = (udc_t*)phys_to_virt(udc_base);
	udc->buffer = (udc_t*)kmalloc(udc_size, GFP_KERNEL);	
    memcpy(udc->buffer, udc_data_from_uboot, udc_size);
	UDC_TRACE("%s: line = %d, buffer: 0x%x, 0x%x, 0x%x, 0x%x\n", __func__, __LINE__, udc->buffer[0], udc->buffer[1], udc->buffer[2], udc->buffer[3]);   

	udc->data_buffer_length  = 0;
       udc->length = udc_get_file_size(udc);
	 if ( udc->data_buffer_length > 0 )
       {
		udc->data_buffer = (unsigned char*)kmalloc(udc->data_buffer_length, GFP_KERNEL);
		memset(udc->data_buffer, 0x0, udc->data_buffer_length);
       } 

	udc->board = udc_board_create(SEC_BOARD);

	//udc_regulator_init();
  	
}




udc_s* udc_create ( void )
{
	udc_s* udc = &g_udc;
       
	udc_init(udc, &udc_base);
	UDC_TRACE("%s: time stamp: 0x%x, 0x%x, 0x%x, 0x%x\n", __func__, udc->buffer[0], udc->buffer[1], udc->buffer[2], udc->buffer[3]);
	
	return udc;

}

//add by huosheng

EXPORT_SYMBOL(udc_search_first_section);
EXPORT_SYMBOL(udc_search_next_section);
EXPORT_SYMBOL(udc_search_value);
EXPORT_SYMBOL(udc_get_item_value);
EXPORT_SYMBOL(udc_get_file_size);
EXPORT_SYMBOL(udc_get_udc);
EXPORT_SYMBOL(udc_init);
EXPORT_SYMBOL(udc_create);
//EXPORT_SYMBOL(udc_adi_write_mask);
#ifdef CONFIG_UDC_GPIO
EXPORT_SYMBOL(udc_get_irq);
EXPORT_SYMBOL(udc_put_irq);
#endif


