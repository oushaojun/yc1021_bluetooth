

/*
	this file contains bluetooth protocol functions
	there is no need to port any functions in this file
*/

#include "mhscpu_conf.h"
#include "bt_uart.h"
#include "yc_hci_boot.h"
#include "bt_protocol.h"
#include "stdio.h"


static u8 g_bt_connected=0;
static u8 g_ble_connected=0;

extern int bt_save_nvram(u8 *nvram);

/*	
	to get bluetooth connect status
	input: none
	output: bt-1 bt3.0 connected, 0- bt3.0 not connected
			ble-1 ble connected, 0-ble not connected
	return: none
*/
void HCI_Get_Connect_Status(u8 *bt, u8 *ble)
{	
	*bt=g_bt_connected;
	*ble=g_ble_connected;
}


/*
	check hci event status
	input: response_packet- packet rcvd from yc1021
	output: none
	return: 0- event check success
			others- fail
*/

int HCI_CHECK_EVENT(HCI_PACKET *response_packet, u8 cmd_opcode)
{
	int ret=-1;

	if(response_packet->packet_type != PACKET_TYPE_EVENT)
		return -2;

	switch(response_packet->opcode)
	{
		case HCI_EVENT_CMD_COMPLETE:
			if(	response_packet->payload[0]==cmd_opcode &&	//check cmd opcode
				response_packet->payload[1]==0x00)	//status is success
				{
					ret=0;
				}
		break;

		case HCI_EVENT_STAUS_RESPONSE:
			if(	response_packet->length==0x01 &&
				response_packet->opcode==cmd_opcode)
			{
				ret=0;
			}
		break;

		case HCI_EVENT_BT_CONNECTED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length==0)
				{
					printk("\r\nbt3.0 connected");
					ret=0;
					g_bt_connected=1;
				}
		break;

		case HCI_EVENT_BLE_CONNECTED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length==0)
				{
					printk("\r\nble connected");
					ret=0;
					g_ble_connected=1;
				}
		break;

		case HCI_EVENT_BT_DISCONNECTED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length==0)
				{
					printk("\r\nbt3.0 disconnected");
					ret=0;
					g_bt_connected=0;
				}
		break;

		case HCI_EVENT_BLE_DISCONNECTED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length==0)
				{
					printk("\r\nble disconnected");
					ret=0;
					g_ble_connected=0;
				}
		break;

		case HCI_EVENT_SPP_DATA_RECEIVED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length)
				{
					ret=0;
				}
		break;

		case HCI_EVENT_BLE_DATA_RECEIVED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length)
				{
					ret=0;
				}
		break;

		case HCI_EVENT_NVRAM_CHANGED:
			if(	response_packet->opcode==cmd_opcode &&
				response_packet->length==0x78)
				{
					bt_save_nvram(response_packet->payload);
					ret=0;
				}
		break;

		default:
			printk("\r\nunknown response packet opcode");
		break;
	}
	
	return ret;
}




/*
	set BT3.0 device address(mac)
	input: address- LSB 6bytes bt3.0 address
	output: none
	return: 0- success
			others- fail
*/

int YC_HCI_CMD_SET_BT_ADDR(u8 *address)
{
	HCI_PACKET packet;
	int ret;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_BT_ADDR;
	packet.length=0x06;
	memcpy(packet.payload, address, 6);

	ret=bt_send_buffer((u8*)&packet, 3+6);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_BT_ADDR);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	set BLE device address(mac)
	input: address- LSB 6 bytes ble address
	output: none
	return: 0- success
			others- fail
*/

int YC_HCI_CMD_SET_BLE_ADDR(u8 *address)
{
	HCI_PACKET packet;
	int ret;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_BLE_ADDR;
	packet.length=0x06;
	memcpy(packet.payload, address, 6);

	ret=bt_send_buffer((u8*)&packet, 3+6);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_BLE_ADDR);
	}
	else
	{
		ret=-3;
	}
	
	return ret;
}



/*
	config bt3.0 and ble visibility
	input: bt_visible-  1 to enable bt3.0 visibility
						0 to disable
		   ble_visible- 1 to enable ble visibility
		   				0 to disable
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_VISIBILITY(u8 bt_visible, u8 ble_visible)
{
	HCI_PACKET packet;
	int ret;

	bt_visible&=0x01;
	ble_visible&=0x01;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_VISIBILITY;
	packet.length=0x01;
	packet.payload[0]=bt_visible|(bt_visible<<1)|(ble_visible<<2);
	
	ret=bt_send_buffer((u8*)&packet, 3+1);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_VISIBILITY);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	config bt3.0 name
	input: name- bt3.0 name string
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_BT_NAME(char *name)
{
	HCI_PACKET packet;
	int ret;

	if(strlen(name)>0x20 || strlen(name)<0x03) return -1;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_BT_NAME;
	packet.length=strlen(name);
	strcpy((char*)packet.payload, name);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_BT_NAME);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	config ble name
	input: name- ble name string
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_BLE_NAME(char *name)
{
	HCI_PACKET packet;
	int ret;

	if(strlen(name)>0x20 || strlen(name)<0x03) return -1;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_BLE_NAME;
	packet.length=strlen(name);
	strcpy((char*)packet.payload, name);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_BLE_NAME);
	}
	else
	{
		ret=-3;
	}

	return ret;
}



/*
	send spp data
	input: 	data- data to send
			len- length of data to send(<=127)
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SEND_SPP_DATA(u8 *data, u16 len)
{
	HCI_PACKET packet;
	int ret;

	if(len > 127) return -1;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SEND_SPP_DATA;
	packet.length=len;
	memcpy(packet.payload, data, len);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SEND_SPP_DATA);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	send ble data
	input: 	data- data to send
			len- length of data to send(<=20)
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SEND_BLE_DATA(u8 *data, u16 len)
{
	HCI_PACKET packet;
	int ret;

	if(len > 20) 
	{
		printk("\r\nsend ble data len error");
		return -1;
	}

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SEND_BLE_DATA;
	packet.length=2+len;
	packet.payload[0]=0x2a;
	packet.payload[1]=0x00;
	memcpy(&packet.payload[2], data, len);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SEND_BLE_DATA);
		if(ret)
		{
			printk("\r\nsend ble data event error");
		}
	}
	else
	{
		ret=-3;
	}

	return ret;
}



/*
	to get yc1021 bluetooth status
	input: none
	output: bt_status
	return: 0- success, others- fail
*/

int YC_HCI_CMD_STATUS_REQUEST(HCI_BT_STATUS *bt_status)
{
	HCI_PACKET packet;
	int ret;

	memset(bt_status, 0, sizeof(*bt_status));

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_STATUS_REQUEST;
	packet.length=0x00;

	ret=bt_send_buffer((u8*)&packet, 3);
	ret=bt_rcv_buffer((u8*)&packet, 4, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_EVENT_STAUS_RESPONSE);
		if(!ret)
		{
			*bt_status=*((HCI_BT_STATUS*)(&packet.payload[0]));
		}
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	to set pairing mode
	input: pair_mode
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_PAIRING_MODE(PAIR_MODE pair_mode)
{
	HCI_PACKET packet;
	int ret;
	
	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_PAIRING_MODE;
	packet.length=0x01;
	packet.payload[0]=pair_mode;

	ret=bt_send_buffer((u8*)&packet, 4);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_PAIRING_MODE);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	to set pin code for bt3.0
	input: pincode string
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_PINCODE(char *pincode)
{
	HCI_PACKET packet;
	int ret;

	if(strlen(pincode) > 0x10) return -1;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_PINCODE;
	packet.length=strlen(pincode);
	strcpy((char*)packet.payload, pincode);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_PINCODE);
	}
	else
	{
		ret=-3;
	}

	return ret;
}


/*
	to config uart flow control
	input: 	flow:
			1- to enable uart flow control
			0- to disable uart flow control
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_UART_FLOW(u8 flow)
{
	HCI_PACKET packet;
	int ret;

	flow&=0x01;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_UART_FLOW;
	packet.length=1;
	packet.payload[0]=flow;

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_UART_FLOW);
	}
	else
	{
		ret=-3;
	}

	return ret;
	
}


/*
	to read yc1021 version
	input: none
	output: version
	return: 0- success, others- fail
*/

int YC_HCI_CMD_VERSION_REQUEST(u16 *version)
{	
	HCI_PACKET packet;
	int ret;

	*version=0;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_VERSION_REQUEST;
	packet.length=0;
	memset(packet.payload, 0, sizeof(packet.payload));

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 7, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_VERSION_REQUEST);
		if(!ret && packet.length==4)
		{
			*version=(packet.payload[2]<<8)|packet.payload[3];
		}
		else
		{	
			ret=-1;
		}
	}
	else
	{
		ret=-3;
	}

	return ret;
	
}



/*
	bt3.0 disconnect
	input: none
	output: none
	return: 0-success, others-fail
*/

int YC_HCI_CMD_BT_DISCONNECT(void)
{
	HCI_PACKET packet;
	int ret;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_BT_DISCONNECT;
	packet.length=0;

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_BT_DISCONNECT);
	}
	else
	{
		ret=-3;
	}

	return ret;
	
}


/*
	ble disconnect
	input: none
	output: none
	return: 0-success, others-fail
*/

int YC_HCI_CMD_BLE_DISCONNECT(void)
{
	HCI_PACKET packet;
	int ret;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_BLE_DISCONNECT;
	packet.length=0;

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_BLE_DISCONNECT);
	}
	else
	{
		ret=-3;
	}

	return ret;
}



/*
	set NVRAM
	input: 	nvram(len=0x78)
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_SET_NVRAM(u8 *nvram)
{
	HCI_PACKET packet;
	int ret;

	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_SET_NVRAM;
	packet.length=0x78;
	memcpy(packet.payload, nvram, 0x78);

	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret>0)
	{
		ret=HCI_CHECK_EVENT(&packet, HCI_CMD_SET_NVRAM);
	}
	else
	{
		ret=-3;
	}

	return ret;
}

//1 to enable command sleep mode
#define SLEEP_BY_COMMAND 0

/*
	enter sleep mode
	input: none
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_ENTER_SLEEP_MODE(void)
{
	int ret;
#if SLEEP_BY_COMMAND
	HCI_PACKET packet;
	
	packet.packet_type=PACKET_TYPE_CMD;
	packet.opcode=HCI_CMD_ENTER_SLEEP_MODE;
	packet.length=0x00;
	
	ret=bt_send_buffer((u8*)&packet, packet.length+3);
	ret=bt_rcv_buffer((u8*)&packet, 5, 200);
	if(ret == 0)
	{
		printk("\r\nyc1021 enter sleep mode ok by cmd");
		HOST_GPIO_CONFIG(YC_SLEEP_PIN, LOW);
		mdelay(20);
	}
	else
	{
		ret=-3;
	}
#else
	printk("\r\nyc1021 enter sleep mode ok by wakeup low");
	HOST_GPIO_CONFIG(YC_SLEEP_PIN, LOW);
	mdelay(20);
	ret=0;
#endif

	return ret;
}


/*
	exit sleep mode
	input: none
	output: none
	return: 0- success, others- fail
*/

int YC_HCI_CMD_EXIT_SLEEP_MODE(void)
{
	int ret;
	
	HOST_GPIO_CONFIG(YC_SLEEP_PIN, HIGH);
	mdelay(50);

#if SLEEP_BY_COMMAND
	ret=YC_BT_init();
	if(!ret)
	{
		printk("\r\nyc1021 exit sleep mode ok by cmd");
	}
#else
	printk("\r\nyc1021 exit sleep mode ok by wakeup high");
	ret=0;
#endif
	
	return ret;
}




void yc1021_test(void)
{
	u8 temp[500]={0};
	u16 templen;
	int ret;
	HCI_BT_STATUS status;

	printk("\r\nyc1021 test");

	YC_BT_init();

	ret=YC_HCI_CMD_SET_BT_ADDR("012345");
	printk("\r\nset bt3.0 address ret=%d", ret);

	ret=YC_HCI_CMD_SET_BLE_ADDR("678901");
	printk("\r\nset ble address ret=%d", ret);

	ret=YC_HCI_CMD_SET_VISIBILITY(1,1);
	printk("\r\nset visibility ret=%d", ret);

	ret=YC_HCI_CMD_SET_BT_NAME("yc1021_spp");
	printk("\r\nset bt3.0 name ret=%d", ret);

	ret=YC_HCI_CMD_SET_BLE_NAME("yc1021_ble");
	printk("\r\nset ble name ret=%d", ret);

	ret=YC_HCI_CMD_STATUS_REQUEST(&status);
	printk("\r\nget status ret=%d, status=0x%02x", ret, *((u8*)&status));

	ret=YC_HCI_CMD_SET_PAIRING_MODE(/*PAIR_MODE_JUST_WORK*/PAIR_MODE_PIN_CODE);
	printk("\r\nset pairing mode ret=%d", ret);

	ret=YC_HCI_CMD_SET_PINCODE("1234");
	printk("\r\nset pincode ret=%d", ret);

	ret=YC_HCI_CMD_SET_UART_FLOW(0);
	printk("\r\nset uart flow ret=%d", ret);

	ret=YC_HCI_CMD_VERSION_REQUEST(&templen);
	printk("\r\nget version ret=%d, version=0x%x", ret, templen);

	ret=YC_HCI_CMD_BT_DISCONNECT();
	printk("\r\nbt3.0 disconnect ret=%d", ret);

	ret=YC_HCI_CMD_BLE_DISCONNECT();
	printk("\r\nble disconnect ret=%d", ret);

	ret=YC_HCI_CMD_SET_NVRAM(temp);
	printk("\r\nset nvram ret=%d", ret);

	ret=YC_HCI_CMD_ENTER_SLEEP_MODE();
	printk("\r\nenter sleep ret=%d", ret);

	ret=YC_HCI_CMD_EXIT_SLEEP_MODE();
	printk("\r\nexit sleep ret=%d", ret);

	ret=YC_HCI_CMD_VERSION_REQUEST(&templen);
	printk("\r\nget version ret=%d, version=0x%x", ret, templen);
}









































