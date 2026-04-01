#ifndef __UDC_H__
#define __UDC_H__



#include <linux/version.h>

#ifdef CONFIG_UDC_VER_2_3_5
#include <linux/kernel.h>             
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/smp_lock.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/input-polldev.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/i2c-dev.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/wakelock.h>
#include <linux/initrd.h>
#include <linux/android_pmem.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/ioctl.h>
#include <linux/leds.h>
#include <mach/bits.h>
#include <mach/clock_common.h>
#include <mach/lcd.h>
#include <mach/io.h>
#include <mach/ldo.h>
#include <mach/mfp.h>
#include <mach/eic.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/flash.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/adi_hal_internal.h>
#include <mach/regs_ana.h>
#include <mach/regs_cpc.h>
#include <mach/clock_sc8800g.h>
#include <mach/sensor_drv.h>
#include <mach/jpeg_exif_header_k.h>
#include <mach/regs_global.h>
#include <mach/chg_drvapi.h>
#include <mach/adc_drvapi.h>

#elif defined CONFIG_UDC_VER_5_1_3 
#include <linux/kernel.h>             
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/console.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/firmware.h>
#include <linux/input-polldev.h>
#include <linux/string.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
//#include <soc/sprd/hardware.h>
//#include <mach/gpio.h>
//#include <soc/sprd/adi.h>
//#include <soc/sprd/globalregs.h>
//#include <mach/irqs.h>
//#include <soc/sprd/pinmap.h>
//#include <soc/sprd/regulator.h>
#include <asm/uaccess.h>





//#include <../drivers/staging/android/ion/ion.h>

//#include <../drivers/staging/android/uapi/ion.h>

//#include <../drivers/video/adf/sprd/core/include/sprd_adf_common.h>

//#include <../drivers/video/adf/sprd/drv/sprd_interface.h>


//#include <../drivers/video/adf/sprd/drv/sprdfb_dispc_reg.h>

#else
#include <linux/kernel.h>             
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/console.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/firmware.h>
#include <linux/input-polldev.h>
#include <linux/string.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/adi.h>
#include <mach/globalregs.h>
#include <mach/irqs.h>
#include <mach/pinmap.h>
#include <mach/regulator.h>
#include <asm/uaccess.h>

#ifdef CONFIG_FB_SC8825
#include <../drivers/video/sc8825/sprdfb_panel.h>
#include <../drivers/video/sc8825/sprdfb_dispc_reg.h>
#include <../drivers/video/sc8825/sprdfb_lcdc_reg.h>
#elif  defined CONFIG_FB_SC8810
#include <../drivers/video/spreadtrum/sprdfb.h>
#elif defined CONFIG_FB_SC7710
#include <../drivers/video/sc8825/sprdfb_panel.h>
#include <../drivers/video/sc8825/sprdfb_lcdc_reg.h>
#endif

//#ifndef CONFIG_UDC_SC8825
//#include <../drivers/media/video/sprd_dcam/common/sensor.h>
//#include <../drivers/media/video/sprd_dcam/common/sensor_drv.h>
//#else
//#include <../drivers/media/video/sprd_sensor/sensor_drv_k.h>
//#endif

//#include <../drivers/media/video/sprd_dcam/common/jpeg_exif_header_k.h>
#endif





#include <../drivers/udc/udc_base.h>

#ifdef CONFIG_UDC_PINMAP
#include <../drivers/udc/udc_pinmap/udc_pinmap.h>
#endif

#ifdef CONFIG_UDC_GPIO
#include <../drivers/udc/udc_gpio/udc_gpio.h>
#endif

#ifdef CONFIG_UDC_REGULATOR
#include <../drivers/udc/udc_regulator/udc_regulator.h>
#endif

#ifdef CONFIG_UDC_I2C
#include <../drivers/udc/udc_i2c/udc_i2c.h>
#endif

#ifdef CONFIG_UDC_BACKLIGHT
#include <../drivers/udc/udc_backlight/udc_backlight.h>
#endif

#ifdef CONFIG_UDC_LCD
#include <../drivers/udc/udc_lcd/udc_lcd.h>
#endif

#ifdef CONFIG_UDC_TP
#include <../drivers/udc/udc_tp/udc_tp.h>
#endif

#ifdef CONFIG_UDC_CAMERA

#ifdef CONFIG_UDC_SC8825
#include <../drivers/udc/udc_camera/udc_camera_sensor.h>
#endif


#include <../drivers/udc/udc_camera/udc_camera_common.h>
#include <../drivers/udc/udc_camera/udc_camera.h>
#endif


#ifdef CONFIG_UDC_FM
#include <../drivers/udc/udc_fm/udc_fm.h>
#endif

#ifdef CONFIG_UDC_GSENSOR
#include <../drivers/udc/udc_gsensor/udc_gsensor.h>
#endif

#ifdef CONFIG_UDC_MSENSOR
#include <../drivers/udc/udc_msensor/udc_msensor.h>
#endif

#ifdef CONFIG_UDC_PSENSOR
#include <../drivers/udc/udc_psensor/udc_psensor.h>
#endif

#ifdef CONFIG_UDC_LSENSOR
#include <../drivers/udc/udc_lsensor/udc_lsensor.h>
#endif


#endif

 
