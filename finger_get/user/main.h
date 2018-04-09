#ifndef MAIN_H
#define MAIN_H

#define ONE_SECONDS										1000  
#define ONE_MINUTES										60000
#define LED_RUNNING_PERIOD						1000
#define FINGER_UART_RX_TIMEOUT  			20
#define LOCK_UART_RX_TIMEOUT					30

#define DATA_FROM_SERVER_TYPE_LORA	  0x01
#define DATA_FROM_SERVER_TYPE_USB	    0x02

#define SET_GET_FINGER_CMD    				0x01
#define UPLOAD_FINGER_INFO_CMD				0xA1
#define FAILED_ACK										0x55

#define TO_SERVER_HEADER_1						0x5A
#define TO_SERVER_HEADER_2						0x5A
#define FROM_SERVER_HEADER_1					0xA5
#define FROM_SERVER_HEADER_2					0xA5
#endif



//  -------		end		----------
