/*
 *drivers/input/touchscreen/ft5x06_ex_fun.c
 *
 *FocalTech IC driver expand function for debug.
 *
 *Copyright (c) 2010  Focal tech Ltd.
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *Note:the error code of EIO is the general error in this file.
 */

//#include "tpd.h"

//#include "tpd_custom_hyn.h"



#include <linux/netdevice.h>
#include <linux/mount.h>
//#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include "hynitron.h"
#include "hynitron_ex_fun.h"
#include "hynitron_ctl.h"

#include <linux/gpio.h>
//#include <soc/sprd/board.h>
//#include "../driver_config.h"

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

static struct i2c_client *client_up;
static struct mutex g_device_mutex;
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern  void cst8xx_ts_reset(void);
extern u8 tp_fm_ver ;


extern struct Upgrade_Info hyn_updateinfo_curr;

#define REG_LEN_1B    1
#define REG_LEN_2B    2


static DEFINE_MUTEX(g_device_mutex);
static struct kobject *k_obj = NULL;
/*
*hyn_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int hyn_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;
	
	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = 1,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 1,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/

int hyn_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;
	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	
	return ret;

}

int hyn_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};
	buf[0] = regaddr;
	buf[1] = regvalue;

	return hyn_i2c_Write(client, buf, sizeof(buf));
}


int hyn_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
	return hyn_i2c_Read(client, &regaddr, 1, regvalue, 1);
}


#if HYN_EN_AUTO_UPDATE

static unsigned char dev_addr;
static unsigned char update_fw_flag;
static unsigned char chip_sumok_flag;
extern unsigned char *p_cst836u_upgrade_firmware;
extern unsigned char  apk_upgrade_flag;

#include <capacitive_hynitron_cst8xx_update.h>

/*****************************************************************/
/*
 *
 */
int hctp_write_bytes(unsigned short reg,unsigned char *buf,unsigned short len,unsigned char reg_len){
	int ret;
    unsigned char mbuf[600];
    if (reg_len == 1){
        mbuf[0] = reg;
        memcpy(mbuf+1,buf,len);
    }else{
        mbuf[0] = reg>>8;
        mbuf[1] = reg;
        memcpy(mbuf+2,buf,len);    
    }
    //ret = hyn_i2c_Write(client, buf, length);
    ret = hyn_i2c_Write(client_up,mbuf,len+reg_len);
	if (ret < 0){
		dev_err(&client_up->dev, "%s i2c write error.\n", __func__);
	}
    return ret;
}
/*
 *
 */
int hctp_read_bytes(unsigned short reg,unsigned char* buf,unsigned short len,unsigned char reg_len){
	int ret;
    unsigned char reg_buf[2];
    if (reg_len == 1){
        reg_buf[0] = reg;
    }else{
        reg_buf[0] = reg>>8;
        reg_buf[1] = reg;
    }
	
    ret = hyn_i2c_Read(client_up,reg_buf,reg_len,buf,len);
	if (ret < 0){
		dev_err(&client_up->dev, "f%s: i2c read error.\n",__func__);
	}
    return ret;
}

/*****************************************************************/
#if HYN_EN_AUTO_UPDATE_CST78xx
/*
 *
 */
static int cst78xx_enter_bootmode(void){
     char retryCnt = 10;

     //cst0xx_reset(1);
     cst8xx_ts_reset();
     mdelay(5);
     while(retryCnt--){
         u8 cmd[3];
         cmd[0] = 0xAA;
         if (-1 == hctp_write_bytes(0xA001,cmd,1,REG_LEN_2B)){  // enter program mode
             mdelay(2); // 4ms
             continue;                   
         }
         if (-1 == hctp_read_bytes(0xA003,cmd,1,REG_LEN_2B)) { // read flag
             mdelay(2); // 4ms
             continue;                           
         }else{
             if (cmd[0] != 0x55){
                 msleep(2); // 4ms
                 continue;
             }else{
                 return 0;
             }
         }
     }
     return -1;
 }
 /*
  *
  */
static int cst78xx_update(u16 startAddr,u16 len,u8* src){
    u16 sum_len;
    u8 cmd[10];
	int ret;

	ret = 0;
	
    if (cst78xx_enter_bootmode() == -1){
       return -1;
    }
    sum_len = 0;
	
 #define PER_LEN	512
    do{
        if (sum_len >= len){
            return -1;
        }
        
        // send address
        cmd[1] = startAddr>>8;
        cmd[0] = startAddr&0xFF;
        hctp_write_bytes(0xA014,cmd,2,REG_LEN_2B);
	#if HYN_IIC_TRANSFER_LIMIT
			{       
				u8 temp_buf[8];
				u16 j,iic_addr;
				iic_addr=0;
				for(j=0; j<128; j++){
					
			    	temp_buf[0] = *((u8*)src+iic_addr+0);
			    	temp_buf[1] = *((u8*)src+iic_addr+1);
					temp_buf[2] = *((u8*)src+iic_addr+2);
					temp_buf[3] = *((u8*)src+iic_addr+3);

			    	hctp_write_bytes((0xA018+iic_addr),(u8* )temp_buf,4,REG_LEN_2B);
					iic_addr+=4;
					if(iic_addr==512) break;
				}

			}
	#else
				hctp_write_bytes(0xA018,src,PER_LEN,REG_LEN_2B);
	#endif 
		
	
        cmd[0] = 0xEE;
        hctp_write_bytes(0xA004,cmd,1,REG_LEN_2B);
		
		if(apk_upgrade_flag==0)
			msleep(300);
		else
			msleep(100);
		
        {
            u8 retrycnt = 50;
            while(retrycnt--){
                cmd[0] = 0;
                hctp_read_bytes(0xA005,cmd,1,REG_LEN_2B);
                if (cmd[0] == 0x55){
                    // success 
                    break;
                }
                msleep(10);
            }

			if(cmd[0]!=0x55)
			{
				ret = -1;
			}
        }
        startAddr += PER_LEN;
        src       += PER_LEN;
        sum_len   += PER_LEN;
    }while(len);
    
    // exit program mode
    cmd[0] = 0x00;
    hctp_write_bytes(0xA003,cmd,1,REG_LEN_2B);

	return ret;
}
/*
 *
 */
static u32 cst78xx_read_checksum(u16 startAddr,u16 len){
    union{
        u32 sum;
        u8 buf[4];
    }checksum;
    char cmd[3];
    char readback[4] = {0};

    if (cst78xx_enter_bootmode() == -1){
       return -1;
    }
    
    cmd[0] = 0;
    if (-1 == hctp_write_bytes(0xA003,cmd,1,REG_LEN_2B)){
        return -1;
    }
    msleep(500);
    
    if (-1 == hctp_read_bytes(0xA000,readback,1,REG_LEN_2B)){
        return -1;   
    }
    if (readback[0] != 1){
        return -1;
    }
    if (-1 == hctp_read_bytes(0xA008,checksum.buf,4,REG_LEN_2B)){
        return -1;
    }
	chip_sumok_flag  = 1;
	
    return checksum.sum;
}
#endif

/*
 *
 */
 
void read_fw_version(struct i2c_client *mclient)
{
	int ret;
    unsigned char reg_buf[2];
	unsigned char buf[6];
	unsigned short fwversion,chipversion;
	unsigned char  fw_module_version,chip_module_version;
	unsigned char  fw_proj_version,chip_proj_version;
	msleep(100);	
	
    reg_buf[0] = 0xA6;
    ret = hyn_i2c_Read(client_up,reg_buf,1,buf,4);
	if (ret < 0){
		dev_err(&client_up->dev, "f%s: i2c read error.\n",__func__);
		return;
	}
	
	fw_proj_version   = *(p_cst836u_upgrade_firmware+0x3BFA+6);
	fw_module_version = *(p_cst836u_upgrade_firmware+0x3BFB+6);
	fwversion   = *(p_cst836u_upgrade_firmware+0x3BFD+6);
	fwversion <<= 8;
	fwversion  += *(p_cst836u_upgrade_firmware+0x3BFC+6);
	
	
	printk("\r\nhyn fwversion: %x\r\n",fwversion);
	
	chipversion  = buf[1];
	chipversion <<= 8;
	chipversion  += buf[0];
	chip_module_version=buf[2];
	chip_proj_version  =buf[3];

	printk("\r\nhyn chipversion: %x\r\n",chipversion);
	printk("\r\nhyn chip_module_version: %x\r\n",chip_module_version);

	if(fw_module_version!=chip_module_version){
	  update_fw_flag = 0;
	  printk("\r\n hyn chip_module_version is not right: %x\r\n",chip_module_version); 
	}else{
		if(chipversion>fwversion)
		{
			update_fw_flag = 0;
			printk("\r\nhyn update_fw_flag: %x\r\n",update_fw_flag);
		}

	}
	

	
	
}

int ctp_hynitron_update(struct i2c_client *mclient){
    unsigned short startAddr;
    unsigned short length;
    unsigned short checksum; 
	unsigned short chipchecksum;
	
	update_fw_flag  = 1;	
	chip_sumok_flag = 0;
	
    client_up = mclient;


	if(apk_upgrade_flag==0)
	read_fw_version(mclient);
	
    dev_addr  = client_up->addr;

#if HYN_EN_AUTO_UPDATE_CST78xx

    client_up->addr = 0x6A;

    if (cst78xx_enter_bootmode() == 0){ 
		
        if(sizeof(app_bin) > 10){
			
            startAddr = *(p_cst836u_upgrade_firmware+1);
			
            length =*(p_cst836u_upgrade_firmware+3);
			
            checksum = *(p_cst836u_upgrade_firmware+5);
			
            startAddr <<= 8; 
			startAddr |= *(p_cst836u_upgrade_firmware+0);

            length <<= 8; 
			length |= *(p_cst836u_upgrade_firmware+2);
			
            checksum <<= 8; 
			checksum |= *(p_cst836u_upgrade_firmware+4);
			
			chipchecksum = cst78xx_read_checksum(startAddr, length);
			
			if(update_fw_flag||chip_sumok_flag==0)
			{	
				printk("\r\nctp_hynitron_update:  low version,  updating!!!\n");
				printk("\r\n CTP cst78xx File, start-0x%04x len-0x%04x fileCheck-0x%04x\r\n",startAddr,length,checksum);
				if(chipchecksum != checksum){				
					cst78xx_update(startAddr, length, (p_cst836u_upgrade_firmware+6));
					length = cst78xx_read_checksum(startAddr, length);
					printk("\r\nCTP cst78xx update %s, checksum-0x%04x",((length==checksum) ? "success" : "fail"),length);
					
					
				}else{
					printk("\r\nCTP cst78xx check pass...");
				}
			}
			else
			{
				printk("\r\nctp_hynitron_update:  high version  not update!!!r\n");
			}
        }
        goto re;
    }
	else
	{
		client_up->addr = dev_addr;
		return -1;
	}
#endif
    
re:
    client_up->addr = dev_addr;
    cst8xx_ts_reset();
	msleep(50);
    hyn_read_reg(client_up, CST8XX_REG_FW_VER, &tp_fm_ver);

    return 0;
}

#endif  //CTP_HYNITRON_EXT==1

#if ANDROID_TOOL_SURPORT   //debug tool support

#if ANDROID_TOOL_SURPORT
static unsigned short g_unnormal_mode = 0;
//static unsigned short g_cst2xx_tx = 16;
//static unsigned short g_cst2xx_rx = 10;
#endif


#define CST8XX_PROC_DIR_NAME	"cst8xx_ts"
#define CST8XX_PROC_FILE_NAME	"cst8xx-update"

static struct proc_dir_entry *g_proc_dir;
static struct proc_dir_entry *g_update_file;
static int CMDIndex = 0;
static int factory_test_flag=0;
#if 0
static struct file *cst2xx_open_fw_file(char *path)
{
	struct file * filp = NULL;
	int ret;
	
	// *old_fs_p = get_fs();
	//set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp)) 
	{
        ret = PTR_ERR(filp);
        return NULL;
    }
    filp->f_op->llseek(filp, 0, 0);
	
    return filp;
}

static void cst2xx_close_fw_file(struct file * filp)
{
//	set_fs(old_fs);
	if(filp)
	    filp_close(filp,NULL);
}

static int cst2xx_read_fw_file(unsigned char *filename, unsigned char *pdata, int *plen)
{
	struct file *fp;
//	mm_segment_t old_fs;
	int size;
	int length;
	int ret = -1;

	if((pdata == NULL) || (strlen(filename) == 0)) 
	{
		printk("file name is null.\n");
		return ret;
	}
	fp = cst2xx_open_fw_file(filename);
	if(fp == NULL) 
	{		
        printk("Open bin file faild.path:%s.\n", filename);
		goto clean;
	}
	
	length = fp->f_op->llseek(fp, 0, SEEK_END); 
	fp->f_op->llseek(fp, 0, 0);	
	size = fp->f_op->read(fp, pdata, length, &fp->f_pos);
	if(size == length) 
	{
    	ret = 0;
    	*plen = length;
	} 	
	else
	{
		printk("read bin file length fail****size:%d*******length:%d .\n", size,length);

	}

clean:
	cst2xx_close_fw_file(fp);
	return ret;
}

#else

static struct file *cst2xx_open_fw_file(char *path, mm_segment_t * old_fs_p)
{
	struct file * filp;
	int ret;
	
	*old_fs_p = get_fs();
	//set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp)) 
	{   
        ret = PTR_ERR(filp);
		printk("cst2xx_open_fw_file:filp_open error.\n");
        return NULL;
    }
    filp->f_op->llseek(filp, 0, 0);
	
    return filp;
}

static void cst2xx_close_fw_file(struct file * filp,mm_segment_t old_fs)
{
	//set_fs(old_fs);
	if(filp)
	    filp_close(filp,NULL);
}

static int cst2xx_read_fw_file(unsigned char *filename, unsigned char *pdata, int *plen)
{
	struct file *fp;
	mm_segment_t old_fs;
	//int size;
	//int length;
	int ret = -1;
	loff_t pos;
	off_t fsize;
	struct inode *inode;
	unsigned long magic;
	
	printk("cst2xx_read_fw_file enter.\n");

	if((pdata == NULL) || (strlen(filename) == 0)) 
		return ret;
		
	fp = cst2xx_open_fw_file(filename, &old_fs);
	if(fp == NULL) 
	{		
        printk("Open bin file faild.path:%s.\n", filename);
		goto clean;
	}

#if 0
	
	length = fp->f_op->llseek(fp, 0, SEEK_END); 
	fp->f_op->llseek(fp, 0, 0);	
	size = fp->f_op->read(fp, pdata, length, &fp->f_pos);
	if(size == length) 
	{
    	ret = 0;
    	*plen = length;
	} 	
	else
	{
		printk("read bin file length fail****size:%d*******length:%d .\n", size,length);

	}

#else 

	if (IS_ERR(fp)) {
		printk("error occured while opening file %s.\n", filename);
		return -EIO;
	}
	inode = fp->f_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;		
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	ret=vfs_read(fp, pdata, fsize, &pos);
	if(ret==fsize){
		printk("vfs_read success.ret:%d.\n",ret);
	}else{
		printk("vfs_read fail.ret:%d.\n",ret);
	}
	filp_close(fp, NULL);
	set_fs(old_fs);
	
	printk("vfs_read done.\n");

#endif

clean:
	cst2xx_close_fw_file(fp, old_fs);
	return ret;
}
#endif
static int cst2xx_apk_fw_dowmload(struct i2c_client *client,
		unsigned char *pdata, int length) 
{ 
	int ret;
	apk_upgrade_flag=1;
	p_cst836u_upgrade_firmware=(unsigned char *)pdata;
	printk("cst2xx_apk_fw_dowmload enter.\n");
	ret=ctp_hynitron_update(client);
	p_cst836u_upgrade_firmware=(unsigned char *)app_bin;
	apk_upgrade_flag=0;
	if (ret < 0)
	{
		printk("online update fw failed.\n");
		return -1;
	}	

	return 0;
}

static ssize_t cst8xx_proc_read_foobar(struct file *page,char __user *user_buf, size_t count, loff_t *data)
{	
	unsigned char buf[100];
	int len = 0;	
	int ret;
	struct i2c_client *client = (struct i2c_client *)PDE_DATA(file_inode(page));

	printk("cst8xx_proc_read_foobar********CMDIndex:%d. \n",CMDIndex);
	
	disable_irq(client->irq);

	if (CMDIndex == 0) {
		sprintf(buf,"Hynitron touchscreen driver 1.0.\n");
		//strcpy(page,buf);	
		len = strlen(buf);
		ret = copy_to_user(user_buf,buf,len);
		
	}
	else if (CMDIndex == 1)
	{   
		buf[0]=0xA6;
	    ret=hyn_i2c_Read(client,(u8 *)buf,1, (u8 *)buf,8);
		if(ret<0){
			printk("cst8xx_proc_read_foobar hyn_i2c_Read fail. \n");
		}else{
			ret = copy_to_user(user_buf,buf,8);
	    	len = 8;
		}

	}
	if(CMDIndex == 2 || CMDIndex == 3||CMDIndex == 4){

		int data_len=80;
		int report_count=0;

		if(CMDIndex == 2)  //read diff
		{
			buf[0] = 0x00;
			buf[1] = 0x07;   
		}
		else if(CMDIndex == 3)         //rawdata
		{  
			buf[0] = 0x00;
			buf[1] = 0x06;
		}
		else if((CMDIndex == 4))        //factory
		{  
			if(factory_test_flag==1){
				buf[0] = 0x00;
				buf[1] = 0x04;
			}else{
				goto END;
			}

		}
		ret = hyn_i2c_Write(client, buf, 2);  
		if(ret < 0) 
		{			
			printk("Write command raw/diff mode failed.error:%d.\n", ret);
			goto END;
		}
 		if(CMDIndex==4)
		mdelay(1500);
		else
		mdelay(10);
	
		for(report_count=0;report_count<16;report_count++){
			unsigned char temp_buf[7];
			ret = i2c_master_recv(client, temp_buf, 6);
			if(ret < 0) 		
			{
				printk("Read raw/diff data failed.error:%d.\n", ret);
				goto END;
			}
			memcpy((unsigned char *)buf+2+report_count*5,(unsigned char *)temp_buf+1,5);
		}	

		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = hyn_i2c_Write(client, buf, 2);  
		if(ret < 0) 
		{			
			printk("Write command raw/diff mode failed.error:%d.\n", ret);
			goto END;
		}

		buf[0] = 4;
		buf[1] = 10;	
    	ret = copy_to_user(user_buf,buf,data_len+2);
    	len = data_len + 2;
        if(CMDIndex == 4)
		cst8xx_ts_reset();
		

	}

END:	
	g_unnormal_mode = 0;
	CMDIndex = 0;	
    enable_irq(client->irq);
	return len;
}

static ssize_t cst8xx_proc_write_foobar(struct file *file, const char __user *buffer,size_t count, loff_t *data)
{
    unsigned char cmd[258];
	unsigned char buf[4];
    unsigned char *pdata = NULL;
	int len;
	int ret;
    int length = 16*1024;
	struct i2c_client *client = (struct i2c_client *)PDE_DATA(file_inode(file));

	
	if (count > 256) 
		len = 256;
	else 
		len = count;

	if (copy_from_user(cmd, buffer, len))  
	{
		printk("copy data from user space failed.\n");
		return -EFAULT;
	}
	
	printk("cst8xx_proc_write_foobar*********cmd:%d*****%d******len:%d .\r\n", cmd[0], cmd[1], len);
	
	if(client==NULL){
		client=client_up;
		printk("client is null.\n");
	}

	if (cmd[0] == 0) 
	{
	    pdata = kzalloc(sizeof(char)*length, GFP_KERNEL);
	    if(pdata == NULL) 
		{
	        printk("zalloc GFP_KERNEL memory fail.\n");
	        return -ENOMEM;
	    }
		
		ret = cst2xx_read_fw_file(&cmd[1], pdata, &length);
	  	if(ret < 0) 
	  	{
			printk("cst2xx_read_fw_file fail.\n");
			if(pdata != NULL) 
			{
				kfree(pdata);
				pdata = NULL;	
			}				
			return -EPERM;
	  	}
		ret = cst2xx_apk_fw_dowmload(client, pdata, length);
	  	if(ret < 0)
	  	{
	        printk("update firmware failed.\n");
			if(pdata != NULL) 
			{
				kfree(pdata);
				pdata = NULL;	
			}	
	        return -EPERM;
		}
		
	}
	else if (cmd[0] == 2) 
	{					
		//cst2xx_touch_release();		
		CMDIndex = cmd[1];			
	}			
	else if (cmd[0] == 3)
	{				
		CMDIndex = 0;
		
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = hyn_i2c_Write(client, buf, 2);  
		if(ret < 0) 
		{			
			printk("Write command raw/diff mode failed.error:%d.\n", ret);		
		}
	}else{

		printk("cmd[0] error:%d.\n",cmd[0]);
	}	


	if(CMDIndex==4)
	factory_test_flag=1;
	else
	factory_test_flag=0;
			
	return count;
}


static const struct file_operations proc_tool_debug_fops = {

	.owner		= THIS_MODULE,
	.read	    = cst8xx_proc_read_foobar,
	.write		= cst8xx_proc_write_foobar, 

};
 int  cst8xx_proc_fs_init(void)
{

	int ret;	
	g_proc_dir = proc_mkdir(CST8XX_PROC_DIR_NAME, NULL);
	if (g_proc_dir == NULL) 
	{
		ret = -ENOMEM;
		goto out;
	}
#if 0
    g_update_file = proc_create(CST8XX_PROC_FILE_NAME, 0777, g_proc_dir,&proc_tool_debug_fops);

   if (g_update_file == NULL)
   {
      ret = -ENOMEM;
      printk("proc_create CST8XX_PROC_FILE_NAME failed.\n");
      goto no_foo;

   }
#else
	g_update_file = proc_create_data(CST8XX_PROC_FILE_NAME, 0777 | S_IFREG, g_proc_dir, &proc_tool_debug_fops, (void *)client_up);
	if (NULL == g_update_file) {
		ret = -ENOMEM;
      	printk("proc_create_data CST8XX_PROC_FILE_NAME failed.\n");
      	goto no_foo;
	}
#endif
 
	return 0;

no_foo:
	remove_proc_entry(CST8XX_PROC_FILE_NAME, g_proc_dir);
out:
	return ret;
}




#ifdef SYSFS_DEBUG 


static ssize_t hyn_tpfwver_show(struct device *dev,	struct device_attribute *attr,char *buf)
{
	ssize_t num_read_chars = 0;
	u8 buf1[10];
	unsigned int chip_version,module_version,project_version,chip_type,checksum;
	
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	memset((u8 *)buf1, 0, 10);
	mutex_lock(&g_device_mutex);


	chip_version=0;
	module_version=0;
	project_version=0;
	chip_type=0;
	checksum=0;
	
	buf1[0]=0xA6;
	if (hyn_i2c_Read(client_up,(u8 *)buf1, 1, (u8 *)buf1,8) < 0)
		num_read_chars = snprintf(buf, 128,"get tp fw version fail!\n");
	else{
		chip_version  =buf1[0];
		chip_version |=buf1[1]<<8;

		module_version=buf1[2];
		project_version=buf1[3];

		chip_type  =buf1[4];
		chip_type |=buf1[5]<<8;

		checksum  =buf1[6];
		checksum |=buf1[7]<<8;
		
		num_read_chars = snprintf(buf, 128, "chip_version: 0x%02X,module_version:0x%02X,project_version:0x%02X,chip_type:0x%02X,checksum:0x%02X .\n",chip_version,module_version, project_version,chip_type,checksum);
	}

	mutex_unlock(&g_device_mutex);

	return num_read_chars;
}

static ssize_t hyn_tpfwver_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}


static ssize_t hyn_tprwreg_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t hyn_tprwreg_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 regaddr = 0xff, regvalue = 0xff;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			printk("please input 2 or 4 character\n");
			goto error_return;
		}
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = kstrtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		printk("%s() - ERROR: The given input was: \"%s\"\n",__func__, buf);
		goto error_return;
	}

	if (2 == num_read_chars) {
		/*read register*/
		regaddr = wmreg;
		if (hyn_read_reg(client_up, regaddr, &regvalue) < 0)
			printk("Could not read the register(0x%02x).\n",regaddr);
		else
			printk("the register(0x%02x) is 0x%02x\n",regaddr, regvalue);
	} else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (hyn_write_reg(client_up, regaddr, regvalue) < 0)
			printk("Could not write the register(0x%02x)\n",regaddr);
		else
			printk("Write 0x%02x into register(0x%02x) successful\n",regvalue, regaddr);
	}

error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t hyn_fwupdate_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/*upgrade from *.i*/
static ssize_t hyn_fwupdate_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int ret;
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	printk("hyn_fwupdate_store enter.\n");
	mutex_lock(&g_device_mutex);
	disable_irq(client_up->irq);
	p_cst836u_upgrade_firmware=(unsigned char *)app_bin;
	apk_upgrade_flag=1;
	ret=ctp_hynitron_update(client_up);
	if(ret<0){
		printk("hyn_fwupdate_store ctp_hynitron_update fail.\n");
	}
	enable_irq(client_up->irq);
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t hyn_fwupgradeapp_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}


/*upgrade from app.bin*/
static ssize_t hyn_fwupgradeapp_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	char fwname[256];
	int ret;
	unsigned char *pdata = NULL;
	int length = 16*1024;
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);


	printk("hyn_fwupgradeapp_store enter.\n");

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "/sdcard/%s", buf);	
	fwname[count-1+8] = '\0';
	
	printk("fwname:%s.\n",fwname);
	pdata = kzalloc(sizeof(char)*length, GFP_KERNEL);
    if(pdata == NULL) 
	{
        printk("hyn_fwupgradeapp_store GFP_KERNEL memory fail.\n");
        return -ENOMEM;
    }

	mutex_lock(&g_device_mutex);
	disable_irq(client_up->irq);

	ret = cst2xx_read_fw_file(fwname, pdata, &length);
  	if(ret < 0) 
  	{
		printk("cst2xx_read_fw_file fail.\n");
		if(pdata != NULL) 
		{
			kfree(pdata);
			pdata = NULL;	
		}			
  	}else{

		ret = cst2xx_apk_fw_dowmload(client_up, pdata, length);
	  	if(ret < 0)
	  	{
	        printk("cst2xx_apk_fw_dowmload failed.\n");
			if(pdata != NULL) 
			{
				kfree(pdata);
				pdata = NULL;	
			}	
		}
	}

	enable_irq(client_up->irq);
	mutex_unlock(&g_device_mutex);
	
	printk("hyn_fwupgradeapp_store exit.\n");
	
	return count;
}



/*sysfs */
/*get the fw version
*example:cat hyntpfwver
*/
static DEVICE_ATTR(hyntpfwver, S_IRUGO | S_IWUSR, hyn_tpfwver_show,
			hyn_tpfwver_store);

/*upgrade from *.i
*example: echo 1 > hynfwupdate
*/
static DEVICE_ATTR(hynfwupdate, S_IRUGO | S_IWUSR, hyn_fwupdate_show,
			hyn_fwupdate_store);

/*read and write register
*read example: echo 88 > hyntprwreg ---read register 0x88
*write example:echo 8807 > hyntprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(hyntprwreg, S_IRUGO | S_IWUSR, hyn_tprwreg_show,
			hyn_tprwreg_store);


/*upgrade from app.bin
*example:echo "*_app.bin" > hynfwupgradeapp
*/
static DEVICE_ATTR(hynfwupgradeapp, S_IRUGO | S_IWUSR, hyn_fwupgradeapp_show,
			hyn_fwupgradeapp_store);

/*add your attr in here*/
static struct attribute *hyn_attributes[] = {
	&dev_attr_hyntpfwver.attr,
	&dev_attr_hynfwupdate.attr,
	&dev_attr_hyntprwreg.attr,
	&dev_attr_hynfwupgradeapp.attr,
	NULL
};

static struct attribute_group hyn_attribute_group = {
	.attrs = hyn_attributes
};
/*create sysfs for debug*/

int hyn_create_sysfs(struct i2c_client *client)
{
	int err;
	client_up=client;
  	if ((k_obj = kobject_create_and_add("hynitron_debug", NULL)) == NULL ) {
     	printk("hynitron_debug sys node create error.\n"); 	
    }
	err = sysfs_create_group(k_obj, &hyn_attribute_group);
	if (0 != err) {
		printk("%s() - ERROR: sysfs_create_group() failed.\n",__func__);
		sysfs_remove_group(k_obj, &hyn_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		printk("cstt836u:%s() - sysfs_create_group() succeeded.\n",__func__);
	}
	return err;
}

void hyn_release_sysfs(struct i2c_client *client)
{
	sysfs_remove_group(k_obj, &hyn_attribute_group);
	mutex_destroy(&g_device_mutex);		
}
#endif 

 
#endif





