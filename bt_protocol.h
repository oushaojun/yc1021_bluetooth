

#ifndef BT_PROTOCOL_H
#define BT_PROTOCOL_H




typedef struct 
{
	unsigned char packet_type;
	unsigned char opcode;
	unsigned char length;
	unsigned char payload[300];
}
HCI_PACKET;


typedef struct
{
	unsigned char bt_visiable:1;
	unsigned char bt_connectable:1;
	unsigned char ble_visible:1;
	unsigned char bt_connected:1;
	unsigned char ble_connected:1;
}
HCI_BT_STATUS;


typedef enum
{
	PAIR_MODE_PIN_CODE=0,
	PAIR_MODE_JUST_WORK=1,
}
PAIR_MODE;


#define  PACKET_TYPE_CMD              0x01
#define  PACKET_TYPE_EVENT            0x02

//命令编码
#define  HCI_CMD_SET_BT_ADDR			0x00  // 设置 BT3.0 地址
#define  HCI_CMD_SET_BLE_ADDR			0x01  // 设置 BLE 地址
#define  HCI_CMD_SET_VISIBILITY			0x02  // 设置可发现和广播
#define  HCI_CMD_SET_BT_NAME			0x03  // 设置 BT3.0 名称
#define  HCI_CMD_SET_BLE_NAME			0x04  // 设置 BLE 名称
#define  HCI_CMD_SEND_SPP_DATA			0x05  // 发送 BT3.0（SPP）数据
#define  HCI_CMD_SEND_BLE_DATA			0x09  // 发送 BLE 数据
#define  HCI_CMD_STATUS_REQUEST			0x0B  // 请求蓝牙状态
#define  HCI_CMD_SET_PAIRING_MODE		0x0C  // 设置配对模式
#define  HCI_CMD_SET_PINCODE			0x0D  // 设置配对码
#define  HCI_CMD_SET_UART_FLOW			0x0E  // 设置 UART 流控
#define  HCI_CMD_SET_UART_BAUD			0x0F  // 设置 UART 波特率
#define  HCI_CMD_VERSION_REQUEST		0x10  // 查询模块固件版本
#define  HCI_CMD_BT_DISCONNECT			0x11  // 断开 BT3.0 连接
#define  HCI_CMD_BLE_DISCONNECT			0x12  // 断开 BLE 连接
#define  HCI_CMD_SET_COD				0x15  // 设置 COD
#define  HCI_CMD_SET_NVRAM				0x26  // 下发 NV 数据
#define  HCI_CMD_ENTER_SLEEP_MODE		0x27  // 进入睡眠模式
#define  HCI_CMD_SPP_DATA_COMPLETE		0x29  // SPP 数据处理完成
#define  HCI_CMD_SET_ADV_DATA			0x2A  // 设置 ADV 数据
#define  HCI_CMD_POWER_REQ				0x2B

// 响应事件
#define  HCI_EVENT_BT_CONNECTED        0x00   // BT3.0 连接建立
#define  HCI_EVENT_BLE_CONNECTED       0x02   // BLE 连接建立
#define  HCI_EVENT_BT_DISCONNECTED     0x03   // BT3.0 连接断开
#define  HCI_EVENT_BLE_DISCONNECTED    0x05   // BLE 连接断开
#define  HCI_EVENT_CMD_COMPLETE        0x06   // 命令已完成
#define  HCI_EVENT_SPP_DATA_RECEIVED   0x07   // 接收到 BT3.0 数据（SPP）
#define  HCI_EVENT_BLE_DATA_RECEIVED   0x08   // 接收到 BLE 数据
#define  HCI_EVENT_I_AM_READY          0x09   // 模块准备好
#define  HCI_EVENT_STAUS_RESPONSE      0x0A   // 状态回复
#define  HCI_EVENT_NVRAM_CHANGED       0x0D   // 上传 NVRAM 数据
#define  HCI_EVENT_UART_EXCEPTION      0x0F   // HCI 包格式错误


int YC_HCI_CMD_SET_BT_ADDR(u8 *address);
int YC_HCI_CMD_SET_BLE_ADDR(u8 *address);
int YC_HCI_CMD_SET_VISIBILITY(u8 bt_visible, u8 ble_visible);
int YC_HCI_CMD_SET_BT_NAME(char *name);
int YC_HCI_CMD_SET_BLE_NAME(char *name);
int YC_HCI_CMD_STATUS_REQUEST(HCI_BT_STATUS *bt_status);
int YC_HCI_CMD_SET_PAIRING_MODE(PAIR_MODE pair_mode);
int YC_HCI_CMD_SET_PINCODE(char *pincode);
int YC_HCI_CMD_SET_UART_FLOW(u8 flow);
int YC_HCI_CMD_VERSION_REQUEST(u16 *version);
int YC_HCI_CMD_BT_DISCONNECT(void);
int YC_HCI_CMD_BLE_DISCONNECT(void);
int YC_HCI_CMD_SET_NVRAM(u8 *nvram);
int YC_HCI_CMD_ENTER_SLEEP_MODE(void);
int YC_HCI_CMD_EXIT_SLEEP_MODE(void);
int YC_HCI_CMD_SEND_SPP_DATA(u8 *data, u16 len);
int YC_HCI_CMD_SEND_BLE_DATA(u8 *data, u16 len);

void HCI_Get_Connect_Status(u8 *bt, u8 *ble);
int HCI_CHECK_EVENT(HCI_PACKET *response_packet, u8 cmd_opcode);


#endif


