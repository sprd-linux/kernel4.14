/* drivers/video/udc/udc_lcd/udc_lcd.c
 *
 *
 * Copyright (C) 2010 freecom
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
#include <drm/drm_panel.h> 
#include <soc/sprd/board.h>

static uint32_t udc_lcd_start(udc_sprd_panel *panel, udc_t key_id,uint16_t left, uint16_t top,uint16_t right,uint16_t bottom);
static uint32_t udc_lcd_mipi_do(udc_sprd_panel *panel, unsigned short *value, unsigned short value_count,uint16_t left, uint16_t top,uint16_t right,uint16_t bottom);

static int udc_sprd_panel_get_modes(struct drm_panel *p);
static int udc_sprd_panel_enable(struct drm_panel *p);
static int udc_sprd_panel_disable(struct drm_panel *p);
static int udc_sprd_panel_prepare(struct drm_panel *p);
static int udc_sprd_panel_unprepare(struct drm_panel *p);



static const struct drm_panel_funcs udc_sprd_panel_funcs = {
	.get_modes = udc_sprd_panel_get_modes,
	.enable = udc_sprd_panel_enable,
	.disable = udc_sprd_panel_disable,
	.prepare = udc_sprd_panel_prepare,
	.unprepare = udc_sprd_panel_unprepare,
};



static udc_item_s g_udc_lcd_item[] = {
	{LCD_FMARK               ,   0,   0},	
	{LCD_NAME               ,   0,   0},
	{LCD_ID               ,   0,   0},
	{LCD_WIDTH            ,   0,   0},
	{LCD_HIGHT            ,   0,   0},
	{LCD_MODE             ,   0,   0},
	{LCD_DIRECTION        ,   0,   0},
	{LCD_BUS_MODE         ,   0,   0},
	{LCD_BUS_WIDTH        ,   0,   0},
	{LCD_TIMING0          ,   0,   0},
	{LCD_TIMING1          ,   0,   0},
	{LCD_READ_ID          ,   0,   0},
	{LCD_INIT_PARA        ,   0,   0},
	{LCD_SET_WINDOW       ,   0,   0},
	{LCD_INVALIDATE_RECT  ,   0,   0},
	{LCD_DIR_NORMAL       ,   0,   0},
	{LCD_DIR_ROT90        ,   0,   0},
	{LCD_DIR_ROT180       ,   0,   0},
	{LCD_DIR_ROT270       ,   0,   0},
	{LCD_DIR_MIRH         ,   0,   0},
	{LCD_DIR_MIRV         ,   0,   0},
	{LCD_DIR_MIRHV        ,   0,   0},
	{LCD_ENTER_SLEEP      ,   0,   0},
	{LCD_EXIST_SLEEP      ,   0,   0},
	{LCD_WORK_MODE       ,   0,   0},
	{LCD_LAN_NUM         ,   0,   0},
	{LCD_PHY_FEQ         ,   0,   0},
	{LCD_H_SYNC_POL      ,   0,   0},
	{LCD_V_SYNC_POL      ,   0,   0},
	{LCD_DE_POL          ,   0,   0},
	{LCD_TE_POL          ,   0,   0},
	{LCD_COLOR_MODE_POL  ,   0,   0},
	{LCD_SHUT_DOWN_POL   ,   0,   0},
	{LCD_POWER_MODE      ,   0,   0},
	{LCD_READ_POWERMODE   ,   0,   0},
	{LCD_SPEED_MODE   ,   0,   0},
	{LCD_FPS   ,   0,   0},	
	{LCD_SUSPEND_MODE   ,   0,   0},	
	{LCD_DSC_COMPRESSION   ,   0,   0},	
	{LCD_POWER_ON_SEQUENCE   ,   0,	0}, 
	{LCD_POWER_OFF_SEQUENCE   ,   0,	0}, 
	{LCD_WIDTH_MM   ,   0,   0},
	{LCD_HEIGHT_MM   ,   0,   0},	
	{LCD_SIMU_WIDTH   ,   0,   0},	
	{LCD_SIMU_HEIGHT   ,   0,   0},	
	{LCD_BURST_MODE   ,   0,	0},	
	{LCD_PIXEL_CLK   ,   0,	0}, 
	{LCD_SUPPLY_VOLTAGE   ,   0,	0}, 
	{LCD_PHY_ESCAPE_CLOCK   ,   0,	0}, 
	{0XFFFF               ,   0,   0}
};


static udc_lcd g_udc_lcd;



static inline struct sprd_panel *to_sprd_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sprd_panel, base);
}


static int udc_sprd_panel_get_modes(struct drm_panel *p)
{
	struct drm_display_mode *mode;
	struct sprd_panel *panel = to_sprd_panel(p);
	//struct device_node *np = panel->slave->dev.of_node;
	u32 surface_width = 0, surface_height = 0;
	int i, mode_count = 0;

	DRM_INFO("%s()\n", __func__);
	mode = drm_mode_duplicate(p->drm, &panel->info.mode);
	if (!mode) {
		DRM_ERROR("failed to alloc mode %s\n", panel->info.mode.name);
		return 0;
	}
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(p->connector, mode);
	mode_count++;

	for (i = 1; i < panel->info.num_buildin_modes; i++)	{
		mode = drm_mode_duplicate(p->drm,
			&(panel->info.buildin_modes[i]));
		if (!mode) {
			DRM_ERROR("failed to alloc mode %s\n",
				panel->info.buildin_modes[i].name);
			return 0;
		}
		mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_DEFAULT;
		drm_mode_probed_add(p->connector, mode);
		mode_count++;
	}
	//jinq:need add by udc
	//of_property_read_u32(np, "sprd,surface-width", &surface_width);
	//of_property_read_u32(np, "sprd,surface-height", &surface_height);
	if (surface_width && surface_height) {
		struct videomode vm = {};

		vm.hactive = surface_width;
		vm.vactive = surface_height;
		vm.pixelclock = surface_width * surface_height * 60;

		mode = drm_mode_create(p->drm);

		mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_BUILTIN |
			DRM_MODE_TYPE_CRTC_C;
		mode->vrefresh = 60;
		drm_display_mode_from_videomode(&vm, mode);
		drm_mode_probed_add(p->connector, mode);
		mode_count++;
	}

	p->connector->display_info.width_mm = panel->info.mode.width_mm;
	p->connector->display_info.height_mm = panel->info.mode.height_mm;

	return mode_count;
}


static int udc_sprd_panel_enable(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s()\n", __func__);

 	udc_lcd_start(panel,LCD_INIT_PARA,0,0,0,0);

	if (panel->backlight) {
		panel->backlight->props.power = FB_BLANK_UNBLANK;
		panel->backlight->props.state &= ~BL_CORE_FBBLANK;
		backlight_update_status(panel->backlight);
	}

	if (panel->info.esd_check_en) {
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(1000));
		panel->esd_work_pending = true;
	}

	return 0;
}


static int udc_sprd_panel_disable(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);

	DRM_INFO("%s()\n", __func__);

	/*
	 * FIXME:
	 * The cancel work should be executed before DPU stop,
	 * otherwise the esd check will be failed if the DPU
	 * stopped in video mode and the DSI has not change to
	 * CMD mode yet. Since there is no VBLANK timing for
	 * LP cmd transmission.
	 */
	if (panel->esd_work_pending) {
		cancel_delayed_work_sync(&panel->esd_work);
		panel->esd_work_pending = false;
	}

	if (panel->backlight) {
		panel->backlight->props.power = FB_BLANK_POWERDOWN;
		panel->backlight->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(panel->backlight);
	}
	udc_lcd_start(panel,LCD_ENTER_SLEEP,0,0,0,0);
	return 0;
}


static int udc_sprd_panel_prepare(struct drm_panel *p)
{
	struct sprd_panel *panel = to_sprd_panel(p);
	struct gpio_timing *timing;
	int items, i, ret, gpio_temp;

	DRM_INFO("%s()\n", __func__);

	ret = regulator_enable(panel->supply);
	if (ret < 0)
		DRM_ERROR("enable lcd regulator failed\n");
	/*

	if (panel->info.avdd_gpio) {
		gpiod_direction_output(panel->info.avdd_gpio, 1);
		mdelay(5);
	}

	if (panel->info.avee_gpio) {
		gpiod_direction_output(panel->info.avee_gpio, 1);
		mdelay(5);
	}
	*/
	items = panel->info.rst_on_seq.items;
	timing = panel->info.rst_on_seq.timing;
	if (timing[0].gpio) {
		for (i = 0; i < items; i++) {
		#ifdef ZCFG_UDC_GPIO_BASE
			gpio_temp = timing[i].gpio + ZCFG_UDC_GPIO_BASE;
		#else
			gpio_temp = timing[i].gpio + 192;
		#endif
			//printk("udc_sprd_panel_prepare:reset_gpio=%d,level=%d,delay=%d\n",gpio_temp,timing[i].level,timing[i].delay);
			gpio_direction_output(gpio_temp,
						timing[i].level);
			mdelay(timing[i].delay);
		}
	}

	return 0;
}



static int udc_sprd_panel_unprepare(struct drm_panel *p)
	{
		struct sprd_panel *panel = to_sprd_panel(p);
		struct gpio_timing *timing;
		int items, i, gpio_temp;
	
		DRM_INFO("%s()\n", __func__);
	/*
		if (panel->info.avee_gpio) {
			gpiod_direction_output(panel->info.avee_gpio, 0);
			mdelay(5);
		}
	
		if (panel->info.avdd_gpio) {
			gpiod_direction_output(panel->info.avdd_gpio, 0);
			mdelay(5);
		}
	*/

		items = panel->info.rst_off_seq.items;
		timing = panel->info.rst_off_seq.timing;
		if (timing[0].gpio) {
			for (i = 0; i < items; i++) {
			#ifdef ZCFG_UDC_GPIO_BASE
				gpio_temp = timing[i].gpio + ZCFG_UDC_GPIO_BASE;
			#else
				gpio_temp = timing[i].gpio + 192;
			#endif
				//printk("udc_sprd_panel_unprepare:reset_gpio=%d,level=%d,delay=%d\n",gpio_temp,timing[i].level,timing[i].delay);
				gpio_direction_output(gpio_temp,
							timing[i].level);
				mdelay(timing[i].delay);
			}
	
		}
	
		regulator_disable(panel->supply);
	
		return 0;
}

static int udc_sprd_panel_gpio_request(struct reset_sequence *seq)
{
	unsigned int items;
	struct gpio_timing *timing;
	int i;

	items = seq->items;
	timing = seq->timing;

	for (i = 0; i < items; i++)
		gpio_request(timing[i].gpio, NULL);
	return 0;
}




static uint32_t udc_lcd_start(udc_sprd_panel *panel, udc_t key_id,
	                                          uint16_t left, uint16_t top,uint16_t right,uint16_t bottom)
{
	udc_t *value;
	uint32_t read_param = 0;
	unsigned short value_count;
    udc_lcd* lcd = udc_get_lcd(SEC_LCD0);  
 // UDC_LCD_TRACE("%s: key_id = %d, pannel_type = %d \n", __func__, key_id, pannel_type);   
	if (key_id >= CONFIG_MAX_ID) {
		UDC_LCD_TRACE("%s invalid sub name id %d\n", __func__, key_id);
		return (uint32_t)-EINVAL;
	}        
	value =   lcd->item[key_id].value;
	value_count = lcd->item[key_id].value_count;

	if ((value != NULL) && (value_count>0 ))
	{
		UDC_LCD_TRACE("%s: value = 0x%x, value_count = %d\n", __func__, *value, value_count);
	#ifdef CONFIG_UDC_LCD_MIPI
		read_param = udc_lcd_mipi_do(panel, value, value_count, left, top, right, bottom);
	#endif			
	}

	return read_param;
}


udc_t udc_lcd_get_value(udc_t key_id, udc_t* value, udc_t value_count)
{
	udc_lcd* lcd = &g_udc_lcd;

	return udc_get_item_value(lcd->item, key_id, value, value_count);
}

udc_lcd* udc_get_lcd(udc_t section_id)
{
	return &g_udc_lcd;
}
EXPORT_SYMBOL(udc_get_lcd);




static uint32_t udc_lcd_mipi_do(udc_sprd_panel *panel, unsigned short *value, unsigned short value_count,
		uint16_t left, uint16_t top,uint16_t right,uint16_t bottom)
{


	#define MAX_DATA   100

	uint32_t i=0, j=0;
	uint8_t read_data[MAX_DATA];
	uint8_t data[MAX_DATA];
	uint16_t cmd,len;
	uint8_t datatype;
	unsigned int *p;

	unsigned int items;
	struct gpio_timing *timing;
	struct mipi_dsi_device *dsi_ctx=panel->slave;

	UDC_LCD_TRACE("%s: value = 0x%x, value_count = %d\n", __func__, *value, value_count);
	
  for (i=0; i<value_count;) 
	{
		cmd = value[i];
	
		if(UDC_LCD_POWER_ON == cmd)
		{
			items = panel->info.rst_on_seq.items;
			timing = panel->info.rst_on_seq.timing;
			items= value[i+1]/3;
			panel->info.rst_on_seq.items=items;
			p = kzalloc(value[i+1]*sizeof(unsigned int), GFP_KERNEL);
			if (!p) {
				printk("error udc_lcd_mipi_do 1 alloc interface fail\n");
			}

			for(j = 0; j < value[i+1]; j++)
			{
				p[j] = (uint8_t)value[i+2+j];
				UDC_LCD_TRACE("UDC_LCD_POWER_ON  data[%d]= %d\n",j,p[j]);
			}
			(panel->info.rst_on_seq.timing)=(struct gpio_timing *)p;
			i =value[i+1]+2;
			//kfree(p);
			udc_sprd_panel_gpio_request(&panel->info.rst_on_seq);

		}
		if(UDC_LCD_POWER_OFF == cmd)
		{
			items = panel->info.rst_off_seq.items;
			timing = panel->info.rst_off_seq.timing;
			items= value[i+1]/3;
			panel->info.rst_off_seq.items=items;
			p = kzalloc(value[i+1]*sizeof(unsigned int), GFP_KERNEL);
			if (!p) {
				printk("error udc_lcd_mipi_do 2 alloc interface fail\n");
			}
			
			for(j = 0; j < value[i+1]; j++)
			{
				p[j] = (uint8_t)value[i+2+j];
				UDC_LCD_TRACE("UDC_LCD_POWER_OFF  data[%d]= %d\n",j,p[j]);
			}
			(panel->info.rst_off_seq.timing)=(struct gpio_timing *)p;
			i =value[i+1]+2;
			//kfree(p);
		}
	
		if(UDC_LCD_MIPI_SET_LP_MODE == cmd)
		{
			i += 1;
		}
		if(UDC_LCD_MIPI_SET_DATA_LP_MODE == cmd)
		{
			i += 1;
		}
		if(UDC_LCD_MIPI_SET_HS_MODE == cmd)
		{
			i += 1;
		}
		if(UDC_LCD_MIPI_SET_DATA_HS_MODE == cmd)
		{
			i += 1;
		}
		if(UDC_LCD_MIPI_SET_CMD_MODE == cmd)
		{
			i += 1;
		}	
			
		if(UDC_LCD_MIPI_EOTP_SET == cmd)
		{
		    UDC_LCD_TRACE("param1 = %d,param2 = %d\n",value[i+1],value[i+2]);
	
			i += 3;
		}		
	
		if(UDC_LCD_RETURN_ID == cmd)
		{	 
			read_data[0] = value[i+1];
			UDC_LCD_TRACE("return id 0x%X\n", read_data[0]);
		}

		if(UDC_LCD_MIPI_FORCE_WRITE ==cmd)
		{
			len = value[i+2];
			if(len > 2)
			{	
				len=value[i+3];
				datatype = (uint8_t)value[i+1];
				for(j = 0; j < len; j++)
				{
					data[j] = (uint8_t)value[i+5+j];
				}
				UDC_LCD_TRACE("data_type = 0x%x,len = %d,param = 0x%x\n",datatype,len,data[0]);
				mipi_dsi_dcs_write_buffer(dsi_ctx, data, len);//type,data,len
				i += len+5;
			}
			else	
			{
				datatype = (uint8_t)value[i+1];
				for(j = 0; j < len; j++)
				{
					data[j] = (uint8_t)value[i+3+j];
				}
				UDC_LCD_TRACE("data_type = 0x%x,len = %d,param = 0x%x\n",datatype,len,data[0]);
				mipi_dsi_dcs_write_buffer(dsi_ctx, data, len);//type,data,len
				i += len+3;
			}
		}

		if(UDC_LCD_MIPI_GEN_WRITE ==cmd)
		{
			if(value[i+1] >2)
			{
				len = value[i+2];
				for(j = 0; j < len; j++)
				{
					data[j] = (uint8_t)value[i+4+j];
		
				}

				//mipi_dsi_generic_write(dsi, cmds->payload, len);
				mipi_dsi_generic_write(dsi_ctx,data, len);//data,len
				i += len+4;
			
			}
			else
			{
				len = value[i+1];
				for(j = 0; j < len; j++)
				{
					data[j] = (uint8_t)value[i+2+j];
				//	UDC_LCD_TRACE("jinq****************data[%d]=0x%x , len=%d*******************\n",j,data[j],len);
				
				}
				mipi_dsi_generic_write(dsi_ctx,data, len);//data,len
				i += len+2;
			}
		
		}
		if(UDC_LCD_MIPI_FORCE_READ ==cmd)
		{ 
			len = value[i+2];
			data[0] = (uint8_t)value[i+1];
			UDC_LCD_TRACE("addr = 0x%x,len = %d\n",data[0],len);

			mipi_dsi_dcs_read(dsi_ctx, data[0], read_data,1);
			read_data[0] = read_data[value[i+3]];   //?DD?y?Y
			i += 4;
		}
		if(UDC_LCD_MIPI_GEN_READ ==cmd)
		{
				//mipi_gen_read(data, len));
		}
		if(UDC_LCD_DELAY_MS == cmd)
		{
			msleep(value[i+1]);
			i += 2;
		}
		udelay(30);
	}
	UDC_LCD_TRACE("read_data = 0x%x\n",read_data[0]);


	return read_data[0];


}



void udc_get_phy_bit_clk(struct sprd_dsi *dsi)
{

	 udc_t param;
	 if(udc_lcd_get_value(LCD_PHY_FEQ, &param,1))
	 {
		dsi->phy->ctx.freq = param*1000;
		dsi->ctx.byte_clk = param*1000 / 8;
	 }
	 else
	 {
		 dsi->phy->ctx.freq = 500000;
		 dsi->ctx.byte_clk = 500000 / 8;

	 }
	  UDC_LCD_TRACE("udc_phy_feq %d\n", dsi->phy->ctx.freq);
}
EXPORT_SYMBOL(udc_get_phy_bit_clk);


void udc_get_phy_escape_clk(struct sprd_dsi *dsi)
{

	 udc_t param;
	 if(udc_lcd_get_value(LCD_PHY_ESCAPE_CLOCK, &param,1))
	 {

		dsi->ctx.esc_clk = param > 20000 ? 20000 : param;
	 }
	 else
	 {
		 dsi->ctx.esc_clk = 20000;

	 }
	  UDC_LCD_TRACE("udc_phy_escape_feq %d\n", dsi->ctx.esc_clk);
}
EXPORT_SYMBOL(udc_get_phy_escape_clk);




int udc_get_display_timing(int num,struct display_timing *dt)
{
	unsigned short mipi_timing[6];
	udc_t param;
	
	if(num==0)
	{
		if (udc_lcd_get_value(LCD_TIMING0,mipi_timing,6))
		{
			dt->hfront_porch.typ = dt->hfront_porch.min =dt->hfront_porch.max =mipi_timing[0];
			dt->hback_porch.typ = dt->hback_porch.min =dt->hback_porch.max =mipi_timing[1];
			dt->hsync_len.typ = dt->hsync_len.min = dt->hsync_len.max = mipi_timing[2];
			dt->vfront_porch.typ =dt->vfront_porch.min =dt->vfront_porch.max= mipi_timing[3];
			dt->vback_porch.typ =dt->vback_porch.min =dt->vback_porch.max = mipi_timing[4];
			dt->vsync_len.typ =dt->vsync_len.min =dt->vsync_len.max = mipi_timing[5];
			UDC_LCD_TRACE("LCD_TIMING0 %d,%d,%d,%d,%d,%d\n", mipi_timing[0],mipi_timing[1],mipi_timing[2],mipi_timing[3],mipi_timing[4],mipi_timing[5]);
		}else{return -EINVAL;}
		if (udc_lcd_get_value(LCD_PIXEL_CLK,&param,1))
		{
			dt->pixelclock.typ =dt->pixelclock.min =dt->pixelclock.max = param*100000; 
			UDC_LCD_TRACE("LCD_PIXEL_CLK %d\n", dt->pixelclock.typ);
		}else{return -EINVAL;}
		
		if(udc_lcd_get_value(LCD_WIDTH,&param,1))
		{
			dt->hactive.typ =dt->hactive.min=dt->hactive.max = param;
			UDC_LCD_TRACE("dt->hactive %d\n", dt->hactive.typ);
		}else{return -EINVAL;}
		
		if(udc_lcd_get_value(LCD_HIGHT,&param,1))
		{
			dt->vactive.typ =dt->vactive.min = dt->vactive.max =  param;
			UDC_LCD_TRACE("dt->vactive %d\n", dt->vactive.typ);
		}else{return -EINVAL;}
		

	}
	else
	{
		if (udc_lcd_get_value(LCD_TIMING1,mipi_timing,6))
		{
			dt->hfront_porch.typ = dt->hfront_porch.min =dt->hfront_porch.max =mipi_timing[0];
			dt->hback_porch.typ = dt->hback_porch.min =dt->hback_porch.max =mipi_timing[1];
			dt->hsync_len.typ = dt->hsync_len.min = dt->hsync_len.max = mipi_timing[2];
			dt->vfront_porch.typ =dt->vfront_porch.min =dt->vfront_porch.max= mipi_timing[3];
			dt->vback_porch.typ =dt->vback_porch.min =dt->vback_porch.max = mipi_timing[4];
			dt->vsync_len.typ =dt->vsync_len.min =dt->vsync_len.max = mipi_timing[5];
			UDC_LCD_TRACE("LCD_TIMING1 %d,%d,%d,%d,%d,%d\n", mipi_timing[0],mipi_timing[1],mipi_timing[2],mipi_timing[3],mipi_timing[4],mipi_timing[5]);

		}
		if (udc_lcd_get_value(LCD_PIXEL_CLK,&param,1))
		{
			dt->pixelclock.typ =dt->pixelclock.min =dt->pixelclock.max = param*100000; 
			UDC_LCD_TRACE("LCD_PIXEL_CLK %d\n", dt->pixelclock.typ);
		}
		
		if(udc_lcd_get_value(LCD_SIMU_WIDTH,&param,1))
		{
			dt->hactive.typ =dt->hactive.min=dt->hactive.max = param;
			UDC_LCD_TRACE("dt->hactive %d\n", dt->hactive.typ);
		}
		
		if(udc_lcd_get_value(LCD_SIMU_HEIGHT,&param,1))
		{
			dt->vactive.typ =dt->vactive.min = dt->vactive.max =  param;
			UDC_LCD_TRACE("dt->vactive %d\n", dt->vactive.typ);
		}

	}
	return 0;
}
EXPORT_SYMBOL(udc_get_display_timing);

static int of_parse_buildin_modes(struct panel_info *info,
	struct device_node *lcd_node)
{
	int i, rc, num_timings;
	struct device_node *timings_np;


	timings_np = of_get_child_by_name(lcd_node, "display-timings");
	if (!timings_np) {
		DRM_ERROR("%s: can not find display-timings node\n",
			lcd_node->name);
		return -ENODEV;
	}

	num_timings = of_get_child_count(timings_np);
	if (num_timings == 0) {
		/* should never happen, as entry was already found above */
		DRM_ERROR("%s: no timings specified\n", lcd_node->name);
		goto done;
	}

	info->buildin_modes = kzalloc(sizeof(struct drm_display_mode) *
				num_timings, GFP_KERNEL);

	for (i = 0; i < num_timings; i++) {
		rc = of_get_drm_display_mode(lcd_node,
			&info->buildin_modes[i], NULL, i);
		if (rc) {
			DRM_ERROR("get display timing failed\n");
			goto entryfail;
		}

		info->buildin_modes[i].width_mm = info->mode.width_mm;
		info->buildin_modes[i].height_mm = info->mode.height_mm;
		info->buildin_modes[i].vrefresh = info->mode.vrefresh;
	}
	info->num_buildin_modes = num_timings;
	DRM_INFO("info->num_buildin_modes = %d\n", num_timings);
	goto done;

entryfail:
	kfree(info->buildin_modes);
done:
	of_node_put(timings_np);

	return 0;
}

int udc_get_power_param(udc_sprd_panel  *panel)
{
	if(panel)
	{
		udc_lcd_start(panel,LCD_POWER_ON_SEQUENCE,0,0,0,0);
		udc_lcd_start(panel,LCD_POWER_OFF_SEQUENCE,0,0,0,0);
	}
	return 0;
}


int udc_lcd_config_panel(udc_lcd* lcd, udc_t section_id)
{
    udc_t ret;
	udc_t param;
	udc_t param1[100];
  	udc_sprd_panel  *panel = lcd->panel;
 	udc_panel_info  *panel_info = &lcd->panel->info;
	int i;


	struct device_node *lcd_node;
	char lcd_path[60];
	sprintf(lcd_path, "/lcds/%s", "lcd_nt35695_truly_mipi_fhd"); 
	lcd_node = of_find_node_by_path(lcd_path);
	if (!lcd_node) {
		DRM_ERROR("could not find lcd_nt35695_truly_mipi_fhd node\n");
		return -ENODEV;
	}
	panel_info->of_node = lcd_node;
//	panel_info = kzalloc(sizeof(struct panel_info), GFP_KERNEL);
//	if (panel_info == NULL)
//		return 0;

	ret = udc_match_item(lcd->udc, &lcd->current_section, lcd->item);
	UDC_LCD_TRACE("%s () line = %d, ret = %d\n", __func__, __LINE__, ret);
       if ( !ret )
	    return 0;	
	   
	  if(udc_lcd_get_value(LCD_WORK_MODE, &param, 1))
	  	{
			if (param == SPRD_DSI_MODE_CMD)
				panel_info->mode_flags = 0;
			else if (param == SPRD_DSI_MODE_VIDEO_BURST)
				panel_info->mode_flags = MIPI_DSI_MODE_VIDEO |
						   MIPI_DSI_MODE_VIDEO_BURST;
			else if (param == SPRD_DSI_MODE_VIDEO_SYNC_PULSE)
				panel_info->mode_flags = MIPI_DSI_MODE_VIDEO |
						   MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
			else if (param == SPRD_DSI_MODE_VIDEO_SYNC_EVENT)
				panel_info->mode_flags = MIPI_DSI_MODE_VIDEO;
	  	}
	  	else
	  	{
			DRM_ERROR("udc dsi work mode is not found! use video mode\n");
			panel_info->mode_flags = MIPI_DSI_MODE_VIDEO |
					   MIPI_DSI_MODE_VIDEO_BURST;
		}

	  UDC_LCD_TRACE("work_mode 0x%x\n", panel_info->mode_flags);

	  if (udc_lcd_get_value(LCD_READ_POWERMODE, param1, 100) )
	  {
	  	for(i=0;i<100;i++)
	  	{
			if(param1[i] == UDC_LCD_MIPI_FORCE_READ)
			{
				panel_info->esd_check_reg = param1[i+1];
				break;
			}
	  	}
		if(panel_info->esd_check_reg == 0)
			UDC_LCD_TRACE("read powemode error\n");
	  }

	  if ( udc_lcd_get_value(LCD_LAN_NUM, &param, 1) )
		  panel_info->lanes = param;
	  	  UDC_LCD_TRACE("lan_number 0x%x\n", panel_info->lanes);

	  	  panel_info->format = MIPI_DSI_FMT_RGB888;

	  if ( udc_lcd_get_value(LCD_WIDTH_MM, &param, 1) )
		  panel_info->mode.width_mm = param;
	  else
		  panel_info->mode.width_mm = 68;
	      UDC_LCD_TRACE("width_mm %d\n", panel_info->mode.width_mm);
	  
	  if ( udc_lcd_get_value(LCD_HEIGHT_MM, &param, 1) )
		  panel_info->mode.height_mm = param;
	  else
		  panel_info->mode.height_mm = 121;
	      UDC_LCD_TRACE("height_mm %d\n", panel_info->mode.height_mm);
	  if (udc_lcd_get_value(LCD_POWER_MODE, &param, 1) )
	  	{
		  panel_info->esd_check_val= param;
		  if(param)
		  {
		  	panel_info->esd_check_en=1;
			panel_info->esd_check_mode=ESD_MODE_TE_CHECK; //ESD_MODE_TE_CHECK
		  	if(param == 0x9c)
		  	{
			panel_info->esd_check_mode=ESD_MODE_REG_CHECK; //ESD_MODE_REG_CHECK
			}
	
			panel_info->esd_check_period=1000;
		  }
	  	}
	    UDC_LCD_TRACE("panel_info->esd_check_en=%d,panel_info->esd_check_mode=%d,\n", panel_info->esd_check_en,panel_info->esd_check_mode);

		panel_info->use_dcs = false;
		udc_get_power_param(panel);
	    of_get_drm_display_mode(lcd_node, &panel_info->mode, 0,OF_USE_NATIVE_MODE);
		panel_info->mode.vrefresh = drm_mode_vrefresh(&panel_info->mode);
		of_parse_buildin_modes(panel_info, lcd_node);

	panel->base.funcs = &udc_sprd_panel_funcs;
	return 1;

}



udc_lcd* udc_lcd_create(udc_t section_id, udc_sprd_panel  *panel)
{
    udc_lcd* lcd = &g_udc_lcd;
	udc32_t lcd_sec_offset = 0;
	udc_t *value;
    udc_section section;
	lcd->udc = udc_get_udc();

	lcd->item = g_udc_lcd_item;
	
	lcd->panel = panel;
    lcd_sec_offset = udc_get_lcd_offset();
    value = lcd->udc->buffer + lcd_sec_offset;
	section.buffer = value;
    section.id = *value++;
	section.size = *value;
    lcd->current_section = section;
	lcd->current_lcd_offset = lcd_sec_offset;

	
	UDC_LCD_TRACE("[%s] section.id=0x%x,section.size=0x%x,lcd_sec_offset = %d \n",
		__func__, lcd->current_section.id, lcd->current_section.size, lcd->current_lcd_offset );
	   
	
	udc_lcd_config_panel(lcd, section_id);
	
	return 	lcd;
}

