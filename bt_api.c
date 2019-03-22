

/*
	bluetooth yc1021 export APIs, optional need to port below functions in this file :
	[if dont implement these two functions, for bt3.0 user need to enter pincode every time link to yc1021]
	int bt_save_nvram(u8 *nvram)
	int bt_read_nvram(u8 *nvram)
*/



#include "mhscpu_conf.h"
#include "bt_uart.h"
#include "yc_hci_boot.h"
#include "bt_protocol.h"
#include "stdio.h"
#include "bt_api.h"


static u8 g_bt_open_flag=0;

/*
	open bluetooth device
	input: 	name- bt3.0 and ble name(0x03-0x20 bytes)
			mac- bt3.0 and ble mac(6bytes)
			pair_mode- mode to set
			pincode- pincode string if pair_mode is PAIR_MODE_PIN_CODE
			nvram- enable bt3.0 nvram to write,1-to enable,0-to disable
	output:	version- yc1021 version
	return: 0-success, others-fail
*/

int bt_open_device(char *name, u8 *mac, PAIR_MODE pair_mode, char *pincode, u8 nvram_enable, u16 *version)
{
	int ret;
	u8 nvram[0x78];

	if(!g_bt_open_flag)
	{
		ret=YC_BT_init();
		if(ret) 
		{
			printk("\r\nYC_BT_init fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_BT_ADDR(mac);
		if(ret) 
		{
			printk("\r\nYC_HCI_CMD_SET_BT_ADDR fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_BLE_ADDR(mac);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_BLE_ADDR fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_VISIBILITY(1,1);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_VISIBILITY fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_BT_NAME(name);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_BT_NAME fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_BLE_NAME(name);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_BLE_NAME fail");
			return ret;
		}

		ret=YC_HCI_CMD_SET_PAIRING_MODE(pair_mode);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_PAIRING_MODE fail");
			return ret;
		}

		if(pair_mode == PAIR_MODE_PIN_CODE)
		{
			ret=YC_HCI_CMD_SET_PINCODE(pincode);
			if(ret)
			{
				printk("\r\nYC_HCI_CMD_SET_PINCODE fail");
				return ret;
			}
		}

		/*
		ret=YC_HCI_CMD_SET_UART_FLOW(0);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_SET_UART_FLOW fail");
			return ret;
		}*/

		ret=YC_HCI_CMD_VERSION_REQUEST(version);
		if(ret)
		{
			printk("\r\nYC_HCI_CMD_VERSION_REQUEST fail");
			return ret;
		}

		if(nvram_enable)
		{
			ret=bt_read_nvram(nvram);
			if(!ret)
			{
				printk("\r\nread nvram ok, write into yc1021");
				ret=YC_HCI_CMD_SET_NVRAM(nvram);
				if(ret)
				{
					printk("\r\nYC_HCI_CMD_SET_NVRAM fail");
					return ret;
				}
			}
			else
			{
				printk("\r\nread nvram fail");
			}
		}

		printk("\r\nbt_open_device success=0x%x", *version);
		
		g_bt_open_flag=1;
	}

	return 0;
}



/*
	close bluetooth device
	input: none
	output: none
	return: 0- success, others- fail
*/

int bt_close_device(void)
{
	if(g_bt_open_flag)
	{
		mdelay(100);
		HOST_GPIO_CONFIG(YC_RESET_PIN, LOW);
		mdelay(20);
		HOST_GPIO_CONFIG(YC_RESET_PIN, HIGH);
		mdelay(150);

		printk("\r\nbt_close_device success");
		g_bt_open_flag=0;
	}

	return 0;
}



/*
	get bluetooth connect status
	input: none
	output: bt-0-disconnected, 1-bt3.0 connected, 2-ble connected
	return: 0- success, others- fail
*/

int bt_get_connect_status(u8 *status)
{
	u8 bt,ble;
	
	HCI_Get_Connect_Status(&bt, &ble);
	if(bt || ble)	*status=(bt&0x01)|((ble&0x01)<<1);
	else			*status=0x00;

	return 0;
}



/*
	disconnect from bluetooth host
	input: 	disconnect:1- to disconnect from host, 0-dont disconnect from host
	output: none
	return: 0-success, others-fail
*/

int bt_disconnect_from_host(u8 disconnect)
{
	int ret=0;
	u8 bt,ble;

	if(disconnect)
	{
		HCI_Get_Connect_Status(&bt, &ble);

		if(bt)			ret=YC_HCI_CMD_BT_DISCONNECT();
		else if(ble)	ret=YC_HCI_CMD_BLE_DISCONNECT();
	}

	return ret;
}



/*
	bluetooth rcv raw data from host, should be called periodically, because some state machine rely on this function
	input: 	buffer_size- max len can rcv
			timeout- timeout to rcv
	output: buffer- to save data
	return: <=0 - error
			>0  - data length actually rcvd
*/

static u8 	g_rcv_buffer[1000];
static u16 	g_rcv_len=0;

int bt_rcv_data_from_host(u8 *buffer, u16 buffer_size, u16 timeout)
{
	u8 temp[500];
	int ret;
	int i,j,k,templen;
	HCI_PACKET *packet;

	i=0;
	j=0;
	ret=bt_rcv_buffer(temp, sizeof(temp), timeout);
	if(ret>0)
	{
		k=ret;
		while(1)
		{
			while(temp[i]!=PACKET_TYPE_EVENT && i<k) 
			{
				i++;
			}
			if(i>=k)
			{
				break;
			}

			packet=(HCI_PACKET*)&temp[i];
			ret=HCI_CHECK_EVENT(packet, packet->opcode);
			if(ret)
			{
				printk("\r\nrcv data parse event fail,opcode=%02x", packet->opcode);
				return -2;
			}
			else
			{
				i+=packet->length+3;
			}


			if(	packet->opcode==HCI_EVENT_SPP_DATA_RECEIVED ||
				packet->opcode==HCI_EVENT_BLE_DATA_RECEIVED)
				{
					if(packet->payload+packet->length > temp+k)
					{
						templen=(packet->payload+packet->length)-(temp+k);
						
						printk("\r\nrcv more data=%d", templen);
						ret=bt_rcv_buffer(temp+k, templen, timeout);
						if(ret<=0)
						{	
							printk("\r\nrcv more data fail");
							return -4;
						}
					}
				}

			if(packet->opcode==HCI_EVENT_SPP_DATA_RECEIVED)
			{
				memcpy(g_rcv_buffer+g_rcv_len, packet->payload, packet->length);
				g_rcv_len+=packet->length;
			}
			else if(packet->opcode==HCI_EVENT_BLE_DATA_RECEIVED)
			{
				memcpy(g_rcv_buffer+g_rcv_len, packet->payload+2, packet->length-2);
				g_rcv_len+=packet->length-2;
			}

			if(g_rcv_len>sizeof(g_rcv_buffer))
			{
				printk("\r\nrcv data parse overflow, index=%d", g_rcv_len);
				g_rcv_len=0;
				return -3;
			}
		}
	}

	if(g_rcv_len)
	{
		j=g_rcv_len>buffer_size?buffer_size:g_rcv_len;
		memcpy(buffer, g_rcv_buffer, j);
		memmove(g_rcv_buffer, g_rcv_buffer+j, g_rcv_len-j);
		g_rcv_len-=j;
	}

	return j;
}

/*
	bt yc1021 send data to host, a connect needed before call this function
	input: data, len
	output: none
	return: 0-success, others-fail
*/
#define SPP_BLOCK_SIZE 127
#define BLE_BLOCK_SIZE 20

int bt_send_data_to_host(u8 *data, u16 len)
{
	u8 status;
	int ret=-3;
	u8 templen;
	
	bt_get_connect_status(&status);
	if(status == 0) return -1;

	if(status == 0x01)
	{
		while(1)
		{
			if(len==0) break;
		
			templen=len>SPP_BLOCK_SIZE?SPP_BLOCK_SIZE:len;
			
			ret=YC_HCI_CMD_SEND_SPP_DATA(data, templen);
			if(!ret)
			{
				data+=templen;
				len-=templen;
			}
			else
			{
				printk("\r\nspp send data fail, ret=%d", ret);
				break;
			}
		}
	}
	else if(status == 0x02)
	{
		while(1)
		{
			if(len==0) break;

			templen=len>BLE_BLOCK_SIZE?BLE_BLOCK_SIZE:len;

			ret=YC_HCI_CMD_SEND_BLE_DATA(data, templen);
			if(!ret)
			{
				data+=templen;
				len-=templen;
			}
			else
			{
				printk("\r\nble send data fail, ret=%d", ret);
				break;
			}
		}
	}
	else
	{
		return -2;
	}

	return ret;
}



/*
	bt yc1021 enter sleep mode, after entering sleep, a wakeup needs to be performed before yc1021 can be functional
	input: none
	output: none
	return: 0-success,others-fail
*/

int bt_enter_sleep_mode(void)
{
	return YC_HCI_CMD_ENTER_SLEEP_MODE();
}


/*
	to wakeup yc1021
	input: none
	output: none
	return: 0-success, others-fail
*/
int bt_exit_sleep_mode(void)
{
	return YC_HCI_CMD_EXIT_SLEEP_MODE();
}


/*
	to save nvram from yc1021, len is 0x78
	input: nvram- data from yc1021
	output: none
	return: 0-success,others-fail
*/
int bt_save_nvram(u8 *nvram)
{
	printk("\r\nsave nvram data");
	return 0;
}

/*
	to read nvram stored in mcu or filesystem which is rcvd from yc1021 and saved by mcu by calling bt_save_nvram
	input: none
	output: nvram-data from yc1021 and previously saved in mcu
	return: 0-success, others-fail
*/
int bt_read_nvram(u8 *nvram)
{
	printk("\r\nread nvram data");
	return -1;
}




void yc1021_api_test(void)
{
	int ret;
	u16 version;
	u8 buffer[500];
	int i;
	u8 status=0,temp;
	
	ret=bt_open_device("sunmi_bt", "543210", PAIR_MODE_PIN_CODE, "1234", 0, &version);
	if(ret)
	{
		printk("\r\nbt_open_device fail");
		return;
	}
	else
	{
		printk("\r\nbt version=0x%x", version);
	}

	while(1)
	{

		bt_get_connect_status(&temp);
		if(temp != status)
		{
			status=temp;
			printk("\r\nconnect status change=0x%02x", status);
		}

		ret=bt_rcv_data_from_host(buffer, sizeof(buffer), 500);
		if(ret>0)
		{
			printk("#");
			/*
			printk("\r\nrcvd raw data %d:", ret);
			for(i=0;i<ret;i++)
			{
				printk("%02x ", buffer[i]);
			}*/

			ret=bt_send_data_to_host(buffer, ret);
			//printk("\r\nsend data to host ret=%d", ret);

			//YC_HCI_CMD_ENTER_SLEEP_MODE();
			//mdelay(3000);
			//YC_HCI_CMD_EXIT_SLEEP_MODE();
			
			if(!strcmp((char*)buffer, "close")) break;

			if(!strcmp((char*)buffer, "cut"))
			{
				ret=bt_disconnect_from_host(1);
				printk("\r\ndisconnect ret=%d", ret);
				continue;
			}
		}
	}

	bt_close_device();
}










