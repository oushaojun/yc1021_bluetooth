

#ifndef BT_UART_H
#define BT_UART_H



typedef struct
{
	unsigned char buf[500];
	unsigned short head;
	unsigned short tail;
	unsigned short count;
}
MG_FIFO;



typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef char s8;
typedef short s16;
typedef unsigned char bool;
typedef unsigned char BOOL;

s32 bt_fifo_enqueue_one(u8 data);
s32 bt_fifo_dequeue_one(u8 *data);
s32 bt_fifo_enqueue(u8 *buf, u8 len);
s32 bt_fifo_dequeue(u8 *buf, u16 len, u16 *real_len);
void bt_fifo_flush(void);

void system_tick_inc(void);
u32 system_tick_get_count(void);
void mdelay(u16 ms);
int system_tick_timeout_check(u32 old_tick, u16 timeout_ms);


#define YC_RESET_PIN 0
#define YC_SLEEP_PIN 1
#define LOW 0
#define HIGH 1

#define DISABLE_FLOW 0
#define FILE_READ_ONLY 0

int bt_send_buffer(u8 *buffer, u16 len);
int bt_rcv_buffer(u8 *buffer, u16 buffer_size, u16 timeout);


/**************************************************************************/
//need to port debug function
#define BT_DEBUG 1

#if BT_DEBUG
	#define printk(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
	#define printk(fmt, ...)
#endif
/**************************************************************************/



#endif



