/*****************************************************************************
*							   Copyright (c)
*                    Avon Building Solutions Pvt Ltd
*	                        All RIGTHS RESERVED
*
* Module Name	: osdp_protocol_master.c
* Created By	: Ranjitkumar Ainapure
* Created Date	: 12-07-2022
* Module
* Description	: OSDP protocol based functions are defined here.
*
* Device Used	:
* Controller	: STM32F207VET6
*                 512 KB -----> Flash Memory.
*                 128 KB -----> RAM Memory.
*
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 12-07-2022
* Changes		: NA
*****************************************************************************/
#include "stdlib.h"
#include "string.h"

/***** Include Header Files *****/
#include "osdp_protocol_master.h"
#include "osdp_sc.h"
#include "user_uart.h"
#include "rs485_driver.h"
#include "led_operation.h"
//#include "user_timer.h"
/***** End of Include Header Files *****/

volatile uint8_t gb_TransmitFrameBuffer[OSDP_TX_FRAME_SIZE]; /*transmit buffer for OSDP frame_data*/
volatile uint8_t gb_ReceiveFrameBuffer[OSDP_RX_FRAME_SIZE];	/*receive buffer for OSDP frame_data*/

volatile uint8_t rcv_idx = 0;	// receive index OSDP frame_data
volatile uint8_t rcv_f = 0;		// Enable receive flag based on OSDP SOM command
volatile uint8_t gb_receiving_f = FALSE;	// Flag used to indicate that, RS485 data received completely.
U8 gb_osdp_rfid_data[7]	= {0};	/* Array to store received RFID data. */
U8 gb_osdp_rfid_len = 0;	/* Store a length of received RFID bytes. */
uint8_t gb_Temp_CRC1[2]={0};	// Array to store CRC 2 byte data.
uint8_t OSDP_CTRL_ID=0;		// Variable to store Control byte as per OSDP protocol.

U8	gb_osdp_emg_rst_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_emg_det_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_cap_det_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_dr_release_f	= FALSE,		// Set this flag once received the signal.
	gb_osdp_rfid_rec_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_rfid_raw_f		= FALSE,
	gb_osdp_acs_denied_f	= FALSE,		// Set this flag once received the signal.
	gb_osdp_free_acs_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_privacy_f		= FALSE,		// Set this flag once received the signal.
	gb_osdp_stand_alone_f	= FALSE,		// Set this flag once received the signal.
	gb_osdp_normal_f		= FALSE;		// Set this flag once received the signal.

uint16_t gb_tframe_length=0;// TransmitFrameBuffer length.

int frame_idx=0;			// Variable used to increment the frame indexing and also used to calculate length of frame.

OSDP_BUZZER gb_Buzzer_Setting = {0, 2, 2, 2, 3};
OSDP_LED gb_LED_Setting = {0, 0, 2, 2, 2, 2, 1, 12, 0, 0, 1, 1, 2, 1};
OSDP_COMMAND gb_Command;
OSDP_FileTransfer gb_ft_data;
OSDP_Comm_SET gb_comm_n= {1, 9600};
PDCAP PD_Info={0};
FLAGS Flag={0};
OSDP_INFO gb_osdp={0,0,1};

struct osdp_pd *pd;

extern bool gb_slave_aux_audio;

/*****************************************************************************
* Function name	: void OSDP_Frame_Build(uint8_t OSDP_CMD, uint8_t slave_address)
* Returns		: None.
* Arguments		: uint8_t OSDP_CMD ---> Pass OSDP Command,
* 				  uint8_t slave_address ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure
* Date created	: 12-07-2022
* Description	: This function takes slave address and build frame for OSDP_POLL,
* 				  CMD_OSDP_ID, OSDP_ISTAT, OSDP_OSTAT, CMD_OSDP_DETECT_EMERGENCY,
* 				  CMD_OSDP_SET_EMERGENCY, CMD_OSDP_INTERLOCK, CMD_OSDP_ACCESS_DENIED,
* 				  CMD_OSDP_CAP, CMD_OSDP_LED, CMD_OSDP_BUZ, CMD_OSDP_COMSET,
* 				  CMD_OSDP_KEYSET, CMD_OSDP_CHLNG, CMD_OSDP_SCRYPT
*
*               :
* Notes			: NA
* Global Variables Affected	: gb_TransmitFrameBuffer[] ---> update transmit buffer
* 							  gb_Temp_CRC1[] ---> calculated crc is updated here
* 							  gb_tframe_length ---> update frame_data length.
*****************************************************************************/
void OSDP_Frame_Build(uint8_t OSDP_CMD, uint8_t slave_address)
{
	frame_idx = 0;
	int tIdx=0;

	frame_idx = Fill_OSDP_Header(frame_idx, slave_address);
	frame_idx = Update_OSDP_Control_ID(frame_idx);

	switch (OSDP_CMD)
	{
		case CMD_OSDP_COMSET:	// OSDP Communication Configuration Command.
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.
			gb_TransmitFrameBuffer[frame_idx++] = gb_comm_n.pd_comn_id;	// Configure PD Id or Slave Number.
			gb_TransmitFrameBuffer[frame_idx++] = (gb_comm_n.pd_baudrate & 0x000000FF);	// Configure PD Baudrate LSB.
			gb_TransmitFrameBuffer[frame_idx++] = (gb_comm_n.pd_baudrate & 0x0000FF00)>>8;	// Configure PD Baudrate LSB1.
			gb_TransmitFrameBuffer[frame_idx++] = (gb_comm_n.pd_baudrate & 0x00FF0000)>>16;// Configure PD Baudrate LSB2.
			gb_TransmitFrameBuffer[frame_idx++] = (gb_comm_n.pd_baudrate & 0xFF000000)>>24;// Configure PD Baudrate MSB.
			break;
		case CMD_OSDP_LED:	// OSDP LED command.
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.
			OSDP_LED_DATA(gb_LED_Setting);
			break;
		case CMD_OSDP_BUZ:	// OSDP BUZZER command.
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.
			OSDP_BUZZER_DATA(gb_Buzzer_Setting);
			break;
		case CMD_OSDP_CHLNG:	// OSDP Challenge and Secure Session Initialization Request command.
			gb_TransmitFrameBuffer[frame_idx++] = 3;
			gb_TransmitFrameBuffer[frame_idx++] = SCS_11;
			gb_TransmitFrameBuffer[frame_idx++] = gb_osdp.scbk_default_f ? 0 : 1;
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.

			for (tIdx=0; tIdx<8; tIdx++)
			{
				gb_osdp.cp_random_num[tIdx] = tIdx+1;
				//gb_osdp.cp_random_num[tIdx] = rand()%255+1;
				gb_TransmitFrameBuffer[frame_idx++] = gb_osdp.cp_random_num[tIdx];
			}
			break;
		case CMD_OSDP_SCRYPT:	// OSDP Server’s Random Number and Server Cryptogram command.
			osdp_compute_cp_cryptogram();
//			Print_Message("\nCalculated CP's cryptogram is:\n");
//			for (int jk=0; jk<16; jk++)
//			{
//				Print_Number(gb_cp_cryptogram[jk]);
//				UART_Debug_PutChar(',');
//			}
			gb_TransmitFrameBuffer[frame_idx++] = 3;
			gb_TransmitFrameBuffer[frame_idx++] = SCS_13;
			gb_TransmitFrameBuffer[frame_idx++] = gb_osdp.scbk_default_f ? 0 : 1;
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.

			for (tIdx=0; tIdx<16; tIdx++)
			{
				gb_TransmitFrameBuffer[frame_idx++] = gb_osdp.cp_cryptogram[tIdx];
			}
			break;
		case CMD_OSDP_KEYSET:	// OSDP KEYSET command which sends the SCBK to the PD.
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.
			gb_TransmitFrameBuffer[frame_idx++] = 0x01;		// 0x01 for Secure channel base key.
			gb_TransmitFrameBuffer[frame_idx++] = 16;		// Key length in bytes.

			memcpy(gb_TransmitFrameBuffer+frame_idx, gb_osdp.scbk_value, 16);
			frame_idx += 16;
			break;
		case CMD_OSDP_SET_EMERGENCY:	// Command code for Set Emergency.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_EMERGENCY;	// SET EMERGENCY COMMAND.
			break;
		case CMD_OSDP_SET_INTERLOCK:	// Command code for Set Interlock.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_INTERLOCK;	// SET INTERLOCK COMMAND.
			break;
		case CMD_OSDP_SET_ACS_DENIED:	// Command code for Set Access Denied Mode.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_ACS_DENIED;	// SET Access Denied COMMAND.
			/*****
			* Data is:
			*****/
			gb_TransmitFrameBuffer[frame_idx++] = gb_slave_aux_audio;	// Get Value of AUX AUDIO.
			break;
		case CMD_OSDP_SET_FREE_ACCESS:	// Command code for Set Free Access Mode.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_FREE_ACCESS;	// SET Free Access Mode COMMAND.
			/*****
			* Data is:
			*****/
			gb_TransmitFrameBuffer[frame_idx++] = gb_slave_aux_audio;	// Get Value of AUX AUDIO.
			break;
		case CMD_OSDP_SET_STAND_ALONE:	// Command code for Set Stand Alone Mode.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_STAND_ALONE;	// SET Stand Alone Mode COMMAND.
			break;
		case CMD_OSDP_SET_NORMAL_MODE:	// Command code for Set Normal Mode.
			gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_SET_NORMAL_MODE;	// SET Normal Mode COMMAND.
			/*****
			* Data is:
			*****/
			gb_TransmitFrameBuffer[frame_idx++] = gb_slave_aux_audio;	// Get Value of AUX AUDIO.
			break;
		default:
			if (TRUE == gb_osdp.mac_enable_f)
			{
				gb_TransmitFrameBuffer[frame_idx++] = 2;
				gb_TransmitFrameBuffer[frame_idx++] = SCS_15;
				//gb_TransmitFrameBuffer[frame_idx++] = 0;
			}
			gb_TransmitFrameBuffer[frame_idx++] = OSDP_CMD;		// OSDP command.
			break;
	}

	Fill_OSDP_Footer_Bytes(frame_idx);
}

/*****************************************************************************
* Function name : int Fill_OSDP_Header(int frm_idx, U8 slvAddress)
* Returns       : int - The updated frame index after filling the header.
* Arguments     : int frm_idx - The current frame index.
*               : U8 slvAddress - The address of the slave device.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : Function is written to fill the OSDP header into the transmit
*                 frame buffer.
* Notes         : This function assumes gb_TransmitFrameBuffer is a global array.
* Global Variables Affected : gb_TransmitFrameBuffer[].
*****************************************************************************/
int Fill_OSDP_Header(int frm_idx, U8 slvAddress)
{
	#if LINE_IDLE
	gb_TransmitFrameBuffer[frm_idx++] = 0xFF;			// Set all bits to one.
	#endif

	gb_TransmitFrameBuffer[frm_idx++] = OSDP_SOM;		// Start of message command
	gb_TransmitFrameBuffer[frm_idx++] = slvAddress;		// Put slave address
	gb_TransmitFrameBuffer[frm_idx++] = 0;				// Fill length of the frame.
	gb_TransmitFrameBuffer[frm_idx++] = 0;				// Fill length of the frame.
	
	return frm_idx;
}

/*****************************************************************************
* Function name : int Update_OSDP_Control_ID(int frm_idx)
* Returns       : int - The updated frame index after updating the control ID.
* Arguments     : int frm_idx - The current frame index.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : Function is written to update the OSDP control ID in the
*                 transmit frame buffer based on the sequence number, CRC
*                 enable flag, and secure channel enable flag.
* Notes         : This function assumes gb_TransmitFrameBuffer is a global array.
*                 It also uses the global structure gb_osdp and the global
*                 variable OSDP_CTRL_ID.
* Global Variables Affected : gb_TransmitFrameBuffer[], OSDP_CTRL_ID.
*****************************************************************************/
int Update_OSDP_Control_ID(int frm_idx)
{
	if ((gb_osdp.sequence_num < 4))
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xFC) | gb_osdp.sequence_num);
	else
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xFC) | 0x01);

	if (gb_osdp.crc_enable_f == 1)
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xFB) | 0x04);
	else
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xFB) | 0);

	if (gb_osdp.sc_enable_f == 1)
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xF7) | 0x08);
	else
	OSDP_CTRL_ID = ((OSDP_CTRL_ID & 0xF7) | 0);

	gb_TransmitFrameBuffer[frm_idx++] = OSDP_CTRL_ID;	// Control ID
	
	return frm_idx;
}

/***********************************************************************************
* Function name	: void OSDP_OUT_Frame_Build(uint8_t slave_address, uint8_t output_no,
* 				  uint8_t control_code, uint8_t Timer_lsb, uint8_t Timer_msb)
* Returns		: None.
* Arguments		: uint8_t slave_address	---> Pass the slave address.
* 				  uint8_t output_no		---> Pass the number of the output.
* 				  uint8_t control_code	---> Enter control code, (see OSDP data sheet)
* 				  uint8_t Timer_lsb		---> Enter output ON time in mili seconds.
* 				  uint8_t Timer_lsb		---> Enter output OFF time in mili seconds.
*
* Created by	: Ranjitkumar Ainapure
* Date created	: 15-07-2022
* Description	: This function takes slave address, output no and control code
* 				  and timer of LSB and MSB to build OUT command.
*               :
* Notes			: NA
* Global Variables Affected	: gb_TransmitFrameBuffer[] ---> update transmit buffer
* 							  gb_Temp_CRC1[] ---> calculated crc is updated here
* 							  gb_tframe_length ---> update frame_data length.
*********************************************************************************/
void OSDP_OUT_Frame_Build(uint8_t slave_address, uint8_t output_no, uint8_t control_code, uint8_t Timer_lsb, uint8_t Timer_msb)
{
	frame_idx = 0;

	frame_idx = Fill_OSDP_Header(frame_idx, slave_address);
	frame_idx = Update_OSDP_Control_ID(frame_idx);
	
	gb_TransmitFrameBuffer[frame_idx++] = CMD_OSDP_OUT;	// CONST_OSDP_OUT command.
	gb_TransmitFrameBuffer[frame_idx++] = output_no;	// Output number (which output need to be updated).
	gb_TransmitFrameBuffer[frame_idx++] = control_code;	// Control code of OSDP_OUT...(refer data sheet for more)
	gb_TransmitFrameBuffer[frame_idx++] = Timer_lsb;	// ON time...
	gb_TransmitFrameBuffer[frame_idx++] = Timer_msb;	// OFF time...
	
	Fill_OSDP_Footer_Bytes(frame_idx);
}

/***********************************************************************************
* Function name	: void OSDP_LED_DATA(OSDP_LED LED)
* Returns		: None.
* Arguments		: OSDP_LED LED	---> Structure variables holds the setting of ISDP LED.
*
* Created by	: Ranjitkumar Ainapure
* Date created	: 16-07-2022
* Description	: This function build frame for configuration provided for LED
* 				  operation at slave side.
*               :
* Notes			: NA
* Global Variables Affected	: gb_TransmitFrameBuffer[] ---> update transmit buffer
* 							  gb_Temp_CRC1[] ---> calculated crc is updated here
* 							  gb_tframe_length ---> update frame_data length.
*********************************************************************************/
void OSDP_LED_DATA(OSDP_LED LED)
{
	gb_TransmitFrameBuffer[frame_idx++] = LED.reader_number;// Reader number.
	gb_TransmitFrameBuffer[frame_idx++] = LED.led_number;	// LED number.
	gb_TransmitFrameBuffer[frame_idx++] = LED.temp_code;	// Temporary code.
	gb_TransmitFrameBuffer[frame_idx++] = LED.temp_on_time;	// Temporary ON time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.temp_off_time;// Temporary OFF time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.temp_on_colour;// Temporary ON time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.temp_off_colour;// Temporary OFF time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.timer_lsb;	// Timer value LSB.
	gb_TransmitFrameBuffer[frame_idx++] = LED.timer_msb;	// Timer value MSB.
	gb_TransmitFrameBuffer[frame_idx++] = LED.permanent_code;// Permanent code.
	gb_TransmitFrameBuffer[frame_idx++] = LED.permanent_on_time;// Permanent ON time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.permanent_off_time;// Permanent OFF time.
	gb_TransmitFrameBuffer[frame_idx++] = LED.permanent_on_colour;// Permanent ON colour.
	gb_TransmitFrameBuffer[frame_idx++] = LED.permanent_off_colour;// Permanent OFF colour.
}

/***********************************************************************************
* Function name	: void OSDP_BUZZER_DATA(OSDP_BUZZER Buzzer);
* Returns		: None.
* Arguments		: OSDP_BUZZER Buzzer	---> Structures variable holds the setting for Buzzer.
*
* Created by	: Ranjitkumar Ainapure
* Date created	: 16-07-2022
* Description	: This function build frame for configuration provided for BUZZER
* 				  operation at slave side.
*               :
* Notes			: NA
* Global Variables Affected	: gb_TransmitFrameBuffer[] ---> update transmit buffer
* 							  gb_Temp_CRC1[] ---> calculated crc is updated here
* 							  gb_tframe_length ---> update frame_data length.
*********************************************************************************/
void OSDP_BUZZER_DATA(OSDP_BUZZER Buzzer)
{
	gb_TransmitFrameBuffer[frame_idx++] = Buzzer.reader_number; // Reader number
	gb_TransmitFrameBuffer[frame_idx++] = Buzzer.tone_code;		// Tone code.
	gb_TransmitFrameBuffer[frame_idx++] = Buzzer.on_time;		// On time value.
	gb_TransmitFrameBuffer[frame_idx++] = Buzzer.off_time;		// Off time value.
	gb_TransmitFrameBuffer[frame_idx++] = Buzzer.counts;		// The number of times to repeat the ON/OFF cycle.
}

void OSDP_FileTransfer_Frame_Build(uint8_t slave_address, OSDP_FileTransfer ft)
{
	//int frame_idx = 6;
	frame_idx = 6;
	gb_TransmitFrameBuffer[0] = OSDP_SOM;		// Start of message command
	gb_TransmitFrameBuffer[1] = slave_address;// Put slave address
	//gb_TransmitFrameBuffer[frame_idx++] = 13;			// frame length LSB
	//gb_TransmitFrameBuffer[frame_idx++] = 0x00;			// frame length MSB
	gb_TransmitFrameBuffer[4] = OSDP_CTRL_ID;	// Control ID
	gb_TransmitFrameBuffer[5] = CMD_OSDP_FILETRANSFER;	// OSDP File Transfer Command.

	gb_TransmitFrameBuffer[frame_idx++] = 1;

	gb_TransmitFrameBuffer[frame_idx++] = ft.total_size & 0x000000FF;
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.total_size & 0x0000FF00) >> 8);
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.total_size & 0x00FF0000) >> 16);
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.total_size & 0xFF000000) >> 24);

	gb_TransmitFrameBuffer[frame_idx++] = ft.offset & 0x000000FF;
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.offset & 0x0000FF00) >> 8);
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.offset & 0x00FF0000) >> 16);
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.offset & 0xFF000000) >> 24);

	gb_TransmitFrameBuffer[frame_idx++] = ft.fragment_size & 0x00FF;
	gb_TransmitFrameBuffer[frame_idx++] = ((ft.fragment_size & 0xFF00) >> 8);

	for (int idx = 0; idx < ft.fragment_size; idx++)	// ft.fragment_size	// As per protocol document whole packet length should not exceed 128 bytes
	{
		gb_TransmitFrameBuffer[frame_idx++] = ft.fragment_data[idx];
	}

	gb_TransmitFrameBuffer[2] = (frame_idx+2) & 0x00FF;
	gb_TransmitFrameBuffer[3] = (((frame_idx+2) & 0xFF00) >> 8);

	//CRC_Calculate_Check(gb_TransmitFrameBuffer, frame_idx, CRC_CALC);// Calculate CRC
	gb_TransmitFrameBuffer[frame_idx++] = gb_Temp_CRC1[0];	// CRC byte LSB
	gb_TransmitFrameBuffer[frame_idx++] = gb_Temp_CRC1[1];	// CRC byte MSB

	gb_tframe_length = frame_idx;	// update frame length.
}

/*****************************************************************************
* Function name	: void Get_OSDP_Frame_Data(uint8_t r_byte)
* Returns		: None.
* Arguments		: uint8_t r_byte ---> holds the received byte from the RS485.
* Created by	: Ranjitkumar Ainapure
* Date created	: 12-07-2022
* Description	: This function receives the data from the RS485, based on
* 				  OSDP SOM command and fills the received data into a buffer
* 				  up to frame length.
*               :
* Notes			: This function must be called in UART Receive interrupt.
* Global Variables Affected	: gb_ReceiveFrameBuffer[] ---> fills buffer with received data.
* 							  gb_osdp.frame_receive_f ---> once reception completes sets this flag
*****************************************************************************/
void Get_OSDP_Frame_Data(volatile uint8_t r_byte)
{
	if ((r_byte == OSDP_SOM) && (rcv_f == 0))	// Check receive byte is =to SOM command.
	{
		rcv_idx = 0;	// make receive index to zero
		rcv_f = 1;		// Enable receive flag.
		gb_receiving_f = TRUE;	// Set this flag to indicate that, Rs485 data receiving.
		ON_YELLOW_LED;
	}

	if (rcv_f == 1)
	{
		gb_ReceiveFrameBuffer[rcv_idx] = r_byte;	// Copy received byte to received frame_data buffer.
		rcv_idx++;	// Update index.
		
		if ((rcv_idx > 2) && (rcv_idx >= gb_ReceiveFrameBuffer[2]))	// receive data up to frame length.
		{
			rcv_f = 0;
			gb_osdp.frame_receive_f = 1;
			gb_receiving_f = FALSE;	// Reset this flag to indicate that, RS485 data received completely.
			OFF_YELLOW_LED;
		}
	}
}

/**************************************************************************************
* Function name	: void Clear_RS485_UART_Flags(void)
* Returns		: Nothing.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 09-10-2022.
* Description	: Function written to clear the flags if inter character error occurred,
*				  while receiving data from the RS485 UART.
*               :
* Notes			: Call this function in 1ms timer ISR.
* Global Variables Affected	: NA
*************************************************************************************/
void Clear_RS485_UART_Flags(void)
{
	if (gb_rs485_iChar_error_f == TRUE)
	{
		gb_rs485_iChar_error_f = FALSE;	// Reset this flag for next detection.
		rcv_idx = 0;
		rcv_f = 0;
		gb_receiving_f = 0;
	}
	else
	{
		__NOP();
	}
}

/**************************************************************************************
* Function name	: unsigned char CheckSum_Calculator(unsigned char* frame_ptr,
* 					unsigned char frame_len, unsigned char op_type)
* Returns		: unsigned char		---> 1 Byte CheckSum value or True or False if op_type is 0.
* Arguments		: uint8_t* frame_ptr	---> Pointer holds the address of checksum data array.
* 				  uint16_t frame_len	---> Frame/data length to calculate checksum.
* 				  uint8_t op_type		---> 1 or 0 to calculate or check.
* Created by	: Ranjitkumar Ainapure
* Date created	: 26-08-2022
* Description	: This function calculates checksum or validate received frame for error detection.
*               :
* Notes			: NA.
* Global Variables Affected	: NA
 *************************************************************************************/
unsigned char CheckSum_Calculator(unsigned char* frame_ptr, unsigned char frame_len, unsigned char op_type)
{
	int sum_frame_data=0;

	for (int idx=0; idx<frame_len; idx++)
	{
		sum_frame_data += frame_ptr[idx];
	}
	sum_frame_data = ~sum_frame_data;
	sum_frame_data += 1;

	if (op_type == 1)
	{
		return (unsigned char)(sum_frame_data & 0xFF);
	}
	else
	{
		if ((sum_frame_data & 0xFF) == frame_ptr[frame_len])
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*****************************************************************************
* Function name	: void Decode_OSDP_Frame_Response(uint8_t *frame_data)
* Returns		: None.
* Arguments		: uint8_t *frame_data ---> Pointer which holds the base address of the
* 				  received frame buffer.
* Created by	: Ranjitkumar Ainapure
* Date created	: 12-07-2022
* Description	: This function decodes the frame received from the slave.
*               :
* Notes			: NA
* Global Variables Affected	: gb_osdp.nack_f ---> if NACK receives sets this flag.
* 							  gb_osdp.crc_error_f ---> if found CRC error sets flag.
*
*****************************************************************************/
void Decode_OSDP_Frame_Response(uint8_t *frame_data)
{
	uint8_t crc_validate;
	uint8_t osdp_command;
	uint8_t dIdx = 0;
	uint8_t dec_f_len = 0;
	uint8_t d_data_idx = 0;
	uint8_t d_data_len = 0;

	dec_f_len = frame_data[2];

	#if DEBUG_FRAME_DATA
	Print_Message("\nReceive frame data is:\n");
	for (int idx=0; idx < dec_f_len; idx++)
	{
		Print_Number(frame_data[idx]);
		UART_Debug_PutChar(',');
	}
	#endif

	if (gb_osdp.crc_enable_f == 1)
		crc_validate = Calculate_CRC16_CCITT(frame_data, (frame_data[2] - 2), CRC_CHECK);
	else
		crc_validate = CheckSum_Calculator(frame_data, (frame_data[2] - 1), CRC_CHECK);
	if (crc_validate == TRUE)
	{
		#if DEBUG_FRAME_DATA
		Print_Message("\nCRC Validated.");
		Print_Message("\nReceived Device Address is : ");
		Print_Number(frame_data[1] & 0x7F);
		#endif
		
		gb_osdp.crc_match_f = 1;
		gb_osdp.rec_dev_address = (frame_data[1] & 0x7F);
		
		if (gb_osdp.mac_enable_f == TRUE)
		{
			/************************************************************************
			 * if Secure channel is discarded by PD and received NACK command without
			 * MAC then below code snippet is used.
			 ************************************************************************/
			if (dec_f_len < 10)
			{
				osdp_command = frame_data[5];	// Get Received command.
				d_data_idx = 6;
				if (gb_osdp.crc_enable_f == TRUE)
					d_data_len = (dec_f_len - d_data_idx - CRC_LEN);
				else
					d_data_len = (dec_f_len - d_data_idx - CKS_LEN);
			}
			else
			{
				osdp_command = frame_data[7];	// Get Received command.
				d_data_idx = 8;
				if (gb_osdp.crc_enable_f == TRUE)
					d_data_len = (dec_f_len - d_data_idx - MAC_LEN - CRC_LEN);
				else
					d_data_len = (dec_f_len - d_data_idx - MAC_LEN - CKS_LEN);
			}
		}
		else if (gb_osdp.sc_enable_f == TRUE)
		{
			osdp_command = frame_data[8];	// Get Received command.
			d_data_idx = 9;
		}
		else
		{
			osdp_command = frame_data[5];	// Get Received command.
			d_data_idx = 6;
			if (gb_osdp.crc_enable_f == TRUE)
				d_data_len = (dec_f_len - d_data_idx - CRC_LEN);
			else
				d_data_len = (dec_f_len - d_data_idx - CKS_LEN);
		}

		uint8_t temp_arr[16]={0};
		switch (osdp_command)
		{
			case REPLY_OSDP_ACK:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived OSDP_ACK");
				#endif
				
				if (gb_osdp.mac_enable_f == TRUE)
					osdp_compute_mac(frame_data, 8);
				gb_osdp.ack_f = TRUE;
			break;
			case REPLY_OSDP_NACK:
				#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
				Print_Message("\nReceived OSDP_NACK");
				#endif
				gb_osdp.nack_f = 1;
				gb_osdp.mac_enable_f = 0;	// If NACK received while SC is enabled.
				gb_osdp.sc_enable_f = 0;	// Need to re establish secure channel. So, resetting both flags.
				switch (frame_data[d_data_idx])
				{
					case 0x00:	// No error
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nNo error.");
						#endif
					break;
					case 0x01:	// Message check character(s) error (bad cksum/crc)
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nBad CheckSum or CRC-16.");
						#endif
						break;
					case 0x02:	// Command length error
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nCommand length error.");
						#endif
						break;
					case 0x03:	// Unknown Command Code – Command not implemented by PD
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nUnknown Command Code – Command not implemented by PD.");
						#endif
						break;
					case 0x04:	// Unexpected sequence number detected in the header
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nUnexpected sequence number detected in the header.");
						#endif
						break;
					case 0x05:	// This PD does not support the security block that was received
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nThis PD does not support the security block that was received.");
						#endif
						break;
					case 0x06:	// Encrypted communication is required to process this command
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nEncrypted communication is required to process this command.");
						#endif
						break;
					case 0x07:	// BIO_TYPE not supported
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nBIO_TYPE not supported.");
						#endif
						break;
					case 0x08:	// BIO_FORMAT not supported
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nBIO_FORMAT not supported.");
						#endif
						break;
					case 0x09:	// Unable to process command record
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nUnable to process command record.");
						#endif
						break;
					default :	// If Unwanted data byte / NACK data received.
						#if (DEBUG_FRAME_DATA || DEBUG_NACK_DATA)
						Print_Message("\nUnwanted NACK received.");
						#endif
						break;
				}
			break;
			case REPLY_OSDP_PDID:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived REPLY_OSDP_PDID\nPDIS/cUID is:\n");
				#endif

				for (dIdx = d_data_idx; dIdx < (d_data_idx+8); dIdx++)
				{
					gb_osdp.rec_client_UID[dIdx - d_data_idx] = frame_data[dIdx];
					#if DEBUG_FRAME_DATA
					Print_Number(gb_osdp.rec_client_UID[dIdx - d_data_idx]);
					UART_Debug_PutChar(',');
					#endif
				}
			break;
			case REPLY_OSDP_PDCAP:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived OSDP_PDCAP\nDevice Capabilities are:\n");
				#endif

				for (dIdx = d_data_idx; dIdx < (d_data_idx+d_data_len); dIdx++)
				{
					#if DEBUG_FRAME_DATA
					Print_Number(frame_data[dIdx]);
					if ((dIdx+1) % 3 == 0)
						UART_Debug_PutChar('|');
					else
						UART_Debug_PutChar(',');
					#endif

					if ((dIdx-1) % 3 == 0)
					{
						switch (frame_data[dIdx-1])
						{
							case 4:	// Reader LED Control.
								PD_Info.num_of_led = frame_data[dIdx+1];
								break;
							case 5:	// Reader Audible Output.
								if (frame_data[dIdx] == 0x01)
									PD_Info.buz_on_off_f = 1;
								else if (frame_data[dIdx] == 0x02)
									PD_Info.buz_timed_f = 1;
								break;
							case 8:	// Check Character Support.
								if (frame_data[dIdx] == 0x01)
									PD_Info.crc_enable = frame_data[dIdx];
								else
									PD_Info.crc_enable = 0;
								break;
							case 9:	// Communication Security.
								if (frame_data[dIdx] == 0x01)
									PD_Info.sc_enable = frame_data[dIdx];
								else
									PD_Info.sc_enable = 0;
								break;
							default:
								break;
						}
					}
				}
				#if DEBUG_FRAME_DATA
				Print_Message("\nNumber of Led's: ");
				Print_Number(PD_Info.num_of_led);
				if (PD_Info.buz_on_off_f == 1)
					Print_Message("\nBuzzer Supports On Off Only.");
				if (PD_Info.buz_timed_f == 1)
					Print_Message("\nBuzzer Supports timed commands.");
				Print_Message("\nCRC-16 Support: ");
				Print_Number(PD_Info.crc_enable);
				Print_Message("\nSecure Channel Enabled: ");
				Print_Number(PD_Info.sc_enable);
				#endif
				gb_osdp.pdcap_read_f = TRUE;
			break;
			case REPLY_OSDP_ISTATR:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived OSDP_ISTATR");
				#endif
				
				memcpy(gb_osdp.istatr_data, frame_data+d_data_idx, d_data_len);
				gb_osdp.istatr_f = TRUE;
				
			break;
			case REPLY_OSDP_OSTATR:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived OSDP_OSTATR");
				#endif
				
				gb_osdp.ostatr_data[0] = frame_data[d_data_idx];
				gb_osdp.ostatr_f = TRUE;
				
			break;
			case REPLY_OSDP_RAW:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_CARD_DATA))
				Print_Message("\nReceived REPLY_OSDP_RAW\n");
				#endif

				
				memcpy(temp_arr, frame_data+d_data_idx, d_data_len);

				if (TRUE == gb_osdp.mac_enable_f)
				{
					osdp_decrypt_data(temp_arr, 16);
					osdp_compute_mac(frame_data, 24);
				}
				#if ((DEBUG_FRAME_DATA) || (DEBUG_CARD_DATA))
				for (dIdx=0; dIdx < d_data_len; dIdx++)
				{
					Print_Number(temp_arr[dIdx]);
					Print_Message(",");
				}
				#endif
				Decode_Application_Reply_Code(temp_arr);
			break;
			case REPLY_OSDP_COM:
				#if (DEBUG_FRAME_DATA)
				Print_Message("\nReceived REPLY_OSDP_COM");
				Print_Message("\nSlave Address set is : ");
				Print_Number(frame_data[d_data_idx]);
				#endif
			break;
			case REPLY_OSDP_CCRYPT:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived REPLY_OSDP_CCRYPT");
				#endif
				memcpy(gb_osdp.pd_client_uid, &frame_data[9], 8);
				memcpy(gb_osdp.pd_random_num, &frame_data[17], 8);
				memcpy(gb_osdp.pd_cryptogram, &frame_data[25], 16);

				osdp_compute_session_keys();
				if (osdp_verify_pd_cryptogram() != 0)
				{
					#if DEBUG_FRAME_DATA
					Print_Message("\nFailed to verify PD cryptogram");
					#endif
				}
				else
				{
					#if DEBUG_FRAME_DATA
					Print_Message("\nPD Cryptogram varified");
					#endif
					gb_osdp.pd_crypt_varified_f = 1;
				}
			break;
			case REPLY_OSDP_RMAC_I:		// OSDP Client Cryptogram Packet and the Initial R-MAC command.
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived REPLY_OSDP_RMAC_I\nInitial MAC is:\n");
				for (int i=9; i<16+9; i++)
				{
					Print_Number(frame_data[i]);
					Print_Message(",");
				}
				#endif
				gb_osdp.pd_crypt_varified_f = 0;
				gb_osdp.mac_enable_f = 1;
				memcpy(gb_osdp.rmac_value, frame_data+9, 16);	// Copy initial rmac_i value to the variable gb_osdp.rmac_value.
			break;
			case REPLY_OSDP_OP_STATE:		// Code for Operational state of the slave device.
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived REPLY_OSDP_OP_STATE & values are : ");
				for (dIdx = d_data_idx; dIdx < (d_data_idx + d_data_len); dIdx++)
				{
					Print_Number(frame_data[dIdx]);
					Print_Message(",");
				}
				#endif
				
				memcpy(gb_osdp.op_stat_data, frame_data+d_data_idx, d_data_len);
				gb_osdp.opstat_f = TRUE;
			break;
			default:
				#if DEBUG_FRAME_DATA
				Print_Message("\nReceived Default Command");
				#endif
			break;
		}
	}
	else
	{
		#if DEBUG_FRAME_DATA
		Print_Message("\nCRC Not Validated.");
		#endif
		gb_osdp.crc_error_f = 1;
	}
}

/**************************************************************************************
* Function name	: void Decode_Application_Reply_Code(U8 *data_arr)
* Returns		: Nothing.
* Arguments		: U8 *data_arr	---> Holds data of application specific commands.
* Created by	: Ranjitkumar Ainapure
* Date created	: 20-12-2022
* Description	: This decodes the received application codes and set flags accordingly
*               :
* Notes			: NA.
* Global Variables Affected	: gb_osdp_emg_rst_f			// Set this flag once received the signal.
							  gb_osdp_emg_det_f			// Set this flag once received the signal.
							  gb_osdp_cap_det_f			// Set this flag once received the signal.
							  gb_osdp_dr_release_f		// Set this flag once received the signal.
							  gb_osdp_rfid_rec_f		// Set this flag once received the signal.
							  gb_osdp_acs_denied_f		// Set this flag once received the signal.
							  gb_osdp_free_acs_f		// Set this flag once received the signal.
							  gb_osdp_stand_alone_f		// Set this flag once received the signal.
							  gb_osdp_normal_f = TRUE	// Set this flag once received the signal.
**************************************************************************************/
void Decode_Application_Reply_Code(U8 *data_arr)
{
	U8 lcl_app_error_f = 0;
	//if (data_arr[2] == 32)	// if bit length matches?
	{
		for (U8 idx=4; idx<7; idx++)
		{
			if (data_arr[idx] != 0x00)	// Compare first 3 byte are equal to 0 or not?
			lcl_app_error_f = 1;		// If error, set this flag.
		}
		
		if (lcl_app_error_f != 1)		// If there is no error then find out code.
		{
			switch (data_arr[7])
			{
				case REPLY_OSDP_EMG_RESET:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nEmergency Reset Signal Received.");
				#endif
				gb_osdp_emg_rst_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_DETECT_EMG:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nEmergency Signal Detection Received.");
				#endif
				gb_osdp_emg_det_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_DETECT_CAP:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nCapsense Detect Signal Received.");
				#endif
				
				gb_osdp_cap_det_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_DR_SIGNAL:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nDoor Release Signal Received.");
				#endif
				
				gb_osdp_dr_release_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_ACS_DENIED:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nAccess Denied Reply Received.");
				#endif
				gb_osdp_acs_denied_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_FREE_ACS:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nFree Access Reply Received.");
				#endif
				gb_osdp_free_acs_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_PRIVACY:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nPrivacy state command Received.");
				#endif
				gb_osdp_privacy_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_STAND_ALONE:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nStand Alone Reply Received.");
				#endif
				gb_osdp_stand_alone_f = TRUE;	// Set this flag once received the signal.
				break;
				case REPLY_OSDP_NORMAL_MODE:
				#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
				Print_Message("\nNormal Mode Reply Received.");
				#endif
				gb_osdp_normal_f = TRUE;	// Set this flag once received the signal.
				break;
				default:
				// Skip; do nothing.
				break;
			}
		}
		else
		{
			lcl_app_error_f = 0;
			
			gb_osdp_rfid_len = 0;
			for (U8 uid = 0; uid < (data_arr[2]/8); uid++)
			{
				gb_osdp_rfid_data[uid] = data_arr[uid + 4];
				gb_osdp_rfid_len ++;
			}
			
			gb_osdp_rfid_raw_f = 1;
			
			//#if ((DEBUG_FRAME_DATA) || (DEBUG_APP_REPLY))
			Print_Message("\nRFID request received.\nUID is : ");
			for (U8 len = 0; len < gb_osdp_rfid_len; len++)
			{
				Print_ASCII_HEX(gb_osdp_rfid_data[len]);
			}
			//#endif
		}
	}
}

/**************************************************************************************
* Function name	: static int CRC_Generate_Table(uint16_t *crc_tbl)
* Returns		: static int		---> Returns 1 (True). at first call.
* Arguments		: uint16_t *crc_tbl ---> Pointer holds the address of CRC table array.
* Created by	: Ranjitkumar Ainapure
* Date created	: 26-08-2022
* Description	: This function generates a table for CRC16 (CRC-CCITT type) 2 Byte CRC.
* 				  or (generate the table for POLY == 0x1021), which is used in
* 				  OSDP protocol for error check.
*               :
* Notes			: This function runs only once at first call.
* Global Variables Affected	: NA
 *************************************************************************************/
static int CRC_Generate_Table(uint16_t *crc_tbl)
{
	uint16_t table_val;
	int idx;
	int jdx;
	
	for (idx = 0; idx < 256; idx++)
	{
		table_val = (uint16_t)(idx << 8);
		for (jdx = 0; jdx < 8; jdx++)
		{
			if ( table_val & 0x8000 )
			{
				table_val = (table_val << 1) ^ 0x1021;
			}
			else
			{
				table_val = (table_val << 1);
			}
		}
		crc_tbl[idx] = table_val;
	}
	return 1;
}

/**************************************************************************************
* Function name	: uint16_t Calculate_CRC16_CCITT(uint8_t* frame_ptr, uint16_t frame_len, uint8_t op_type)
* Returns		: uint16_t	---> Returns Calculated CRC, if op_type is =to zero i.e CRC check.
* 				  returns True if CRC match else False if CRC mismatch.
* Arguments		: uint8_t* frame_ptr	---> Pointer holds the address of CRC data array.
* 				  uint16_t frame_len	---> Frame/data length to calculate CRC.
* 				  uint8_t op_type		---> 1 or 0 to calculate or check.
* Created by	: Ranjitkumar Ainapure
* Date created	: 26-08-2022
* Description	: This function calculates 2 byte CRC. Also verify received CRC.
*               :
* Notes			: NA.
* Global Variables Affected	: gb_Temp_CRC1[0] ---> CRC byte LSB
* 							  gb_Temp_CRC1[1] ---> CRC byte MSB
 *************************************************************************************/
/*****
 * table based CRC – this is the "direct table" mode.
 *****/
uint16_t Calculate_CRC16_CCITT(uint8_t* frame_ptr, uint16_t frame_len, uint8_t op_type)
{
	static uint8_t crc_table_valid_f;	// CRC Table not initialized
	static uint16_t cCrcTable[256];		// CRC table – working copy.
	if ( crc_table_valid_f == 0 )
	{
		crc_table_valid_f = CRC_Generate_Table(&cCrcTable[0]);
	}

	uint16_t crc_val;
	uint16_t recvd_crc = 0;

	int idx ;
	for (idx = 0, crc_val = 0x1D0F; idx < frame_len; idx++ )
	{
		crc_val = (crc_val<<8) ^ cCrcTable[ ((crc_val>>8) ^ frame_ptr[idx]) & 0xFF];
	}

	if (op_type == 1)
	{
		gb_Temp_CRC1[0] = crc_val & 0x00FF;		// CRC byte LSB
		gb_Temp_CRC1[1] = (crc_val & 0xFF00)>>8;	// CRC byte MSB

		return crc_val;
	}
	else
	{
		recvd_crc = frame_ptr[frame_len+1];		// Put Higher Byte.
		recvd_crc <<= 8;
		recvd_crc |= frame_ptr[frame_len+0];	// Put Lower Byte.

		/* If received CRC and Calculated CRC matches */
		if (recvd_crc == crc_val)
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*****************************************************************************
* Function name : void Fill_OSDP_Footer_Bytes(int frmIdx)
* Returns       : Nothing
* Arguments     : int frmIdx - The current index in the frame buffer where
*                 the footer bytes should start being written.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : This function appends the footer bytes to the OSDP frame
*                 including frame length, MAC, and CRC or checksum, depending
*                 on the configuration flags.
* Notes         : The function assumes that global structures and flags like
*                 gb_osdp, gb_TransmitFrameBuffer, and others are defined and
*                 properly initialized.
* Global Variables Affected : gb_TransmitFrameBuffer, gb_tframe_length
*****************************************************************************/
void Fill_OSDP_Footer_Bytes(int frmIdx)
{
	if ((gb_osdp.mac_enable_f == TRUE) && (gb_osdp.crc_enable_f == TRUE))
	{
		gb_TransmitFrameBuffer[3] = (((frmIdx-1)+6) & 0x00FF);		// frame length LSB
		gb_TransmitFrameBuffer[4] = (((frmIdx-1)+6) & 0xFF00) >> 8;	// frame length MSB
	}
	else if ((gb_osdp.mac_enable_f == TRUE) && (gb_osdp.crc_enable_f == FALSE))
	{
		gb_TransmitFrameBuffer[3] = (((frmIdx-1)+5) & 0x00FF);		// frame length LSB
		gb_TransmitFrameBuffer[4] = (((frmIdx-1)+5) & 0xFF00) >> 8;	// frame length MSB
	}
	else if (gb_osdp.crc_enable_f == TRUE)
	{
		gb_TransmitFrameBuffer[3] = (((frmIdx-1)+2) & 0x00FF);		// frame length LSB
		gb_TransmitFrameBuffer[4] = (((frmIdx-1)+2) & 0xFF00) >> 8;	// frame length MSB
	}
	else
	{
		gb_TransmitFrameBuffer[3] = (((frmIdx-1)+1) & 0x00FF);		// frame length LSB
		gb_TransmitFrameBuffer[4] = (((frmIdx-1)+1) & 0xFF00) >> 8;	// frame length MSB
	}

	if (gb_osdp.mac_enable_f == TRUE)
	{
		osdp_compute_mac(&gb_TransmitFrameBuffer[1], frmIdx-1);
		for (int i=0; i<4; i++)
		{
			gb_TransmitFrameBuffer[frmIdx++] = gb_osdp.cmac_value[i];
		}
	}

	if (gb_osdp.crc_enable_f == 1)
	{
		Calculate_CRC16_CCITT(&gb_TransmitFrameBuffer[1], frmIdx-1, CRC_CALC);// Calculate CRC
		gb_TransmitFrameBuffer[frmIdx++] = gb_Temp_CRC1[0];	// CRC byte LSB
		gb_TransmitFrameBuffer[frmIdx++] = gb_Temp_CRC1[1];	// CRC byte MSB
	}
	else
	{
		unsigned char cks_val = CheckSum_Calculator(&gb_TransmitFrameBuffer[1], frmIdx-1, CRC_CALC);// Calculate CheckSum
		gb_TransmitFrameBuffer[frmIdx++] = cks_val;	// One Byte CheckSum value.
	}

	gb_tframe_length = frmIdx;	// update frame length to be sent.
}