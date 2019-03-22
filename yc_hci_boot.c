

/*
	this file contail functions to support hci boot, following functions need to port:
	
	extern void HOST_DELAY_MS(uint32_t ms);
	extern int HOST_uartSendData(uint8_t* pbuff, uint16_t len);
	extern int HOST_uartRecieveData(uint8_t* pbuff, uint16_t len, uint16_t timeout_ms);
	extern void HOST_GPIO_CONFIG(uint8_t gpio_number, bool level);
	extern void HOST_GPIO_INIT(void);
	extern void HOST_UART_CONFIG(bool flow_enalbe, uint32_t baudrate);
*/




#include "bt_uart.h"
#include "yc_hci_patch.h"
#include "mhscpu_conf.h"
#include "stdio.h"
#include "yc_hci_boot.h"

//#define YC_UART_CLOCK 48000000
#define YC_DEFAULT_BAUDRATE 115200
//#define YC_WORK_BAUDRATE 921600

#define YC_PATCH_FILE_NAME "YC_BTPatchFile.bin"
#define YC_PATCH_FILE_LEN_SIZE 2
#define YC_PATCH_BLOCK_LEN_SIZE 1

#define YC_CMD					0x01
#define YC_CMD_OGF			0xfc
#define YC_CMD_RESET		0x00
#define YC_CMD_BAUD			0x02
#define YC_CMD_ECHO			0x05

#define YC_SUCCESS 	0
#define YC_ERROR 	1

//*************************************
//Need HOST to implement APIs below.
//*************************************
extern void HOST_DELAY_MS(uint32_t ms);
extern int HOST_uartSendData(uint8_t* pbuff, uint16_t len);
extern int HOST_uartRecieveData(uint8_t* pbuff, uint16_t len, uint16_t timeout_ms);
extern void HOST_GPIO_CONFIG(uint8_t gpio_number, bool level);
extern void HOST_UART_CONFIG(bool flow_enalbe, uint32_t baudrate);
extern int HOST_fileOpen(uint8_t* file_name, int io_config);
extern uint32_t HOST_fileRead(int fileHandle, uint32_t offset, uint32_t len, uint8_t* buff);//return read length?
extern int HOST_fileClose(int fileHandle);
extern void HOST_GPIO_INIT(void);


void HOST_DELAY_MS(uint32_t ms)
{
	mdelay(ms);
}

int HOST_uartSendData(uint8_t* pbuff, uint16_t len)
{
	return bt_send_buffer(pbuff, len);
}

int HOST_uartRecieveData(uint8_t* pbuff, uint16_t len, uint16_t timeout_ms)
{
	return bt_rcv_buffer(pbuff, len, timeout_ms);
}

void HOST_GPIO_INIT(void)
{
	//reset pin: PB14
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Remap=GPIO_Remap_1;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	
	GPIO_SetBits(GPIOB, GPIO_Pin_14);

	//sleep pin: PB15
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Remap=GPIO_Remap_1;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	
	GPIO_SetBits(GPIOB, GPIO_Pin_15);
}

void HOST_GPIO_CONFIG(uint8_t gpio_number, bool level)
{
	if(YC_RESET_PIN == gpio_number)
	{
		if(level == LOW)		GPIO_ResetBits(GPIOB, GPIO_Pin_14);
		else if(level == HIGH)	GPIO_SetBits(GPIOB, GPIO_Pin_14);
	}
	else if(YC_SLEEP_PIN == gpio_number)
	{
		if(level == LOW)		GPIO_ResetBits(GPIOB, GPIO_Pin_15);
		else if(level == HIGH)	GPIO_SetBits(GPIOB, GPIO_Pin_15);
	}
}

void HOST_UART_CONFIG(bool flow_enalbe, uint32_t baudrate)
{
	UART_InitTypeDef UART_InitStructure;
	UART_FIFOInitTypeDef UART_FIFOInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	/************init uart1******************/
	GPIO_PinRemapConfig(GPIOB, GPIO_Pin_12 | GPIO_Pin_13, GPIO_Remap_3);  

	UART_StructInit(&UART_InitStructure);
	UART_FIFOStructInit(&UART_FIFOInitStruct);
	
	UART_InitStructure.UART_BaudRate = baudrate;
	UART_InitStructure.UART_WordLength = UART_WordLength_8b;
	UART_InitStructure.UART_StopBits = UART_StopBits_1;
	UART_InitStructure.UART_Parity = UART_Parity_No;

	UART_FIFOInitStruct.FIFO_Enable = ENABLE;
	UART_FIFOInitStruct.FIFO_DMA_Mode = UART_FIFO_DMA_Mode_1;
	UART_FIFOInitStruct.FIFO_RX_Trigger = UART_FIFO_RX_Trigger_1_4_Full;
	UART_FIFOInitStruct.FIFO_TX_Trigger = UART_FIFO_TX_Trigger_1_4_Full;
	UART_FIFOInitStruct.FIFO_TX_TriggerIntEnable = ENABLE;

	UART_Init(UART1, &UART_InitStructure);
	UART_FIFOInit(UART1, &UART_FIFOInitStruct);
	UART_ITConfig(UART1, UART_IT_RX_RECVD, ENABLE);

	//uart1 irq
	NVIC_InitStructure.NVIC_IRQChannel = UART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

int HOST_fileOpen(uint8_t* file_name, int io_config)
{
	return (int)yc_patch;
}

uint32_t HOST_fileRead(int fileHandle, uint32_t offset, uint32_t len, uint8_t* buff)
{
	u8 (*pbuf)[sizeof(yc_patch)]=(u8 (*)[sizeof(yc_patch)])fileHandle;	

	if(offset <= sizeof(*pbuf))
	{
		memcpy(buff, (u8*)pbuf+offset, len);
		return len;
	}
	else
	{
		return 0;
	}
}

int HOST_fileClose(int fileHandle)
{
	return 0;
}






static int YC_isCmdComplete(uint8_t* pEvent, uint8_t len, uint8_t ocf)
{
	if(0 == len || NULL == pEvent){
		return YC_ERROR;
	}
	if((0x04 	== pEvent[0])//packet type == event
	&&(0x0e == pEvent[1])//event opcode == command complete
	&&(len 	== pEvent[2] + 3)
	&&(ocf 	== pEvent[4])
	&&(YC_CMD_OGF 	== pEvent[5])
	&&(0x00 	== pEvent[6])){//status == successful
		return YC_SUCCESS;
	}else{
		return YC_ERROR;
	}
}

static int YC_softReset(void)
{
	uint8_t len = 0;
	uint8_t event[7];
	uint8_t cmd[] = {
	YC_CMD, 
	YC_CMD_RESET, 
	YC_CMD_OGF, 
	0x00
	};
//	int ret;
	
	if(sizeof(cmd) != HOST_uartSendData(cmd,sizeof(cmd))){
		return YC_ERROR;
	}
	HOST_DELAY_MS(50);
	len = HOST_uartRecieveData(event,sizeof(event),50);
	return YC_isCmdComplete(event, len, /*YC_CMD_ECHO*/YC_CMD_RESET);
}

//static int YC_setBaudrate(uint32_t baud)
//{
//	if(0 == baud){
//		return YC_ERROR;
//	}
//	uint32_t baud_param = (uint32_t)YC_UART_CLOCK/(uint32_t)baud;
//	uint8_t cmd[6] = {0};
//	
//	cmd[0] = YC_CMD;
//	cmd[1] = YC_CMD_BAUD; 
//	cmd[2] = YC_CMD_OGF;
//	cmd[3] = 0x02;//len
//	cmd[4] = (uint8_t)(baud_param & 0xff);
//	cmd[5] = (uint8_t)(baud_param >> 8 & 0x0f);
//	if(sizeof(cmd) != HOST_uartSendData(cmd,sizeof(cmd))){
//		return YC_ERROR;
//	}else{
//		return YC_SUCCESS;
//	}
//}



static int YC_Echo(void)
{
	uint8_t len = 0;
	uint8_t event[7];
	uint8_t cmd[] = {
	YC_CMD, 
	YC_CMD_ECHO, 
	YC_CMD_OGF, 
	0x00
	};
	
	if(sizeof(cmd) != HOST_uartSendData(cmd,sizeof(cmd))){// Send YC_CMD_ECHO.
		return YC_ERROR;//return an error.
	}
	HOST_DELAY_MS(2);
	len = HOST_uartRecieveData(event,sizeof(event),50);//Recievce a uart packet.
	return YC_isCmdComplete(event, len, YC_CMD_ECHO);
}

static int YC_Patch(void)
{
	int handle; //file handle.
	int offset; //read offset.
	int len; //read len.
	int rtn_val; //return value.
	uint8_t buff[256]; //file read buffer.
	uint8_t event[7]; //uart rx buff
	int file_size; // file size.
	int i;
	
	//Open patch file.
	printk("\r\n5.1 open file");
	handle = HOST_fileOpen(YC_PATCH_FILE_NAME, FILE_READ_ONLY);
	if(0 == handle){
		return YC_ERROR;
	}
	
	//Get File Size.
	printk("\r\n5.2 read file size");
	len=YC_PATCH_FILE_LEN_SIZE;	//by oushaojun
	offset=0;
	rtn_val = HOST_fileRead(handle, offset, YC_PATCH_FILE_LEN_SIZE, buff);
	if(len != rtn_val){
		return YC_ERROR;
	}
	offset = YC_PATCH_FILE_LEN_SIZE;
	file_size = buff[0] + (buff[1] << 8);

	printk("\r\n5.3 filesize=%d,patching:[", file_size);
	//Get and send blocks.
	while(offset < file_size){
		
		//Get block len.
		len=YC_PATCH_BLOCK_LEN_SIZE;	//by oushaojun
		rtn_val = HOST_fileRead(handle, offset, YC_PATCH_BLOCK_LEN_SIZE, buff);
		if(len != rtn_val){
			printk("\r\n5.4 fail 1");
			return YC_ERROR;
		}
		offset += YC_PATCH_BLOCK_LEN_SIZE;
		len = buff[0];
		
		//Get block data.
		rtn_val = HOST_fileRead(handle, offset, len, buff);
		if(len != rtn_val){
			printk("\r\n5.4 fail 2");
			return YC_ERROR;
		}
		offset += len;
		
		//Send block data;
		if(len != HOST_uartSendData(buff,len)){// Send data.
			printk("\r\n5.4 fail 3");
			return YC_ERROR;//return an error.
		}
		HOST_DELAY_MS(2);
		
		//Wait for a command complete.
		len = HOST_uartRecieveData(event,sizeof(event),50);//Recievce a uart packet.
		if((0x04 != event[0])//packet type == event
			||(0x0e != event[1])//event opcode == command complete
			||(YC_CMD_OGF != event[5])
			||(0x00 != event[6])){//status == successful

			printk("\r\n5.4 fail 4:");
			for(i=0;i<sizeof(event);i++)
			{
				printk("%02x ", event[i]);
			}
			return YC_ERROR;
		}
		
		printk("#");
		
	}//end of while(offset < file_size)
	
	printk("]\r\n5.4 success");
	//Close YC patch file.
	HOST_fileClose(handle);
	
	// end of YC_Patch().
	return YC_SUCCESS;
}


static int YC_Get_Ready(void)
{
	u16 len = 0;
	u8 	event[3];
    int status = YC_ERROR;
	len = HOST_uartRecieveData(event,sizeof(event), 1000);
    if(len == 3)
    {
        if((event[0] == 0x02)&&(event[1] == 0x09)&&(event[2] == 0x00))
        {
            status = YC_SUCCESS;
        }
    }
    return status;
}


int YC_BT_init(void)
{	
	int rtn_val;

	printk("\r\nYC_BT_init,\r\n1 init gpio & hard reset");

	//init gpio for reset pin and sleep pin
	HOST_GPIO_INIT();

	//sleep pin high to normal work mode,only pull-down to enter sleep mode
	HOST_GPIO_CONFIG(YC_SLEEP_PIN, HIGH);
	
	//STEP.1. Hard reset.
	HOST_GPIO_CONFIG(YC_RESET_PIN, LOW);
	HOST_DELAY_MS(20);
	HOST_GPIO_CONFIG(YC_RESET_PIN, HIGH);
	HOST_DELAY_MS(150);
	
	//STEP.2. Initialize HOST UART.
	printk("\r\n2 config uart");
	HOST_UART_CONFIG(DISABLE_FLOW, YC_DEFAULT_BAUDRATE);
	
	//STEP.3. Soft reset.
	printk("\r\n3 softReset");
	rtn_val = YC_softReset();
	if(YC_SUCCESS != rtn_val){
		return rtn_val;
	}
	
	//STEP.4. Set YC UART Baudrate
	//rtn_val = YC_setBaudrate(YC_WORK_BAUDRATE);
	//if(YC_SUCCESS != rtn_val){
	//	return rtn_val;
	//}
	
	//STEP.5. Set HOST UART Baudrate
	//HOST_UART_CONFIG(DISABLE_FLOW, YC_WORK_BAUDRATE);
	//HOST_DELAY_MS(1ms);
	
	//STEP.6. Echo
	printk("\r\n4 echo");
	rtn_val = YC_Echo();
	if(YC_SUCCESS != rtn_val){
		return rtn_val;
	}
	
	//STEP.7. Download YC Patch.
	printk("\r\n5 patch");
	rtn_val=YC_Patch();
	if(YC_SUCCESS != rtn_val)
	{
		return rtn_val;
	}

	//check if chip is ready
	printk("\r\n6 get ready,");
	rtn_val=YC_Get_Ready();
	if(YC_SUCCESS != rtn_val)
	{
		printk("fail");
		return rtn_val;
	}
	printk("success");
	
	return rtn_val;
	
}
