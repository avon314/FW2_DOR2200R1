/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_osdp.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 07/11/2023.
* Module
* Description	: The data transmission from the master to slave and vice versa
				  functions are written here. And rtos tasks also defined for
				  transmission task and reception task.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 07/11/2023.
* Changes		: NA.
*****************************************************************************/

/* User Includes */
#include "app_osdp.h"
#include "osdp_protocol_master.h"
#include "user_timer.h"
#include "rs485_driver.h"
#include "user_uart.h"

#include "mb_osdp.h"
#include "interlock.h"
#include "definitions.h"
#include "emergency.h"
#include "fire_functionality.h"
#include "op_func.h"
#include "door_config.h"
#include "mb_tcp_server.h"
#include "config_mode.h"
#include "digital_ip_app.h"
#include "group_config.h"
/***** Global Variables *****/
OSDP_APP osdp_app =
{
	.gb_enable_poll = 1,		/* Set flag at power on */
	.gb_poll_flag = 0,			/* Set to zero at power on */
	.gb_frame_not_rcvd_f = 0,	/* Initialize flag value to low at power on. */
	.gb_retry_f = 0,			/* Zero at power on */
	.gb_transmit_f = 0,			/* Flag value zero at power on */
	.gb_rd_oprt_state_f = 0,	/* Flag must be reset state at power on */
	.gb_rd_ip_status_f = 0,		/* Flag must be reset state at power on */
	.polling_device = 0,		/* Variable used t fill the current polling device */
	.gb_poll_time = 0,			/* Set poll value / time zero power on. */
	.gb_frame_reply_time = 0	/* Set frame reply time zero power on. */
};

/* Global structure variables */
OSDP_APP copyosdp_app;

SLAVE_DATA slv_data[TOTAL_SLAVES];
SLAVE_DATA slvTemp_data[TOTAL_SLAVES] = {0};
TOTALDEV inSysDeviceList[TOTAL_SLAVES];
TOTALDEV PollDevice[TOTAL_SLAVES];

/* Global variables */
extern U8 Find_Device_Index_InGroup(U8, U8);

extern xSemaphoreHandle xSemaphore;

U8 gb_power_on_flag = 1;
U8 inSystem_dev_len = 0;
U8 polling_dev_len = 0;
//U16 gb_online_time[TOTAL_SLAVES] = {0};
bool gb_slave_aux_audio = 0;

/*****************************************************************************
* Function name	: void OSDP_Transmit_Task(void *pvParameters)
* Returns		: nothing.
* Arguments    	: void *pvParameters ---> UNUSED pointer.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: This task is responsible for handling the transmission of data
				  over the OSDP protocol. It likely involves tasks such as preparing
				  data for transmission, managing communication timing, and sending
				  data packets to other devices within the OSDP network.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void OSDP_Transmit_Task(void *pvParameters)
{
	UNUSED(pvParameters);
	
	/***** Fill default values *****/
	for (U8 idx = 0; idx < TOTAL_SLAVES; idx++)
	{
		// Set default values for alarm state, door state, lock state, and operation state
		slvTemp_data[idx].alarmState = 0xFF;
		slvTemp_data[idx].doorState = 0xFF;
		slvTemp_data[idx].lockState = 0xFF;
		slvTemp_data[idx].oprtState = 0xFF;
	}

	/***** Fill default values *****/
	for (U8 idx = 0; idx < SIZE_DATA_BUF; idx ++)
	{
		mbTemp.regDataBuf[idx] = 0xFF;
	}

	while (1)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		/* Attempt to take the semaphore (wait for it to be available) */
		if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
		if (gb_config_mode_f == FALSE)
		{			
			if ((osdp_app.gb_retry_f == FLAG_SET) && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_retry_f = FLAG_RST;	/* reset the flag for next retrial. */
				/********************************************************/
				
				osdp_app.gb_retry_count ++;	/* Increment the retry counts */
				
				U8 lcl_one_dev_offline = FLAG_RST;
				U8 err_retry_count_val = 20;
				
				for (U8 chk = 0; chk < TOTAL_SLAVES; chk ++)
				{
					if (iDeviceFlag[chk].gb_noResponse_f == FLAG_SET)
					{
						/*	If any one of the slaves is not responding,
							and its power becomes on, on that time the RS485
							line remain steady for some seconds so, to overcome
							this issue increasing the count greater than
							"MAX_RETRY_COUNT".
						*/
						lcl_one_dev_offline = FLAG_SET;
						break;	/* Break the for loop. */
					}
				}
				
				if (lcl_one_dev_offline == FLAG_SET)
				{
					U8 lclRetryCount = 0;
					U8 lcldevIdx = Find_Device_Index_InSystem(osdp_app.last_dev_address);
					
					if (slv_data[lcldevIdx].oprtState == SLV_OP_STATE_PRIVACY)
					{
						lclRetryCount = err_retry_count_val;
					}
					else
					lclRetryCount = 0;
					
					if (osdp_app.gb_retry_count >= lclRetryCount)
					{
						lcl_one_dev_offline = FLAG_RST;
					}
				}
				
				if ((osdp_app.gb_retry_count > MAX_RETRY_COUNT) && (lcl_one_dev_offline == FLAG_RST))
				{
					/* If the maximum or defined retry counts meets, transmit next command */
					osdp_app.gb_retry_count = 0;	/* make counts zero to start again */
				
					#if DEBUG_ALL || DEBUG_APP_OSDP_TX
					Print_Message("\nMaximum retrials completed.");
					#endif
					
					Reset_Flags_Error_After_Retry(osdp_app.last_dev_address);
					Select_Command_transmission();	/* Select next command to transmit */
					osdp_app.gb_transmit_f = FLAG_RST;	/* Reset the flag to enable next device poll. */
					
					U8 devIdx = Find_Device_Index_InSystem(osdp_app.last_dev_address);
					iDeviceFlag[devIdx].gb_noResponse_f = FLAG_SET;	/* Set flag if no response from slave. */
					//iDeviceFlag[devIdx].is_dev_online = FLAG_RST;
					//gb_online_time[devIdx] = DEV_ONLINE_TIME;
					
					/*iflags.rst_noRsp_control_f = FLAG_SET;*/
				}
				else
				{
					#if DEBUG_ALL || DEBUG_APP_OSDP_TX
					Print_Message("\nSending retry command ");
					Print_Number(osdp_app.gb_retry_count);
					#endif
				
					osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
					osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to retrial the same frame */
				}
			}
			else if ((emgGroup.send_emg_command_f == FLAG_SET) && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				// Disable polling
				osdp_app.gb_poll_flag = FLAG_RST;

				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				// Print debug message
				Print_Message("\nSending Emergency command to sAddress : ");
				Print_Number(emgGroup.tx_address[osdp_app.emgTransIdx]);
				#endif

				// Build OSDP frame for setting Emergency command
				OSDP_Frame_Build(CMD_OSDP_SET_EMERGENCY, emgGroup.tx_address[osdp_app.emgTransIdx]);
				osdp_app.emgTransIdx++;

				// Reset emgAckIdx if emgTransIdx is 1
				if (osdp_app.emgTransIdx == 1)
				osdp_app.emgAckIdx = 0;

				// Reset emgTransIdx and send_emg_command_f if emgTransIdx >= emgGroup.tx_length
				if (osdp_app.emgTransIdx >= emgGroup.tx_length)
				{
					osdp_app.emgTransIdx = 0;
					emgGroup.send_emg_command_f = FLAG_RST;
				}

				// Set the flag to send data
				osdp_app.gb_transmit_f = FLAG_SET;
			}
			else if ((emgGroup.reset_emg_command_f == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending Emergency reset command to sAddress : ");
				Print_Number(emgGroup.tx_address[osdp_app.emgTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_NORMAL_MODE, emgGroup.tx_address[osdp_app.emgTransIdx]);
				osdp_app.emgTransIdx ++;
				if (osdp_app.emgTransIdx == 1)
				osdp_app.emgAckIdx = 0;
				if (osdp_app.emgTransIdx >= emgGroup.tx_length)
				{
					osdp_app.emgTransIdx = 0;
					emgGroup.reset_emg_command_f = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.put_into_free_access == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending Free Access command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.freeTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_FREE_ACCESS, iLock.tx_address[osdp_app.freeTransIdx]);
				osdp_app.freeTransIdx ++;
				if (osdp_app.freeTransIdx == 1)
				osdp_app.freeAckIdx = 0;
				if (osdp_app.freeTransIdx >= iLock.tx_length)
				{
					osdp_app.freeTransIdx = 0;
					osdp_app.put_into_free_access = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.put_into_acs_denied == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending Access denied command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.acsTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_ACS_DENIED, iLock.tx_address[osdp_app.acsTransIdx]);
				osdp_app.acsTransIdx ++;
				if (osdp_app.acsTransIdx == 1)
				osdp_app.acsAckIdx = 0;
				if (osdp_app.acsTransIdx >= iLock.tx_length)
				{
					osdp_app.acsTransIdx = 0;
					osdp_app.put_into_acs_denied = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.put_into_privacy_state == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending Privacy State command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.prvTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_PRIVACY, iLock.tx_address[osdp_app.prvTransIdx]);
				osdp_app.prvTransIdx ++;
				if (osdp_app.prvTransIdx == 1)
				osdp_app.prvAckIdx = 0;
				if (osdp_app.prvTransIdx >= iLock.tx_length)
				{
					osdp_app.prvTransIdx = 0;
					osdp_app.put_into_privacy_state = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.gb_rd_oprt_state_f == 1) && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_rd_oprt_state_f = 0;
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				OSDP_Frame_Build(CMD_OSDP_READ_OP_STATE, 1);
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.gb_rd_ip_status_f == 1) && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_rd_ip_status_f = 0;
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				OSDP_Frame_Build(CMD_OSDP_ISTAT, 1);
				//OSDP_Frame_Build(CMD_OSDP_SET_NORMAL_MODE, 1);
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((iflags.chk_ip_status == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending CMD_OSDP_ISTAT command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.istTransIdx]);
				
				Print_Message("\ntx Length is : ");
				Print_Number(iLock.tx_length);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_ISTAT, iLock.tx_address[osdp_app.istTransIdx]);
				osdp_app.istTransIdx ++;
				if (osdp_app.istTransIdx == 1)
				osdp_app.istRecIdx = 0;
				if (osdp_app.istTransIdx >= iLock.tx_length)
				{
					osdp_app.istTransIdx = 0;
				
					iflags.chk_ip_status = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((iflags.chk_oprt_status == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending CMD_OSDP_READ_OP_STATE command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.oprTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_READ_OP_STATE, iLock.tx_address[osdp_app.oprTransIdx]);
				osdp_app.oprTransIdx ++;
				if (osdp_app.oprTransIdx == 1)
				osdp_app.oprRecIdx = 0;
				if (osdp_app.oprTransIdx >= iLock.tx_length)
				{
					osdp_app.oprTransIdx = 0;
					iflags.chk_oprt_status = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((iflags.send_osdp_out_cmd == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending OSDP_OUT command to sAddress : ");
				Print_Number(iLock.actionSlave);
				#endif
			
				iflags.send_osdp_out_cmd = FLAG_RST;
				OSDP_OUT_Frame_Build(iLock.actionSlave, 0, 2, 0, 0);
				iflags.chk_osdp_out_ack = FLAG_SET;
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			
			
			}
			else if ((iflags.put_into_interlock == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending CMD_OSDP_SET_INTERLOCK command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.ilockTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_INTERLOCK, iLock.tx_address[osdp_app.ilockTransIdx]);
				osdp_app.ilockTransIdx ++;
				if (osdp_app.ilockTransIdx == 1)
				osdp_app.ilockRecIdx = 0;
				if (osdp_app.ilockTransIdx >= iLock.tx_length)
				{
					osdp_app.ilockTransIdx = 0;
					iflags.put_into_interlock = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((iflags.send_drt_osdp_out_cmd == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending CMD_OSDP_OUT command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.outTransIdx]);
				#endif
			
				OSDP_OUT_Frame_Build(iLock.tx_address[osdp_app.outTransIdx], 0, 2, 0, 0);
				osdp_app.outTransIdx ++;
				if (osdp_app.outTransIdx == 1)
				{
					osdp_app.outAckIdx = 0;
					iflags.chk_drt_osdp_out_ack = FLAG_SET;
				}
			
				if (osdp_app.outTransIdx >= iLock.tx_length)
				{
					osdp_app.outTransIdx = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((iflags.put_into_normal_state == FLAG_SET)  && (osdp_app.gb_poll_flag == FLAG_SET))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Disable polling */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_TX
				Print_Message("\nSending OSDP_Normal command to sAddress : ");
				Print_Number(iLock.tx_address[osdp_app.norTransIdx]);
				#endif
			
				OSDP_Frame_Build(CMD_OSDP_SET_NORMAL_MODE, iLock.tx_address[osdp_app.norTransIdx]);
				osdp_app.norTransIdx ++;
				if (osdp_app.norTransIdx == 1)
				{
					osdp_app.norAckIdx = 0;
					osdp_app.norReadyIdx = FLAG_SET;
				}
			
			
				if (osdp_app.norTransIdx >= iLock.tx_length)
				{
					osdp_app.norTransIdx = 0;
					iflags.put_into_normal_state = FLAG_RST;
				}
			
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
			else if ((osdp_app.gb_poll_flag == FLAG_SET) && (osdp_app.gb_enable_poll == TRUE))
			{
				osdp_app.gb_poll_flag = FLAG_RST;	/* Reset flag till next poll time set */
				/************************************************/
			
				static U8 slvNum = 0;	/* Initialize the index value to zero */
				
// 				if ((iDeviceFlag[slvNum].gb_noResponse_f == FLAG_SET)
// 				&& (iDeviceFlag[slvNum].is_dev_online == FLAG_SET))
// 				{
// 					iDeviceFlag[slvNum].is_dev_online = FLAG_RST;
// 					osdp_app.polling_device = PollDevice[slvNum].slv_addr;	/* Get the current polling device */
// 				}
// 				else if ((iDeviceFlag[slvNum].gb_noResponse_f == FLAG_SET)
// 				&& (iDeviceFlag[slvNum].is_dev_online == FLAG_RST))
// 				{
// 					// Skip do not poll that particular device.
// 				}
// 				else
				{
					osdp_app.polling_device = PollDevice[slvNum].slv_addr;	/* Get the current polling device */
				}
				
				OSDP_Frame_Build(CMD_OSDP_POLL, osdp_app.polling_device);	/* Build the OSDP frame for polling */
				
				slvNum++;	/* Increment the index value */
				if (slvNum >= polling_dev_len)
				{
					/* Once the maximum device address reached start from the beginning */
					slvNum = 0;
				}
			
				/************************************************/
				osdp_app.gb_transmit_f = FLAG_SET;	/* Set the flag to send data */
			}
		
		
		
		
			/************************* Transmit Data *********************************/
			if ((osdp_app.gb_transmit_f == FLAG_SET) && (gb_receiving_f == FLAG_RST))
			{
				osdp_app.gb_transmit_f = FLAG_RST;	/* Reset the flag for next transmit detection */
			
				/* Get device address before transmit. */
				osdp_app.last_dev_address = gb_TransmitFrameBuffer[2];
				/* Put an OSDP frame to the RS485 line */
				Send_OSDP_Frame_To_Slave(gb_TransmitFrameBuffer, gb_tframe_length);
			}
		}
		
		/* Release the semaphore when done with the critical section */
		xSemaphoreGive(xSemaphore);
		}
		
		/* Delay must be used to suspend the task */
		vTaskDelay(1);
	}
}

/*****************************************************************************
* Function name	: void OSDP_Receive_Task(void *pvParameters)
* Returns		: nothing.
* Arguments    	: void *pvParameters ---> UNUSED pointer.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: This task likely involves tasks such as waiting for incoming
				  messages, parsing received data packets, and possibly performing
				  actions based on the received information.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void OSDP_Receive_Task(void *pvParameters)
{
	UNUSED(pvParameters);
	U8 slIdx = 0;
	
	for (;;)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		/* Attempt to take the semaphore (wait for it to be available) */
		if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
		if (gb_config_mode_f == FALSE)
		{
			if (gb_osdp.frame_receive_f == FLAG_SET)
			{
				/* If OSDP frame received successfully */
				gb_osdp.frame_receive_f = FLAG_RST;	/* Reset flag next reception. */
			
				osdp_app.gb_frame_reply_time = 0;	/* Once received the frame reset the count value. */
				/* Decode the received frame. */
				Decode_OSDP_Frame_Response(gb_ReceiveFrameBuffer);
				slIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
				Send_Slave_Status_Modbus(slIdx);
				
				if (iDeviceFlag[slIdx].gb_noResponse_f == FLAG_SET)
				{
					#if DEBUG_ALL || DEBUG_APP_OSDP_RX
					Print_Message("\nSlave address ");
					Print_Number(gb_osdp.rec_dev_address);
					Print_Message(" responding/Power Up.");
					#endif
					
					iDeviceFlag[slIdx].gb_noResponse_f = FLAG_RST;
				}
				
				    // after a reply is fully decoded OR the reply timeout fires
					osdp_app.gb_poll_time = INTER_FRAME_GAP_MS;  
			}
		
			if (gb_osdp.crc_match_f == TRUE)
			{
				// Reset the CRC match flag
				gb_osdp.crc_match_f = 0;

				// Reset the retry count for OSDP application
				if (osdp_app.gb_retry_count > 0)
				{
					osdp_app.gb_retry_count = 0;
					Select_Command_transmission();	/* Select next command to transmit */
				}
				
				if ((osdp_app.gb_retry_f == FLAG_SET) || (osdp_app.gb_frame_not_rcvd_f == FLAG_SET))
				{
					/*	The reply came just after the reply timeout. Do not resend the
						frame: a second ACK of the same command would be counted twice. */
					osdp_app.gb_frame_not_rcvd_f = FLAG_RST;
					osdp_app.gb_retry_f = FLAG_RST;
					Select_Command_transmission();
				}
			}
			else if (gb_osdp.crc_error_f == TRUE)
			{
				gb_osdp.crc_error_f = 0;
			
				osdp_app.gb_enable_poll = FALSE;	/* Disable polling */
				osdp_app.gb_retry_f = FLAG_SET;		/* Enable retry frame flag */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_RX
				Print_Message("\nCRC error.");
				#endif
			}
			else if (osdp_app.gb_frame_not_rcvd_f == FLAG_SET)
			{
				osdp_app.gb_frame_not_rcvd_f = FLAG_RST;	/* Reset flag to observe next count */
			
				osdp_app.gb_enable_poll = FALSE;	/* Disable polling */
				osdp_app.gb_retry_f = FLAG_SET;		/* Enable retry frame flag */
			
				#if DEBUG_ALL || DEBUG_APP_OSDP_RX
				Print_Message("\nFrame not received from the sAddress : ");
				Print_Number(osdp_app.last_dev_address);
				#endif
			}
			else
			{
				/* Do nothing */
			}
		
		
			/*********************** Get Data ********************/
			/*	A reply belongs to the running command sequence only if it comes from
				the device the last command was sent to and that command was not a
				poll. Poll replies (ACK / ISTATR from any door) must not be counted as
				command acknowledgements, otherwise a sequence advances early and the
				next ACK is credited to the wrong door. */
			U8 lcl_cmd_reply = ((gb_last_tx_cmd != CMD_OSDP_POLL)
								&& (gb_osdp.rec_dev_address == osdp_app.last_dev_address));
			
			if (gb_osdp.nack_f == FLAG_SET)
			{
				gb_osdp.nack_f = FLAG_RST;
			
				osdp_app.gb_enable_poll = 1;
			}
			else if (gb_osdp.ack_f == FLAG_SET)
			{
				gb_osdp.ack_f = FLAG_RST;
			
				if (lcl_cmd_reply == FLAG_RST)
				{
					/* ACK to a poll, or from a device no command was sent to: not part of a sequence. */
				}
				else if (emgGroup.chkEmgAckFlag == FLAG_SET)
				{
					// Increment the emergency acknowledgment index
					osdp_app.emgAckIdx ++;

					// Check if the acknowledgment index exceeds the length of the emergency transmission list
					if (osdp_app.emgAckIdx >= emgGroup.tx_length)
					{
						// Reset the acknowledgment index to 0
						osdp_app.emgAckIdx = 0;

						// Reset the flag to check emergency acknowledgment
						emgGroup.chkEmgAckFlag = FLAG_RST;

						// Reset the command in process flag
						iflags.command_in_process = FLAG_RST;

						// Reassign whole group flags
						ReAssign_Whole_Group_Flags();

						// Enable polling for OSDP application
						osdp_app.gb_enable_poll = 1;
					}
				}
				else if (emgGroup.chk_sd_emgAck_flag == FLAG_SET)
				{
					// Increment the emergency acknowledgment index
					osdp_app.emgAckIdx++;

					// Check if the acknowledgment index exceeds the length of the emergency transmission list
					if (osdp_app.emgAckIdx >= emgGroup.tx_length)
					{
						// Reset the acknowledgment index to 0
						osdp_app.emgAckIdx = 0;

						// Reset the flag to check single door emergency acknowledgment
						emgGroup.chk_sd_emgAck_flag = FLAG_RST;

						// Reset the command in process flag
						iflags.command_in_process = FLAG_RST;

						// Enable polling for OSDP application
						osdp_app.gb_enable_poll = 1;
					}
				}
				else if (osdp_app.chk_free_acs_ack_f == FLAG_SET)
				{
					// Increment the free access acknowledgment index
					osdp_app.freeAckIdx++;

					// Check if the acknowledgment index exceeds the length of the free access transmission list
					if (osdp_app.freeAckIdx >= iLock.tx_length)
					{
						// Reset the acknowledgment index to 0
						osdp_app.freeAckIdx = 0;

						// Reset the flag to check free access acknowledgment
						osdp_app.chk_free_acs_ack_f = FLAG_RST;

						// Reset the command in process flag
						iflags.command_in_process = FLAG_RST;

						// Reassign whole group flags
						ReAssign_Whole_Group_Flags();

						// Enable polling for OSDP application
						osdp_app.gb_enable_poll = 1;
					}
				}
				else if (osdp_app.chk_singleDr_fa_ack_f == FLAG_SET)
				{
					osdp_app.freeAckIdx ++;
					if (osdp_app.freeAckIdx >= iLock.tx_length)
					{
						osdp_app.freeAckIdx = 0;
					
						osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
						iflags.rd_ip_sts_for_one_dr = FLAG_SET;
						Set_Flags_To_Check_IP_Status();
					}
				}
				else if (osdp_app.chk_acs_dnd_ack_f == FLAG_SET)
				{
					osdp_app.acsAckIdx ++;
					if (osdp_app.acsAckIdx >= iLock.tx_length)
					{
						osdp_app.acsAckIdx = 0;
					
						osdp_app.chk_acs_dnd_ack_f = FLAG_RST;
						iflags.command_in_process = FLAG_RST;
						ReAssign_Whole_Group_Flags();
						osdp_app.gb_enable_poll = 1;
					}
				}
				else if (osdp_app.chk_singleDr_ad_ack_f == FLAG_SET)
				{
					osdp_app.acsAckIdx ++;
					if (osdp_app.acsAckIdx >= iLock.tx_length)
					{
						osdp_app.acsAckIdx = 0;
					
						osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
						iflags.rd_ip_sts_for_one_dr = FLAG_SET;
						Set_Flags_To_Check_IP_Status();
					}
				}
				else if (osdp_app.chk_prv_ack_f == FLAG_SET)
				{
					osdp_app.prvAckIdx ++;
					if (osdp_app.prvAckIdx >= iLock.tx_length)
					{
						osdp_app.prvAckIdx = 0;
					
						osdp_app.gb_enable_poll = 1;
						osdp_app.chk_prv_ack_f = FLAG_RST;
						iflags.command_in_process = FLAG_RST;
					}
				}
				else if ((iflags.chk_normal_state_ack == FLAG_SET) && (osdp_app.norReadyIdx == FLAG_SET))
				{
					osdp_app.norAckIdx ++;
					if (osdp_app.norAckIdx >= iLock.tx_length)
					{
						osdp_app.norAckIdx = 0;
						osdp_app.norReadyIdx = FLAG_RST;
						iflags.chk_normal_state_ack = FLAG_RST;
						iflags.command_in_process = FLAG_RST;
						U8 grpIdx = Find_Device_Group(iLock.actionSlave);
						grpData[grpIdx].command_executing_f = FLAG_RST;
					
						if (gb_power_on_flag == 1)
						{
							iflags.chk_ip_status = FLAG_SET;
							iflags.chk_ip_sts_ack = FLAG_SET;
							iflags.command_in_process = FLAG_SET;
							osdp_app.istTransIdx = 0;
						}
						else if (osdp_app.free_to_normal_f == FLAG_SET)
						{
							osdp_app.free_to_normal_f = FLAG_RST;
							ReAssign_Whole_Group_Flags();
// 							iflags.chk_noRsp_f_aft_freeRst = FLAG_SET;
							
							osdp_app.gb_enable_poll = 1;
						}
						else
						osdp_app.gb_enable_poll = 1;
					
						#if DEBUG_ALL || DEBUG_APP_OSDP_RX
						Print_Message("\nSet to normal state completed.");
						#endif
					
						iflags.chk_flags_after_normal = FLAG_SET;
						iflags.chk_noRsp_f_aft_freeRst = FLAG_SET;
					}
				}
				else if (iflags.chk_osdp_ilck_ack == FLAG_SET)
				{
					osdp_app.ilockRecIdx ++;
					if (osdp_app.ilockRecIdx >= iLock.tx_length)
					{
						osdp_app.ilockRecIdx = 0;
						iflags.chk_osdp_ilck_ack = FLAG_RST;
	// 					iflags.command_in_process = FLAG_RST;
	// 					U8 grpIdx = Find_Device_Group(iLock.actionSlave);
	// 					grpData[grpIdx].command_executing_f = FLAG_RST;
					
						#if DEBUG_ALL || DEBUG_APP_OSDP_RX
						Print_Message("\niLock ACK received from iLockSequence devices.");
						#endif
					
						if (iflags.ilock_by_force_dev_f == FLAG_SET)
						{
							iflags.ilock_by_force_dev_f = FLAG_RST;
						
							iflags.command_in_process = FLAG_RST;
							osdp_app.gb_enable_poll = 1;
						}
						else if (iflags.ilock_by_input_f == FLAG_SET)
						{
							iflags.ilock_by_input_f = FLAG_RST;
						
							iflags.command_in_process = FLAG_RST;
							osdp_app.gb_enable_poll = 1;
						}
						else if (iflags.chk_itd_time_value == FLAG_SET)
						{
							#if DEBUG_ALL || DEBUG_APP_OSDP_RX
							Print_Message("\nStart ITD timer for sAddress : ");
							Print_Number(iLock.actionSlave);
							#endif
						
							iflags.chk_itd_time_value = FLAG_RST;
							iflags.command_in_process = FLAG_RST;
							osdp_app.gb_enable_poll = 1;
						
							U8 dvIdx = 0;
							dvIdx = Find_Device_Index_InSystem(iLock.actionSlave);
							iLockDevice[dvIdx].itdCounts = 0;
						}
						else
						{
							#if DEBUG_ALL || DEBUG_APP_OSDP_RX
							Print_Message("\nSend OSDP_OUT command to sAddress : ");
							Print_Number(iLock.actionSlave);
							#endif
						
							Set_Flags_To_Send_OSDP_OUT();
						
							U8 grpIdx = Find_Device_Group(iLock.actionSlave);
							grpData[grpIdx].command_executing_f = FLAG_SET;
						}
					}
				}
				else if (iflags.chk_osdp_out_ack == FLAG_SET)
				{
					iflags.chk_osdp_out_ack = FLAG_RST;
					U8 dvIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
				
					if (iDeviceFlag[dvIdx].door_req_flag == FLAG_SET)
					{
						iDeviceFlag[dvIdx].chk_slvdrActive_state = FLAG_SET;
						iDeviceFlag[dvIdx].doorActiveState = FLAG_RST;
						iDeviceFlag[dvIdx].executing_dr_request_f = 0;
					
						#if DEBUG_ALL || DEBUG_APP_OSDP_RX
						Print_Message("\nCheck for slave 'Door-Active-State' of sAddress : ");
						Print_Number(gb_osdp.rec_dev_address);
						#endif
						
						/*	The slave ACKs OSDP_OUT even when it refuses to release (e.g. it
							is in interlock). Read its operational state to confirm that the
							release cycle really started. The door request sequence ends
							when this read completes. */
						Start_Release_Cycle_Check(gb_osdp.rec_dev_address);
					}
					else
					{
						#if DEBUG_ALL || DEBUG_APP_OSDP_RX
						Print_Message("\nOSDP_OUT ack received from sAddress : ");
						Print_Number(gb_osdp.rec_dev_address);
						#endif
					
						osdp_app.gb_enable_poll = 1;
						iflags.command_in_process = FLAG_RST;	/* DRT OSDP_OUT sequence is complete. */
						iDeviceFlag[dvIdx].chk_for_next_door_close = FLAG_SET;
						iDeviceFlag[dvIdx].chk_slvdrActive_state = FLAG_SET;
					}
				}
				else if (iflags.chk_drt_osdp_out_ack == FLAG_SET)
				{
					U8 dvIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
					iDeviceFlag[dvIdx].chk_for_next_door_close = FLAG_SET;
					iDeviceFlag[dvIdx].chk_slvdrActive_state = FLAG_SET;
				
					osdp_app.outAckIdx ++;
					if (osdp_app.outAckIdx >= iLock.tx_length)
					{
						osdp_app.outAckIdx = 0;
						iflags.chk_drt_osdp_out_ack = FLAG_RST;
						iflags.command_in_process = FLAG_RST;	/* DRT OSDP_OUT sequence is complete. */
					
						#if DEBUG_ALL || DEBUG_APP_OSDP_RX
						Print_Message("\nOSDP_OUT ack received from devices.");
						#endif
					
						osdp_app.gb_enable_poll = 1;
					}
				}
				else
				{
					// Skip.
				}
			}
			else if (gb_osdp.opstat_f == FLAG_SET)
			{
				/* Once operational state of the slave received */
				gb_osdp.opstat_f = FLAG_RST;	// Make this flag zero once received.
			
				U8 lclDvIdx = 0;
				lclDvIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
				/* Store them into an array */			
				slv_data[lclDvIdx].oprtState = gb_osdp.op_stat_data[0];
				slv_data[lclDvIdx].alarmState = gb_osdp.op_stat_data[1];
				Send_Slave_Status_Modbus(lclDvIdx);
			
				if (slv_data[lclDvIdx].oldOprtState != slv_data[lclDvIdx].oprtState)
				{
					Get_Group_Operation_State();
					slv_data[lclDvIdx].oldOprtState = slv_data[lclDvIdx].oprtState;
				}
			
			
				if ((slv_data[lclDvIdx].oprtState == SLV_OP_STATE_NORMAL)
				&& (iDeviceFlag[lclDvIdx].is_action_dev_normal == FLAG_SET))
				{
					iDeviceFlag[lclDvIdx].is_action_dev_normal = FLAG_RST;
					Reset_Input_ILock_Flags();
				}
			
				if ((iflags.chk_opt_sts_ack == FLAG_SET)
				&& (lcl_cmd_reply == FLAG_SET) && (gb_last_tx_cmd == CMD_OSDP_READ_OP_STATE))
				{
					osdp_app.oprRecIdx ++;
					if (osdp_app.oprRecIdx >= iLock.tx_length)
					{
						osdp_app.oprRecIdx = 0;
					
						iflags.opt_sts_read_success = FLAG_SET;
						iflags.chk_opt_sts_ack = FLAG_RST;
					
						if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
						{
							/*Do nothing*/
						}
						else
						{
							iflags.command_in_process = FLAG_RST;
							U8 grpIdx = Find_Device_Group(iLock.actionSlave);
							grpData[grpIdx].command_executing_f = FLAG_RST;
						}
						osdp_app.gb_enable_poll = 1;
					}
				}
			}
			else if (gb_osdp.istatr_f == FLAG_SET)
			{
				/* Once received the input status of the Slave */
				gb_osdp.istatr_f = FLAG_RST;	/* Reset flag for next reception */
			
				U8 dvIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
			
				slv_data[dvIdx].doorState = gb_osdp.istatr_data[IDX_DPS];	// value of DPS
				slv_data[dvIdx].lockState = gb_osdp.istatr_data[IDX_LFB];	// value of LFB
				slv_data[dvIdx].ipTypeState = gb_osdp.istatr_data[IDX_IP_TYPE_STATE];	// value of Input type state activated or not.
				slv_data[dvIdx].ipType = gb_osdp.istatr_data[IDX_IP_TYPE];			// value of Input type push or switch
				slv_data[dvIdx].ipStateAction = gb_osdp.istatr_data[IDX_STATE_ACTION];	// value of State action local or global.
				slv_data[dvIdx].auxAudioSelect = gb_osdp.istatr_data[IDX_AUX_AUDIO];		// value of Aux audio sel or de-select.

				Send_Slave_Status_Modbus(dvIdx);
			
				if ((iflags.chk_ip_sts_ack == FLAG_SET)
				&& (lcl_cmd_reply == FLAG_SET) && (gb_last_tx_cmd == CMD_OSDP_ISTAT))
				{
					osdp_app.istRecIdx ++;
					if (osdp_app.istRecIdx >= iLock.tx_length)
					{
						osdp_app.istRecIdx = 0;
						
						iflags.chk_ip_sts_ack = FLAG_RST;
						iflags.ip_sts_read_success = FLAG_SET;
						
						if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
						{
							/* Do nothing */
						}
						else
						{
							iflags.command_in_process = FLAG_RST;
							U8 grpIdx = Find_Device_Group(iLock.actionSlave);
							grpData[grpIdx].command_executing_f = FLAG_RST;
						}
					}
				}
			
				Check_Door_Active_State(dvIdx);
			}
		}
		
		/* Release the semaphore when done with the critical section */
		xSemaphoreGive(xSemaphore);
		}
		
		vTaskDelay(1);
	}
}

/*****************************************************************************
* Function name	: void OSDP_Poll_Delay(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 28/11/2023.
*
* Description	: Function written to set the poll flag.
*               :
* Notes			: Call This function in 1 ms of timer interrupt.
* Global Variables Affected : osdp_app.gb_poll_time ---> decrements till zero.
							  osdp_app.gb_poll_flag ---> Sets the flag.
*****************************************************************************/
void OSDP_Poll_Delay(void)
{
	/***** Checks Poll Time *****/
	if ((osdp_app.gb_poll_time > 0)/* && (gb_enable_poll_f == TRUE)*/)
	{
		osdp_app.gb_poll_time --;	/* Decrement here */
		if (osdp_app.gb_poll_time == 0)
		{
			/* If time reaches zero set the flag to poll */
			osdp_app.gb_poll_flag = FLAG_SET;
			osdp_app.gb_poll_time = POLL_DELAY;	// Refill POLL Timer value.
		}
	}
	/***** End of Checks Poll Time *****/
}

/*****************************************************************************
* Function name	: void OSDP_Frame_Response_Time(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 29/11/2023.
*
* Description	: Function written to set the frame not received flag
*				  after the defined time exceeds.
*               :
* Notes			: Call This function in 1 ms of timer interrupt.
* Global Variables Affected : osdp_app.gb_frame_reply_time ---> decrements till zero.
							  osdp_app.gb_frame_not_rcvd_f ---> Sets the flag.
*****************************************************************************/
void OSDP_Frame_Response_Time(void)
{
	/*****
	Decrement timer value. within this time frame
	must be received otherwise frame not received flag will be set.
	*****/
	if (osdp_app.gb_frame_reply_time > 0)
	{
		osdp_app.gb_frame_reply_time --;
		if (osdp_app.gb_frame_reply_time == 0)
		{
			/* If frame not received within defined frame receive time the set the flag */
			osdp_app.gb_frame_not_rcvd_f = FLAG_SET;
		}
	}
	/***** End of Decrement timer of max frame response time. *****/
}

/*****************************************************************************
* Function name	: void Send_OSDP_Frame_To_Slave(U8 *bufdata, U8 buflen)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 29/11/2023.
*
* Description	: Function written to send an osdp frame to slave using
*				  RS485 and loading a variable with frame reply delay to
*				  monitor frame error.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_frame_reply_time ---> decrements till zero.
*****************************************************************************/
void Send_OSDP_Frame_To_Slave(U8 *bufdata, U8 buflen)
{
	Send_data_On_RS485(bufdata, buflen);
	osdp_app.gb_frame_reply_time = FRAME_REPLY_DELAY;
}

/*****************************************************************************
* Function name	: void Select_Command_transmission(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 01/12/2023.
*
* Description	: Function written to select osdp commands to transmit.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Select_Command_transmission(void)
{
// 	if (iflags.command_in_process == FLAG_SET)
// 	{
// 		// do nothing
// 	}
// 	else
	osdp_app.gb_enable_poll = 1;
}

/*****************************************************************************
* Function name	: U8 Map_Operation_State_To_MBReg(U8 opStateValue)
* Returns		: U8 ---> Value of operation type to the mb register.
* Arguments    	: U8 opStateValue ---> pass value of operation state type from the osdp
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/12/2023.
*
* Description	: Function written to Map the value of operation state with the
*				  with the modbus registers.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Map_Operation_State_To_MBReg(U8 opStateValue)
{
	/* Fill as defined for the Modbus registers. */
	if (opStateValue == SLV_OP_STATE_NORMAL)
	{
		opStateValue = 0;	/* Controller state Normal */
	}
	else if (opStateValue == SLV_OP_STATE_DOOR_ACTIVE)
	{
		opStateValue = 1;	/* Controller state Door released. */
	}
	else if (opStateValue == SLV_OP_STATE_FREE_ACCESS)
	{
		opStateValue = 2;	/* Controller state Free Access. */
	}
	else if (opStateValue == SLV_OP_STATE_ACCESS_DENIED)
	{
		opStateValue = 3;	/* Controller state Deny Access. */
	}
	else if (opStateValue == SLV_OP_STATE_EMERGENCY)
	{
		opStateValue = 4;	/* Controller state in an Emergency. */
	}
	else if (opStateValue == SLV_OP_STATE_PRIVACY)
	{
		opStateValue = 5;	/* Controller state Privacy. */
	}
	else
	{
		opStateValue = 0;	/* Controller state Normal */
	}
	
	return opStateValue;
}

/*****************************************************************************
* Function name	: U8 Find_Device_Index_InSystem(U8 dev_address)
* Returns		: U8 ---> returns base index of the device.
* Arguments    	: U8 slvAddress ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to find device base index.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Find_Device_Index_InSystem(U8 dev_address)
{
	for (U8 zdx = 0; zdx < TOTAL_SLAVES; zdx++)
	{
		if (dev_address == inSysDeviceList[zdx].slv_addr)	// if device address and interlocking slave address matches?
		{
			return zdx;	// base index of the the row of interlocking devices.
		}
	}
}

/*****************************************************************************
* Function name	: void Check_Door_Active_State(U8 rvidx)
* Returns		: Nothing.
* Arguments    	: U8 rvidx ---> Pass received device index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to check whether the slave door is
*				  entered to and reset from the door active state.
*               :
* Notes			: NA.
* Global Variables Affected : iDeviceFlag[rvidx].doorActiveState, iflags.command_in_process
							  grpData[grpIdx].command_executing_f, iDeviceFlag[rvidx].chk_slvdrActive_state
							  iDeviceFlag[rvidx].start_itd_time, iDeviceFlag[rvidx].itd_timer_running
							  iLockDevice[rvidx].itdCounts.
*****************************************************************************/
void Check_Door_Active_State(U8 rvidx)
{
	U8 lcl_addr = inSysDeviceList[rvidx].slv_addr;
	
	/*	Note: this runs for every ISTATR, including replies that arrive while
		another door's command sequence is in progress. It must not change
		iflags.command_in_process or iLock.actionSlave, which belong to that
		sequence. */
	if ((iDeviceFlag[rvidx].chk_slvdrActive_state == FLAG_SET) && (iDeviceFlag[rvidx].door_req_flag == FLAG_SET))
	{
		if (slv_data[rvidx].lockState == UNLOCKED)
		{
			#if DEBUG_ALL || DEBUG_APP_OSDP_RX
			Print_Message("\nDoor active state of sAddress ");
			Print_Number(lcl_addr);
			Print_Message(" is activated.");
			#endif
			
			iDeviceFlag[rvidx].doorActiveState = FLAG_SET;
		}
		else if ((slv_data[rvidx].lockState == LOCKED) && (slv_data[rvidx].doorState == CLOSED)
		&& (iDeviceFlag[rvidx].doorActiveState == FLAG_SET))
		{
			/* The door was seen unlocked and is now closed and locked: release cycle is over. */
			#if DEBUG_ALL || DEBUG_APP_OSDP_RX
			Print_Message("\nDoor active state of sAddress ");
			Print_Number(lcl_addr);
			Print_Message(" is de activated.");
			#endif
			
			End_Release_Cycle(rvidx);
		}
		else
		{
			/*	Closed and locked, but the unlock was never reported: the slave may
				still be in ETD, or the unlock report was lost. Ending the cycle here
				would start ITD early and normalise the group while this door is
				still released. The periodic op-state check decides instead. */
		}
	}
	else if ((iDeviceFlag[rvidx].chk_for_next_door_close == FLAG_SET)
	&& (iDeviceFlag[rvidx].chk_slvdrActive_state == FLAG_SET))
	{
		if (slv_data[rvidx].lockState == UNLOCKED)
		{
			#if DEBUG_ALL || DEBUG_APP_OSDP_RX
			Print_Message("\nDoor active state of sAddress ");
			Print_Number(lcl_addr);
			Print_Message(" is activated.");
			#endif
			
			iDeviceFlag[rvidx].doorActiveState = FLAG_SET;
		}
		else if ((slv_data[rvidx].lockState == LOCKED) && ((slv_data[rvidx].doorState == CLOSED)))
		{
			if ((iflags.command_in_process == FLAG_RST)
			&& (emgGroup.e_executing_command == FLAG_RST)
			&& (fireFlags.f_executing_command == FLAG_RST)
			&& (iflags.ad_executing_command == FLAG_RST)
			)
			{
				#if DEBUG_ALL || DEBUG_APP_OSDP_RX
				Print_Message("\nDoor active state of sAddress ");
				Print_Number(lcl_addr);
				Print_Message(" is de activated.");
				#endif
				
				iDeviceFlag[rvidx].chk_slvdrActive_state = FLAG_RST;
				iDeviceFlag[rvidx].doorActiveState = FLAG_RST;
				iDeviceFlag[rvidx].chk_for_next_door_close = FLAG_RST;
				
				iLock.tx_address[0] = lcl_addr;
				iLock.tx_length = 1;
				Set_Flags_Put_Into_Normal(0);
			}
		}
	}
}

/*****************************************************************************
* Function name	: void Reset_Flags_Error_After_Retry(U8 rstDevAdd)
* Returns		: Nothing.
* Arguments    	: U8 rstDevAdd ---> Device address to force reset.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset the flags after defined retrials
*				  if error occured while reception on osdp.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Reset_Flags_Error_After_Retry(U8 rstDevAdd)
{
	if (iflags.put_into_normal_state == FLAG_SET)
	{
		if (fireFlags.reset_from_aux_ip == FLAG_SET)
		{
			__NOP();
		}
		else
		{
			//iflags.put_into_normal_state = FLAG_RST;
			//osdp_app.norTransIdx = 0;
			__NOP();
		}
	}
	
	if (iflags.chk_ip_status == FLAG_SET)
	{
		/*****
			Forcefully setting the flag to indicate that, ack received from slave.
			If slave device is off or not responding, while reading input status.
		*****/
		gb_osdp.istatr_f = FLAG_SET;
		gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
		
		/*
		iflags.chk_ip_status = FLAG_RST;
		osdp_app.istTransIdx = 0;
		iflags.chk_ip_after_normal = FLAG_RST;
		osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
		iflags.rd_ip_sts_for_one_dr = FLAG_RST;
		osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
		*/
	}
	if (iflags.chk_oprt_status == FLAG_SET)
	{
		/*****
			Forcefully setting the flag to indicate that, ack received from slave.
			If slave device is off or not responding, while reading operation status.
		*****/
		gb_osdp.opstat_f = FLAG_SET;
		gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
		
		/*
		iflags.chk_oprt_status = FLAG_RST;
		osdp_app.oprTransIdx = 0;
		osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
		iflags.rd_ip_sts_for_one_dr = FLAG_RST;
		osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
		*/
	}
	
	if (iflags.send_osdp_out_cmd == FLAG_SET)
	{
		//iflags.send_osdp_out_cmd = FLAG_RST;
	}
	
	if (iflags.put_into_interlock == FLAG_SET)
	{
// 		iflags.put_into_interlock = FLAG_RST;
// 		osdp_app.ilockTransIdx = 0;
	}
	
	if (iflags.chk_normal_state_ack == FLAG_SET)
	{
		if (fireFlags.reset_from_aux_ip == FLAG_SET)
		{
			/* Forcefully setting the flag to indicate that, ack received from slave.
				If slave device is off or not responding, while resetting from aux input.
			*/
			gb_osdp.ack_f = FLAG_SET;
			gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
		}
		else
		{
// 			iflags.chk_normal_state_ack = FLAG_RST;
// 			osdp_app.norAckIdx = 0;
			
			gb_osdp.ack_f = FLAG_SET;
			gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
		}
	}
	
	if (iflags.chk_osdp_ilck_ack == FLAG_SET)
	{
		/*
		iflags.chk_osdp_ilck_ack = FLAG_RST;
		osdp_app.ilockRecIdx = 0;
		if (iflags.chk_itd_time_value == FLAG_SET)
		{
			iflags.chk_itd_time_value = FLAG_RST;
		}
		*/
		
		
		/*
			Forcefully setting the flag to indicate that, ack received from slave.
			If slave device is off or not responding, while putting into an interlock.
		*/
		gb_osdp.ack_f = FLAG_SET;
		gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
	}
	
	if (iflags.chk_osdp_out_ack == FLAG_SET)
	{
		/*
		iflags.chk_osdp_out_ack = FLAG_RST;
		U8 dvIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
		if (iDeviceFlag[dvIdx].door_req_flag == FLAG_SET)
		{
			iDeviceFlag[dvIdx].executing_dr_request_f = 0;
			iDeviceFlag[dvIdx].door_req_flag = FLAG_RST;
		}
		*/
		
		/*
			Forcefully setting the flag to indicate that, ack received from slave.
			If slave device is off or not responding, while sending door release command.
		*/
		gb_osdp.ack_f = FLAG_SET;
		gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
	}
	
// 	if (iflags.chk_opt_sts_ack == FLAG_SET)
// 	{
// 		iflags.chk_opt_sts_ack = FLAG_RST;
// 		osdp_app.oprRecIdx = 0;
// 		osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
// 		iflags.rd_ip_sts_for_one_dr = FLAG_RST;
// 		osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
// 	}
// 	
// 	if (iflags.chk_ip_sts_ack == FLAG_SET)
// 	{
// 		iflags.chk_ip_sts_ack = FLAG_RST;
// 		osdp_app.istRecIdx = 0;
// 		osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
// 		iflags.rd_ip_sts_for_one_dr = FLAG_RST;
// 		osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
// 	}

// 	if (osdp_app.put_into_free_access == FLAG_SET)
// 	{
// 		osdp_app.put_into_free_access = FLAG_RST;
// 		osdp_app.freeTransIdx = 0;
// 		osdp_app.freeAckIdx = 0;
// 	}
	
	/* For an emergency and free access, not clearing flags */
// 	if (osdp_app.put_into_acs_denied == FLAG_SET)
// 	{
// 		osdp_app.put_into_acs_denied = FLAG_RST;
// 		osdp_app.acsTransIdx = 0;
// 	}
// 	if (osdp_app.chk_acs_dnd_ack_f == FLAG_SET)
// 	{
// 		osdp_app.chk_acs_dnd_ack_f = FLAG_RST;
// 		osdp_app.acsAckIdx = 0;
// 	}

	if (osdp_app.put_into_privacy_state == FLAG_SET)
	{
// 		osdp_app.put_into_privacy_state = FLAG_RST;
// 		osdp_app.prvTransIdx = 0;
	}
	
	if (osdp_app.chk_prv_ack_f == FLAG_SET)
	{
		/*
		osdp_app.chk_prv_ack_f = FLAG_RST;
		osdp_app.prvAckIdx = 0;
		*/
		
		/*
			Forcefully setting the flag to indicate that, ack received from slave.
			If slave device is off or not responding, while sending door release command.
		*/
		gb_osdp.ack_f = FLAG_SET;
		gb_osdp.rec_dev_address = rstDevAdd;	/* Force Update dev address, bcz of no respone */
	}
	
	//iflags.command_in_process = FLAG_RST;
	gb_receiving_f = 0;
}

/*****************************************************************************
* Function name	: void Match_Operation_Status_For_Output(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to match operational state to the output
				  door state and operational state array.
*               :
* Notes			: NA.
* Global Variables Affected : door_state, operational_state.
*****************************************************************************/
void Match_Operation_Status_For_Output(void)
{
	for (U8 stIdx = 0; stIdx < TOTAL_SLAVES; stIdx++)
	{
		/************ Door state ******************/
		if ((slv_data[stIdx].doorState == CLOSED) && (slv_data[stIdx].lockState == UNLOCKED))
		{
			door_state[stIdx] = 0x02;	/* Door Closed */
		}
		else if ((slv_data[stIdx].doorState == OPEN) && (slv_data[stIdx].lockState == UNLOCKED))
		{
			door_state[stIdx] = 0x01;	/* Door open */
		}
		else if ((slv_data[stIdx].doorState == CLOSED) && (slv_data[stIdx].lockState == LOCKED))
		{
			door_state[stIdx] = 0x03;	/* Door locked */
		}
		else
		{
			/*Do nothing*/
		}
		
		/***************** Operational State *****************/
		if (slv_data[stIdx].oprtState == SLV_OP_STATE_ACCESS_DENIED)
		{
			operational_state[stIdx] = 0x04;	/* Door in access denied */
		}
		else if (slv_data[stIdx].oprtState == SLV_OP_STATE_EMERGENCY)
		{
			operational_state[stIdx] = 0x05;	/* Door in an emergency */
		}
		else if (slv_data[stIdx].oprtState == SLV_OP_STATE_INTERLOCK)
		{
			operational_state[stIdx] = 0x06;	/* Door in an interlock */
		}
		else if (slv_data[stIdx].oprtState == SLV_OP_STATE_PRIVACY)
		{
			operational_state[stIdx] = 0x07;	/* Door in privacy */
		}
		else if (slv_data[stIdx].oprtState == SLV_OP_STATE_NORMAL)
		{
			operational_state[stIdx] = 0x08;	/* Door in normal */
		}
		else if (slv_data[stIdx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
		{
			/* This is added to avoid ambiguity in normal state */
			operational_state[stIdx] = 0x00;	/* Door in door active state */
		}
		else
		{
			/*Do nothing*/
		}
	}
}

/*****************************************************************************
* Function name	: void Find_Polling_Devices(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to device addresses to poll.
*               :
* Notes			: NA.
* Global Variables Affected : emgGroupArray[lcl_gpIdx].group_in_emergency.
*							  emgGroup.tx_address, emgGroup.tx_length.
*****************************************************************************/
void Find_Polling_Devices(void)
{
	U8 lclPollIdx = 0;
	U8 lclInsysIdx = 0;
	inSystem_dev_len = 0;
	polling_dev_len = 0;
	
	#if DEBUG_ALL || DEBUG_APP_OSDP_TX
	Print_Message("\nPolling Devices are : ");
	#endif
	
	for (U8 polIdx = 0; polIdx < TOTAL_SLAVES; polIdx++)
	{
		if (door_osdp_id[polIdx] > 0)
		{
			//inSysDeviceList[lclIdx].slv_addr = door_osdp_id[polIdx];
			PollDevice[lclPollIdx].slv_addr = door_osdp_id[polIdx];
			lclPollIdx ++;
			
			#if DEBUG_ALL || DEBUG_APP_OSDP_TX
			Print_Number(door_osdp_id[polIdx]);
			Print_Message("-");
			#endif
		}
		inSysDeviceList[lclInsysIdx].slv_addr = door_osdp_id[polIdx];
		lclInsysIdx ++;
	}
	
	if (lclPollIdx > 0)
	{
		polling_dev_len = lclPollIdx;
	}
	
	inSystem_dev_len = lclInsysIdx;
}

/*****************************************************************************
* Function name : void Reset_devNoResponse_flags(void)
* Returns       : Nothing
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 24/07/2024.
* Description   : This function resets the no-response flags for devices. It checks
*                 if the reset control flag is set and no command is currently in
*                 process. If these conditions are met, it resets the control flag
*                 and iterates through all slave devices to reset their no-response
*                 flags.
* Notes         : The function assumes that the global structures and flags like
*                 iflags, iDeviceFlag, and TOTAL_SLAVES are defined and properly
*                 initialized.
* Global Variables Affected : iflags.rst_noRsp_control_f, iDeviceFlag[].noFrame_ctrl_f
*****************************************************************************/
void Reset_devNoResponse_flags(void)
{
	// Check if reset control flag is set and no command is currently in process
	if ((iflags.rst_noRsp_control_f == FLAG_SET) && (iflags.command_in_process == FLAG_RST))
	{
		// Reset the reset control flag
		iflags.rst_noRsp_control_f = FLAG_RST;
		
		// Iterate through all slave devices
		for (U8 devIdx = 0; devIdx < TOTAL_SLAVES; devIdx ++)
		{
			// If the no-response flag for the device is set, reset the no-frame control flag
			if (iDeviceFlag[devIdx].gb_noResponse_f == FLAG_SET)
			{
				iDeviceFlag[devIdx].noFrame_ctrl_f = FLAG_RST;
			}
			
// 			if (iDeviceFlag[devIdx].dps_lfb_error_f == FLAG_SET)
// 			{
// 				iDeviceFlag[devIdx].drLock_control_f = FLAG_RST;
// 				
// 				Print_Message("\n AAA 444 aaaaaaaaaaaaaaa");
// 			}
		}
	}
}
