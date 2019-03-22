
#ifndef BT_API_H
#define BT_API_H



#include "bt_uart.h"


int bt_read_nvram(u8 *nvram);
int bt_save_nvram(u8 *nvram);
int bt_open_device(char *name, u8 *mac, PAIR_MODE pair_mode, char *pincode, u8 nvram_enable, u16 *version);
int bt_close_device(void);
int bt_get_connect_status(u8 *status);
int bt_disconnect_from_host(u8 disconnect);
int bt_rcv_data_from_host(u8 *buffer, u16 buffer_size, u16 timeout);
int bt_send_data_to_host(u8 *data, u16 len);



#endif

