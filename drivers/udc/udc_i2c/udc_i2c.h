/* drivers/udc/udc_i2c/udc_i2c.h
 *
 *
 * Copyright (C) 2013 Spreadtrum
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
 
#ifndef __UDC_I2C_H__
#define __UDC_I2C_H__

//#define  UDC_I2C_DEBUG

#ifdef UDC_I2C_DEBUG
#define UDC_I2C_TRACE printk
#else
#define UDC_I2C_TRACE(...)
#endif

typedef enum _rc {
    RC_SUCCESS = 0,   
    RC_ERROR,               // general error
    RC_PROTOCOL,            // protocol error 
    RC_SYNTAX,              // syntax error
    RC_EOF,                 // end of file
    RC_IMPLEMENTATION,      // not implemented yet
    RC_SOFTWARE,            // error in software
    RC_CONFIG,              // configuration error
    RC_MEMORY,              // memory allocation error 
    RC_CONTENTION,          // contention error
    RC_NOTFOUND,            // not found
    RC_DISKSPACE,           // out of disk space
    RC_SHUTDOWN,            // service shutdown
    RC_EXPIRED,             // expired
    RC_TIMEOUT,             // timeout
    RC_INVALID_PARAMETER,   // invalid parameter
    RC_LAST                 // all errors are less than this
} RC;

//$--_Ec-----------------------------------------------------------------------
//  Available exit codes.
// ----------------------------------------------------------------------------
typedef enum _ec {
    EC_SUCCESS = 0,   
    EC_ERROR,           // general error
    EC_LAST             // all errors are less than this
} EC;

// ----------------------------------------------------------------------------
//  Macros.
// ----------------------------------------------------------------------------

#define RC_SUCCEEDED(x) \
    ((x) == RC_SUCCESS)

#define RC_FAILED(x) \
    ((x) != RC_SUCCESS)

#define EC_SUCCEEDED(x) \
    ((x) == EC_SUCCESS)

#define EC_FAILED(x) \
    ((x) != EC_SUCCESS)


#define MAX_UDC_I2C_LEN 2
#define MAX_MSG_BUFFER_SIZE 100

typedef struct _udc_i2c_block{
	udc32_t length;
       udc32_t pos;
	udc8_t* buffer;
}udc_i2c_block;

typedef struct _udc_i2c_cmd_wait{
       udc_i2c_block cmd;
	udc8_t count;
	udc8_t time;
	ulong start_jiffies;
}udc_i2c_cmd_wait;


typedef struct _udc_i2c_extend_cmd{
       udc_t code;
       udc_i2c_cmd_wait wait;
}udc_i2c_extend_cmd;


typedef struct _udc_i2c_di{
	udc_t   bits;
       udc_t   len;
   	udc8_t* buffer;
}udc_i2c_di;


typedef struct _udc_i2c_item{
	udc_i2c_block cmd;
	udc_i2c_block read;
	udc_i2c_block write;
	
	udc8_t salve;
	udc_i2c_di reg;
	udc_i2c_di data;
	udc_i2c_di mask;
}udc_i2c_item;


typedef struct _udc_i2c_datasource{
       udc_i2c_block cmd;
	udc_i2c_block read;
	udc_i2c_block write;
	udc8_t  rx_buffer[MAX_MSG_BUFFER_SIZE];
	udc8_t  tx_buffer[MAX_MSG_BUFFER_SIZE]; 
	udc_i2c_item item;
	udc_i2c_extend_cmd extend_cmd;
}udc_i2c_datasource;



typedef struct _udc_i2c{
	udc_s* udc;
       udc_i2c_info* info;
	udc_i2c_datasource* ds;   
}udc_i2c;



extern int udc_i2c_execute(udc_i2c* i2c, udc8_t * buffer, udc32_t count);
//old interface
extern int udc_i2c_rxdata(udc_s* udc, udc_i2c_info* i2c_info, udc_t reg_addr,udc8_t *rxdata, int length);
extern int udc_i2c_txdata(udc_s* udc, udc_i2c_info* i2c_info, udc_t reg_addr,udc_t *txdata, int length);
extern int udc_i2c_do(udc_s* udc, udc_i2c_info* i2c_info,udc_t * reg_values, udc_t reg_values_count);



#endif


