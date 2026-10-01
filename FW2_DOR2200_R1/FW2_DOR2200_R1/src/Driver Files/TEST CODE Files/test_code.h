/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: test_code.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 19/08/2023.
* Module
* Description	: Defines constants and macros for test_code.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 19/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef TEST_CODE_H_
#define TEST_CODE_H_

/***** System Includes *****/
#include "asf.h"

/***** User Defined Includes *****/
#include "definitions.h"

/***** Definitions *****/
/*T ---> TEST*/
#define TTX_FRAME_SIZE		(100)	// Bytes
#define URX_BUF_LEN			(100)

#define IDX_TFRAME_LEN		(0)
#define IDX_TDEST_ADD		(1)
#define IDX_TSOURCE_ADD		(2)
#define IDX_TPERIPHERAL		(3)
#define IDX_TOPERATION		(4)

#define UART_SOF			(0x23)
#define UART_EOF			(0x24)
#define DESTINATION_ADD		(2)
#define SOURCE_ADD			(1)

#define TCODE_UART_PERIPHERAL	(0x00)
#define TCODE_LED_PERIPHERAL	(0x01)
#define TCODE_INPUT_PERIPHERAL	(0x05)
#define TCODE_RELAY_PERIPHERAL	(0x09)

#define TOP_TYPE_TX				(0x01)
#define TOP_TYPE_RX				(0x00)
#define TOP_TYPE_ON				(0x01)
#define TOP_TYPE_OFF			(0x00)

/*****************************************/
#define NUM_OF_INPUTS			(8)
#define NUM_OF_OUTPUTS			(4)

/***** ENABLE OR DISABLE test codes *****/
#define EN_TEST_CODE		(ENABLE)
#define EN_UART_TESTCODE	(ENABLE)
#define EN_LED_TESTCODE		(ENABLE)
#define EN_INPUT_TESTCODE	(ENABLE)
#define EN_RELAY_TESTCODE	(ENABLE)

/***** Structure Variables *****/
typedef struct
{
	U8 frame_ready_f : 1;
	U8 frame_length;
	U8 frame_idx;
	U8 frame_data[TTX_FRAME_SIZE];
}TEST;


/***** Global Variables *****/
extern U8 gb_uart_rec_complete_f;	// Flag used to say that complete data received after a EOF.
extern U8 gb_uart_receive_buff[URX_BUF_LEN];	// Right now 100 characters are able to receive.
extern U8 gb_uart_rec_buff_len;	// Variable used to update receive buffer length.

/***** Function Prototypes *****/
void Get_Test_UART_Data(U8);
void Execute_Test_Code(void);
void Create_Test_frame(U8 PER_NUMBER, U8 SUB_PERIPHERAL_NUM, U8 OP_TYPE);
void Make_LED_On_Off(U8 op_type);

#endif /* TEST_CODE_H_ */