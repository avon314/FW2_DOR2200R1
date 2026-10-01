/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: test_code.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 19/08/2023.
* Module
* Description	:	
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 19/08/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"
#include "compiler.h"

/***** User Defined Includes *****/
#include "test_code.h"
#include "definitions.h"
#include "user_uart.h"
#include "CRC.h"
#include "relay_output.h"
#include "digital_input.h"

U8 uart_rec_idx = 0;	// Variable used to update character receive index.
U8 uart_able_to_rec_f = 0;	// Flag used to set after SOF is received.
U8 gb_crc_validate = 0;	// Variable to check calculated CRC correct or not.
U8 gb_uart_rec_complete_f = 0;	// Flag used to say that complete data received after a EOF.
U8 gb_uart_receive_buff[URX_BUF_LEN] = {0};	// Right now 100 characters are able to receive.
U8 gb_uart_rec_buff_len = 0;	// Variable used to update receive buffer length.
U8 gb_test_frame_rec_f = 0;	// Flag used to say that test frame received successful after validation.

TEST testCode = {0};
U8 input_arr[NUM_OF_INPUTS] = {0};
U8 relay_number = 0;
U8 ip_idx = 0;

/*****************************************************************************
* Function name	: void Get_Test_UART_Data(U8 rec_byte)
* Returns		: Nothing.
* Arguments    	: U8 rec_byte ---> pass a received byte to a function.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
*
* Description	:	Get whole frame data in this function. If test code is enabled.
					or if not enabled direct get number of bytes into an array.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Get_Test_UART_Data(U8 rec_byte)
{
	#if (EN_TEST_CODE == uTRUE)
		if (rec_byte == UART_SOF)
		{
			uart_rec_idx = 0;	// Make index value 0 to fill from start.
			uart_able_to_rec_f = uSET;	// Set this flag once SOF is received.
		}
		else if (rec_byte == UART_EOF)
		{
			uart_able_to_rec_f = uRESET;	// Reset this flag once completely received data.
			gb_uart_rec_complete_f = uSET;	// Set this flag to say that data received completely.
			gb_uart_rec_buff_len = uart_rec_idx;	// Update received number of bytes.
		}
		else if (uart_able_to_rec_f == uSET)
		{
			gb_uart_receive_buff[uart_rec_idx] = rec_byte;
			uart_rec_idx ++;	// Increment index after a char is received.
		}
		else
		{
			__NOP();
		}
	#endif
}

/*****************************************************************************
* Function name	: void Execute_Test_Code(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
*
* Description	:	Decoding of received test frame is written in this.
					And based on peripheral id and operation transmit or receive
					can be executed.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Execute_Test_Code(void)
{
	#if (EN_TEST_CODE == uENABLE)
	if (gb_uart_byte_rec_f == uSET)
	{
		gb_uart_byte_rec_f = uFALSE;	// Reset this flag once received a byte.
		Get_Test_UART_Data(gb_uart_rec_byte);
	}
	else
	{
		__NOP();
	}
	
	if (gb_uart_rec_complete_f == uSET)
	{
		gb_uart_rec_complete_f = uRESET;	// Reset this flag once completely received data after reading a flag.		
		
		if (gb_uart_rec_buff_len < URX_BUF_LEN)
		gb_crc_validate = CRC_Calculate_Check(gb_uart_receive_buff, gb_uart_rec_buff_len, CRC_CHECK);
		
		if (gb_crc_validate == uTRUE)
		{
			/*Yes CRC matches.*/
			if (gb_uart_receive_buff[IDX_TFRAME_LEN] == gb_uart_rec_buff_len)
			{
				/*If frame length and received bytes are matches.*/
				if ((gb_uart_receive_buff[IDX_TDEST_ADD] == DESTINATION_ADD)
				&& (gb_uart_receive_buff[IDX_TSOURCE_ADD] == SOURCE_ADD))
				{
					gb_test_frame_rec_f = uSET;	// Set this flag to say frame received correctly.
				}
				else
				{
					// Destination or source address doesn't match.
				}
			}
			else
			{
				// Length doesn't matches.
			}
		}
		else
		{
			// CRC error!.
		}
	}
	else
	{
		__NOP();
	}
	
	
	if (gb_test_frame_rec_f == uSET)
	{
		gb_test_frame_rec_f = uRESET;	// Reset this flag once read received frame.
		
		switch (gb_uart_receive_buff[IDX_TPERIPHERAL])
		{
			case TCODE_UART_PERIPHERAL :
				#if (EN_UART_TESTCODE == uENABLE)
				switch (gb_uart_receive_buff[IDX_TOPERATION])
				{
					case TOP_TYPE_TX : 
						Create_Test_frame(TCODE_UART_PERIPHERAL, 0, TOP_TYPE_TX);
					break;
					case TOP_TYPE_RX : 
						Create_Test_frame(TCODE_UART_PERIPHERAL, 0, TOP_TYPE_RX);
					break;
					default:
						// If any error!
					break;
				}
				#endif
			break;
			case TCODE_LED_PERIPHERAL :
				#if (EN_LED_TESTCODE == uENABLE)
				switch (gb_uart_receive_buff[IDX_TOPERATION])
				{
					case TOP_TYPE_ON :
						Make_LED_On_Off(TOP_TYPE_ON);
						Create_Test_frame(TCODE_UART_PERIPHERAL, 0, TOP_TYPE_ON);
					break;
					case TOP_TYPE_OFF :
						Make_LED_On_Off(TOP_TYPE_OFF);
						Create_Test_frame(TCODE_UART_PERIPHERAL, 0, TOP_TYPE_OFF);
					break;
					default:
						// If any error!
					break;
				}
				#endif
			break;
			case TCODE_INPUT_PERIPHERAL :
				#if (EN_INPUT_TESTCODE == uENABLE)
					ip_idx = 0;
					input_arr[ip_idx++] = digInput.ip1_htl_f;
					input_arr[ip_idx++] = digInput.ip2_htl_f;
					input_arr[ip_idx++] = digInput.ip3_htl_f;
					input_arr[ip_idx++] = digInput.ip4_htl_f;
					input_arr[ip_idx++] = digInput.ip5_htl_f;
					input_arr[ip_idx++] = digInput.ip6_htl_f;
					input_arr[ip_idx++] = digInput.ip7_htl_f;
					input_arr[ip_idx++] = digInput.ip8_htl_f;
					
					for (int jdx = 0; jdx < NUM_OF_INPUTS; jdx++)
					{
						if (NUM_OF_INPUTS > 1)
						{
							Create_Test_frame(TCODE_INPUT_PERIPHERAL, (jdx + 1), input_arr[jdx]);
							
							while ((testCode.frame_ready_f == uRESET) && (gb_uart_ready_f == uSET));
							testCode.frame_ready_f = uRESET;	// Reset this flag once you send the data
							Send_Frame_On_UART(testCode.frame_data, testCode.frame_length);
						}
						else
						{
							Create_Test_frame(TCODE_INPUT_PERIPHERAL, (jdx + 1), input_arr[jdx]);
						}
					}
				#endif
			break;
			case TCODE_RELAY_PERIPHERAL :
				#if (EN_RELAY_TESTCODE == uENABLE)
				relay_number = gb_uart_receive_buff[IDX_TPERIPHERAL + 1];
				switch (gb_uart_receive_buff[IDX_TOPERATION + 1])
				{
					case TOP_TYPE_ON :
						Operate_Relay(relay_number, TOP_TYPE_ON);
						Create_Test_frame(TCODE_RELAY_PERIPHERAL, relay_number, TOP_TYPE_ON);
					break;
					case TOP_TYPE_OFF :
						Operate_Relay(relay_number, TOP_TYPE_OFF);
						Create_Test_frame(TCODE_RELAY_PERIPHERAL, relay_number, TOP_TYPE_OFF);
					break;
					default:
					// If any error!
					break;
				}
				#endif
			break;
			default:
				
			break;
		}
		
		for (U32 idx = 0; idx < gb_uart_rec_buff_len; idx++)
		{
			/*Clear the Buffer after executing the data.*/
			gb_uart_receive_buff[idx] = 0;
		}
		gb_uart_rec_buff_len = 0;	// Clear length.
		
	}
	else
	{
		__NOP();
	}
	
	if ((testCode.frame_ready_f == uSET) && (gb_uart_ready_f == uSET))
	{
		testCode.frame_ready_f = uRESET;	// Reset this flag once you send the data
		
		Send_Frame_On_UART(testCode.frame_data, testCode.frame_length);
	}
	else
	{
		__NOP();
	}
	#endif
}

void Create_Test_frame(U8 PER_NUMBER, U8 SUB_PERIPHERAL_NUM, U8 OP_TYPE)
{
	#if (EN_TEST_CODE == uENABLE)
	testCode.frame_idx = 0;
	testCode.frame_data[testCode.frame_idx++] = UART_SOF;	// Start of frame.
	testCode.frame_data[testCode.frame_idx++] = 8;			// Frame length (update at last).
	testCode.frame_data[testCode.frame_idx++] = 0x01;		// Destination Address.
	testCode.frame_data[testCode.frame_idx++] = 0x02;		// Source Address.
	testCode.frame_data[testCode.frame_idx++] = PER_NUMBER;	// Peripheral Number.
	if ((PER_NUMBER == TCODE_INPUT_PERIPHERAL) || (PER_NUMBER == TCODE_RELAY_PERIPHERAL))
	{
		testCode.frame_data[testCode.frame_idx++] = SUB_PERIPHERAL_NUM;	// Sub Peripheral Number.
	}
	testCode.frame_data[testCode.frame_idx++] = OP_TYPE;	// Operation Type.
	if (testCode.frame_idx < TTX_FRAME_SIZE)
	CRC_Calculate_Check(&testCode.frame_data[1], (testCode.frame_idx - 1), CRC_CALC);
	testCode.frame_data[testCode.frame_idx++] = gb_Temp_CRC[0];	// CRC lower byte.
	testCode.frame_data[testCode.frame_idx++] = gb_Temp_CRC[1];	// CRC higher byte.
	testCode.frame_data[testCode.frame_idx++] = UART_EOF;		// End of Frame.
	testCode.frame_ready_f = uSET;	// Set this flag to say that frame is ready to send.
	testCode.frame_length = testCode.frame_idx;
	#endif
}

void Make_LED_On_Off(U8 op_type)
{
	switch (op_type)
	{
		case TOP_TYPE_ON : 
			pio_set(PIOD, PIO_PD31);
		break;
		case TOP_TYPE_OFF : 
			pio_clear(PIOD, PIO_PD31);
		break;
		default:
		break;
	}
}