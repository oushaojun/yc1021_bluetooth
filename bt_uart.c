

/*
	this file contains some functions need to be ported before enable success bluetooth communication
	functions need to port list below:
	
	system_tick_inc	- should be placed in 1ms system tick isr
	bt_fifo_enqueue_one	- should be placed in uart rcv isr to save uart data
	bt_send_buffer	- should be implemented by user
*/


#include "bt_uart.h"
#include "mhscpu_conf.h"

static volatile MG_FIFO g_bt_fifo=
{	.head=0,
	.tail=0,
	.count=0,
};



/*
	system tick init and functions
*/
static volatile u32 g_system_tick=0;

void system_tick_inc(void)
{
	g_system_tick++;
}


u32 system_tick_get_count(void)
{
	return g_system_tick;
}


void mdelay(u16 ms)
{
	u32 old_time,new_time,duration;

	old_time=system_tick_get_count();

	while(1)
	{
		new_time=system_tick_get_count();
		
		if(new_time>=old_time)	duration=new_time-old_time;
		else					duration=(u32)(~0)-old_time+new_time;

		if(duration>=ms) break;
	}
}


/*
	to check if timeout
	input:	old_tick - previous tick count
			timeout_ms - ms timeout
	output: none
	return: 0- timeout
			others- not timeout
*/
int system_tick_timeout_check(u32 old_tick, u16 timeout_ms)
{
	u32 new_time,duration;

	new_time=system_tick_get_count();
	if(new_time>=old_tick)	duration=new_time-old_tick;
	else					duration=(u32)(~0)-old_tick+new_time;

	if(duration >= timeout_ms)	return 0;
	else						return -1;
}


/*
	fifo enqueue one item
	input: data- byte to enqueue
	output: none
	return: 0- success
			others- fail
*/

s32 bt_fifo_enqueue_one(u8 data)
{
	if(g_bt_fifo.count >= sizeof(g_bt_fifo.buf)) return -1;

	g_bt_fifo.buf[g_bt_fifo.head]=data;

	g_bt_fifo.count++;
	g_bt_fifo.head++;
	if(g_bt_fifo.head >= sizeof(g_bt_fifo.buf))
	{
		g_bt_fifo.head=0;
	}

	return 0;
}

/*
	fifo dequeue one item
	input: none
	output: data- byte to dequeue
	return: 0- success
			others- fail
*/

s32 bt_fifo_dequeue_one(u8 *data)
{
	if(g_bt_fifo.count == 0) return -1;

	*data=g_bt_fifo.buf[g_bt_fifo.tail];

	g_bt_fifo.count--;
	g_bt_fifo.tail++;
	if(g_bt_fifo.tail >= sizeof(g_bt_fifo.buf))
	{
		g_bt_fifo.tail=0;
	}

	return 0;
}

/*
	fifo get count
	intput: none
	output: none
	return: real fifo count
*/

s32 bt_fifo_get_count(void)
{
	return g_bt_fifo.count;
}

/*
	fifo enqueue
	input: 	buf- data to enqueue
			len- len to enqueue
	output: none
	return: 0 -success
			others -fail
*/

s32 bt_fifo_enqueue(u8 *buf, u8 len)
{
	s32 ret=0;

	while(len)
	{
		if(bt_fifo_enqueue_one(*buf++))
		{
			ret=-1;
			break;
		}
		else
		{
			len--;
		}
	}

	return ret;
}

/*
	fifo dequeue
	input: 	len- len want to dequeue
	output: buf- buf to store data
			real_len- real len dequeued
	return: 0- success
			others- fail
*/

s32 bt_fifo_dequeue(u8 *buf, u16 len, u16 *real_len)
{
	s32 ret;
	
	ret=bt_fifo_get_count();
	if(ret<=0) return -1;

	if(len>ret) len=ret;

	*real_len=len;
	ret=0;
	while(len)
	{
		if(bt_fifo_dequeue_one(buf++))
		{	
			ret=-2;
			break;
		}
		else
		{
			len--;
		}
	}

	return ret;
}



/*
	flush fifo
*/
void bt_fifo_flush(void)
{
	g_bt_fifo.count=0;
	g_bt_fifo.head=0;
	g_bt_fifo.tail=0;
}



/*
	send uart buffer
	input: 	buffer- data to send
			len- len of data to send
	output: none
	return: <=0 - error
			>0  - success, len sent finally
*/

int bt_send_buffer(u8 *buffer, u16 len)
{	
	int i;
	
	for(i=0;i<len;i++)
	{
		while(!UART_IsTXEmpty(UART1));
		UART_SendData(UART1,buffer[i]);
	}

	return len;
}


/*	
	rcv uart buffer, if no enough data, wait until timeout
	input: 	buffer_size- size of buffer
			timeout- timeout(ms)
	output: buffer
	return: <=0 - error
			>0	- success, len rcvd finally
*/

int bt_rcv_buffer(u8 *buffer, u16 buffer_size, u16 timeout)
{
	u16 len,rcv_len;
	int ret;
	u32 time;

	if(buffer_size == 0) return 0;

	time=system_tick_get_count();
	rcv_len=0;
	while(1)
	{
		ret=bt_fifo_dequeue(buffer, buffer_size, &len);
		if(!ret)
		{
			//printf("[%d]", len);
			buffer+=len;
			buffer_size-=len;
			rcv_len+=len;
			
			if(buffer_size == 0) 
			{
				ret=rcv_len;
				break;
			}
		}

		if(!system_tick_timeout_check(time, timeout))	
		{
			//printf("\r\nbt rcv timeout");
			ret=rcv_len;
			break;
		}
	}

	return ret;
}







