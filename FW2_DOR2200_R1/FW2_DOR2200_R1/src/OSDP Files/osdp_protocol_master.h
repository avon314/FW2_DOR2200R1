/*****************************************************************************
*							   Copyright (c)
*                    Avon Building Solutions Pvt Ltd
*	                        All RIGTHS RESERVED
*
* Module Name	: osdp_protocol_master.h
* Created By	: Ranjitkumar Ainapure
* Created Date	: 12-07-2022
* Module
* Description	: Header file for osdp_protocol_master.c
*                 Defines constants and macros for osdp_protocol_master.c
*
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 12-07-2022
* Changes		: NA
*****************************************************************************/
#ifndef OSDP_PROTOCOL_MASTER_H_
#define OSDP_PROTOCOL_MASTER_H_

#include "asf.h"

#define TRUE	(1)
#define FALSE	(0)

#ifndef OSDP_TX_FRAME_SIZE
#define OSDP_TX_FRAME_SIZE	(128)		// Default Transmit frame size.
#endif

#ifndef OSDP_RX_FRAME_SIZE
#define OSDP_RX_FRAME_SIZE	(128)		// Default Receive frame size.
#endif

#ifndef MAX_REPLY_DELAY
#define MAX_REPLY_DELAY		(200)		// 200 ms maximum reply delay.
#endif

#ifndef OFFLINE_TIME
#define OFFLINE_TIME		(8000)	// 8000 ms Off line time.
#endif

#define LINE_IDLE	(TRUE)
#define CRC_CALC	(TRUE)
#define CRC_CHECK	(FALSE)
#define CRC_LEN		(2)		// CRC-16 length 2 byte.
#define CKS_LEN		(1)		// Checksum byte 1.
#define MAC_LEN		(4)		// MAC length 4 byte.

#define DEBUG_FRAME_DATA	(FALSE)
#define DEBUG_NACK_DATA		(FALSE)
#define DEBUG_CARD_DATA		(FALSE)
#define DEBUG_APP_REPLY		(FALSE)

typedef enum
{
	/***** OSDP Commands *****/
	OSDP_SOM		= 0x53, // Start of message command.
	CMD_OSDP_POLL	= 0x60, // OSDP POLL command.
	CMD_OSDP_ID		= 0x61,	// OSDP ID Report Request command.
	CMD_OSDP_CAP	= 0x62,	// OSDP Capability request command.
	CMD_OSDP_ISTAT	= 0x65, // OSDP Input Status command.
	CMD_OSDP_OSTAT	= 0x66, // OSDP Input Status command.
	CMD_OSDP_OUT	= 0x68, // OSDP OUT command.
	CMD_OSDP_LED	= 0x69, // OSDP LED command.
	CMD_OSDP_BUZ	= 0x6A, // OSDP BUZZER command.
	CMD_OSDP_COMSET	= 0x6E,	// OSDP Communication Configuration Command.
	CMD_OSDP_KEYSET	= 0x75,	// OSDP KeySet command is used to set SCBK to PD.
	CMD_OSDP_CHLNG	= 0x76,	// OSDP Challenge and Secure Session Initialization Request command.
	CMD_OSDP_SCRYPT	= 0x77,	// OSDP Server’s Random Number and Server Cryptogram command.
	CMD_OSDP_FILETRANSFER	= 0x7C,	// OSDP File Transfer Command.
	BROADCAST_ADD	= 0x7F,	// Special Broadcast Address.
	/***** End of OSDP Commands *****/

	/***** OSDP REPLY COMMANDS *****/
	REPLY_OSDP_ACK		= 0x40, // OSDP ACK command.
	REPLY_OSDP_NACK		= 0x41, // OSDP NACK command.
	REPLY_OSDP_PDID		= 0x45,	// OSDP Device Identification Report command.
	REPLY_OSDP_PDCAP	= 0x46,	// OSDP Capability response command.
	REPLY_OSDP_ISTATR	= 0x49, // OSDP Input Status Reply.
	REPLY_OSDP_OSTATR	= 0x4A, // OSDP Output Status Reply.
	REPLY_OSDP_RAW		= 0x50,	// OSDP Card data Reply.
	REPLY_OSDP_COM		= 0x54,	// OSDP Communication Configuration Report Reply command.
	REPLY_OSDP_CCRYPT	= 0x76,	// OSDP Client's ID and Client's Random Number reply command.
	REPLY_OSDP_RMAC_I	= 0x78,	// OSDP Client Cryptogram Packet and the Initial R-MAC command.
	/***** End of OSDP REPLY COMMANDS *****/

	/***** Application Specific OSDP Commands *****/
	CMD_OSDP_SET_EMERGENCY		= 0x35,	// Set Emergency command.
	CMD_OSDP_SET_INTERLOCK		= 0x36,	// Door Interlock command.
	CMD_OSDP_SET_ACS_DENIED		= 0x37,	// Door Access denied.
	CMD_OSDP_SET_FREE_ACCESS	= 0x38,	// Door Free Access mode.
	CMD_OSDP_SET_STAND_ALONE	= 0x39,	// Door Set to stand alone mode.
	CMD_OSDP_SET_NORMAL_MODE	= 0x3A,	// Door Set to Normal mode.
	CMD_OSDP_READ_OP_STATE		= 0x3B,	// Read Operational state of the slave.
	CMD_OSDP_SET_PRIVACY		= 0x3D,
	/***** End of Application Specific OSDP Commands *****/
	
	/***** Application specific Reply Codes *****/
	REPLY_OSDP_EMG_RESET		= 0x00,	// Reply code for Reset Emergency signal.
	REPLY_OSDP_DETECT_EMG		= 0x01,	// Reply code for Detect Emergency signal.
	REPLY_OSDP_DETECT_CAP		= 0x02,	// Reply code for Detect Cap sense / Proximity signal.
	REPLY_OSDP_DR_SIGNAL		= 0x03,	// Reply code for Door release signal.
	REPLY_OSDP_ACS_DENIED		= 0x04,	// Reply code for Access Denied Receive or not?
	REPLY_OSDP_FREE_ACS			= 0x05,	// Reply code for Free Access.
	REPLY_OSDP_STAND_ALONE		= 0x06,	// Reply code for Stand Alone.
	REPLY_OSDP_NORMAL_MODE		= 0x07,	// Reply code for Normal Mode.
	REPLY_OSDP_PRIVACY			= 0x08,	// Reply code for Privacy state.
	REPLY_OSDP_OP_STATE			= 0x3C	// Reply code for Operational State of the device.
	/***** End of Application specific Reply Codes *****/

}OSDP_COMMAND;

typedef struct
{
	uint8_t reader_number;	// The reader number.
	uint8_t tone_code;		// Requested Tone State.
	uint8_t on_time;		// The ON duration of the sound, in units of 100 ms,  Must be nonzero unless the tone code is 0x01 (off.)
	uint8_t off_time;		// The OFF duration of the sound, in units of 100 ms
	uint8_t counts;			// The number of times to repeat the ON/OFF cycle, 0 = tone continues until another tone command is received.
}OSDP_BUZZER;

typedef struct
{
	uint8_t reader_number;			// Reader number.
	uint8_t led_number;				// Led number to be operated.
	uint8_t temp_code;				// Temporary code.
	uint8_t temp_on_time;			// An 8 bit ON duration of the flash, in units of 100 ms
	uint8_t temp_off_time;			// An 8 bit OFF duration of the flash, in units of 100 ms
	uint8_t temp_on_colour;			// The color to set during the ON time
	uint8_t temp_off_colour;		// The color to set during the OFF time
	uint8_t timer_lsb;				// An 16 bit timer value LSB, in units of 100 ms
	uint8_t timer_msb;				// An 16 bit timer value MSB, in units of 100 ms (zero value means “forever”).

	uint8_t permanent_code;			// The permanent command/mode to return to after the timer expires
	uint8_t permanent_on_time;		// An 8 bit ON duration of the flash, in units of 100 ms
	uint8_t permanent_off_time;		// An 8 bit OFF duration of the flash, in units of 100 ms
	uint8_t permanent_on_colour;	// The color to set during the ON time
	uint8_t permanent_off_colour;	// The color to set during the OFF time

}OSDP_LED;

typedef struct
{
	int total_size;
	int offset;
	uint16_t fragment_size;
	uint8_t* fragment_data;
}OSDP_FileTransfer;

typedef struct
{
	uint8_t pd_comn_id;
	uint32_t pd_baudrate;
}OSDP_Comm_SET;

typedef struct
{
	uint8_t crc_enable:1;
	uint8_t sc_enable:1;
	uint8_t buz_on_off_f:1;
	uint8_t buz_timed_f:1;
	uint8_t num_of_led;
	uint8_t num_of_buz;
	uint16_t rec_buf_size;
	uint16_t largest_combined_msize;
}PDCAP;

typedef struct
{
	uint8_t NACK:1;
}FLAGS;

typedef struct
{
	uint8_t crc_enable_f:1;		// flag used to enable crc or checksum.
	uint8_t sc_enable_f:1;		// flag used to enable-disable secure channel.
	uint8_t scbk_default_f:1;	// flag used to enable-disable default SCBK.

	uint8_t mac_enable_f:1;		// flag sets if secure channel established successfully.
	uint8_t pdcap_read_f:1;		// flag sets when pd capability read completes.
	volatile uint8_t frame_receive_f:1;	// flag sets when complete frame received.
	uint8_t ack_f:1;			// flag sets if ACK command received.
	uint8_t nack_f:1;			// flag sets if NACK received from the PD.
	uint8_t istatr_f:1;			// flag sets if ISTATR received from the PD.
	uint8_t ostatr_f:1;			// flag sets if OSTATR received from the PD.
	uint8_t opstat_f:1;			// flag sets if ISTATR received from the PD.
	uint8_t crc_match_f:1;		// flag sets if CRC or Checksum matches.
	uint8_t crc_error_f:1;		// flag sets if CRC or Checksum mismatches.
	uint8_t pd_crypt_varified_f:1;	// flag sets if PD's cryptogram verified.

	uint8_t rec_client_UID[8];	// Array used to store received client UID (8 byte).

	uint8_t pd_client_uid[16];	// 16 Byte array holds client's UID.
	uint8_t pd_random_num[16];	// 16 Byte array holds PD's random number.
	uint8_t pd_cryptogram[16];	// 16 Byte array holds PD's Cryptogram.
	uint8_t cp_random_num[16];	// 16 Byte array holds CP's random number.
	uint8_t cp_cryptogram[16];	// 16 Byte array holds CP's Cryptogram.
	uint8_t rmac_value[16];		// 16 Byte array holds rmac value.
	uint8_t cmac_value[16];		// 16 Byte array holds cmac value.
	uint8_t master_key[16];		// 16 Byte array holds Master Key value.
	uint8_t scbk_value[16];		// 16 Byte array holds SCBK value.
	uint8_t istatr_data[8];		// 8 Byte of array holds ip status. (Array size can be varied).
	uint8_t ostatr_data[1];		// 1 Byte of array holds op status. (Array size can be varied).
	uint8_t op_stat_data[2];	// 2 Byte of array holds operational status. (Array size can be varied).
	
	uint8_t sequence_num;		// Variable used to update the sequence number accordingly.
	uint8_t rec_dev_address;	// Variable used to fill received device address.
}OSDP_INFO;

extern U8	gb_osdp_emg_rst_f,		// Set this flag once received the signal.
			gb_osdp_emg_det_f,		// Set this flag once received the signal.
			gb_osdp_cap_det_f,		// Set this flag once received the signal.
			gb_osdp_dr_release_f,	// Set this flag once received the signal.
			gb_osdp_rfid_rec_f,		// Set this flag once received the signal.
			gb_osdp_rfid_raw_f,
			gb_osdp_acs_denied_f,	// Set this flag once received the signal.
			gb_osdp_free_acs_f,		// Set this flag once received the signal.
			gb_osdp_privacy_f,		// Set this flag once received the signal.
			gb_osdp_stand_alone_f,	// Set this flag once received the signal.
			gb_osdp_normal_f;		// Set this flag once received the signal.

extern OSDP_COMMAND gb_Command;
extern OSDP_BUZZER gb_Buzzer_Setting;
extern OSDP_LED gb_LED_Setting;
extern OSDP_FileTransfer gb_ft_data;
extern OSDP_Comm_SET gb_comm_n;
extern PDCAP PD_Info;
extern FLAGS Flag;
extern OSDP_INFO gb_osdp;

extern volatile uint8_t gb_TransmitFrameBuffer[OSDP_TX_FRAME_SIZE], gb_ReceiveFrameBuffer[OSDP_TX_FRAME_SIZE];
extern volatile uint8_t gb_last_tx_cmd;	// Command code of the last frame built for transmission.
extern volatile uint8_t gb_receiving_f;	// Flag used to indicate that, RS485 data received completely.
extern volatile uint8_t rcv_idx ;	// receive index OSDP frame_data
extern volatile uint8_t rcv_f;		// Enable receive flag based on OSDP SOM command
extern uint16_t gb_tframe_length;
extern struct osdp_pd *pd;
extern U8 gb_osdp_rfid_data[7];	/* Array to store received RFID data. */
extern U8 gb_osdp_rfid_len;	/* Store a length of received RFID bytes. */

/***** Function Prototypes *****/
void OSDP_Frame_Build(uint8_t OSDP_CMD, uint8_t slave_address);
void OSDP_OUT_Frame_Build(uint8_t slave_address, uint8_t output_no, uint8_t control_code, uint8_t Timer_lsb, uint8_t Timer_msb);
void OSDP_LED_DATA(OSDP_LED LED);
void OSDP_BUZZER_DATA(OSDP_BUZZER Buzzer_Setting);
void Get_OSDP_Frame_Data(volatile uint8_t r_byte);
void Clear_RS485_UART_Flags(void);
void OSDP_FileTransfer_Frame_Build(uint8_t slave_address, OSDP_FileTransfer);
void Decode_OSDP_Frame_Response(uint8_t *frame_data);
void Decode_Application_Reply_Code(U8 *data_arr);
void Fill_OSDP_Footer_Bytes(int frmIdx);
unsigned char CheckSum_Calculator(unsigned char* frame_ptr, unsigned char frame_len, unsigned char op_type);
uint16_t Calculate_CRC16_CCITT(uint8_t* frame_ptr, uint16_t frame_len, uint8_t op_type);
int Fill_OSDP_Header(int frm_idx, U8 slvAddress);
int Update_OSDP_Control_ID(int frm_idx);
/***** End of Function Prototypes *****/

#endif /* INC_OSDP_OSDP_PROTOCOL_H_ */
