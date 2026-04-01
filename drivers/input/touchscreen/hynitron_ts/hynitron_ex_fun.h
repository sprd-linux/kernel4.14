#ifndef __HYNITRON_EX_FUN_H__
#define __HYNITRON_EX_FUN_H__

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#ifndef CONFIG_64BIT
//#include <mach/irqs.h>
#endif
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>

#define HYN_UPGRADE_AA					0xAA
#define HYN_UPGRADE_55 					0x55

#define    BL_VERSION_LZ4        0
#define    BL_VERSION_Z7        1
#define    BL_VERSION_GZF        2


/*****************************************************************************/
#define HYN_PAGE_SIZE                                        128
#define HYN_PACKET_LENGTH        		       128
#define HYN_SETTING_BUF_LEN      		128
#define HYN_DMA_BUF_SIZE 				1024

#define HYN_UPGRADE_LOOP				30

#define HYN_FACTORYMODE_VALUE			0x40
#define HYN_WORKMODE_VALUE			0x00

#define HYN_IIC_TRANSFER_LIMIT 1

#define HYN_EN_AUTO_UPDATE                   	          1
#define HYN_EN_AUTO_UPDATE_CST78xx          		      1
#define ANDROID_TOOL_SURPORT		      				  1
#define SYSFS_DEBUG


/*
*hyn_write_reg- write register
*@client: handle of i2c
*@regaddr: register address
*@regvalue: register value
*
*/

int hyn_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen);

int hyn_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);

int hyn_write_reg(struct i2c_client * client,u8 regaddr, u8 regvalue);

int hyn_read_reg(struct i2c_client * client,u8 regaddr, u8 *regvalue);
int ctp_hynitron_update(struct i2c_client *mclient);
int  cst8xx_proc_fs_init(void);
int hyn_create_sysfs(struct i2c_client *client);
void hyn_release_sysfs(struct i2c_client *client);

#endif
