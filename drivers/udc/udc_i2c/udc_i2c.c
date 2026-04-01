/* drivers/udc/udc_i2c/udc_i2c.c
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

#include <linux/udc.h>


static int udc_i2c_to_handle_endian_mask(udc_i2c_item* item, udc8_t* buffer )
{
     udc8_t* p = NULL;
     udc8_t bit16_low = 0;
     udc8_t bit16_high = 0;
     udc8_t bit8 = 0;	 
     udc32_t i = 0;
     udc8_t*mask;
	 
     p = buffer;
     mask = item->mask.buffer;
	
    //8bits data
    if (8 == item->data.bits   )
    {
           if ( item->mask.len )
           {
		    for( i = 0; i < item->data.len; i += 1 )
		    {
			      bit8 = *(p) & mask[0];
			     *p = bit8;
			      p ++;
		    }	

           }
	    else
	    {
	    	   for( i = 0; i < item->data.len; i += 1 )
	    	   {
			   bit8 = *p;
			   *p = bit8;
		           p ++;
	    	    }	
	    }
   }
   else if (16 == item->data.bits )
    {
           if ( item->mask.len )
           {
		    for( i = 0; i < item->data.len; i += 2 )
		    {
		            bit16_high = *p & mask[1];
	                   bit16_low = *( p + 1) & mask[0];
					
			    *p = bit16_low;
			    *(p + 1) = bit16_high;
				   
			     p +=2;
		   }		 
	    }
	    else
	    {
		    for( i = 0; i < item->data.len; i += 2 )
		    {
		            bit16_high = *p;
	                   bit16_low = *( p + 1);
		           		
			    *p = bit16_low;
                         *(p + 1) = bit16_high;
			     p +=2;
		    }		 
	    }
    }	

    return RC_SUCCESS;	 
}

static int udc_i2c_read_data(udc_i2c* i2c, udc_i2c_item* item)
{
       int     ret              = RC_SUCCESS;
	struct i2c_client	*client = i2c->info->i2c_client ;
	struct i2c_msg msgs[2] = {0};
       udc8_t reg[2];
	int i, count;
	udc8_t* buffer = NULL;
		
    	UDC_I2C_TRACE("%s: START!!\r\n", __func__);


       count = MIN(MAX_MSG_BUFFER_SIZE, item->data.len);

       if (item->data.len >  MAX_MSG_BUFFER_SIZE)
       {
	       buffer = (udc8_t* )kmalloc(item->data.len, GFP_KERNEL);
		   
		if  (NULL == buffer )
			return RC_MEMORY;

	 }
	else
	{
		buffer = i2c->ds->rx_buffer;
	}


       if  ( 0 == item->reg.len )
       {

              msgs[0].addr = client->addr;
		msgs[0].flags = client->flags & I2C_M_TEN;
		msgs[0].flags |= I2C_M_RD;
		msgs[0].len = item->data.len;
              msgs[0].buf = buffer;		
	                            
		i2c_transfer(client->adapter, &msgs[0], 1);
       }
	else
	{
		//change LSB to MSB  
		if ( 2 == item->reg.len )
		{
			reg[0] = item->reg.buffer[1];
			reg[1] = item->reg.buffer[0];
	       }
		else
		{
			reg[0] = item->reg.buffer[0];
		}

		msgs[0].addr = client->addr;
		msgs[0].flags = 0;
		msgs[0].len = item->reg.len;
		msgs[0].buf = reg;			

	       msgs[1].addr = client->addr;
		msgs[1].flags = I2C_M_RD;
		msgs[1].len = item->data.len;
		msgs[1].buf = buffer;		
			   
		i2c_transfer(client->adapter, &msgs[0], 2);

	}

#ifdef UDC_I2C_DEBUG
    UDC_I2C_TRACE("%s: client->addr = 0x%x, reg.len = %d, REG: ",  __func__, client->addr, item->reg.len);
	for ( i = 0; i < item->reg.len; i++ )
	{
		  UDC_I2C_TRACE("0x%x, ", reg[i]);
	}
	UDC_I2C_TRACE("\r\n");
	 
	UDC_I2C_TRACE("%s: data.len = 0x%x, MSB data: ", __func__, item->data.len);
	for ( i = 0; i < item->data.len; i++ )
       {
		 UDC_I2C_TRACE("0x%x, ", buffer[i]);
	}
	UDC_I2C_TRACE("\r\n");
#endif

	udc_i2c_to_handle_endian_mask(item, buffer);

      // 记住每次读出的数据
       for ( i = 0; i < count; i++ )
       {
              i2c->ds->rx_buffer[i] =  buffer[i]; 
	}

      //返回每次读到的数据
	if ( NULL != item->read.buffer)
	{
		for ( i = 0; i < item->data.len; i++ )
	       {
	              item->read.buffer[i] = buffer[i]; 
		}
	}
	
#ifdef UDC_I2C_DEBUG

       UDC_I2C_TRACE("%s: data.len = 0x%x, LSB data: ", __func__, item->data.len);
	for ( i = 0; i < item->data.len; i++ )
       {
		UDC_I2C_TRACE("0x%x, ", buffer[i]);
	}
	UDC_I2C_TRACE("\r\n");
	UDC_I2C_TRACE("%s: END!!\r\n", __func__);
#endif


     if (item->data.len >  MAX_MSG_BUFFER_SIZE)
     {
	     if ( buffer )
	     {
		     kfree(buffer);
		     buffer = NULL;
	      }
     } 
	return ret;

}



static int udc_i2c_write_data(udc_i2c* i2c, udc_i2c_item* item)
{
       int     ret              = RC_SUCCESS;
	int i, j, index;
	struct i2c_client	*client = i2c->info->i2c_client ;
	struct i2c_msg msgs[1] = {0};
	udc8_t* buffer = NULL;
	udc8_t* data = NULL;
	udc8_t* data_source = NULL;
	udc32_t count;
	   
#ifdef UDC_I2C_DEBUG
	
       UDC_I2C_TRACE("%s: START!!\r\n", __func__);

       UDC_I2C_TRACE("%s: reg.len = %d, REG: ", __func__, item->reg.len);

       for ( i = 0; i < item->reg.len; i++ )
       {
		 UDC_I2C_TRACE("0x%x, ", item->reg.buffer[i]);
	}
	UDC_I2C_TRACE("\r\n");   

       UDC_I2C_TRACE("%s: data.len = %d, DATA: ", __func__, item->data.len);
	for ( i = 0; i < item->data.len; i++ )
       {
		 UDC_I2C_TRACE("0x%x,", item->data.buffer[i]);
	}
	UDC_I2C_TRACE("\r\n");   
 
#endif	  

       if (item->reg.len + item->data.len >  MAX_MSG_BUFFER_SIZE)
       {
	       buffer = (udc8_t* )kmalloc(item->reg.len + item->data.len, GFP_KERNEL);
		   
		if  (NULL == buffer )
			return RC_MEMORY;

	 }
	else
	{
		buffer = i2c->ds->tx_buffer;
	}
	
       memset(buffer, 0x0, item->reg.len + item->data.len);
      
		
       if ( 1 == item->reg.len )
       {
		buffer[0] = item->reg.buffer[0];
		data = &buffer[1];
	}//change LSB to MSB  
	else if ( 2 == item->reg.len )
	{
		buffer[0] = item->reg.buffer[1];
		buffer[1] = item->reg.buffer[0];
              data = &buffer[2];
	}
	else
	{
		data = &buffer[0];
	}
	   
	data_source = item->data.buffer;

	UDC_I2C_TRACE("%s: data.buffer = 0x%x, buffer = 0x%x\r\n", __func__,  item->data.buffer,  item->write.buffer);

	   
	if ( item->data.len )
       {
              //数据来自buffer
		if ( item->data.buffer == item->write.buffer )
		{	
			
			if ( item->data.bits == 8)
			{
                            count = item->data.len;
				for ( i = 0; i < count; i++ )
				{
					data[i] = data_source[ i ];
					
				}
			}
			else if ( item->data.bits == 16)
			{
			 	count = item->data.len / 2;
				 //change LSB to MSB  
			       for ( i = 0; i < count; i++ )
				{
				       index = 2 * i;
					data[index] = data_source[ index + 1];
					data[index + 1] = data_source[ index];
				}
			}
			
		}// 数据来自配置文件
		else
		{
			if ( item->data.bits == 8)
			{
				count = item->data.len;
				
				 for ( i = 0; i < count; i++ )
				{
					data[i] = data_source[ i * 2 ];
				}
			}
			else if ( item->data.bits == 16)
			{
			       count = item->data.len / 2;

				 //change LSB to MSB  
			       for ( i = 0; i < count; i++ )
				{
				       index = 2 * i;
					data[index] = data_source[ index + 1];
					data[index + 1] = data_source[ index];
				}
			}
			

		}
			 
	}

  
	 
     msgs[0].addr = client->addr;
     msgs[0].flags = 0;
     msgs[0].len = item->reg.len+ item->data.len;
     msgs[0].buf = buffer;		
	                            
     i2c_transfer(client->adapter, &msgs[0], 1);

#ifdef UDC_I2C_DEBUG
		
	UDC_I2C_TRACE("%s: msgs[0].len = %d, MSG: ", __func__, msgs[0].len);
       for ( i = 0; i < msgs[0].len; i++ )
       {
		 UDC_I2C_TRACE("0x%x, ", buffer[i]);
	}
	UDC_I2C_TRACE("\r\n");      
       UDC_I2C_TRACE("%s: END!!\r\n\r\n", __func__);
#endif	
      
     if (item->reg.len + item->data.len >  MAX_MSG_BUFFER_SIZE)
     {
	     if ( buffer )
	     {
		     kfree(buffer);
		     buffer = NULL;
	      }
     } 
     return ret;

}


static int udc_i2c_write_data_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       int i;
	udc8_t* buffer;    
	udc8_t* rx_buffer;
	udc_i2c_item read_item = {0};


       UDC_I2C_TRACE("%s: START!!\r\n", __func__);


        if (item->data.len > 0)
       {
	       buffer = (udc8_t* )kmalloc(item->data.len, GFP_KERNEL);
		   
		if  (NULL == buffer )
			return RC_MEMORY;

		memset(buffer, 0x0, item->data.len);
       
	}

	rx_buffer =  i2c->ds->rx_buffer;
	   
	
       UDC_I2C_TRACE("%s: data.len = %d:\r\n ",  __func__,  item->data.len);
		
	for ( i = 0; i < item->data.len; i++ )
       {
             UDC_I2C_TRACE("R=0x%02x, D=0x%02x, M=0x%02x",  rx_buffer[i], item->data.buffer[i] , item->mask.buffer[i]);

		buffer[i] =  item->data.buffer[i];
     	       rx_buffer[i] &= ~ item->mask.buffer[i];
		buffer[i] |= rx_buffer[i]; 

		UDC_I2C_TRACE("W: 0x%x, \r\n", buffer[i]);
	}
       UDC_I2C_TRACE("\r\n");
	item->write.buffer = buffer;
	item->data.buffer = buffer;
	
	udc_i2c_write_data(i2c, item);

       if ( item->data.len > 0)
       {
	     if ( buffer )
	     {
		     kfree(buffer);
		     buffer = NULL;
	      }
      } 	
      UDC_I2C_TRACE("%s: END!!\r\n\r\n", __func__);
	
}

static int udc_i2c_reg00_data08_read(udc_i2c* i2c, udc_i2c_item* item)
{
	item->data.bits = 8;
	item->data.len = 1;
	
	item->cmd.length = 2;
	item->read.length = 1;
	return udc_i2c_read_data(i2c, item);
}


static int udc_i2c_reg00_data08_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
	item->data.bits = 8;
	item->data.len = 1;
	
       item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[2];

	item->cmd.length = 4;
	item->read.length = 1;
	
	return udc_i2c_read_data(i2c, item);
}

static int udc_i2c_reg00_data08_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];
	   
	item->data.bits = 8;
	item->data.len =  count;
	
	
       item->cmd.length = 4;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);
}


static int udc_i2c_reg00_data08_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
        udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
		
       item->data.bits = 8;
	item->data.len = count;
	
      
	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[2];
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}

static int udc_i2c_reg00_data16_read(udc_i2c* i2c, udc_i2c_item* item)
{
	item->data.bits = 16;
	item->data.len = 2;
	

      	item->cmd.length = 2;
	item->read.length = 2;

	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg00_data16_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
	item->data.bits = 16;
	item->data.len = 2;
	

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[2];
	
	item->cmd.length = 4;
	item->read.length = 2;

	return udc_i2c_read_data(i2c, item);
	
	
}

static int udc_i2c_reg00_data16_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{     
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];
	   
	item->data.bits = 16;
	item->data.len = 2 * count;
	
      
	item->cmd.length = 4;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg00_data16_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
	   
       item->data.bits = 16;
	item->data.len = 2 * count;
	
      
	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[2];
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}

static int udc_i2c_reg08_data08_read(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	
		
	item->cmd.length = 4;
	item->read.length = 1;

	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg08_data08_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];

	item->data.bits = 8;
	item->data.len = 1;
	

	item->mask.bits = 8;
       item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;
	item->read.length = 1;
	
	return udc_i2c_read_data(i2c, item);
	
}

static int udc_i2c_reg08_data08_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{
        udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
	    
	item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 8;
	item->data.len = count;
	
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}



static int udc_i2c_reg08_data08_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[7] << 8) | item->cmd.buffer[6] ;
	   
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 8;
	item->data.len = count ;
	

       item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 8;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);


}


static int udc_i2c_reg08_data16_read(udc_i2c* i2c, udc_i2c_item* item)
{
  	item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	


	item->cmd.length = 4;
       item->read.length = 2;
		
	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg08_data16_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
	item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];

	item->data.bits = 16;
	item->data.len = 2;
	
	
	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
      	item->cmd.length = 6;
	item->read.length = 2;
	
	return udc_i2c_read_data(i2c, item);
}

static int udc_i2c_reg08_data16_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
	   
	item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 16;
	item->data.len = 2 * count;
	
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}



static int udc_i2c_reg08_data16_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[7] << 8) | item->cmd.buffer[6] ;
	   
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 16;
	item->data.len = 2 * count;
	

       item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 8;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}


static int udc_i2c_reg16_data08_read(udc_i2c* i2c, udc_i2c_item* item)
{
	item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	
		
       item->cmd.length = 4;
	item->read.length = 1;
	
	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg16_data08_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
	item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];

	item->data.bits = 8;
	item->data.len = 1;
	

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;
	item->read.length =1;
	
	return udc_i2c_read_data(i2c, item);

}

static int udc_i2c_reg16_data08_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
	   
	item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 8;
	item->data.len = count;
	
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);
}



static int udc_i2c_reg16_data08_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[7] << 8) | item->cmd.buffer[6] ;
	   
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 8;
	item->data.len = count;
	

       item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 8;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}


static int udc_i2c_reg16_data16_read(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	
		
       item->cmd.length = 4;
	item->read.length = 2;
	
	return udc_i2c_read_data(i2c, item);

}



static int udc_i2c_reg16_data16_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];

	item->data.bits = 16;
	item->data.len = 2;
	

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;
	item->read.length =2;
	
	return udc_i2c_read_data(i2c, item);

}

static int udc_i2c_reg16_data16_multi_read(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4] ;
	   
	item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 16;
	item->data.len = 2 * count;
	
	
       item->cmd.length = 6;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);

}


static int udc_i2c_reg16_data16_multi_read_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[7] << 8) | item->cmd.buffer[6] ;
	   
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
			
	item->data.bits = 16;
	item->data.len = 2 * count;
	

       item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 8;
	item->read.length = item->data.len;

	return udc_i2c_read_data(i2c, item);
	
}


static int udc_i2c_reg00_data08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[2];
	
	item->cmd.length = 4;

	return udc_i2c_write_data(i2c, item);
}


static int udc_i2c_reg00_data08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[2];

	item->mask.bits = 8;
       item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];

	item->cmd.length = 6;
	
	return udc_i2c_write_data_mask(i2c, item);
}

static int udc_i2c_reg00_data08_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{ 
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];

       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 4 + 2 * count;

	 return udc_i2c_write_data(i2c, item);

}


static int udc_i2c_reg00_data08_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];

       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 8;
	item->mask.len =  count;
	item->mask.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 4 + 4 * count;

	 return udc_i2c_write_data_mask(i2c, item);
}

static int udc_i2c_reg00_buffer08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = item->write.buffer;
	
	item->cmd.length = 2;
	item->write.length = 1;

	return udc_i2c_write_data(i2c, item);
}

static int udc_i2c_reg00_buffer08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
      	
       item->data.bits = 8;
	item->data.len =  1;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[2];
	
       item->cmd.length = 4;
	item->write.length = 1;

	 return udc_i2c_write_data_mask(i2c, item);
}


static int udc_i2c_reg00_data16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[2];
	
	item->cmd.length = 4;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg00_data16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[2];

	item->mask.bits = 16;
       item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];

	item->cmd.length = 6;
	
	return udc_i2c_write_data_mask(i2c, item);
}


static int udc_i2c_reg00_data16_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];

       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 4 + 2 * count;

	 return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg00_data16_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[3] << 8) | item->cmd.buffer[2];

       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 16;
	item->mask.len =  2 * count;
	item->mask.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 4 + 4 * count;

	 return udc_i2c_write_data_mask(i2c, item);
}


static int udc_i2c_reg00_buffer16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = item->write.buffer;
	
	item->cmd.length = 2;
	item->write.length = 1;

	return udc_i2c_write_data(i2c, item);
}

static int udc_i2c_reg00_buffer16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
      	
       item->data.bits = 16;
	item->data.len =  2;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[2];
	
       item->cmd.length = 4;
	item->write.length = 2;

	 return udc_i2c_write_data_mask(i2c, item);
}



static int udc_i2c_reg08_data08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;

	return udc_i2c_write_data(i2c, item);
}



static int udc_i2c_reg08_data08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[6];
	
	item->cmd.length = 8;

	return udc_i2c_write_data_mask(i2c, item);

}


static int udc_i2c_reg08_data08_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 6 + 2 * count;

	 return udc_i2c_write_data(i2c, item);
}



static int udc_i2c_reg08_data08_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
 	udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[6];

	item->mask.bits = 8;
	item->mask.len =  count;
	item->mask.buffer = &item->cmd.buffer[8];
	
       item->cmd.length = 6 + 4 * count;

	 return udc_i2c_write_data_mask(i2c, item);
	 
}

static int udc_i2c_reg08_buffer08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = item->write.buffer;
	
	item->cmd.length = 4;
	item->write.length = 1;

	return udc_i2c_write_data(i2c, item);
}

static int udc_i2c_reg08_buffer08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  1;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 6;
	item->write.length = 1;

	 return udc_i2c_write_data_mask(i2c, item);
}


static int udc_i2c_reg08_data16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg08_data16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[6];
	
	item->cmd.length = 8;

	return udc_i2c_write_data_mask(i2c, item);

}

static int udc_i2c_reg08_data16_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 6 + 2 * count;

	return udc_i2c_write_data(i2c, item);
	 
}



static int udc_i2c_reg08_data16_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
 	udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[6];

	item->mask.bits = 16;
	item->mask.len =  2 * count;
	item->mask.buffer = &item->cmd.buffer[8];
	
       item->cmd.length = 6 + 4 * count;

	return udc_i2c_write_data_mask(i2c, item);
	
}

static int udc_i2c_reg08_buffer16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len = 1;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = item->write.buffer;

	
	item->cmd.length = 4;
	item->write.length = 2;

	return udc_i2c_write_data(i2c, item);


}

static int udc_i2c_reg08_buffer16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 8;
	item->reg.len =  1;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2 ;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 6;
	item->write.length = 2;

	 return udc_i2c_write_data_mask(i2c, item);

}


static int udc_i2c_reg16_data08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg16_data08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[6];
	
	item->cmd.length = 8;

	return udc_i2c_write_data_mask(i2c, item);

}

static int udc_i2c_reg16_data08_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 6 + 2 * count;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg16_data08_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
 	udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  count;
	item->data.buffer = &item->cmd.buffer[6];

	item->mask.bits = 8;
	item->mask.len =  count;
	item->mask.buffer = &item->cmd.buffer[8];
	
       item->cmd.length = 6 + 4 * count;

	return udc_i2c_write_data_mask(i2c, item);


}



static int udc_i2c_reg16_buffer08_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 8;
	item->data.len = 1;
	item->data.buffer = item->write.buffer;
	
	item->cmd.length = 4;
	item->write.length = 1;

	return udc_i2c_write_data(i2c, item);

}

static int udc_i2c_reg16_buffer08_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 8;
	item->data.len =  1;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 8;
	item->mask.len = 1;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 6;
	item->write.length = 1;

	return udc_i2c_write_data_mask(i2c, item);
}





static int udc_i2c_reg16_data16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[4];
	
	item->cmd.length = 6;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg16_data16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = &item->cmd.buffer[4];

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[6];
	
	item->cmd.length = 8;

	return udc_i2c_write_data_mask(i2c, item);


}

static int udc_i2c_reg16_data16_multi_write(udc_i2c* i2c, udc_i2c_item* item)
{
       udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[6];
	
       item->cmd.length = 6 + 2 * count;

	return udc_i2c_write_data(i2c, item);

}



static int udc_i2c_reg16_data16_multi_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
 	udc_t count = (item->cmd.buffer[5] << 8) | item->cmd.buffer[4];

       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2 * count;
	item->data.buffer = &item->cmd.buffer[6];

	item->mask.bits = 16;
	item->mask.len =  2 * count;
	item->mask.buffer = &item->cmd.buffer[8];
	
       item->cmd.length = 6 + 4 * count;

	return udc_i2c_write_data_mask(i2c, item);
}


static int udc_i2c_reg16_buffer16_write(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len = 2;
	item->reg.buffer = &item->cmd.buffer[2];
	
	item->data.bits = 16;
	item->data.len = 2;
	item->data.buffer = item->write.buffer;

	
	item->cmd.length = 4;
	item->write.length = 2;

	return udc_i2c_write_data(i2c, item);

}

static int udc_i2c_reg16_buffer16_write_mask(udc_i2c* i2c, udc_i2c_item* item)
{
       item->reg.bits = 16;
	item->reg.len =  2;
	item->reg.buffer = &item->cmd.buffer[2];
	
       item->data.bits = 16;
	item->data.len =  2;
	item->data.buffer = item->write.buffer;

	item->mask.bits = 16;
	item->mask.len = 2;
	item->mask.buffer = &item->cmd.buffer[4];
	
       item->cmd.length = 6;
	item->write.length = 2;

	 return udc_i2c_write_data_mask(i2c, item);

}



static int udc_i2c_command(udc_i2c* i2c, udc_i2c_item* item)
{
       int     ret              = RC_SUCCESS;
	udc8_t cmd, sub_cmd;  
	sub_cmd = item->cmd.buffer[0];  
	cmd = item->cmd.buffer[1];

	switch( cmd )
	 {
        case 0x00: //READ(0)(8)
             	{
			ret = udc_i2c_reg00_data08_read(i2c, item);	
        	}
		break;
		
	 case 0x01: //READ(0)(8)(M)
	      	{
                       ret = udc_i2c_reg00_data08_read_mask(i2c, item);	
	 	}
		break;

	 case 0x20: //MULTI_READ(0)(8)
             	{
			ret = udc_i2c_reg00_data08_multi_read(i2c, item);	
        	}
		break;
		
	 case 0x21: //MULTI_READ(0)(8)(M)
	      	{
                       ret = udc_i2c_reg00_data08_multi_read_mask(i2c, item);	
	 	}
		break;
		
   	case 0x02: //READ(0)(16)		      	
      	       {
                       ret = udc_i2c_reg00_data16_read(i2c, item);  
	 	}
		break;

	case 0x03: //READ(0)(16)(M)		      	
      	       {
                       ret = udc_i2c_reg00_data16_read_mask(i2c, item);  
	 	}
		break;

	case 0x22: //MULTI_READ(0)(16)		      	
      	       {
                       ret = udc_i2c_reg00_data16_multi_read(i2c, item);  
	 	}
		break;

	case 0x23: //MULTI_READ(0)(16)(M)		      	
      	       {
                       ret = udc_i2c_reg00_data16_multi_read_mask(i2c, item);  
	 	}
		break;
	
		
	 case 0x04: //READ(8)(8)		      	
      	       {
                      ret =  udc_i2c_reg08_data08_read(i2c, item);  
	 	}
		break;

	case 0x05: //READ(8)(8)(M)		      	
      	       {
                      ret =  udc_i2c_reg08_data08_read_mask(i2c, item);   
	 	}
		break;

	case 0x24: //MULTI_READ(8)(8)		      	
      	       {
                      ret =  udc_i2c_reg08_data08_multi_read(i2c, item);  
	 	}
		break;

	case 0x25: //MULTI_READ(8)(8)(M)		      	
      	       {
                      ret =  udc_i2c_reg08_data08_multi_read_mask(i2c, item);   
	 	}
		break;
	

	case 0x06: //READ(8)(16)		      	
      	       {
                     ret =  udc_i2c_reg08_data16_read(i2c, item);     
	 	}
		break;
	case 0x07: //READ(8)(16)(M)		      	
      	       {
                     ret =  udc_i2c_reg08_data16_read_mask(i2c, item);       
	 	}
		break;

	case 0x26: //MULTI_READ(8)(16)		      	
      	       {
                     ret =  udc_i2c_reg08_data16_multi_read(i2c, item);     
	 	}
		break;
	case 0x27: //READ(8)(16)(M)		      	
      	       {
                     ret =  udc_i2c_reg08_data16_multi_read_mask(i2c, item);       
	 	}
		break;
		
	case 0x08: //READ(16)(8)		      	
      	       {
                    ret =  udc_i2c_reg16_data08_read(i2c, item);        
	 	}
		break;
	case 0x09: //READ(16)(8)(M)		      	
      	       {
                    ret =  udc_i2c_reg16_data08_read_mask(i2c, item);           
	 	}
		break;

	case 0x28: //MULTI_READ(16)(8)		      	
      	       {
                    ret =  udc_i2c_reg16_data08_multi_read(i2c, item);        
	 	}
		break;
	case 0x29: //MULTI_READ(16)(8)(M)		      	
      	       {
                    ret =  udc_i2c_reg16_data08_multi_read_mask(i2c, item);           
	 	}
		break;
		
	case 0x0A: //READ(16)(16)		      	
      	       {
                     ret =  udc_i2c_reg16_data16_read(i2c, item);   
	 	}
		break;
	case 0x0B: //READ(16)(16)(M)		      	
      	       {
                     ret =  udc_i2c_reg16_data16_read_mask(i2c, item);   
	 	}
		break;	
	case 0x2A: //MULTI_READ(16)(16)		      	
      	       {
                     ret =  udc_i2c_reg16_data16_multi_read(i2c, item);   
	 	}
		break;
	case 0x2B: //MULTI_READ(16)(16)(M)		      	
      	       {
                     ret =  udc_i2c_reg16_data16_multi_read_mask(i2c, item);   
	 	}
		break;
		
	case 0x40://WRITE(0)(8)
              {
			ret = udc_i2c_reg00_data08_write(i2c, item);
		}
	       break;
	case 0x41://WRITE(0)(8)(M)
                 {
		      ret = udc_i2c_reg00_data08_write_mask(i2c, item);
		}
		break;
		
	case 0x60://MULTI_WRITE(0)(8)
                 {
			ret = udc_i2c_reg00_data08_multi_write(i2c, item);
		}
	       break;
	case 0x61://MULTI_WRITE(0)(8)(M)
                 {
		      ret = udc_i2c_reg00_data08_multi_write_mask(i2c, item);
		}
		break;	
		
	case 0x80://WRITE_BUFFER(0)(8)
                 {
	            ret = udc_i2c_reg00_buffer08_write(i2c, item);
		}
		break;
		
	case 0x81://WRITE_BUFFER(0)(8)(M)
                 {
			ret = udc_i2c_reg00_buffer08_write_mask(i2c, item);
		}
		break;	
		
	case 0x42://WRITE(0)(16)
                 {
			ret = udc_i2c_reg00_data16_write(i2c, item);
		}
		break;
	case 0x43://WRITE(0)(16)(M)
                 {
	             ret = udc_i2c_reg00_data16_write_mask(i2c, item);
		}
		break;
	case 0x62://MULTI_WRITE(0)(16)
                 {
			ret = udc_i2c_reg00_data16_multi_write(i2c, item);
		}
		break;
	case 0x63://MULTI_WRITE(0)(16)(M)
                 {
	             ret = udc_i2c_reg00_data16_multi_write_mask(i2c, item);
		}
		break;	
	case 0x82://WRITE_BUFFER(0)(16)
                 {
	            ret = udc_i2c_reg00_buffer16_write(i2c, item);
		}
		break;
		
	case 0x83://WRITE_BUFFER(0)(16)(M)
                 {
			ret = udc_i2c_reg00_buffer16_write_mask(i2c, item);
		}
		break;	
		
	case 0x44://WRITE(8)(8)
	        {
			ret = udc_i2c_reg08_data08_write(i2c, item);
		 }
		 break;
	case 0x45://WRITE(8)(8)(M)
	       {
			ret = udc_i2c_reg08_data08_write_mask(i2c, item);
		}
		break;

	case 0x64://MULTI_WRITE(8)(8)
	        {
			ret = udc_i2c_reg08_data08_multi_write(i2c, item);
		 }
		 break;
	case 0x65://MULTI_WRITE(8)(8)(M)
	       {
			ret = udc_i2c_reg08_data08_multi_write_mask(i2c, item);
		}
		break;	

	case 0x84://WRITE_BUFFER(8)(8)
                 {
	            ret = udc_i2c_reg08_buffer08_write(i2c, item);
		}
		break;
	case 0x85://WRITE_BUFFER(8)(8)(M)
                 {
			ret = udc_i2c_reg08_buffer08_write_mask(i2c, item);
		}
		break;
		
	case 0x46://WRITE(8)(16)
	       {
			ret = udc_i2c_reg08_data16_write(i2c, item);
		}
		break;
	case 0x47://WRITE(8)(16)(M)
	       {
			ret = udc_i2c_reg08_data16_write_mask(i2c, item);
		}
		break;
	case 0x66://MULTI_WRITE(8)(16)
	       {
			ret = udc_i2c_reg08_data16_multi_write(i2c, item);
		}
		break;
	case 0x67://MULTI_WRITE(8)(16)(M)
	       {
			ret = udc_i2c_reg08_data16_multi_write_mask(i2c, item);
		}
		break;
		
	case 0x86://WRITE_BUFFER(8)(16)
                 {
	            ret = udc_i2c_reg08_buffer08_write(i2c, item);
		}
		break;
	case 0x87://WRITE_BUFFER(8)(16)
              {
			ret = udc_i2c_reg08_buffer16_write_mask(i2c, item);
		}
		break;
		
	case 0x48://WRITE(16)(8)
	       {
			ret = udc_i2c_reg16_data08_write(i2c, item);
		}
		break;	
	case 0x49://WRITE(16)(8)(M)
	       {
			ret = udc_i2c_reg16_data08_write_mask(i2c, item);
		}
		break;

	case 0x68://MULTI_WRITE(16)(8)
	       {
			ret = udc_i2c_reg16_data08_multi_write(i2c, item);
		}
		break;	
	case 0x69://MULTI_WRITE(16)(8)(M)
	       {
			ret = udc_i2c_reg16_data08_multi_write_mask(i2c, item);
		}
		break;

	case 0x88://WRITE_BUFFER(16)(8)
	       {
			ret = udc_i2c_reg16_buffer08_write(i2c, item);
		}
		break;
	case 0x89://WRITE_BUFFER(16)(16)
	       {
			ret = udc_i2c_reg16_buffer08_write_mask(i2c, item);
		}
		break;
		
		
	case 0x4A://WRITE(16)(16)
	       {
			ret = udc_i2c_reg16_data16_write(i2c, item);
		}
		break;
	case 0x4B://WRITE(16)(16)(M)
	       {
			ret = udc_i2c_reg16_data16_write_mask(i2c, item);
                 }
		break;

	case 0x6A://MULTI_WRITE(16)(16)
	       {
			ret = udc_i2c_reg16_data16_multi_write(i2c, item);
		}
		break;
	case 0x6B://MULTI_WRITE(16)(16)(M)
	       {
			ret = udc_i2c_reg16_data16_multi_write_mask(i2c, item);
                 }
		break;

	case 0x8A://BUFFER_WRITE(16)(16)
	       {
			ret = udc_i2c_reg16_buffer16_write(i2c, item);
		}
		break;
	case 0x8B://BUFFER__WRITE(16)(16)(M)
	       {
			ret = udc_i2c_reg16_buffer16_write_mask(i2c, item);
		}
		break;
	
	default:
	 	
	 	break;
		
	 }

	return ret;
}

static int udc_i2c_extend_command(udc_i2c* i2c,  udc_i2c_item* item)
{
       int     ret   = RC_SUCCESS;
	udc8_t code, sub_code;  
       udc8_t* cmd;
	udc_i2c_datasource* ds= i2c->ds;
       udc_i2c_extend_cmd *extend_cmd = &ds->extend_cmd;
	  

	cmd =  item->cmd.buffer;
	sub_code = cmd[0];  
	code = cmd[1];

	UDC_I2C_TRACE("%s: cmd = 0x%x, sub_cmd = 0x%x\r\n", __func__,  code, sub_code);

	switch( sub_code )
	{
	case 0x00://WAIT_STATUS start
             	{
                     udc_i2c_cmd_wait* wait = &extend_cmd->wait;
      
			item->cmd.length = 4;
	              wait->cmd.pos = ds->cmd.pos;
			wait->count = cmd[3];
			wait->time = cmd[2];
			
			wait->start_jiffies = jiffies;
		}
		break;
		
	case 0x01://WAIT_STATUS end
		{
			udc_t is_timeout = 0;	
			udc_t wait_status = (cmd[3]<<8) | cmd[2] ; 
			udc_t status = (ds->rx_buffer[1]<<8) | ds->rx_buffer[0] ; 
			udc_i2c_cmd_wait* wait = &extend_cmd->wait;
			
			is_timeout = time_after(jiffies,  wait->start_jiffies + msecs_to_jiffies( wait->count *  wait->time));


			UDC_I2C_TRACE("%s: is_timeout = 0x%x, wait_status = 0x%x, status = 0x%x, count = 0x%x, time = 0x%x\r\n", __func__, 
				is_timeout, wait_status, status, wait->count, wait->time);
					  
	              // 等到状态
			if ( wait_status == status )
		       {
				item->cmd.length = 4;
				ret   = RC_SUCCESS;
				UDC_I2C_TRACE("%s: line = %d\r\n", __func__, __LINE__);
		
			}// 超时
			else if ( is_timeout )
			{
				item->cmd.length = 4;
				ret   = RC_TIMEOUT;
				UDC_I2C_TRACE("%s: line = %d\r\n", __func__, __LINE__);
	    		}
			else
			{
				msleep_interruptible(wait->time);
				if (signal_pending(current))
				{
	  	  			item->cmd.length = 4;
					ret   = RC_ERROR;
					UDC_I2C_TRACE("%s: line = %d\r\n", __func__, __LINE__);
				}
				else
				{    
				       item->cmd.length = 0;
				       // 重复执行 标准I2C命令
					ds->cmd.pos  = wait->cmd.pos + 4;
					UDC_I2C_TRACE("%s: line = %d, cmd.pos = %d, \r\n", __func__, __LINE__, ds->cmd.pos );
				}
			}

			
		}
		break;
		
	case 0x02://READ_STATUS start
	      	{
			item->cmd.length = 4;
		}
		break;
		
	case 0x03://READ _STATUS end
		{
			udc_t read_status = (cmd[3]<<8) | cmd[2] ;
			udc_t status = (ds->rx_buffer[1]<<8) | ds->rx_buffer[0] ;   
			
			item->cmd.length = 4;
		       
	              UDC_I2C_TRACE("%s: read_status = 0x%x, status = 0x%x\r\n", __func__, 
					read_status, status);
				 
			if ( read_status == status )
		       {
				ret   = RC_SUCCESS;
			}
			else
			{
				ret   = RC_ERROR;
			}
		}
		break;
		
	case 0xff: // delay
	       {
			udc_t delay_time = (cmd[3]<<8) | cmd[2] ;
			
			item->cmd.length = 4;

			msleep(delay_time);
		}
		break;
		
	default:
		break;
		
	}

	return ret;
	
}
static int udc_i2c_item_init(udc_i2c_item* item)
{
       memset(item, 0x0, sizeof(udc_i2c_item));
}

int udc_i2c_datasource_init(udc_i2c* i2c, udc8_t * buffer, udc32_t count)
{
 	int     ret = RC_SUCCESS;
	udc_i2c_datasource* ds = i2c->ds;
	
	memset(ds, 0x0, sizeof(udc_i2c_datasource));



	ds->cmd.length = i2c->info->value_count * sizeof(udc_t);
	ds->cmd.buffer = (udc8_t*)i2c->info->value;
    
       ds->read.length = count * sizeof(udc_t);
	ds->read.buffer = buffer;

       ds->write.length = count * sizeof(udc_t);
	
	if ( ds->write.length  > 0 )
       {
	       ds->write.buffer = (udc8_t* )kmalloc(ds->write.length, GFP_KERNEL);
		   
		if  (NULL == ds->write.buffer )
			return RC_MEMORY;

		memcpy(ds->write.buffer, buffer, ds->write.length);

	 }

	return ret;
}


 int udc_i2c_execute(udc_i2c* i2c, udc8_t * buffer, udc32_t count)
{
       int     ret              = RC_SUCCESS;
	udc8_t code, sub_code;  
	udc_i2c_datasource data_source;
	udc_i2c_datasource* ds;
	udc_i2c_item* item;
	
#ifdef UDC_I2C_DEBUG
       int i;
       UDC_I2C_TRACE("%s: START!!\r\n", __func__);
       UDC_I2C_TRACE("%s: mask_bits=0x%x, addr_bits=0x%x, value_bits=0x%x, value_count=%d, VALUE: ", __func__, i2c->info->mask_bits, i2c->info->reg_addr_bits, i2c->info->reg_value_bits, i2c->info->value_count);
       for ( i = 0; i <  i2c->info->value_count;  i ++)
       {
          UDC_I2C_TRACE("0x%x, ", i2c->info->value[i]);
       }
	UDC_I2C_TRACE("\r\n");   
#endif	   
      
      
       i2c->ds = &data_source;
       ds =   i2c->ds;
	
	udc_i2c_datasource_init(i2c, buffer, count);
	 
	item = &ds->item;
		
	while ( 1 )
       {
               ds->cmd.pos += item->cmd.length;
		 if ( ds->cmd.pos >= ds->cmd.length )	  
		 	break;

		 if ( ds->read.buffer && ds->read.pos + item->read.length  < ds->read.length ) 	   
		 	ds->read.pos += item->read.length;

		if ( ds->write.buffer && ds->write.pos + item->write.length  < ds->write.length ) 	   
		 	ds->write.pos += item->write.length;

		 udc_i2c_item_init(item);
		
               item->cmd.buffer = &ds->cmd.buffer[ds->cmd.pos];

		 if ( ds->read.buffer && ds->read.pos < ds->read.length ) 	     
		 	item->read.buffer = &ds->read.buffer[ds->read.pos];
		 
		 if ( ds->write.buffer && ds->write.pos < ds->write.length ) 	     
		 	item->write.buffer = &ds->write.buffer[ds->write.pos];
               
              code = item->cmd.buffer[1];
		sub_code = item->cmd.buffer[0];  
		   
		UDC_I2C_TRACE("%s: cmd = 0x%x, sub_cmd = 0x%x\r\n", __func__,  code, sub_code);

		if ( 0xff == code ) //Extend Command
			ret = udc_i2c_extend_command(i2c, item);
		else				   
              	ret = udc_i2c_command(i2c, item);
		
	}
	   
       UDC_I2C_TRACE("%s: ret = %d, END!!\r\n\r\n", __func__, ret);

     	if (ds->write.length > 0)
       {
        
		  if ( ds->write.buffer)
		  {
		  	kfree(ds->write.buffer);
		       ds->write.buffer = NULL;
		  }	   
	}


	return ret;   

}





/////////////////////////////////////////////////////////////////
//////old functions
//////////////////////////////////////////////////////////////////////////

int udc_i2c_read_status(udc_s* udc,udc_i2c_info* i2c_info, udc_t status_reg, udc_t status_value,udc_t status_mask)
{
	int   ret = 0;   
	udc_t read_value=0;
	ret = udc_i2c_rxdata(udc, i2c_info, status_reg , &read_value, 1);
	
	if  ( (read_value& status_mask) ==  status_value)
		ret = -1;
	else
		ret = 0;


	return ret;
	
	
}

int udc_i2c_wait(udc_s* udc, udc_i2c_info* i2c_info, udc_t wait_count, udc_t wait_time, udc_t reg, udc_t value, udc_t reg_mask)
{
  udc_t reg_read_value;
	int ret = -EINVAL;   
  ulong jiffies_comp= 0;
  udc_t is_timeout=0;
	udc_t timeout_count=0;	
  timeout_count = wait_count*wait_time;   
	jiffies_comp = jiffies;
	while(1) 
	{
		msleep_interruptible(wait_time);
		if (signal_pending(current))
  	  		break;
		ret = udc_i2c_rxdata(udc, i2c_info, reg, &reg_read_value,1);  //register_read
		is_timeout = time_after(jiffies,  jiffies_comp + msecs_to_jiffies( timeout_count ));
		UDC_TRACE("udc_i2c_wait: reg = 0x%x,value= 0x%x,reg_mask =0x%x,reg_read_value=0x%x,wait_count = %d, wait_time =  %d, is_timeout =  %d,ret =  %d\n", 
			reg,value,reg_mask,reg_read_value,wait_count, wait_time, is_timeout, ret); 

		if ( is_timeout || ( (ret >= 0) &&( ( reg_read_value & reg_mask)==value) ) )//reg_mask
		{
    			break;
		}
	}
	if ( (ret >= 0) &&( ( reg_read_value & reg_mask)==value) )
	{
		ret=0;
		UDC_TRACE("udc_i2c_wait value=0x%x success\n",value);
	}
	else if(is_timeout)
 	{
		ret = -1;
		UDC_TRACE("udc_i2c_wait timeout \n");
		
	}
	else if(ret)
	{	
		ret = -1;
		UDC_TRACE("udc_i2c_wait read reg=0x%x fail\n",reg);
	}
	
	return ret;
}

int udc_i2c_rxdata(udc_s* udc,udc_i2c_info* i2c_info, udc_t reg_addr,udc8_t *rxdata, int length)
{
	int ret = -1;
	udc8_t cmd[2] = {0};    
	udc8_t w_cmd_num = 0;
	udc8_t* p=NULL;
	udc8_t temp=0;
	int i=0;

	struct i2c_client	*i2c_client = i2c_info->i2c_client ;
	if(UDC_I2C_REG_16BIT == i2c_info->reg_addr_bits)
	{
		cmd[w_cmd_num++] = (udc8_t)(reg_addr >>8);				
		cmd[w_cmd_num++] = (udc8_t)(reg_addr);	
	}
	if(UDC_I2C_REG_8BIT == i2c_info->reg_addr_bits)
	{				
		cmd[w_cmd_num++] = (udc8_t)(reg_addr);	
	}

	if(UDC_I2C_VALUE_16BIT == i2c_info->reg_value_bits)
	{
		length *=2;
	}

	struct i2c_msg msgs[] = 
	{
        {
            .addr   = i2c_client->addr,
            .flags  = 0,
            .len    = w_cmd_num,
            .buf    = cmd,
        },
        {
            .addr   = i2c_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
        },
    };
    ret = i2c_transfer(i2c_client->adapter, msgs,2);

    if(UDC_I2C_VALUE_16BIT == i2c_info->reg_value_bits)
    {
    	p = rxdata;
    	for(i=0;i<length;i+=2)
    	{
   	 temp=*p;
   	 *p = *(p+1);
   	 *(p+1)=temp;
   	 p+=2;
   	 }
    }
    
    if (ret < 0)
    {
        UDC_TRACE("%s i2c read error: %d", __func__, ret);
    }
	
    return ret;
}


int udc_i2c_txdata(udc_s* udc,udc_i2c_info* i2c_info, udc_t reg_addr,udc_t* value,int length)
{
	int ret = -1;
	udc8_t* txdata = udc->data_buffer;
	udc_t data_length=0;
	udc_t addr_length =0;
	udc_t write_length=0;
	udc8_t* p=NULL;
	udc8_t temp=0;
	udc_t i=0,j=0;

	addr_length=1;
	data_length=length;
	struct i2c_client	*i2c_client = i2c_info->i2c_client ;

	//write reg_addr
	if (UDC_I2C_REG_8BIT == i2c_info->reg_addr_bits)
	{	

		txdata[i++] = (udc8_t)(reg_addr);
	}
	else if (UDC_I2C_REG_16BIT == i2c_info->reg_addr_bits)
	{	
		addr_length*=2;
		txdata[i++] = (udc8_t)(reg_addr >>8);
		txdata[i++] = (udc8_t)(reg_addr);	
	}
	else
	{
		UDC_TRACE("udc_i2c_txdata error:i2c_info->reg_addr_bits is neither 8bits nor 16bits");
	}



	//write reg_value
	if(data_length)
	{
		if ( UDC_I2C_VALUE_8BIT == i2c_info->reg_value_bits)
		{	
			data_length = length;
			for(j=0;j<data_length;j++)
			{
				txdata[i++] = (udc8_t)value[j];
			}
		}
		else if (UDC_I2C_VALUE_16BIT == i2c_info->reg_value_bits)
		{	
			data_length = 2*length;
			memcpy(txdata+i, value, data_length);

			p=  txdata+i;
			for(i=0;i<data_length;i+=2)
				{	
				 temp=*p;
				*p = *(p+1);
				*(p+1)=temp;
				p+=2;
			}
		}
		else
		{
			UDC_TRACE("udc_i2c_txdata error:i2c_info->reg_value_bits is neither 8bits nor 16bits");
		}
	}
	write_length = data_length+addr_length;
	struct i2c_msg msg[] = 
	{
		{
			.addr	= i2c_client->addr,
			.flags	= 0,
			.len	= write_length,
			.buf	= txdata,
		}, 
	};
	//UDC_TRACE("i2c_client->addr= 0x%x\n",i2c_client->addr);
	//UDC_TRACE("write_length =%u \n",write_length);
	//UDC_TRACE("txdata[0]=0x%x, txdata[1]=0x%x, txdata[2]=0x%x,txdata[3]=0x%x\n",txdata[0],txdata[1],txdata[2],txdata[3]);
	ret = i2c_transfer(i2c_client->adapter, msg, 1);
	if (ret < 0)
		UDC_TRACE("%s i2c write error: %d", __func__, ret);

	return ret;
}




int udc_i2c_do(udc_s* udc, udc_i2c_info* i2c_info,udc_t * reg_values, udc_t reg_values_count)
{
	uint32_t i=0, j=0;
	int     ret              = 0;
	udc_t mask_bits,value_count,reg, reg_value,  data_count, reg_mask;
    udc_t read_reg_value, write_reg_value;
	udc_t* value;
	udc_t wait_count, wait_time;
	udc_t status_mask, status_reg, status_value;
	mask_bits = i2c_info->mask_bits;
	value_count = i2c_info->value_count;
	value = i2c_info->value;
	udc_t* write_value;
	udc_t write_addr;
	udc_t *data_value;
       udc_i2c i2c = {0};

	UDC_TRACE("%s: mask_bits = 0x%x\n", __func__, i2c_info->mask_bits);
		
       if (mask_bits & 0xff00 )
       {	 
      		i2c.udc = udc;
		i2c.info = i2c_info;
		return udc_i2c_execute(&i2c, (udc8_t*)reg_values, reg_values_count);
	}
	  
	if ( 0 == mask_bits )  //no mask bits
	{
		for( i = 0; i < value_count; )
		{
			reg = value[i];
			reg_value = value[ i + 1];
			i += 2;	
			UDC_TRACE("%s : reg = 0x%x, reg_value=0x%x\n", __func__, reg, reg_value);
			if ( UDC_WAIT_FLAG == reg )
			{
				wait_count = ( reg_value>> 8 ) & 0xff;
				wait_time = ( reg_value>> 0 ) & 0xff;
				
				reg = value[i];
				reg_value = value[ i + 1];
				reg_mask = value[ i + 2];
			       i += 3;

               		 ret = udc_i2c_wait(udc, i2c_info, wait_count, wait_time, reg, reg_value, reg_mask);

	   		}
			else if ( UDC_DELAY_FLAG == reg )
			{ 
			       wait_time = reg_value;
				msleep(wait_time);
			}
			else if ( UDC_READ_FLAG == reg )
			{ 
				if (j < reg_values_count )
				{	
					if (UDC_UNDEFINED == reg_value)
					{
						i2c_master_recv(i2c_info->i2c_client , &reg_values[j++], 1);
					}
					else
					{
						udc_i2c_rxdata(udc, i2c_info, reg_value, &reg_values[j++], 1);	
					}	
				}
				else
				{
					UDC_TRACE("%s : line = %d, j = %d out of range reg_value_count = %d\n", __func__, __LINE__, j, reg_values_count);
				}
			}
                    	else if ( UDC_READ_STATUS_FLAG == reg )
			{ 
                           
				status_reg = value[i];
				status_value = value[ i + 1];
				status_mask = value[i+2];
				i += 3;

				ret = udc_i2c_read_status(udc, i2c_info, status_reg, status_value,status_mask);
			}
			else if (UDC_MULTI_WRITE_FLAG == reg)
			{	
				write_addr = value[i-1];
				data_count = value[i];
				data_value = value+i+1;
				i+=3+data_count;
				if (UDC_UNDEFINED == write_addr)
				{
					i2c_master_send(i2c_info->i2c_client, data_value,data_count);
				}
				else
				{
					udc_i2c_txdata(udc, i2c_info,write_addr,data_value,data_count);
				}
			}
			else
			{
				if (UDC_UNDEFINED == reg_value && j < reg_values_count )
				{
					write_reg_value = reg_values[j++];
				}
				else
				{
					write_reg_value = reg_value;			
				}
				//ret = udc_i2c_txdata(udc,reg,write_reg_value);
				udc_i2c_txdata(udc, i2c_info, reg,&write_reg_value,1);
			}
			
		}
	}
	else     //have mask bits
	{
		for ( i = 0; i < value_count; )
		{
                     reg = value[i];
			reg_value = value[i+1];
			reg_mask = value[i+2];
                    	i += 3;	
						
			UDC_TRACE("udc_i2c_do : reg=0x%x, reg_value=0x%x, reg_mask = 0x%x\n", reg, reg_value, reg_mask);

			if ( UDC_WAIT_FLAG == reg )
			{
				wait_count = ( reg_value>> 8 ) & 0xff;
				wait_time = ( reg_value>> 0 ) & 0xff;
				
				reg = value[i];
				reg_value = value[ i + 1];
				reg_mask = value[ i + 2];
				i += 3;
				ret = udc_i2c_wait(udc, i2c_info, wait_count, wait_time, reg, reg_value, reg_mask);
				
			}
			else if ( UDC_DELAY_FLAG == reg )
			{
				wait_time = reg_value;
				msleep(wait_time);
			}
			else if ( UDC_READ_FLAG == reg )
			{ 
				  if ( j < reg_values_count )
				  {
				  	if (UDC_UNDEFINED == reg_value)
					{
						i2c_master_recv(i2c_info->i2c_client , &reg_values[j++], 1);
					}
					else
					{
						udc_i2c_rxdata(udc, i2c_info, reg_value, &reg_values[j] , 1);
					}
					reg_values[j++] &= reg_mask;
				  }
				  else
				  {
					UDC_TRACE("%s : j = %d out of range reg_value_count = %d\n", __func__, j, reg_values_count);
				  }
			}
			else if ( UDC_MULTI_READ_FLAG == reg )
			{ 
				data_count = reg_mask;
				if (j < reg_values_count )
				{	
					if (UDC_UNDEFINED == reg_value)
					{
						i2c_master_recv(i2c_info->i2c_client , reg_values, data_count);
					}
					else
					{
						udc_i2c_rxdata(udc, i2c_info, reg_value,reg_values,data_count);
					}
				}
				else
				{
					UDC_TRACE("%s : line = %d, j = %d out of range reg_value_count = %d\n", __func__, __LINE__, j, reg_values_count);
				}
			}
			else if (UDC_MULTI_WRITE_FLAG == reg)
			{	
				write_addr = value[i-1];
				data_count = value[i];
				data_value = value+i+1;
				i+=3+data_count;
				if (UDC_UNDEFINED == write_addr)
				{
					i2c_master_send(i2c_info->i2c_client, data_value,data_count);
				}
				else
				{
      				udc_i2c_txdata(udc, i2c_info,write_addr,data_value,data_count);
				}
			}
			else if ( UDC_READ_STATUS_FLAG == reg )
			{ 
				status_reg = value[i];
				status_value = value[ i + 1];
				status_mask = value[i+2];
				i += 3;
				ret = udc_i2c_read_status(udc, i2c_info, status_reg, status_value,status_mask);
			}
			else
			{
				if (UDC_UNDEFINED == reg_value && j < reg_values_count )
				{
					write_reg_value = reg_values[j++];
				}
				else
				{
					write_reg_value = reg_value;
				}
				
				if ( (0xff == reg_mask && 8 == mask_bits) || (0xffff == reg_mask && 16 == mask_bits))
				{
					//ret = udc_i2c_txdata(udc,reg,write_reg_value);
			
					udc_i2c_txdata(udc, i2c_info, reg,&write_reg_value,1);
				}
				else
				{	
					//ret = udc_i2c_rxdata(udc, reg, &read_reg_value, 1);
					udc_i2c_rxdata(udc, i2c_info, reg, &read_reg_value, 1);
					read_reg_value &= ~reg_mask;
					write_reg_value |= read_reg_value;
					udc_i2c_txdata(udc, i2c_info, reg,&write_reg_value,1);
				}
			}
			
		}
	}
	return ret;	
}


EXPORT_SYMBOL(udc_i2c_rxdata);
EXPORT_SYMBOL(udc_i2c_txdata);
EXPORT_SYMBOL(udc_i2c_do);


