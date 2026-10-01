/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: interlock.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 02/12/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 02/12/2023.
* Changes		: NA.
*****************************************************************************/

/* User Includes */
#include "interlock.h"
#include "user_uart.h"
#include "group_config.h"
#include "string.h"
#include "definitions.h"
#include "digital_ip_app.h"
#include "input_config.h"
#include "fire_functionality.h"
#include "emergency.h"
#include "app_aux_input.h"
#include "mb_tcp_server.h"
#include "general_application.h"

/***** Macro definitions *****/


/* Structure Variables */
INFO_GROUP grpData[NUM_OF_GROUPS];
INFO_GROUP tempGrpData[NUM_OF_GROUPS];
GROUP_DEV_INFO groupInfo;
ILCKFLAG iflags;
ILCKFLAG iDeviceFlag[TOTAL_SLAVES];
ILCKFLAG copyiflags;
ILCKFLAG copyiDeviceFlag[TOTAL_SLAVES];
ILCK iLock;
ILCK iLockDevice[TOTAL_SLAVES];
IP_CNTRL ip_control;
MBFLAGS mbGetFlags[TOTAL_SLAVES];

/*****************************************************************************
* Function name	: U8 Find_Device_Group(U8 devAddress)
* Returns		: U8 (unsigned char ---> returns base index of the group.
* Arguments    	: U8 devAddress ---> Pass device address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function to find group index by device address.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Find_Device_Group(U8 devAddress)
{
	for (U8 gpIdx = 0; gpIdx < MAX_GROUPS; gpIdx++)
	{
		for (U8 dvIdx = 0; dvIdx < grpData[gpIdx].numOfDevs; dvIdx++)
		{
			if (devAddress == grpData[gpIdx].devices[dvIdx])
			{
				return gpIdx;
			}
		}
	}
}

/*****************************************************************************
* Function name	: U8 Find_Device_Index_InGroup(U8 gpIdx, U8 devAddress)
* Returns		: U8 (unsigned char ---> returns base index of the the row of interlocking devices.).
* Arguments    	: U8 devAddress ---> Pass device address.
*				  U8 gpIdx		---> Pass group index of the device.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function to find device index by its address in a group.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Find_Device_Index_InGroup(U8 gpIdx, U8 devAddress)
{
	for (U8 dvIdx = 0; dvIdx < grpData[gpIdx].numOfDevs; dvIdx++)
	{
		if (devAddress == grpData[gpIdx].devices[dvIdx])
		{
			return dvIdx;
		}
	}
}

/*****************************************************************************
* Function name	: U8 Number_Of_Doors_InGroup(U8 gpNumber, U16 drdata)
* Returns		: U8 (unsigned char ---> returns number of doors in a group).
* Arguments    	: U8 gpNumber	---> Pass group number.
*				  U16 drdata	---> Pass door data to find devices.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function to find number of devices in a group.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Number_Of_Doors_InGroup(U8 gpNumber, U16 drdata)
{
	return Count_Set_Bits(drdata);
}

/*****************************************************************************
* Function name	: U8 Count_Set_Bits(U16 data)
* Returns		: U8 (unsigned char ---> returns number of set bits).
* Arguments    	: U16 data	---> Pass data to find set bits.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function to find number of set bits.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Count_Set_Bits(U16 data)
{
	U8 count = 0;
	for (U8 i = 0; i < 16; i++)
	{
		count += (data >> i) & 1;
	}
	
	return count;
}

/*****************************************************************************
* Function name	: void Find_iLockSequence_Of_Group(U8 grpNmIdx)
* Returns		: Nothing.
* Arguments    	: U8 grpNmIdx	---> Pass index of the group.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function to find defined interlocking sequence of the group.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Find_iLockSequence_Of_Group(U8 grpNmIdx)
{
	U8 slvAddIdx = 0;	/* Define local variable */
	
	for (U8 slvNum = 0; slvNum < TOTAL_SLAVES; slvNum++)
	{
		if (Group[grpNmIdx].door[slvNum].slave_addr > 0)
		{
			/* If There is valid slave address */
			grpData[grpNmIdx].devices[slvAddIdx] = Group[grpNmIdx].door[slvNum].slave_addr;
			
			/* Find ITD Value */
			U16 lclItdValue = 0;
			lclItdValue = Group[grpNmIdx].door[slvNum].itd_hb;	// Get Higher Byte.
			lclItdValue = (lclItdValue << 8) + Group[grpNmIdx].door[slvNum].itd_lb;	// Get Lower Byte.
			
			grpData[grpNmIdx].iLockInfo[slvAddIdx].defItdValue = lclItdValue;
			/* End of Find ITD Value */
			
			U8 iLockDevIdx = 0;
			for (U8 slvNum1 = 0; slvNum1 < TOTAL_SLAVES; slvNum1++)
			{
				if (slvNum1 < (TOTAL_SLAVES / 2))
				{
					if (Group[grpNmIdx].door[slvNum].ils_door_lb & (1 << slvNum1))
					{
						grpData[grpNmIdx].iLockInfo[slvAddIdx].ilockSeq[iLockDevIdx++] = Group[grpNmIdx].door[slvNum1].slave_addr;
					}
				}
				else
				{
					if (Group[grpNmIdx].door[slvNum].ils_door_hb & (1 << (slvNum1 - 8)))
					{
						grpData[grpNmIdx].iLockInfo[slvAddIdx].ilockSeq[iLockDevIdx++] = Group[grpNmIdx].door[slvNum1].slave_addr;
					}
				}
				
			}
			grpData[grpNmIdx].iLockInfo[slvAddIdx].numOfiLckDevs = iLockDevIdx;
			slvAddIdx++;
		}
	}
	
	grpData[grpNmIdx].numOfDevs = slvAddIdx;
}

/*****************************************************************************
* Function name	: void Process_Interlocking(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function written to handle all the interlocking related tasks.
*               :
* Notes			: Call this function in never ending loop.
* Global Variables Affected : gb_osdp_cap_det_f, gb_osdp_dr_release_f, gb_osdp_rfid_rec_f
							  iflags.ip_sts_read_success, iflags.opt_sts_read_success
*****************************************************************************/
void Process_Interlocking(void)
{
	if ((gb_osdp_cap_det_f == FLAG_SET))
	{
		gb_osdp_cap_det_f = FLAG_RST;
		
		#if DEBUG_ALL || DEBUG_INTERLOCK
		Print_Message("\nCapsense signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address);
		#endif
		
		/* Call below function to take action if door in normal state */
		Get_Door_Access(gb_osdp.rec_dev_address);
	}
	else if ((gb_osdp_dr_release_f == FLAG_SET))
	{
		gb_osdp_dr_release_f = FLAG_RST;
		
		#if DEBUG_ALL || DEBUG_INTERLOCK
		Print_Message("\nDR(Door station) signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address);
		#endif
		
		/* Call below function to take action if door in normal state */
		Get_Door_Access(gb_osdp.rec_dev_address);
	}
	else if ((gb_osdp_rfid_rec_f == FLAG_SET))
	{
		gb_osdp_rfid_rec_f = FLAG_RST;
		
		#if DEBUG_ALL || DEBUG_INTERLOCK
		Print_Message("\nRFID DR signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address);
		#endif
		
		/* Call below function to take action if door in normal state */
		Get_Door_Access(gb_osdp.rec_dev_address);
	}
	
	
	if (iflags.ip_sts_read_success == FLAG_SET)
	{
		iflags.ip_sts_read_success = FLAG_RST;
		/* After successful completion of read input status. */
		
		#if DEBUG_ALL || DEBUG_INTERLOCK
		Print_Message("\nInput status read completed.");
		#endif
		
		/* Set flags to read opearation status */
		Set_Flags_To_Check_OPRT_Status();
		
		if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
		{
			/* Do nothing */
		}
		else if (iflags.chk_ip_after_normal == FLAG_SET)
		{
			iflags.chk_ip_after_normal = FLAG_RST;
			/* Check operational state after sending
			normal command from the master */
			iflags.chk_oprt_sts_after_normal = FLAG_SET;
		}
		else
		{
			/* Set group command executing flag. */
			U8 grpIdx = Find_Device_Group(iLock.actionSlave);
			grpData[grpIdx].command_executing_f = FLAG_SET;
		}
	}
	
	if (iflags.opt_sts_read_success == FLAG_SET)
	{
		iflags.opt_sts_read_success = FLAG_RST;
		/* After successful completion of read operational status */
		
		#if DEBUG_ALL || DEBUG_INTERLOCK
		Print_Message("\nOperational status read completed.");
		#endif
		
		if (iflags.chk_noRsp_f_aft_freeRst == FLAG_SET)
		{
			iflags.chk_noRsp_f_aft_freeRst = FLAG_RST;
			
			iflags.rst_noRsp_control_f = FLAG_SET;
		}
		
		if (gb_read_flag_val_pwrOn == FLAG_SET)
		{
			gb_read_flag_val_pwrOn = FLAG_RST;
			
			// Set / Reset Input flags at power on.
			Handle_PowerOn_Input_Flags();
		}
		
		if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
		{
			/*Do nothing*/
		}
		else
		{
			/* Reset below flags, after completion */
			emgGroup.e_executing_command = FLAG_RST;
			fireFlags.f_executing_command = FLAG_RST;
			iflags.ad_executing_command = FLAG_RST;
		}
		
		U8 err_status_f = 0;
		
		U8 lclDevIdx = 0;
		lclDevIdx = Find_Device_Index_InSystem(iLock.actionSlave);
		if (iDeviceFlag[lclDevIdx].chk_ip_sts_after_itd == FLAG_SET)
		{
			/* below routine is to check input status of the doors after the ITD timer complete. */
			if ((slv_data[lclDevIdx].doorState == OPEN) || (slv_data[lclDevIdx].lockState == UNLOCKED))
			{
				err_status_f = 1;
				
				#if DEBUG_ALL || DEBUG_INTERLOCK
				Print_Message("\nsAddress : ");
				Print_Number(iLock.actionSlave);
				Print_Message("'s input unhealthy.");
				
				if (slv_data[lclDevIdx].doorState == OPEN)
				{
					Print_Message("\tDPS error.");
				}
				if (slv_data[lclDevIdx].lockState == UNLOCKED)
				{
					Print_Message("\tLFB error.");
				}
				#endif
			}
			else
			{
				// No error!.
			}
		}
		
		if (err_status_f != 1)
		{
			/* If there is no error. */
			for (U8 idx = 0; idx < iLock.tx_length; idx++)
			{
				/* below routine is to check input status of the doors. */
				U8 dvIdx = 0;
		 		dvIdx = Find_Device_Index_InSystem(iLock.tx_address[idx]);
			
				if ((slv_data[dvIdx].oprtState == SLV_OP_STATE_NORMAL) || (slv_data[dvIdx].oprtState == SLV_OP_STATE_INTERLOCK))
				{
					if ((slv_data[dvIdx].doorState == CLOSED) && (slv_data[dvIdx].lockState == LOCKED))
					{
						err_status_f = 0;
					}
					else
					{
						#if DEBUG_ALL || DEBUG_INTERLOCK
						Print_Message("\nsAddress : ");
						Print_Number(inSysDeviceList[dvIdx].slv_addr);
						Print_Message("'s input unhealthy.");
					
						if (slv_data[dvIdx].doorState == OPEN)
						{
							Print_Message("\tDPS error.");
						}
						if (slv_data[dvIdx].lockState == UNLOCKED)
						{
							Print_Message("\tLFB error.");
						}
						#endif
						
						err_status_f = 1;
						break;
					}
				}
			}
		}
		
		if (err_status_f == 1)
		{
			err_status_f = 0;
			/* There is an error in the input state or operational state */
			
			#if DEBUG_ALL || DEBUG_INTERLOCK
			Print_Message("\nSome of the input or operational status are read unhealthy.");
			#endif
			
			if ((emgGroup.chk_fip_after_emgRst_f == FLAG_SET) || (fireFlags.chk_fip_after_ipRst_f == FLAG_SET))
			{
				emgGroup.chk_fip_after_emgRst_f = FLAG_RST;
				fireFlags.chk_fip_after_ipRst_f = FLAG_RST;
				
				Reset_Fire_Control_Flags();
			}
			
			if (iDeviceFlag[lclDevIdx].chk_ip_sts_after_itd == FLAG_SET)
			{
				iDeviceFlag[lclDevIdx].chk_ip_sts_after_itd = FLAG_RST;
				
				#if DEBUG_ALL || DEBUG_INTERLOCK
				Print_Message("\nRestart the ITD time of sAddress : ");
				Print_Number(iLock.actionSlave);
				#endif
				
				iDeviceFlag[lclDevIdx].start_itd_time = FLAG_SET;
				iLockDevice[lclDevIdx].itdCounts = 0;
			}
			else if (iflags.chk_oprt_sts_after_normal == FLAG_SET)
			{
				iflags.chk_oprt_sts_after_normal = FLAG_RST;
				
				Reset_Input_ILock_Flags();
				if (iflags.recheck_nonIlock_ips_f == FLAG_SET)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_RST;
					Reset_Input_NonILock_Flags();
				}
				Reset_DoorIP_Control_Flags();
			}
			else if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
			{
				iflags.rd_ip_sts_for_one_dr = FLAG_RST;
				
				iflags.command_in_process = FLAG_RST;
			}
			else
			{
				/*Do nothing*/
			}
			
		}
		else
		{
			/* Input and Operational state of the slave devices are healthy */
			#if DEBUG_ALL || DEBUG_INTERLOCK
			Print_Message("\nAll the input and operational status are read healthy.");
			#endif
			
			if ((emgGroup.chk_fip_after_emgRst_f == FLAG_SET) || (fireFlags.chk_fip_after_ipRst_f == FLAG_SET))
			{
				emgGroup.chk_fip_after_emgRst_f = FLAG_RST;
				fireFlags.chk_fip_after_ipRst_f = FLAG_RST;
				
				Reset_Fire_Control_Flags();
			}
			
			if (gb_power_on_flag == 1)
			{
				gb_power_on_flag = 0;
				
				Get_Group_Operation_State();
			}
			else if (iDeviceFlag[lclDevIdx].chk_for_normal_state == FLAG_SET)
			{
				iDeviceFlag[lclDevIdx].chk_for_normal_state = FLAG_RST;
				
				iDeviceFlag[lclDevIdx].chk_ip_sts_after_itd = FLAG_RST;
				iDeviceFlag[lclDevIdx].ilock_by_device = 0;
				iDeviceFlag[lclDevIdx].itd_timer_running = FLAG_RST;
				iDeviceFlag[lclDevIdx].is_action_dev_normal = FLAG_SET;
				
				Reset_From_Interlock(iLock.actionSlave);
				if (iLock.tx_length > 0)
				{
					Set_Flags_Put_Into_Normal(iLock.actionSlave);
				}
				
				#if DEBUG_ALL || DEBUG_INTERLOCK
				Print_Message("\nSo, put to normal state.");
				#endif
			}
			else if (iflags.chk_oprt_sts_after_normal == FLAG_SET)
			{
				iflags.chk_oprt_sts_after_normal = FLAG_RST;
				
				Reset_Input_ILock_Flags();
				if (iflags.recheck_nonIlock_ips_f == FLAG_SET)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_RST;
					Reset_Input_NonILock_Flags();
				}
				Reset_DoorIP_Control_Flags();
			}
			else if (iflags.rd_ip_sts_for_one_dr == FLAG_SET)
			{
				iflags.rd_ip_sts_for_one_dr = FLAG_RST;
				
				iflags.command_in_process = FLAG_RST;
			}
			else
			{
				U8 dvIdx = Find_Device_Index_InSystem(iLock.actionSlave);
				iDeviceFlag[dvIdx].ilock_by_device = FLAG_SET;
				Set_Flags_Put_Into_Interlock(iLock.actionSlave);
				
			}
		}
	}
	
	U8 lcl_wait_till_execution = FLAG_RST;
	for (U8 check = 0; check < TOTAL_SLAVES; check++)
	{
		if (iDeviceFlag[check].executing_dr_request_f == FLAG_SET)
		{
			/* If request is executing of the door, set below flag */
			lcl_wait_till_execution = FLAG_SET;
			break;
		}
	}
	
	if (lcl_wait_till_execution == FLAG_RST)
	{
		/* There no request routine executing of the door */
		for (U8 itdIdx = 0; itdIdx < TOTAL_SLAVES; itdIdx++)
		{
			if ((iDeviceFlag[itdIdx].itd_time_completed == FLAG_SET) && (iflags.command_in_process == FLAG_RST))
			{
				iDeviceFlag[itdIdx].itd_time_completed = FLAG_RST;
				iDeviceFlag[itdIdx].chk_for_normal_state = FLAG_SET;
			
				iLock.actionSlave = inSysDeviceList[itdIdx].slv_addr;
				
				#if DEBUG_ALL || DEBUG_INTERLOCK
				Print_Message("\nITD timer completed of sAddress : ");
				Print_Number(iLock.actionSlave);
				#endif
				
				
 				Find_Main_iLock_Sequence_With(iLock.actionSlave);
				Find_iLock_Sequence();
				
				iDeviceFlag[itdIdx].chk_ip_sts_after_itd = FLAG_SET;
				osdp_app.gb_enable_poll = 0;
				iflags.chk_ip_status = FLAG_SET;
				iflags.chk_ip_sts_ack = FLAG_SET;
				iflags.command_in_process = FLAG_SET;
				U8 grpIdx = Find_Device_Group(iLock.actionSlave);
				grpData[grpIdx].command_executing_f = FLAG_SET;
				osdp_app.istTransIdx = 0;	
			}
		}
	}
}

/*****************************************************************************
* Function name	: void Find_Main_iLock_Sequence_With(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function written to find configured interlocking sequence
*				  with the slave device.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Find_Main_iLock_Sequence_With(U8 slvAddress)
{
	U8 lcl_gpIdx = 0;
	U8 lcl_dvIdx = 0;
	lcl_gpIdx = Find_Device_Group(slvAddress);
	lcl_dvIdx = Find_Device_Index_InGroup(lcl_gpIdx, slvAddress);
	iLock.tx_length = 0;
	for (U8 idx = 0; idx < grpData[lcl_gpIdx].iLockInfo[lcl_dvIdx].numOfiLckDevs; idx++)
	{
		iLock.tx_address[idx] = grpData[lcl_gpIdx].iLockInfo[lcl_dvIdx].ilockSeq[idx];
		iLock.tx_length ++;
	}
	
	/***** Get ITD Value *****/
	U8 cmDevIdx = 0;
	cmDevIdx = Find_Device_Index_InSystem(slvAddress);
	iLockDevice[cmDevIdx].itdValue =(1000 * grpData[lcl_gpIdx].iLockInfo[lcl_dvIdx].defItdValue);
	/***** End of Get ITD Value *****/
}


/*****************************************************************************
* Function name	: void Interlock_Time_Delay(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Call a function to match ITD counts with defined ITD timer.
*               :
* Notes			: Call this routine in timer interrupt routine.
* Global Variables Affected : NA.
*****************************************************************************/
void Interlock_Time_Delay(void)
{
	U8 itdIdx = 0;
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
	Match_ITD_Counts(itdIdx++);
}

/*****************************************************************************
* Function name	: void Match_ITD_Counts(U8 dvIdx)
* Returns		: Nothing.
* Arguments    	: U8 dvIdx ---> Pass device index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Match ITD counts with defined ITD timer.
*               :
* Notes			: NA.
* Global Variables Affected : iDeviceFlag[dvIdx].start_itd_time
*							  iDeviceFlag[dvIdx].itd_time_completed
*****************************************************************************/
void Match_ITD_Counts(U8 dvIdx)
{
	if (iDeviceFlag[dvIdx].start_itd_time == FLAG_SET)
	{
		iLockDevice[dvIdx].itdCounts ++;
		if (iLockDevice[dvIdx].itdCounts >= iLockDevice[dvIdx].itdValue)
		{
			iLockDevice[dvIdx].itdCounts = 0;
			iDeviceFlag[dvIdx].start_itd_time = FLAG_RST;
			iDeviceFlag[dvIdx].itd_time_completed = FLAG_SET;
		}
	}
}

/*****************************************************************************
* Function name	: void Keep_Monotoring_IPStatus(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is to monitor the input status and set error flags.
*               :
* Notes			: NA.
* Global Variables Affected : iDeviceFlag[dvIdx].dps_lfb_error_f.
*****************************************************************************/
void Keep_Monotoring_IPStatus(void)
{
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++)
	{
		U8 grpIdx = Find_Device_Group(inSysDeviceList[dvIdx].slv_addr);
		
		if ((slv_data[dvIdx].doorState == OPEN) || (slv_data[dvIdx].lockState == UNLOCKED))
		{
			if (Is_Device_Normal_And_Other_Flags_Reset(dvIdx, grpIdx) == FLAG_SET)
			{
				iDeviceFlag[dvIdx].dps_lfb_error_f = FLAG_SET;
			}
			else
			{
				__NOP();
			}
		}
		else if ((slv_data[dvIdx].doorState == CLOSED) && (slv_data[dvIdx].lockState == LOCKED))
		{
			if (Is_Device_Normal_And_Other_Flags_Reset(dvIdx, grpIdx) == FLAG_SET)
			{
				iDeviceFlag[dvIdx].dps_lfb_error_f = FLAG_RST;
			}
			else
			{
				__NOP();
			}
		}
	}
}

/*****************************************************************************
* Function name	: void Force_Interlock_By_DoorState(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function written to find force interlocking devices
*				  and put them into an interlock based on error flag set.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Force_Interlock_By_DoorState(void)
{
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++)
	{
		if ((iDeviceFlag[dvIdx].dps_lfb_error_f == FLAG_SET)
		&& (iDeviceFlag[dvIdx].drLock_control_f == FLAG_RST)
		&& (iflags.command_in_process == FLAG_RST))
		{
			iDeviceFlag[dvIdx].drLock_control_f = FLAG_SET;
			
			Find_Main_iLock_Sequence_With(inSysDeviceList[dvIdx].slv_addr);
			Find_iLock_Sequence_And_Self(inSysDeviceList[dvIdx].slv_addr);
			
			if (iLock.tx_length > 0)
			{
				iflags.ilock_by_force_dev_f = FLAG_SET;
				Set_Flags_Put_Into_Interlock(inSysDeviceList[dvIdx].slv_addr);
			}
		}
		else if ((iDeviceFlag[dvIdx].dps_lfb_error_f == FLAG_RST)
		&& (iDeviceFlag[dvIdx].drLock_control_f == FLAG_SET)
		&& (iflags.command_in_process == FLAG_RST))
		{
			iDeviceFlag[dvIdx].drLock_control_f = FLAG_RST;
			
			Reset_From_Interlock(inSysDeviceList[dvIdx].slv_addr);
			if (iLock.tx_length > 0)
			{
				Set_Flags_Put_Into_Normal(inSysDeviceList[dvIdx].slv_addr);
			}
		}
	}
}

/*****************************************************************************
* Function name	: void Force_Interlock_By_NoFrame(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function written to find force interlocking devices
*				  and put them into an interlock based on error flag set.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Force_Interlock_By_NoFrame(void)
{
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++)
	{
		if ((iDeviceFlag[dvIdx].gb_noResponse_f == FLAG_SET)
		&& (iDeviceFlag[dvIdx].noFrame_ctrl_f == FLAG_RST)
		&& (iflags.command_in_process == FLAG_RST))
		{
			iDeviceFlag[dvIdx].noFrame_ctrl_f = FLAG_SET;
			
			Find_Main_iLock_Sequence_With(inSysDeviceList[dvIdx].slv_addr);
			Find_iLock_Sequence_And_Self(inSysDeviceList[dvIdx].slv_addr);
			
			if (iLock.tx_length > 0)
			{
				iflags.ilock_by_force_dev_f = FLAG_SET;
				Set_Flags_Put_Into_Interlock(inSysDeviceList[dvIdx].slv_addr);
			}
			
			break;
		}
		else if ((iDeviceFlag[dvIdx].gb_noResponse_f == FLAG_RST)
		&& (iDeviceFlag[dvIdx].noFrame_ctrl_f == FLAG_SET)
		&& (iflags.command_in_process == FLAG_RST))
		{
			iDeviceFlag[dvIdx].noFrame_ctrl_f = FLAG_RST;
			Handle_Slave_Dev_Response_PowerUp(dvIdx);
			break;
		}
	}
}

/*****************************************************************************
* Function name	: void Reset_From_Interlock(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset from interlock logic. Checked that
*				  whether the devices are in other iLock sequence or not.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Reset_From_Interlock(U8 slvAddress)
{
	U8 lclUpdateDevices = 0;
	U8 fnlDevIdx = 0;
	U8 lcl_dev_in_other_grp_f = 0;
	
	/* Clear the buffer with filling 0 values. */
	memset(iLock.utx_address, 0, TOTAL_SLAVES);
	
	if (slvAddress > 0)
	{
		/* If there is valid slave address? */
		Find_Main_iLock_Sequence_With(slvAddress);
		Find_iLock_Sequence();
	}
	
	U8 ilockLength = 0;
	memcpy(iLock.rst_address, iLock.tx_address, iLock.tx_length);
	ilockLength = iLock.tx_length;
	
	for (U8 rstIdx = 0; rstIdx < ilockLength; rstIdx++)
	{
		for (U8 tDevIdx = 0; tDevIdx < TOTAL_SLAVES; tDevIdx++)
		{
			if ((iDeviceFlag[tDevIdx].itd_timer_running == FLAG_SET)
			|| (iDeviceFlag[tDevIdx].doorActiveState == FLAG_SET)
			|| (iDeviceFlag[tDevIdx].chk_slvdrActive_state == FLAG_SET)
			|| (iDeviceFlag[tDevIdx].executing_dr_request_f == FLAG_SET))
			{
				Find_Main_iLock_Sequence_With(inSysDeviceList[tDevIdx].slv_addr);
				Find_iLock_Sequence();
			
				for (U8 nRstIdx = 0; nRstIdx < iLock.tx_length; nRstIdx++)
				{
					if (iLock.rst_address[rstIdx] == iLock.tx_address[nRstIdx])
					{
						// If device is in interlock with other device.
						lcl_dev_in_other_grp_f = 1;
						break;
					}
					else
					{
						// do nothing.
					}
				}
				
				if (lcl_dev_in_other_grp_f == 1)
				{
					break;
				}
				else
				{
					// do nothing.
 				}
			}
		}
		
		if (lcl_dev_in_other_grp_f == 1)
		{
			// skip, if device is in other interlock group.
			lcl_dev_in_other_grp_f = 0;
			
			if (lclUpdateDevices != 1)
			{
				lclUpdateDevices = 2;
			}		
		}
		else
		{
			U8 dvIdx = Find_Device_Index_InSystem(iLock.rst_address[rstIdx]);
			if ((slv_data[dvIdx].oprtState == SLV_OP_STATE_INTERLOCK)
			|| (slv_data[dvIdx].oprtState == SLV_OP_STATE_NORMAL))
			{
				iLock.utx_address[fnlDevIdx++] = iLock.rst_address[rstIdx];
				lclUpdateDevices = 1;
			}
		}
	}
	
	if (lclUpdateDevices == 2)
	{
		/* The devices are in interlock with other device. */
		if (slvAddress > 0)
		{
			/* If valid slave address, reset only parent device */
			iflags.reset_from_iLock_f = FLAG_SET;
			iLock.tx_address[0] = slvAddress;
			iLock.tx_length = 1;
		}
		else
		{
			/* Make an array empty */
			memset(iLock.tx_address, 0, ilockLength);
			iLock.tx_length = 0;
		}
	}
	else if (lclUpdateDevices == 1)
	{
		/* If any one of the devices is not in interlock with other device. */
		/* Update transmit array with updated reset device array */
		memcpy(iLock.tx_address, iLock.utx_address, fnlDevIdx);
		iLock.tx_length = fnlDevIdx;
		
		if (slvAddress > 0)
		{
			/* If valid slave address, append parent device with above array */
			iflags.reset_from_iLock_f = FLAG_SET;
			iLock.tx_address[iLock.tx_length++] = slvAddress;
		}
	}
	else
	{
		/* If single interlocking sequence is running */
		if (slvAddress > 0)
		{
			/* If valid slave address, append parent device. */
			iflags.reset_from_iLock_f = FLAG_SET;
			iLock.tx_address[iLock.tx_length++] = slvAddress;
		}
	}
}

/*****************************************************************************
* Function name	: void Find_iLock_Sequence(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written find an interlocking sequence of slave
*				  devices who are in the operation state, Normal and interlock.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
void Find_iLock_Sequence(void)
{
	U8 lcl_iIdx = 0;
	for (U8 idx = 0; idx < iLock.tx_length; idx++)
	{
		U8 dvIdx = Find_Device_Index_InSystem(iLock.tx_address[idx]);
		
		if ((slv_data[dvIdx].oprtState == SLV_OP_STATE_INTERLOCK) || (slv_data[dvIdx].oprtState == SLV_OP_STATE_NORMAL))
		{
			iLock.tx_address[lcl_iIdx++] = iLock.tx_address[idx];
		}
	}
	iLock.tx_length = lcl_iIdx;
}

/*****************************************************************************
* Function name	: void Find_iLock_Sequence_And_Self(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written find an interlocking sequence of slave
*				  devices who are in the operation state Normal and interlock.
*               :
* Notes			: The passing slave device is also appended in the buffer.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
void Find_iLock_Sequence_And_Self(U8 slvAddress)
{
	U8 lcl_iIdx = 0;
	iLock.tx_address[iLock.tx_length++] = slvAddress;
	
	for (U8 idx = 0; idx < iLock.tx_length; idx++)
	{
		U8 dvIdx = Find_Device_Index_InSystem(iLock.tx_address[idx]);
		
		if (Check_Any_DR_Command_Executing(dvIdx) == FLAG_SET)
		{
			iLock.tx_address[lcl_iIdx++] = iLock.tx_address[idx];
		}
	}
	
	if (lcl_iIdx == 0)
	{
		memset(iLock.tx_address, 0, iLock.tx_length);
		iLock.tx_length = 0;
	}
	else
	{
		iLock.tx_length = lcl_iIdx;
	}
}

/*****************************************************************************
* Function name	: void Find_Group_Devices(U8 lcl_gpIdx)
* Returns		: Nothing.
* Arguments    	: U8 lcl_gpIdx ---> Pass group index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to sort / get group devices.
*               :
* Notes			: NA.
* Global Variables Affected : groupInfo.length, groupInfo.slaves[].
*****************************************************************************/
void Find_Group_Devices(U8 lcl_gpIdx)
{
	groupInfo.length = 0;
	for (U8 idx = 0; idx < grpData[lcl_gpIdx].numOfDevs; idx++)
	{
		groupInfo.slaves[idx] = grpData[lcl_gpIdx].devices[idx];
		groupInfo.length ++;
	}
}

/*****************************************************************************
* Function name	: bool Find_iLockSequence_With_Input(U8 IpIdx)
* Returns		: bool ---> returns 1 if there is a valid slave device else returns 0.
* Arguments    	: U8 IpIdx ---> Pass index number of the input or (input number).
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get interlocking sequence with input.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
bool Find_iLockSequence_With_Input(U8 IpIdx)
{
	U8 lclSlvIdx = 0;
		
	for (U8 grpNmIdx = 0; grpNmIdx < MAX_GROUPS; grpNmIdx++)
	{
		if (IS_BIT_SET(Group[grpNmIdx].ips_in_grp, IpIdx) == IP_ENABLED)
		{
			/* Input is enabled in group. */
			U16 lcliLckIps = 0;
			lcliLckIps = Group[grpNmIdx].input[IpIdx].ils_ips_hb;
			lcliLckIps <<= 8;
			lcliLckIps |= Group[grpNmIdx].input[IpIdx].ils_ips_lb;
			
			for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++)
			{
				if (IS_BIT_SET(lcliLckIps, dvIdx) == FLAG_SET)
				{
					if (Check_Any_DR_Command_Executing(dvIdx) == FLAG_SET)
					{
						iLock.tx_address[lclSlvIdx++] = inSysDeviceList[dvIdx].slv_addr;
					}
				}
			}
		}
	}
	
	#if (DEBUG_ALL || DEBUG_INTERLOCK)
	Print_Message("\n\niLock seq of Input \"");
	Print_Number(IpIdx + 1);
	Print_Message("\" is : ");
	for (U8 idx = 0; idx < lclSlvIdx; idx++)
	{
		Print_Message("-");
		Print_Number(iLock.tx_address[idx]);
	}
	#endif
	
	if (lclSlvIdx > 0)
	{
		iLock.tx_length = lclSlvIdx;
		return 1;
	}
	else
	return 0;
}

/*****************************************************************************
* Function name	: U8 Fill_Array_With_iLock_Devices(U8* UpdArray, U8 lenUpArr, U8* oldArray, U8 lenOldArr)
* Returns		: U8 (lenUpArr) ---> length of updated array.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to fill an update array from the old array.
*				  The update array holds the address of slave devices, update array
*				  do not repeats the slave addresses in it.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Fill_Array_With_iLock_Devices(U8* UpdArray, U8 lenUpArr, U8* oldArray, U8 lenOldArr)
{
	for (U8 oldIdx = 0; oldIdx < lenOldArr; oldIdx++)
	{
		U8 foundSlv = 0;
		// Check if the current slave of oldArray exists in UpdArray.
		for (U8 upIdx = 0; upIdx < lenUpArr; upIdx++)
		{
			if (oldArray[oldIdx] == UpdArray[upIdx])
			{
				foundSlv = 1; // slave already exists in UpdArray.
				break;
			}
		}
		
		// If the slave does not exist in UpdArray, add it.
		if (!foundSlv)
		{
			// Find an empty spot in UpdArray and add the slave.
			UpdArray[lenUpArr] = oldArray[oldIdx];
			lenUpArr ++;
		}
	}
	
	return lenUpArr;
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_Into_Interlock(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to put slave devices
*				  into an interlock state.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.put_into_interlock, iflags.chk_osdp_ilck_ack
							  osdp_app.gb_enable_poll, osdp_app.ilockTransIdx
							  iflags.command_in_process.
*****************************************************************************/
void Set_Flags_Put_Into_Interlock(U8 slvAddress)
{
	osdp_app.gb_enable_poll = 0;
	iflags.put_into_interlock = FLAG_SET;
	iflags.chk_osdp_ilck_ack = FLAG_SET;
	osdp_app.ilockTransIdx = 0;
	iflags.command_in_process = FLAG_SET;
	if (slvAddress > 0)
	{
		U8 grpIdx = Find_Device_Group(slvAddress);
		grpData[grpIdx].command_executing_f = FLAG_SET;
	}
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_Into_Normal(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to put slave devices
*				  into an Normal state.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.put_into_normal_state, iflags.chk_normal_state_ack
							  osdp_app.gb_enable_poll, osdp_app.norTransIdx
							  iflags.command_in_process.
*****************************************************************************/
void Set_Flags_Put_Into_Normal(U8 slvAddress)
{
	osdp_app.gb_enable_poll = 0;
	iflags.put_into_normal_state = FLAG_SET;
	iflags.chk_normal_state_ack = FLAG_SET;
	osdp_app.norTransIdx = 0;
	iflags.command_in_process = FLAG_SET;
	
	if (slvAddress > 0)
	{
		U8 grpIdx = Find_Device_Group(iLock.actionSlave);
		grpData[grpIdx].command_executing_f = FLAG_SET;
	}
}

/*****************************************************************************
* Function name	: void Keep_Monitoring_IP_And_TakeAction(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: A function is implemented to continuously monitor digital
*				  input signals. Based on the state of these signals and the
*				  configuration set through utility software, specific functions
*				  are invoked to perform predefined actions.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
							  ip_control.ip1_control_f, ip_control.ip2_control_f
							  ip_control.ip3_control_f, ip_control.ip4_control_f
							  ip_control.ip5_control_f, ip_control.ip6_control_f
							  
*****************************************************************************/
void Keep_Monitoring_IP_And_TakeAction(void)
{
	switch (gb_ip1_func)
	{
		case IP_ILOCK : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				Interlock_By_The_Input_Number(IP_1_IDX);
			}
		break;
		case IP_ILOCK_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				Normal_By_The_Input_Number(IP_1_IDX);
			}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR : 
		case MOM_DOR_DR : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_1_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
		break;
		case TOG_DOR_DR_N : 
		case MOM_DOR_DR_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_1_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Set_Flags_Put_Into_Normal(0);
				}
			}
		break;
		case TOG_GRP_DR : 
		case MOM_GRP_DR : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				Group_DoorRelease((gb_input[IP_1_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_DR_N : 
		case MOM_GRP_DR_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				Group_DR_To_Normal((gb_input[IP_1_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_DR : 
		case MOM_ALLDOR_DR : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_FA_Group_Devices();
				
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Free_Access();
				}
			}
		break;
		case TOG_ALLDOR_DR_N : 
		case MOM_ALLDOR_DR_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_FA_Group_Devices();
				
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
				}
			}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_1_IDX].door_grp_number - 1);
				
				if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				Door_AD_To_Normal((gb_input[IP_1_IDX].door_grp_number - 1));
			}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				Group_AccessDenied((gb_input[IP_1_IDX].door_grp_number - 17));
				
			}
		break;
		case TOG_GRP_AD_N : 
		case MOM_GRP_AD_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				Group_AD_To_Normal((gb_input[IP_1_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_AD : 
		case MOM_ALLDOR_AD : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Access_Denied();
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_ALLDOR_AD_N : 
		case MOM_ALLDOR_AD_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_DOR_DRT : 
		case MOM_DOR_DRT : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_1_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
				{
					iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
					Set_Flags_To_Send_OSDP_OUT();
				}
			}
		break;
		case TOG_DOR_DRT_N : 
		case MOM_DOR_DRT_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_1_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
				{
					/*DRT ongoing*/
				}
			}
		break;
		case TOG_GRP_DRT : 
		case MOM_GRP_DRT : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_1_IDX].door_grp_number - 17);
				
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						osdp_app.gb_enable_poll = 0;
						iflags.send_drt_osdp_out_cmd = FLAG_SET;
						iflags.command_in_process = FLAG_SET;
						osdp_app.outTransIdx = 0;
					}
				}
			}
		break;
		case TOG_GRP_DRT_N : 
		case MOM_GRP_DRT_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
				
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_1_IDX].door_grp_number - 17);
				
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						/*Group devices are*/
					}
				}
			}
		break;
		case TOG_ALLDOR_DRT : 
		case MOM_ALLDOR_DRT : 
			if (Is_IP1_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		break;
		case TOG_ALLDOR_DRT_N : 
		case MOM_ALLDOR_DRT_N : 
			if (Is_IP1_Control_Flag_Set() == TRUE)
			{
				ip_control.ip1_control_f = FLAG_RST;
			
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
			
				if (isValidFireDevice)
				{
					/*DRT to Normal devices are*/
				}
			}
		break;
		default:
		break;
	}
	switch (gb_ip2_func)
	{
		case IP_ILOCK :
		if (Is_IP2_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip2_control_f = FLAG_SET;
			
			Interlock_By_The_Input_Number(IP_2_IDX);
		}
		break;
		case IP_ILOCK_N :
		if (Is_IP2_Control_Flag_Set() == TRUE)
		{
			ip_control.ip2_control_f = FLAG_RST;
			
			Normal_By_The_Input_Number(IP_2_IDX);
		}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR :
		case MOM_DOR_DR :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_2_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
		break;
		case TOG_DOR_DR_N :
		case MOM_DOR_DR_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_2_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Set_Flags_Put_Into_Normal(0);
				}
			}
		break;
		case TOG_GRP_DR :
		case MOM_GRP_DR :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
			
				Group_DoorRelease((gb_input[IP_2_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_DR_N :
		case MOM_GRP_DR_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
			
				Group_DR_To_Normal((gb_input[IP_2_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_DR :
		case MOM_ALLDOR_DR :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_FA_Group_Devices();
			
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Free_Access();
				}
			}
		break;
		case TOG_ALLDOR_DR_N :
		case MOM_ALLDOR_DR_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_FA_Group_Devices();
				
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
				}
			}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_2_IDX].door_grp_number - 1);
				
				if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				Door_AD_To_Normal((gb_input[IP_2_IDX].door_grp_number - 1));
			}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
				
				Group_AccessDenied((gb_input[IP_2_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_AD_N :
		case MOM_GRP_AD_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				Group_AD_To_Normal((gb_input[IP_2_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_AD :
		case MOM_ALLDOR_AD :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Access_Denied();
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_ALLDOR_AD_N :
		case MOM_ALLDOR_AD_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_DOR_DRT :
		case MOM_DOR_DRT :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_2_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
				{
					iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
					Set_Flags_To_Send_OSDP_OUT();
				}
			}
		break;
		case TOG_DOR_DRT_N :
		case MOM_DOR_DRT_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_2_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
				{
					/* DRT ongoing */
				}
			}
		break;
		case TOG_GRP_DRT :
		case MOM_GRP_DRT :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
				
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_2_IDX].door_grp_number - 17);
				
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						osdp_app.gb_enable_poll = 0;
						iflags.send_drt_osdp_out_cmd = FLAG_SET;
						iflags.command_in_process = FLAG_SET;
						osdp_app.outTransIdx = 0;
					}
				}
			}
		break;
		case TOG_GRP_DRT_N :
		case MOM_GRP_DRT_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
			
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_2_IDX].door_grp_number - 17);
			
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						/* DRT to Normal devices are */
					}
				}
			}
		break;
		case TOG_ALLDOR_DRT :
		case MOM_ALLDOR_DRT :
			if (Is_IP2_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		break;
		case TOG_ALLDOR_DRT_N :
		case MOM_ALLDOR_DRT_N :
			if (Is_IP2_Control_Flag_Set() == TRUE)
			{
				ip_control.ip2_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					/* DRT to Normal devices are */
				}
			}
		break;
		default:
		break;
	}
	switch (gb_ip3_func)
	{
		case IP_ILOCK :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				Interlock_By_The_Input_Number(IP_3_IDX);
			}
		break;
		case IP_ILOCK_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
				
				Normal_By_The_Input_Number(IP_3_IDX);
			}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR :
		case MOM_DOR_DR :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_3_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
		break;
		case TOG_DOR_DR_N :
		case MOM_DOR_DR_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_3_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Set_Flags_Put_Into_Normal(0);
				}
			}
		break;
		case TOG_GRP_DR :
		case MOM_GRP_DR :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				Group_DoorRelease((gb_input[IP_3_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_DR_N :
		case MOM_GRP_DR_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				Group_DR_To_Normal((gb_input[IP_3_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_DR :
		case MOM_ALLDOR_DR :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_FA_Group_Devices();
			
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Free_Access();
				}
			}
		break;
		case TOG_ALLDOR_DR_N :
		case MOM_ALLDOR_DR_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_FA_Group_Devices();
			
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
				}
			}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_3_IDX].door_grp_number - 1);
			
				if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				Door_AD_To_Normal((gb_input[IP_3_IDX].door_grp_number - 1));
			}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				Group_AccessDenied((gb_input[IP_3_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_AD_N :
		case MOM_GRP_AD_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				Group_AD_To_Normal((gb_input[IP_3_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_AD :
		case MOM_ALLDOR_AD :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_AD_Group_Devices();
			
				if (isValidFireDevice)
				{
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Access_Denied();
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_ALLDOR_AD_N :
		case MOM_ALLDOR_AD_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_AD_Group_Devices();
			
				if (isValidFireDevice)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_DOR_DRT :
		case MOM_DOR_DRT :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_3_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
				{
					iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
					Set_Flags_To_Send_OSDP_OUT();
				}
			}
		break;
		case TOG_DOR_DRT_N :
		case MOM_DOR_DRT_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_3_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
				{
					/* DRT ongoing */
				}
			}
		break;
		case TOG_GRP_DRT :
		case MOM_GRP_DRT :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_3_IDX].door_grp_number - 17);
			
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						osdp_app.gb_enable_poll = 0;
						iflags.send_drt_osdp_out_cmd = FLAG_SET;
						iflags.command_in_process = FLAG_SET;
						osdp_app.outTransIdx = 0;
					}
				}
			}
		break;
		case TOG_GRP_DRT_N :
		case MOM_GRP_DRT_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
			
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_3_IDX].door_grp_number - 17);
			
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						/* DRT to Normal devices are */
					}
				}
			}
		break;
		case TOG_ALLDOR_DRT :
		case MOM_ALLDOR_DRT :
			if (Is_IP3_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
			
				if (isValidFireDevice)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		break;
		case TOG_ALLDOR_DRT_N :
		case MOM_ALLDOR_DRT_N :
			if (Is_IP3_Control_Flag_Set() == TRUE)
			{
				ip_control.ip3_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					/* DRT to Normal devices are */
				}
			}
		break;
		default:
		break;
	}
	switch (gb_ip4_func)
	{
		case IP_ILOCK :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
				
				Interlock_By_The_Input_Number(IP_4_IDX);
			}
		break;
		case IP_ILOCK_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
				
				Normal_By_The_Input_Number(IP_4_IDX);
			}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR :
		case MOM_DOR_DR :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_4_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
		break;
		case TOG_DOR_DR_N :
		case MOM_DOR_DR_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_4_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Set_Flags_Put_Into_Normal(0);
				}
			}
		break;
		case TOG_GRP_DR :
		case MOM_GRP_DR :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				Group_DoorRelease((gb_input[IP_4_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_DR_N :
		case MOM_GRP_DR_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				Group_DR_To_Normal((gb_input[IP_4_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_DR :
		case MOM_ALLDOR_DR :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_FA_Group_Devices();
				
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Free_Access();
				}
			}
		break;
		case TOG_ALLDOR_DR_N :
		case MOM_ALLDOR_DR_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_FA_Group_Devices();
			
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
				}
			}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_4_IDX].door_grp_number - 1);
			
				if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				Door_AD_To_Normal((gb_input[IP_4_IDX].door_grp_number - 1));
			}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				Group_AccessDenied((gb_input[IP_4_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_AD_N :
		case MOM_GRP_AD_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				Group_AD_To_Normal((gb_input[IP_4_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_AD :
		case MOM_ALLDOR_AD :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_AD_Group_Devices();
			
				if (isValidFireDevice)
				{
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Access_Denied();
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_ALLDOR_AD_N :
		case MOM_ALLDOR_AD_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_DOR_DRT :
		case MOM_DOR_DRT :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_4_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
				{
					iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
					Set_Flags_To_Send_OSDP_OUT();
				}
			}
		break;
		case TOG_DOR_DRT_N :
		case MOM_DOR_DRT_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_4_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
				{
					/*DRT ongoing*/
				}
			}
		break;
		case TOG_GRP_DRT :
		case MOM_GRP_DRT :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_4_IDX].door_grp_number - 17);
			
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						osdp_app.gb_enable_poll = 0;
						iflags.send_drt_osdp_out_cmd = FLAG_SET;
						iflags.command_in_process = FLAG_SET;
						osdp_app.outTransIdx = 0;
					}
				}
			}
		break;
		case TOG_GRP_DRT_N :
		case MOM_GRP_DRT_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
			
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_4_IDX].door_grp_number - 17);
			
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						/* DRT to Normal devices are */
					}
				}
			}
		break;
		case TOG_ALLDOR_DRT :
		case MOM_ALLDOR_DRT :
			if (Is_IP4_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
			
				if (isValidFireDevice)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		break;
		case TOG_ALLDOR_DRT_N :
		case MOM_ALLDOR_DRT_N :
			if (Is_IP4_Control_Flag_Set() == TRUE)
			{
				ip_control.ip4_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					/* DRT to Normal devices are */
				}
			}
		break;
		default:
		break;
	}
	switch (gb_ip5_func)
	{
		case IP_ILOCK :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				Interlock_By_The_Input_Number(IP_5_IDX);
			}
		break;
		case IP_ILOCK_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				Normal_By_The_Input_Number(IP_5_IDX);
			}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR :
		case MOM_DOR_DR :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_5_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
				
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
		break;
		case TOG_DOR_DR_N :
		case MOM_DOR_DR_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_5_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Set_Flags_Put_Into_Normal(0);
				}
			}
		break;
		case TOG_GRP_DR :
		case MOM_GRP_DR :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
			
				Group_DoorRelease((gb_input[IP_5_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_DR_N :
		case MOM_GRP_DR_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				Group_DR_To_Normal((gb_input[IP_5_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_DR :
		case MOM_ALLDOR_DR :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_FA_Group_Devices();
				
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Free_Access();
				}
			}
		break;
		case TOG_ALLDOR_DR_N :
		case MOM_ALLDOR_DR_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_FA_Group_Devices();
			
				if (isValidFireDevice)
				{
					fireFlags.f_executing_command = FLAG_SET;
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
				}
			}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_5_IDX].door_grp_number - 1);
				
				if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
					iLock.tx_length = 1;
					
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				Door_AD_To_Normal((gb_input[IP_5_IDX].door_grp_number - 1));
			}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				Group_AccessDenied((gb_input[IP_5_IDX].door_grp_number - 17));
			}
		break;
		case TOG_GRP_AD_N :
		case MOM_GRP_AD_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				Group_AD_To_Normal((gb_input[IP_5_IDX].door_grp_number - 17));
			}
		break;
		case TOG_ALLDOR_AD :
		case MOM_ALLDOR_AD :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
			
				U8 isValidFireDevice = 0;
			
				fireFlags.is_indInput_detect_f = FLAG_SET;
				isValidFireDevice = Find_All_AD_Group_Devices();
			
				if (isValidFireDevice)
				{
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Access_Denied();
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_ALLDOR_AD_N :
		case MOM_ALLDOR_AD_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				
				fireFlags.is_indInput_detect_f = FLAG_RST;
				isValidFireDevice = Find_All_AD_Group_Devices();
				
				if (isValidFireDevice)
				{
					iflags.recheck_nonIlock_ips_f = FLAG_SET;
					Clear_Whole_Group_Flags();
					Set_Flags_Put_Into_Normal(0);
					osdp_app.free_to_normal_f = FLAG_SET;
					iflags.ad_executing_command = FLAG_SET;
				}
			}
		break;
		case TOG_DOR_DRT :
		case MOM_DOR_DRT :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
			
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_5_IDX].door_grp_number - 1);
			
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
				{
					iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
					Set_Flags_To_Send_OSDP_OUT();
				}
			}
		break;
		case TOG_DOR_DRT_N :
		case MOM_DOR_DRT_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				U8 lcl_dr_idx = 0;
				lcl_dr_idx = (gb_input[IP_5_IDX].door_grp_number - 1);
				
				if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
				{
					/* DRT ongoing */
				}
			}
		break;
		case TOG_GRP_DRT :
		case MOM_GRP_DRT :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_5_IDX].door_grp_number - 17);
				
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						osdp_app.gb_enable_poll = 0;
						iflags.send_drt_osdp_out_cmd = FLAG_SET;
						iflags.command_in_process = FLAG_SET;
						osdp_app.outTransIdx = 0;
					}
				}
			}
		break;
		case TOG_GRP_DRT_N :
		case MOM_GRP_DRT_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				U8 lcl_grp_idx = 0;
				lcl_grp_idx = (gb_input[IP_5_IDX].door_grp_number - 17);
				
				Find_Group_Devices(lcl_grp_idx);
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_DRT();
					if (iLock.tx_length > 0)
					{
						/* DRT to Normal devices are */
					}
				}
			}
		break;
		case TOG_ALLDOR_DRT :
		case MOM_ALLDOR_DRT :
			if (Is_IP5_Control_Flag_Reset() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_SET;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		break;
		case TOG_ALLDOR_DRT_N :
		case MOM_ALLDOR_DRT_N :
			if (Is_IP5_Control_Flag_Set() == TRUE)
			{
				ip_control.ip5_control_f = FLAG_RST;
				
				U8 isValidFireDevice = 0;
				isValidFireDevice = Find_All_DRT_Group_Devices();
				
				if (isValidFireDevice)
				{
					/* DRT to Normal devices are */
				}
			}
		break;
		default:
		break;
	}
	switch (gb_ip6_func)
	{
		case IP_ILOCK :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			Interlock_By_The_Input_Number(IP_6_IDX);
		}
		break;
		case IP_ILOCK_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			Normal_By_The_Input_Number(IP_6_IDX);
		}
		break;
		
		/********************** If toggle configured ****************************/
		case TOG_DOR_DR :
		case MOM_DOR_DR :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 lcl_dr_idx = 0;
			lcl_dr_idx = (gb_input[IP_6_IDX].door_grp_number - 1);
			
			if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
			{
				iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
				iLock.tx_length = 1;
				
				Set_Flags_Put_SD_Into_Free_Access();
			}
		}
		break;
		case TOG_DOR_DR_N :
		case MOM_DOR_DR_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 lcl_dr_idx = 0;
			lcl_dr_idx = (gb_input[IP_6_IDX].door_grp_number - 1);
			
			if (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
			{
				iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
				iLock.tx_length = 1;
				
				iflags.recheck_nonIlock_ips_f = FLAG_SET;
				fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
				Set_Flags_Put_Into_Normal(0);
			}
		}
		break;
		case TOG_GRP_DR :
		case MOM_GRP_DR :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			Group_DoorRelease((gb_input[IP_6_IDX].door_grp_number - 17));
		}
		break;
		case TOG_GRP_DR_N :
		case MOM_GRP_DR_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			Group_DR_To_Normal((gb_input[IP_6_IDX].door_grp_number - 17));
		}
		break;
		case TOG_ALLDOR_DR :
		case MOM_ALLDOR_DR :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 isValidFireDevice = 0;
			
			fireFlags.is_indInput_detect_f = FLAG_SET;
			isValidFireDevice = Find_All_FA_Group_Devices();
			
			if (isValidFireDevice)
			{
				fireFlags.f_executing_command = FLAG_SET;
				Clear_Whole_Group_Flags();
				Set_Flags_Put_Into_Free_Access();
			}
		}
		break;
		case TOG_ALLDOR_DR_N :
		case MOM_ALLDOR_DR_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 isValidFireDevice = 0;
			
			fireFlags.is_indInput_detect_f = FLAG_RST;
			isValidFireDevice = Find_All_FA_Group_Devices();
			
			if (isValidFireDevice)
			{
				fireFlags.f_executing_command = FLAG_SET;
				iflags.recheck_nonIlock_ips_f = FLAG_SET;
				fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
				Clear_Whole_Group_Flags();
				Set_Flags_Put_Into_Normal(0);
				osdp_app.free_to_normal_f = FLAG_SET;
			}
		}
		break;
		/*************************** If momentary configured ******************/
		case TOG_DOR_AD : 
		case MOM_DOR_AD :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 lcl_dr_idx = 0;
			lcl_dr_idx = (gb_input[IP_6_IDX].door_grp_number - 1);
			
			if ((slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_EMERGENCY)
			&& (slv_data[lcl_dr_idx].oprtState != SLV_OP_STATE_FREE_ACCESS)
			)
			{
				iLock.tx_address[0] = inSysDeviceList[lcl_dr_idx].slv_addr;
				iLock.tx_length = 1;
				
				Set_Flags_Put_SD_Into_Access_Denied();
			}
		}
		break;
		case TOG_DOR_AD_N : 
		case MOM_DOR_AD_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			Door_AD_To_Normal((gb_input[IP_6_IDX].door_grp_number - 1));
		}
		break;
		case TOG_GRP_AD : 
		case MOM_GRP_AD :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			Group_AccessDenied((gb_input[IP_6_IDX].door_grp_number - 17));
		}
		break;
		case TOG_GRP_AD_N :
		case MOM_GRP_AD_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			Group_AD_To_Normal((gb_input[IP_6_IDX].door_grp_number - 17));
		}
		break;
		case TOG_ALLDOR_AD :
		case MOM_ALLDOR_AD :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 isValidFireDevice = 0;
			
			fireFlags.is_indInput_detect_f = FLAG_SET;
			isValidFireDevice = Find_All_AD_Group_Devices();
			
			if (isValidFireDevice)
			{
				Clear_Whole_Group_Flags();
				Set_Flags_Put_Into_Access_Denied();
				iflags.ad_executing_command = FLAG_SET;
			}
		}
		break;
		case TOG_ALLDOR_AD_N :
		case MOM_ALLDOR_AD_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 isValidFireDevice = 0;
			
			fireFlags.is_indInput_detect_f = FLAG_RST;
			isValidFireDevice = Find_All_AD_Group_Devices();
			
			if (isValidFireDevice)
			{
				iflags.recheck_nonIlock_ips_f = FLAG_SET;
				Clear_Whole_Group_Flags();
				Set_Flags_Put_Into_Normal(0);
				osdp_app.free_to_normal_f = FLAG_SET;
				iflags.ad_executing_command = FLAG_SET;
			}
		}
		break;
		case TOG_DOR_DRT :
		case MOM_DOR_DRT :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 lcl_dr_idx = 0;
			lcl_dr_idx = (gb_input[IP_6_IDX].door_grp_number - 1);
			
			if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_NORMAL)
			{
				iLock.actionSlave = inSysDeviceList[lcl_dr_idx].slv_addr;
				Set_Flags_To_Send_OSDP_OUT();
			}
		}
		break;
		case TOG_DOR_DRT_N :
		case MOM_DOR_DRT_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 lcl_dr_idx = 0;
			lcl_dr_idx = (gb_input[IP_6_IDX].door_grp_number - 1);
			
			if (slv_data[lcl_dr_idx].oprtState == SLV_OP_STATE_DOOR_ACTIVE)
			{
				/* DRT ongoing */
			}
		}
		break;
		case TOG_GRP_DRT :
		case MOM_GRP_DRT :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 lcl_grp_idx = 0;
			lcl_grp_idx = (gb_input[IP_6_IDX].door_grp_number - 17);
			
			Find_Group_Devices(lcl_grp_idx);
			if (groupInfo.length > 0)
			{
				Sort_Group_devices_For_DRT();
				if (iLock.tx_length > 0)
				{
					osdp_app.gb_enable_poll = 0;
					iflags.send_drt_osdp_out_cmd = FLAG_SET;
					iflags.command_in_process = FLAG_SET;
					osdp_app.outTransIdx = 0;
				}
			}
		}
		break;
		case TOG_GRP_DRT_N :
		case MOM_GRP_DRT_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 lcl_grp_idx = 0;
			lcl_grp_idx = (gb_input[IP_6_IDX].door_grp_number - 17);
			
			Find_Group_Devices(lcl_grp_idx);
			if (groupInfo.length > 0)
			{
				Sort_Group_devices_For_DRT();
				if (iLock.tx_length > 0)
				{
					/* DRT to Normal devices are */
				}
			}
		}
		break;
		case TOG_ALLDOR_DRT :
		case MOM_ALLDOR_DRT :
		if (Is_IP6_Control_Flag_Reset() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_SET;
			
			U8 isValidFireDevice = 0;
			isValidFireDevice = Find_All_DRT_Group_Devices();
			
			if (isValidFireDevice)
			{
				osdp_app.gb_enable_poll = 0;
				iflags.send_drt_osdp_out_cmd = FLAG_SET;
				iflags.command_in_process = FLAG_SET;
				osdp_app.outTransIdx = 0;
			}
		}
		break;
		case TOG_ALLDOR_DRT_N :
		case MOM_ALLDOR_DRT_N :
		if (Is_IP6_Control_Flag_Set() == TRUE)
		{
			ip_control.ip6_control_f = FLAG_RST;
			
			U8 isValidFireDevice = 0;
			isValidFireDevice = Find_All_DRT_Group_Devices();
			
			if (isValidFireDevice)
			{
				/* DRT to Normal devices are */
			}
		}
		break;
		default:
		break;
	}
}

/*****************************************************************************
* Function name : void Interlock_By_The_Input_Number(U8 inpIdx)
* Returns       : Nothing
* Arguments     : U8 inpIdx - Index of the input to be checked for interlock
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if a device is in an interlock sequence
*                 based on the input index provided. If the device is found in
*                 an interlock sequence, it sets the interlock by input flag
*                 and triggers the process to set flags for interlock.
* Notes         : The function assumes that global structures and flags like
*                 iflags and related functions are defined and properly initialized.
* Global Variables Affected : iflags.ilock_by_input_f.
*****************************************************************************/
void Interlock_By_The_Input_Number(U8 inpIdx)
{
	U8 isDevInIlck = 0;
	isDevInIlck = Find_iLockSequence_With_Input(inpIdx);  // Check if the device is in an interlock sequence based on the input index
	
	if (isDevInIlck == FLAG_SET)
	{
		iflags.ilock_by_input_f = FLAG_SET;  // Set the interlock by input flag
		Set_Flags_Put_Into_Interlock(0);  // Trigger the process to set flags for interlock
	}
}

/*****************************************************************************
* Function name : void Normal_By_The_Input_Number(U8 inpIdx)
* Returns       : Nothing
* Arguments     : U8 inpIdx - Index of the input to be checked for normal state
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if a device is in an interlock sequence
*                 based on the input index provided. If the device is found in
*                 an interlock sequence, it resets the interlock state and
*                 triggers the process to set the device to normal state.
* Notes         : The function assumes that global structures and flags like
*                 iLock and related functions are defined and properly initialized.
* Global Variables Affected : iLock.tx_length
*****************************************************************************/
void Normal_By_The_Input_Number(U8 inpIdx)
{
	U8 isDevInIlck = 0;
	isDevInIlck = Find_iLockSequence_With_Input(inpIdx);  // Check if the device is in an interlock sequence based on the input index
	
	if (isDevInIlck == FLAG_SET)
	{
		Reset_From_Interlock(0);  // Reset the interlock state
		if (iLock.tx_length > 0)
		{
			Set_Flags_Put_Into_Normal(0);  // Trigger the process to set the device to normal state
		}
	}
}


/*****************************************************************************
* Function name : U8 No_Priority_Flags_Set(void)
* Returns       : U8 - Returns 1 if no priority commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if any priority commands are executing by
*                 checking various flags. If none of the priority flags are set,
*                 it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 iflags, emgGroup, fireFlags, and related flags are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 No_Priority_Flags_Set(void)
{
	// Check if no priority commands are executing by checking various flags
	if ((iflags.command_in_process == FLAG_RST)
	&& (emgGroup.e_executing_command == FLAG_RST)
	&& (fireFlags.f_executing_command == FLAG_RST)
	&& (iflags.ad_executing_command == FLAG_RST))
	{
		return 1;	// Return 1, if no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP1_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP1 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP1 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP1_Control_Flag_Reset(void)
{
	if ((ip_control.ip1_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP1_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP1 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP1 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP1_Control_Flag_Set(void)
{
	if ((ip_control.ip1_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP2_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP2 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP2 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP2_Control_Flag_Reset(void)
{
	if ((ip_control.ip2_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP2_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP2 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP2 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP2_Control_Flag_Set(void)
{
	if ((ip_control.ip2_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP3_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP3 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP3 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP3_Control_Flag_Reset(void)
{
	if ((ip_control.ip3_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP3_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP3 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP3 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP3_Control_Flag_Set(void)
{
	if ((ip_control.ip3_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP4_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP4 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP4 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP4_Control_Flag_Reset(void)
{
	if ((ip_control.ip4_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP4_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP4 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP4 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP4_Control_Flag_Set(void)
{
	if ((ip_control.ip4_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP5_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP5 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP5 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP5_Control_Flag_Reset(void)
{
	if ((ip_control.ip5_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP5_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP5 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP5 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP5_Control_Flag_Set(void)
{
	if ((ip_control.ip5_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP6_Control_Flag_Reset(void)
* Returns       : U8 - Returns 1 if IP6 control flag is reset and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP6 control flag is reset and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP6_Control_Flag_Reset(void)
{
	if ((ip_control.ip6_control_f == FLAG_RST) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '0' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '1' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name : U8 Is_IP6_Control_Flag_Set(void)
* Returns       : U8 - Returns 1 if IP6 control flag is set and no priority
*                      commands are executing, else 0
* Arguments     : None
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function checks if the IP6 control flag is set and
*                 verifies that no priority commands are executing. If both
*                 conditions are met, it returns 1, otherwise, it returns 0.
* Notes         : The function assumes that global structures and flags like
*                 ip_control, and the function No_Priority_Flags_Set are defined
*                 and properly initialized.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_IP6_Control_Flag_Set(void)
{
	if ((ip_control.ip6_control_f == FLAG_SET) && (No_Priority_Flags_Set() == TRUE))
	{
		return 1;	// Return 1, if control flag is '1' & no priority commands are executing.
	}
	else
	{
		return 0;	// Return 0, if control flag is '0' & any one of the priority commands is executing.
	}
}

/*****************************************************************************
* Function name	: void Reset_Input_ILock_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset input control flags.
*               :
* Notes			: NA.
* Global Variables Affected : ip_control.ip1_control_f, ip_control.ip2_control_f
							  ip_control.ip3_control_f, ip_control.ip4_control_f
							  ip_control.ip5_control_f, ip_control.ip6_control_f
*****************************************************************************/
void Reset_Input_ILock_Flags(void)
{
	if ((gb_input[IP_1_IDX].ip_en == FLAG_SET) && (gb_input[IP_1_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip1_func == IP_ILOCK)
		ip_control.ip1_control_f = 0;
	}
	
	if ((gb_input[IP_2_IDX].ip_en == FLAG_SET) && (gb_input[IP_2_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip2_func == IP_ILOCK)
		ip_control.ip2_control_f = 0;
	}
	
	if ((gb_input[IP_3_IDX].ip_en == FLAG_SET) && (gb_input[IP_3_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip3_func == IP_ILOCK)
		ip_control.ip3_control_f = 0;
	}
	
	if ((gb_input[IP_4_IDX].ip_en == FLAG_SET) && (gb_input[IP_4_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip4_func == IP_ILOCK)
		ip_control.ip4_control_f = 0;
	}
	
	if ((gb_input[IP_5_IDX].ip_en == FLAG_SET) && (gb_input[IP_5_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip5_func == IP_ILOCK)
		ip_control.ip5_control_f = 0;
	}
	
	if ((gb_input[IP_6_IDX].ip_en == FLAG_SET) && (gb_input[IP_6_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if (gb_ip6_func == IP_ILOCK)
		ip_control.ip6_control_f = 0;
	}
}

/*****************************************************************************
* Function name	: void Reset_Input_NonILock_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset input control flags.
*               :
* Notes			: NA.
* Global Variables Affected : ip_control.ip1_control_f, ip_control.ip2_control_f
							  ip_control.ip3_control_f, ip_control.ip4_control_f
							  ip_control.ip5_control_f, ip_control.ip6_control_f
*****************************************************************************/
void Reset_Input_NonILock_Flags(void)
{
	if ((gb_input[IP_1_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_1_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip1_func == TOG_DOR_AD) || (gb_ip1_func == TOG_GRP_AD) || (gb_ip1_func == TOG_ALLDOR_AD)
		|| (gb_ip1_func == TOG_DOR_DR) || (gb_ip1_func == TOG_GRP_DR) || (gb_ip1_func == TOG_ALLDOR_DR)
		|| (gb_ip1_func == TOG_DOR_DRT) || (gb_ip1_func == TOG_GRP_DRT) || (gb_ip1_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip1_control_f = 0;
		}
	}
	
	if ((gb_input[IP_2_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_2_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip2_func == TOG_DOR_AD) || (gb_ip2_func == TOG_GRP_AD) || (gb_ip2_func == TOG_ALLDOR_AD)
		|| (gb_ip2_func == TOG_DOR_DR) || (gb_ip2_func == TOG_GRP_DR) || (gb_ip2_func == TOG_ALLDOR_DR)
		|| (gb_ip2_func == TOG_DOR_DRT) || (gb_ip2_func == TOG_GRP_DRT) || (gb_ip2_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip2_control_f = 0;
		}
	}
	
	if ((gb_input[IP_3_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_3_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip3_func == TOG_DOR_AD) || (gb_ip3_func == TOG_GRP_AD) || (gb_ip3_func == TOG_ALLDOR_AD)
		|| (gb_ip3_func == TOG_DOR_DR) || (gb_ip3_func == TOG_GRP_DR) || (gb_ip3_func == TOG_ALLDOR_DR)
		|| (gb_ip3_func == TOG_DOR_DRT) || (gb_ip3_func == TOG_GRP_DRT) || (gb_ip3_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip3_control_f = 0;
		}
	}
	
	if ((gb_input[IP_4_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_4_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip4_func == TOG_DOR_AD) || (gb_ip4_func == TOG_GRP_AD) || (gb_ip4_func == TOG_ALLDOR_AD)
		|| (gb_ip4_func == TOG_DOR_DR) || (gb_ip4_func == TOG_GRP_DR) || (gb_ip4_func == TOG_ALLDOR_DR)
		|| (gb_ip4_func == TOG_DOR_DRT) || (gb_ip4_func == TOG_GRP_DRT) || (gb_ip4_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip4_control_f = 0;
		}
	}
	
	if ((gb_input[IP_5_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_5_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip5_func == TOG_DOR_AD) || (gb_ip5_func == TOG_GRP_AD) || (gb_ip5_func == TOG_ALLDOR_AD)
		|| (gb_ip5_func == TOG_DOR_DR) || (gb_ip5_func == TOG_GRP_DR) || (gb_ip5_func == TOG_ALLDOR_DR)
		|| (gb_ip5_func == TOG_DOR_DRT) || (gb_ip5_func == TOG_GRP_DRT) || (gb_ip5_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip5_control_f = 0;
		}
	}
	
	if ((gb_input[IP_6_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_6_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((gb_ip6_func == TOG_DOR_AD) || (gb_ip6_func == TOG_GRP_AD) || (gb_ip6_func == TOG_ALLDOR_AD)
		|| (gb_ip6_func == TOG_DOR_DR) || (gb_ip6_func == TOG_GRP_DR) || (gb_ip6_func == TOG_ALLDOR_DR)
		|| (gb_ip6_func == TOG_DOR_DRT) || (gb_ip6_func == TOG_GRP_DRT) || (gb_ip6_func == TOG_ALLDOR_DRT))
		{
			ip_control.ip6_control_f = 0;
		}
	}
}

/*****************************************************************************
* Function name	: void Reset_DoorIP_Control_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset input control flags.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Reset_DoorIP_Control_Flags(void)
{
	for (U8 rstFlg = 0; rstFlg < TOTAL_SLAVES; rstFlg++)
	{
		iDeviceFlag[rstFlg].drLock_control_f = FLAG_RST;
	}
}

/*****************************************************************************
* Function name	: void Get_Group_Operation_State(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get operation state of the group.
*               :
* Notes			: NA.
* Global Variables Affected : grpData[grpIdx].operationState.
*****************************************************************************/
void Get_Group_Operation_State(void)
{
	for (U8 grpIdx = 0; grpIdx < MAX_GROUPS; grpIdx++)
	{
		Find_Group_Devices(grpIdx);
		if (groupInfo.length > 0)
		{
			grpData[grpIdx].operationState = Determine_Group_Operation_state();
		}
	}
}

/*****************************************************************************
* Function name	: U8 Determine_Group_Operation_state(void)
* Returns		: U8 ---> returns 1 for group state is valid, 0 for invalid.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to determine the group operational state.
*				  Also, the function sorts the group state priority wise.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Determine_Group_Operation_state(void)
{
	enum {op_invalid, op_valid};
	bool lcl_oprt_val = 0;
	
	for (U8 dvIdx = 0; dvIdx < groupInfo.length; dvIdx++)
	{
		U8 lcl_dvIdx = 0;
		lcl_dvIdx = Find_Device_Index_InSystem(groupInfo.slaves[dvIdx]);
		if (slv_data[lcl_dvIdx].oprtState == SLV_OP_STATE_EMERGENCY)
		{
			lcl_oprt_val = op_valid;
		}
		else
		{
			lcl_oprt_val = op_invalid;
			break;
		}
	}
	if (lcl_oprt_val == op_valid)
	{
		return GRP_IN_AN_EMERGENCY;
	}
	
	for (U8 dvIdx = 0; dvIdx < groupInfo.length; dvIdx++)
	{
		U8 lcl_dvIdx = 0;
		lcl_dvIdx = Find_Device_Index_InSystem(groupInfo.slaves[dvIdx]);
		if (slv_data[lcl_dvIdx].oprtState == SLV_OP_STATE_FREE_ACCESS)
		{
			lcl_oprt_val = op_valid;
		}
		else
		{
			lcl_oprt_val = op_invalid;
			break;
		}
	}
	if (lcl_oprt_val == op_valid)
	{
		return GRP_IN_FREE_ACCESS;
	}
	
	for (U8 dvIdx = 0; dvIdx < groupInfo.length; dvIdx++)
	{
		U8 lcl_dvIdx = 0;
		lcl_dvIdx = Find_Device_Index_InSystem(groupInfo.slaves[dvIdx]);
		if (slv_data[lcl_dvIdx].oprtState == SLV_OP_STATE_ACCESS_DENIED)
		{
			lcl_oprt_val = op_valid;
		}
		else
		{
			lcl_oprt_val = op_invalid;
			break;
		}
	}
	if (lcl_oprt_val == op_valid)
	{
		return GRP_IN_ACCESS_DENIED;
	}
	
	for (U8 dvIdx = 0; dvIdx < groupInfo.length; dvIdx++)
	{
		U8 lcl_dvIdx = 0;
		lcl_dvIdx = Find_Device_Index_InSystem(groupInfo.slaves[dvIdx]);
		if (slv_data[lcl_dvIdx].oprtState == SLV_OP_STATE_INTERLOCK)
		{
			lcl_oprt_val = op_valid;
			break;
		}
		else
		{
			lcl_oprt_val = op_invalid;
		}
	}
	if (lcl_oprt_val == op_valid)
	{
		return GRP_IN_AN_INTERLOCK;
	}
	
	for (U8 dvIdx = 0; dvIdx < groupInfo.length; dvIdx++)
	{
		U8 lcl_dvIdx = 0;
		lcl_dvIdx = Find_Device_Index_InSystem(groupInfo.slaves[dvIdx]);
		if (slv_data[lcl_dvIdx].oprtState == SLV_OP_STATE_NORMAL)
		{
			lcl_oprt_val = op_valid;
		}
		else
		{
			lcl_oprt_val = op_invalid;
			break;
		}
	}
	if (lcl_oprt_val == op_valid)
	{
		return GRP_IN_NORMAL;
	}
	
	return GRP_STATE_CANT_DEFINE;
}

/*****************************************************************************
* Function name	: void Set_Flags_To_Check_IP_Status(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to read input status
*				  of the devices.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.chk_ip_status, iflags.chk_ip_sts_ack
							  osdp_app.gb_enable_poll, osdp_app.istTransIdx
							  iflags.command_in_process.
*****************************************************************************/
void Set_Flags_To_Check_IP_Status(void)
{
	osdp_app.gb_enable_poll = 0;
	iflags.chk_ip_status = FLAG_SET;
	iflags.chk_ip_sts_ack = FLAG_SET;
	osdp_app.istTransIdx = 0;
	iflags.command_in_process = FLAG_SET;
}

/*****************************************************************************
* Function name	: void Set_Flags_To_Check_OPRT_Status(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to read the slave
*				  operational state.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.chk_oprt_status, iflags.chk_opt_sts_ack
							  osdp_app.gb_enable_poll, osdp_app.oprTransIdx
							  iflags.command_in_process.
*****************************************************************************/
void Set_Flags_To_Check_OPRT_Status(void)
{
	osdp_app.gb_enable_poll = 0;
	iflags.chk_oprt_status = FLAG_SET;
	iflags.chk_opt_sts_ack = FLAG_SET;
	osdp_app.oprTransIdx = 0;
	iflags.command_in_process = FLAG_SET;
}

/*****************************************************************************
* Function name	: void Set_Flags_To_Send_OSDP_OUT(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to send an OSDP out
*				  command.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.send_osdp_out_cmd.
							  osdp_app.gb_enable_pol.
							  iflags.command_in_process.
*****************************************************************************/
void Set_Flags_To_Send_OSDP_OUT(void)
{
	osdp_app.gb_enable_poll = 0;
	iflags.send_osdp_out_cmd = FLAG_SET;
	iflags.command_in_process = FLAG_SET;
}

/*****************************************************************************
* Function name	: void Monitor_Flags_After_Normal(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set required flags to check input flags
*				  and other flags to take an appropriate action after the devices
*				  enter to normal state.
*               :
* Notes			: NA.
* Global Variables Affected : iflags.chk_flags_after_normal, iflags.reset_from_iLock_f
							  iflags.chk_ip_after_normal, iLock.tx_address
							  iLock.tx_length.
*****************************************************************************/
void Monitor_Flags_After_Normal(void)
{
	if ((iflags.chk_flags_after_normal == FLAG_SET) && (iflags.command_in_process == FLAG_RST))
	{
		iflags.chk_flags_after_normal = FLAG_RST;
		
		if (iflags.reset_from_iLock_f == FLAG_SET)
		{
			iflags.reset_from_iLock_f = FLAG_RST;
			
			if (iflags.chk_noRsp_f_aft_freeRst == FLAG_SET)
			{
				iflags.chk_noRsp_f_aft_freeRst = FLAG_RST;
				
				iflags.rst_noRsp_control_f = FLAG_SET;
			}
		}
		else
		{
			iflags.chk_ip_after_normal = FLAG_SET;
			memcpy(iLock.tx_address, PollDevice, polling_dev_len);
			iLock.tx_length = polling_dev_len;
			
			Set_Flags_To_Check_IP_Status();
		}
	}
}

/*****************************************************************************
* Function name	: void Get_mbHolding_Reg_Data(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get data from the modbus registers and
*				  take an action accordingly.
*               :
* Notes			: NA.
* Global Variables Affected : .
*****************************************************************************/
void Get_mbHolding_Reg_Data(void)
{
	/*Check if the received Modbus function code is for writing to a single register*/
	if (mbRecieve.funCode == PRESET_SINGLE_REG)
	{
		if ((mbRecieve.regAddress >= minHoldRegAdd) && (mbRecieve.regAddress <= maxHoldRegAdd))
		{
			int mbValue = 0;	// Variable to store the received register value
			U8 devIndex = 0;	// Variable to store the device index
			static int tempRegVal[TOTAL_SLAVES] = {0};	// Static array to store temporary register values
			
			// Calculate the device index based on the register address
			devIndex = (mbRecieve.regAddress - minHoldRegAdd);
			
			// Combine the two bytes of the register data into an integer value
			mbValue = mbRecieve.regDataBuf[0];
			mbValue *= 256;
			mbValue += mbRecieve.regDataBuf[1];
			
			/* Check if the received register value is different from the previous value*/
			if (tempRegVal[devIndex] != mbValue)
			{
				// Update the temporary register value with the new value
				tempRegVal[devIndex] = mbValue;
				
				// Determine the action based on the received register value
				if (mbValue == 0x00)
				{
					// Set the normal state flag for the corresponding device
					mbGetFlags[devIndex].normal_f = FLAG_SET;
				}
				else if ((mbValue == 0x01) || (mbValue == 0x02))
				{
					// Set the free access flag for the corresponding device
					mbGetFlags[devIndex].free_acs_f = FLAG_SET;
				}
				else if (mbValue == 0x03)
				{
					// Set the access denied flag for the corresponding device
					mbGetFlags[devIndex].acs_dnd_f = FLAG_SET;
				}
				else if (mbValue == 0x04)
				{
					// Set the emergency flag for the corresponding device
					mbGetFlags[devIndex].emg_f = FLAG_SET;
				}
				else if (mbValue == 0x05)
				{
					// Set the privacy flag for the corresponding device
					mbGetFlags[devIndex].prvc_f = FLAG_SET;
				}
			}
		}
	}
	
	// Iterate through all devices in the system
	for (U8 devIdx = 0; devIdx < inSystem_dev_len; devIdx++)
	{
		// Check if no command is currently in process and no emergency, fire, access denied, or alarm command is executing
		if ((iflags.command_in_process == FLAG_RST)
		&& (emgGroup.e_executing_command == FLAG_RST)
		&& (fireFlags.f_executing_command == FLAG_RST)
		&& (iflags.ad_executing_command == FLAG_RST)
		)
		{
			// Check if the emergency flag is set for the current device
			if (mbGetFlags[devIdx].emg_f == FLAG_SET)
			{
				// Reset the emergency flag
				mbGetFlags[devIdx].emg_f = FLAG_RST;
				
				// Configure emergency command parameters
				emgGroup.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
				emgGroup.tx_length = 1;
				
				// Set flags to initiate emergency command execution
				emgGroup.send_emg_command_f = FLAG_SET;
				osdp_app.gb_enable_poll = 0;
				emgGroup.chk_sd_emgAck_flag = FLAG_SET;
				iflags.command_in_process = FLAG_SET;
				osdp_app.emgTransIdx = 0;
			}
			// Check if the free access flag is set for the current device
			else if (mbGetFlags[devIdx].free_acs_f == FLAG_SET)
			{
				// Reset the free access flag
				mbGetFlags[devIdx].free_acs_f = FLAG_RST;
				
				// Check if the device is not in emergency state
				if (slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
				{
					// Configure parameters for putting the device into free access mode
					iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
					iLock.tx_length = 1;
					Set_Flags_Put_SD_Into_Free_Access();
				}
			}
			// Check if the access denied flag is set for the current device
			else if (mbGetFlags[devIdx].acs_dnd_f == FLAG_SET)
			{
				// Reset the access denied flag
				mbGetFlags[devIdx].acs_dnd_f = FLAG_RST;
				
				// Check if the device is not in emergency or free access state
				if ((slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[devIdx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					// Configure parameters for putting the device into access denied mode
					iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
					iLock.tx_length = 1;
					Set_Flags_Put_SD_Into_Access_Denied();
				}
			}
			// Check if the privacy flag is set for the current device
			else if (mbGetFlags[devIdx].prvc_f == FLAG_SET)
			{
				// Reset the privacy flag
				mbGetFlags[devIdx].prvc_f = FLAG_RST;
				
				// Check if the device is not in emergency or free access state
				if ((slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
				&& (slv_data[devIdx].oprtState != SLV_OP_STATE_FREE_ACCESS)
				)
				{
					// Configure parameters for putting the device into privacy mode
					iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
					iLock.tx_length = 1;
					Set_Flags_Put_Into_Privacy();
				}
			}
			// Check if the normal flag is set for the current device
			else if (mbGetFlags[devIdx].normal_f == FLAG_SET)
			{
				// Reset the normal flag
				mbGetFlags[devIdx].normal_f = FLAG_RST;
				
				// Configure parameters for putting the device into normal mode
				iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
				iLock.tx_length = 1;
				Set_Flags_Put_Into_Normal(0);
			}
		}
	}

	// Check if the flag to preset multiple registers is set
	if (mbTransmit.preset_mReg_f == FLAG_SET)
	{
		// Reset the flag after processing
		mbTransmit.preset_mReg_f = FLAG_RST;
		
		// Arrays to store data
		static U8 mbDrData[TOTAL_SLAVES] = {0}; // Array to store Modbus data
		static U8 preDrData[TOTAL_SLAVES] = {0}; // Array to store previous Modbus data
		U8 mbDrLength = 0; // Length of Modbus data
		
		// Extract Modbus data from the application holding buffer
		for (U8 mbIdx = 0; mbIdx < (inSystem_dev_len * 2); (mbIdx += 2))
		{
			// Combine two bytes into a single value and store in mbDrData array
			mbDrData[mbDrLength] = mbTransmit.appHoldingBuf[mbIdx];
			mbDrData[mbDrLength] *= 256;
			mbDrData[mbDrLength] |= mbTransmit.appHoldingBuf[(mbIdx + 1)];
			
			// Increment the Modbus data length
			mbDrLength ++;
		}
		
		// Compare current Modbus data with previous Modbus data for each device
		for (U8 mbIdx = 0; mbIdx < inSystem_dev_len; mbIdx ++)
		{
			// If the current Modbus data is different from the previous Modbus data
			if (preDrData[mbIdx] != mbDrData[mbIdx])
			{
				// Update previous Modbus data
				preDrData[mbIdx] = mbDrData[mbIdx];
				
				// Determine the action based on the Modbus data value
				if (mbDrData[mbIdx] == 0x00)
				{
					// Set normal state flag for the corresponding device
					mbGetFlags[mbIdx].normal_f = FLAG_SET;
				}
				else if ((mbDrData[mbIdx] == 0x01) || (mbDrData[mbIdx] == 0x02))
				{
					// Set free access flag for the corresponding device
					mbGetFlags[mbIdx].free_acs_f = FLAG_SET;
				}
				else if (mbDrData[mbIdx] == 0x03)
				{
					// Set access denied flag for the corresponding device
					mbGetFlags[mbIdx].acs_dnd_f = FLAG_SET;
				}
				else if (mbDrData[mbIdx] == 0x04)
				{
					// Set emergency flag for the corresponding device
					mbGetFlags[mbIdx].emg_f = FLAG_SET;
				}
				else if (mbDrData[mbIdx] == 0x05)
				{
					// Set privacy flag for the corresponding device
					mbGetFlags[mbIdx].prvc_f = FLAG_SET;
				}
			}
		}
	}
}

/*****************************************************************************
* Function name	: void Get_Door_Access(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to activate door release signal if the
*				  perticular door is in normal state, and no other command flag
*				  is executing.
*               :
* Notes			: NA.
* Global Variables Affected : iDeviceFlag[devIdx].door_req_flag, grpData[grpIdx].command_executing_f
*****************************************************************************/
void Get_Door_Access(U8 slvAddress)
{
	// Find the index of the device in the system based on its slave address
	U8 devIdx = Find_Device_Index_InSystem(slvAddress);

	// Check if the device is in normal operating state and no command is currently in process
	if ((slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL)
	&& (iflags.command_in_process == FLAG_RST))
	{
		// Set flags to indicate door access request execution
		iDeviceFlag[devIdx].executing_dr_request_f = FLAG_SET;
		iLock.actionSlave = slvAddress;
		Find_Main_iLock_Sequence_With(iLock.actionSlave);
		Find_iLock_Sequence();
		iDeviceFlag[devIdx].door_req_flag = FLAG_SET;

		// Set flags to check IP status
		Set_Flags_To_Check_IP_Status();

		// Find the group index of the device and set flags to indicate command execution
		U8 grpIdx = Find_Device_Group(iLock.actionSlave);
		grpData[grpIdx].command_executing_f = FLAG_SET;
		
		/***** delete *****/
		iflags.temp_f = 1;
	}
}

/*****************************************************************************
* Function name	: void Handle_Slave_Dev_Response_PowerUp(U8 devIdx)
* Returns		: Nothing.
* Arguments    	: U8 devIDX ---> Pass Index of the device.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 04/07/2024.
* Description	: Function is written to Handle the operation state of the slave
*				  when slave device power resets.
*               :
* Notes			: NA.
* Global Variables Affected : gb_osdp_emg_det_f, gb_osdp.rec_dev_address
*****************************************************************************/
void Handle_Slave_Dev_Response_PowerUp(U8 devIdx)
{
    // Find the group index for the device's slave address
    U8 grpIdx = Find_Device_Group(inSysDeviceList[devIdx].slv_addr);
    
    // Check if the device is in emergency operation state
    if (slv_data[devIdx].oprtState == SLV_OP_STATE_EMERGENCY)
    {
        // Set the global emergency detection flag and record the device address
        gb_osdp_emg_det_f = FLAG_SET;
        gb_osdp.rec_dev_address = inSysDeviceList[devIdx].slv_addr;
        Check_Emergency_Signal(); // Check for emergency signal
    }
    else if ((slv_data[devIdx].oprtState == SLV_OP_STATE_FREE_ACCESS)
    /*&& (slv_data[devIdx].ipTypeState == FALSE)*/
    /*&& (inSysDeviceList[devIdx].slv_addr != 6)*/)
    {
        // Check if free access is activated by the device
        if (fireFlags.free_acs_active_by[devIdx] == FLAG_SET)
        {
            // Check if the device's IP type state is reset
            if (slv_data[devIdx].ipTypeState == FLAG_RST)
            {
                /* The global free access was activated by the device, but its power was off.
                   Now, its power is on, so reset the group's state to normal. */
				
				Put_Normal_From_FreeAccess(inSysDeviceList[devIdx].slv_addr);
            }
            else
            {
                __NOP(); // No operation
                // gb_osdp_free_acs_f = FLAG_SET;
                // gb_osdp.rec_dev_address = inSysDeviceList[devIdx].slv_addr;
            }
        }
		else if ((fireGroup[grpIdx].grp_in_free_access == FLAG_RST)
		&& (fireGroup[grpIdx].grp_in_acs_denied == FLAG_RST)
		&& (emgGroupArray[grpIdx].group_in_emergency == FLAG_RST))
		{
			/* If group is not in free access after power reset. */
			// Reset the device from interlock state
			Reset_From_Interlock(inSysDeviceList[devIdx].slv_addr);
			if (iLock.tx_length > 0)
			{
				Set_Flags_Put_Into_Normal(inSysDeviceList[devIdx].slv_addr);
			}
			
		}
		else if ((fireGroup[grpIdx].grp_in_acs_denied == FLAG_SET)
		|| (emgGroupArray[grpIdx].group_in_emergency == FLAG_SET))
		{
			/*	After a slave power reset, a group can be access denied state
				or in an emergency state.
				So to execute this, goto statement is using below.
				 */
			if (fireGroup[grpIdx].grp_in_acs_denied == FLAG_SET)
			{
				slv_data[devIdx].oprtState = SLV_OP_STATE_NORMAL;
				slv_data[devIdx].lockState = LOCKED;
			}
			goto ERR_SLV_PWR_UP;
		}
        else
        {
            // Set flags for free access
            iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
            iLock.tx_length = 1;
            Set_Flags_Put_Into_Free_Access();
        }
    }
    else if (slv_data[devIdx].oprtState == SLV_OP_STATE_ACCESS_DENIED)
    {
        Take_An_Action_On_Group_AccessD_State(devIdx, grpIdx);
    }
	else if (slv_data[devIdx].oprtState == SLV_OP_STATE_PRIVACY)
	{
		iflags.slv_in_prvc_N_pwr_UP = FLAG_SET;
		// Reset the device from interlock state
		Reset_From_Interlock(inSysDeviceList[devIdx].slv_addr);
		Set_Reset_Privacy_State(inSysDeviceList[devIdx].slv_addr); // Set/reset privacy state for the slave
	}
    else
    {
		ERR_SLV_PWR_UP :
        // Check if the group's operation state is undefined
        if (grpData[grpIdx].operationState == GRP_STATE_CANT_DEFINE)
        {
            // Check if the group is in emergency state
            if (emgGroupArray[grpIdx].group_in_emergency == FLAG_SET)
            {
                // Check if the emergency state is set for the group
                if (IS_BIT_SET(grp_emergency_state, grpIdx) == FLAG_SET)
                {
                    // Find devices in the group and copy them to a local array
                    Find_Group_Devices(grpIdx);
                    
                    if (groupInfo.length > 0)
                    {
                        memcpy(emgGroup.tx_address, groupInfo.slaves, groupInfo.length);
                        emgGroup.tx_length = groupInfo.length;

                        // Set emergency flags for the group and update group emergency status
                        emgGroupArray[grpIdx].emg_detect_f = FLAG_SET;
                        emgGroupArray[grpIdx].group_in_emergency = FLAG_SET;

                        // Debug messages if enabled
                        #if DEBUG_ALL || DEBUG_FIRE_FUN
                        Print_Message("\nEmergency input enabled in group : ");
                        Print_Number(grpIdx + 1);
                        Print_Message("\nGroup devices are : ");
                        for (U8 prt = 0; prt < groupInfo.length; prt++)
                        {
                            Print_Number(groupInfo.slaves[prt]);
                            Print_Message(",");
                        }
                        #endif

                        // Clear all group flags and set emergency command flags
                        Clear_Whole_Group_Flags();
                        emgGroup.send_emg_command_f = FLAG_SET;
                        osdp_app.gb_enable_poll = 0;
                        emgGroup.chkEmgAckFlag = FLAG_SET;
                        iflags.command_in_process = FLAG_SET;
                        osdp_app.emgTransIdx = 0;
                        osdp_app.gb_poll_time = POLL_DELAY; // Refill POLL Timer value.
                    }
                }
            }
			else if (fireGroup[grpIdx].grp_in_free_access == FLAG_SET)
			{
				iLock.tx_address[0] = inSysDeviceList[devIdx].slv_addr;
				iLock.tx_length = 1;
				Set_Flags_Put_SD_Into_Free_Access();
			}
            else if (fireGroup[grpIdx].grp_in_acs_denied == FLAG_SET)
            {
                // Find devices in the group and sort them for access denied state
                Find_Group_Devices(grpIdx);
				Print_Message("\nGroup devices : ");
				for (U8 idx = 0; idx < groupInfo.length; idx ++)
				{
					Print_Message("-");
					Print_Number(groupInfo.slaves[idx]);
				}
				
                if (groupInfo.length > 0)
                {
                    Sort_Group_devices_For_Access_Denied();
                   
				    Print_Message("\nSorted Group devices : ");
                    for (U8 idx = 0; idx < iLock.tx_length; idx ++)
                    {
	                    Print_Message("-");
	                    Print_Number(iLock.tx_address[idx]);
                    }
					
                    // If there are devices to be put into access denied state
                    if (iLock.tx_length > 0)
                    {
                        fireGroup[grpIdx].acsD_req_detect_f = FLAG_SET;
                        fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
                        fireFlags.chk_op_sts_after_cmd = FLAG_SET;
                        Clear_Whole_Group_Flags();
                        Set_Flags_Put_Into_Access_Denied();
                        iflags.ad_executing_command = FLAG_SET;
                    }
                }
            }
            else
            {
                // Check each device in the group for emergency activation
                for (U8 dvIdx = 0; dvIdx < grpData[grpIdx].numOfDevs; dvIdx++)
                {
                    U8 emgIdx = Find_Device_Index_InSystem(grpData[grpIdx].devices[dvIdx]);
                    
                    if (emgGroup.emg_act_by[emgIdx] == FLAG_SET)
                    {
                        gb_osdp_emg_det_f = FLAG_SET;
                        gb_osdp.rec_dev_address = grpData[grpIdx].devices[dvIdx];
                        Check_Emergency_Signal();
                    }
                }
            }
        }
        else
        {
            // Reset the device from interlock state
            Reset_From_Interlock(inSysDeviceList[devIdx].slv_addr);
            if (iLock.tx_length > 0)
            {
                Set_Flags_Put_Into_Normal(inSysDeviceList[devIdx].slv_addr);
            }
        }
    }
}

/*****************************************************************************
* Function name	: void Operation_State_AUX_Input_Action(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 04/07/2024.
* Description	: Function is written to Handle the operation state based on Aux
*				  Input state and door state.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Operation_State_AUX_Input_Action(void)
{
	// Loop through all slave devices
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++)
	{
		// Find the group index for the device's slave address
		U8 grpIdx = Find_Device_Group(inSysDeviceList[dvIdx].slv_addr);
		
		// Check if the door is open or the lock is unlocked
		if ((slv_data[dvIdx].doorState == OPEN) || (slv_data[dvIdx].lockState == UNLOCKED))
		{
			// If the aux input control flag is reset, set it
			if ((Validate_Flags_For_OPRT_STS_AUX_IP_Control(dvIdx, grpIdx) == FLAG_SET)
			&& (iDeviceFlag[dvIdx].oprt_state_ax_ip_control_f == FLAG_RST))
			{
				iDeviceFlag[dvIdx].oprt_state_ax_ip_control_f = FLAG_SET;
			}
		}
		// Check if the door is closed and the lock is locked
		else if ((slv_data[dvIdx].doorState == CLOSED) && (slv_data[dvIdx].lockState == LOCKED))
		{
			// If the aux input control flag is set, reset it and handle the aux input action
			if (iDeviceFlag[dvIdx].oprt_state_ax_ip_control_f == FLAG_SET)
			{
				iDeviceFlag[dvIdx].oprt_state_ax_ip_control_f = FLAG_RST;
				
				Handle_Oprtion_State_On_AuxInput(dvIdx);
			}
		}
	}
}

U8 Validate_Flags_For_OPRT_STS_AUX_IP_Control(U8 rcdvIdx, U8 rcgrpIdx)
{
	if (((slv_data[rcdvIdx].oprtState == SLV_OP_STATE_NORMAL)
	|| (slv_data[rcdvIdx].oprtState == SLV_OP_STATE_ACCESS_DENIED))
	&& (iDeviceFlag[rcdvIdx].executing_dr_request_f == FLAG_RST)
	&& (iDeviceFlag[rcdvIdx].chk_slvdrActive_state == FLAG_RST)
	&& (iDeviceFlag[rcdvIdx].doorActiveState == FLAG_RST)
	&& (iDeviceFlag[rcdvIdx].itd_timer_running == FLAG_RST)
	&& (fireGroup[rcgrpIdx].grp_in_free_access == FLAG_RST)
	&& (emgGroupArray[rcgrpIdx].group_in_emergency == FLAG_RST)
	&& (iflags.rd_ip_sts_for_one_dr == FLAG_RST))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*****************************************************************************
* Function name	: void Handle_Oprtion_State_On_AuxInput(U8 devIdx)
* Returns		: Nothing.
* Arguments    	: U8 devIdx ---> Pass the device index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 04/07/2024.
* Description	: Function is written to Handle the operation state based on Aux
*				  Input state and door state.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Handle_Oprtion_State_On_AuxInput(U8 devIdx)
{
    // Find the group index for the device's slave address
    U8 grpIdx = Find_Device_Group(inSysDeviceList[devIdx].slv_addr);
    
    // Check if the device operation state is 'ACCESS DENIED'
    if (slv_data[devIdx].oprtState == SLV_OP_STATE_ACCESS_DENIED)
    {
		Take_An_Action_On_Group_AccessD_State(devIdx, grpIdx);
    }
    else
    {
        // If the group's operation state cannot be defined
        if (grpData[grpIdx].operationState == GRP_STATE_CANT_DEFINE)
        {
            // If the group is in access denied state
            if (fireGroup[grpIdx].grp_in_acs_denied == FLAG_SET)
            {
                Find_Group_Devices(grpIdx);
                if (groupInfo.length > 0)
                {
                    Sort_Group_devices_For_Access_Denied();
                    if (iLock.tx_length > 0)
                    {
                        fireGroup[grpIdx].acsD_req_detect_f = FLAG_SET;
                        fireGroup[grpIdx].grp_in_acs_denied = FLAG_SET;
                        fireFlags.chk_op_sts_after_cmd = FLAG_SET;
                        Clear_Whole_Group_Flags();
                        Set_Flags_Put_Into_Access_Denied();
                        iflags.ad_executing_command = FLAG_SET;
                    }
                }
            }
            else
            {
                __NOP(); // No Operation (do nothing)
            }
        }
        else
        {
            // Reset from interlock and set flags to normal if the group is not in 'cannot define' state
            Reset_From_Interlock(inSysDeviceList[devIdx].slv_addr);
            if (iLock.tx_length > 0)
            {
                Set_Flags_Put_Into_Normal(inSysDeviceList[devIdx].slv_addr);
            }
        }
    }
}

/*****************************************************************************
* Function name : U8 Is_Device_Normal_And_Other_Flags_Reset(U8 devIdx, U8 grpIndx)
* Returns       : U8 - Returns 1 if the device is in a normal state and all
*                 relevant flags are reset, otherwise returns 0.
* Arguments     : U8 devIdx - The index of the device to be checked.
*               : U8 grpIndx - The index of the group to which the device belongs.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : Function is written to validate whether a device is in a
*                 normal state and if its device-specific and group-specific
*                 flags are all reset.
* Notes         : This function assumes the existence of the global structures
*                 slv_data, iDeviceFlag, fireGroup, emgGroupArray, and iflags.
* Global Variables Affected : None.
*****************************************************************************/
U8 Is_Device_Normal_And_Other_Flags_Reset(U8 devIdx, U8 grpIndx)
{
	/* Validate if the device is normal and all relevant flags are reset */
	if ((slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL)
	&& (iDeviceFlag[devIdx].executing_dr_request_f == FLAG_RST)
	&& (iDeviceFlag[devIdx].chk_slvdrActive_state == FLAG_RST)
	&& (iDeviceFlag[devIdx].doorActiveState == FLAG_RST)
	&& (iDeviceFlag[devIdx].itd_timer_running == FLAG_RST)
	&& (fireGroup[grpIndx].grp_in_free_access == FLAG_RST)
	&& (emgGroupArray[grpIndx].group_in_emergency == FLAG_RST)
	&& (fireGroup[grpIndx].grp_in_acs_denied == FLAG_RST)
	&& (iflags.rd_ip_sts_for_one_dr == FLAG_RST)
	)
	{
		return 1;	/* return request is valid */
	}
	else
	{
		return 0;	/* return request is not valid */
	}
}

/*****************************************************************************
* Function name : U8 Check_Any_DR_Command_Executing(U8 devIdx)
* Returns       : U8 = 1 if no DR command is executing and the device is in a normal or interlock state
*                 with specific conditions met, otherwise 0.
* Arguments     : U8 devIdx - Index of the device to check
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/204.
* Description   : This function checks if a device is in a normal or interlock
*                 state and that no DR (Door Request) command is currently
*                 executing. It ensures specific flags and states are reset
*                 and the door is closed and locked.
* Notes         : The function assumes that global structures and flags like
*                 slv_data and iDeviceFlag are defined and properly initialized.
* Global Variables Affected : None
*****************************************************************************/
U8 Check_Any_DR_Command_Executing(U8 devIdx)
{
	if (((slv_data[devIdx].oprtState == SLV_OP_STATE_INTERLOCK) || (slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL))
	&& ((iDeviceFlag[devIdx].executing_dr_request_f == FLAG_RST)
	&& (iDeviceFlag[devIdx].doorActiveState == FLAG_RST)
	&& (iDeviceFlag[devIdx].itd_timer_running == FLAG_RST)
	&& (iDeviceFlag[devIdx].chk_slvdrActive_state == FLAG_RST)
	&& (slv_data[devIdx].doorState == CLOSED)
	&& (slv_data[devIdx].lockState == LOCKED)))
	{
		return 1;	// return 1 if all the conditions are satisfied.
	}
	else
	{
		return 0;	// return 0, if not satisfied.
	}
}

/*****************************************************************************
* Function name : void Take_An_Action_On_Group_AccessD_State(U8 dvIdx, U8 grupIdx)
* Returns       : Nothing
* Arguments     : U8 dvIdx - Index of the device in the system device list
*                 U8 grupIdx - Index of the group the device belongs to
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : This function takes an action based on the access denied state
*                 of a group. If the group is not in access denied state and the
*                 device was previously in access denied state, it powers up the device
*                 and sets it to normal state. If the group is in access denied state,
*                 it sets the device into access denied state.
* Notes         : The function assumes that global structures and flags like
*                 fireGroup, inSysDeviceList, iLock, and related functions are defined
*                 and properly initialized.
* Global Variables Affected : iLock.tx_address, iLock.tx_length
*****************************************************************************/
void Take_An_Action_On_Group_AccessD_State(U8 dvIdx, U8 grupIdx)
{
	// If the group is not in access denied state
    if (fireGroup[grupIdx].grp_in_acs_denied == FLAG_RST)
    {
        /* If the device was in 'access denied' state and powered off,
        but after that, the group returns to normal state, then power up the device */
        Reset_From_Interlock(inSysDeviceList[dvIdx].slv_addr);
        if (iLock.tx_length > 0)
        {
            Set_Flags_Put_Into_Normal(inSysDeviceList[dvIdx].slv_addr);
        }
    }
    else
    {
        // If the group is in access denied state
        iLock.tx_address[0] = inSysDeviceList[dvIdx].slv_addr;
        iLock.tx_length = 1;
		
        Set_Flags_Put_Into_Access_Denied();
    }
}

/*****************************************************************************
* Function name : void Group_DR_To_Normal(U8 grpNIdx)
* Returns       : Nothing
* Arguments     : U8 grpIdx - Index of the group to be set to normal state
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function sets a group of devices to normal state. It first
*                 finds all devices in the specified group, sorts them for free access,
*                 and if there are devices to process, sets various flags and puts the
*                 group into normal state.
* Notes         : The function assumes that global structures and flags like
*                 groupInfo, iLock, fireFlags, fireGroup, and related functions are
*                 defined and properly initialized.
* Global Variables Affected : fireFlags.f_executing_command, fireGroup[grpIdx].fire_req_detect_f,
*                             fireGroup[grpIdx].grp_in_free_access, iflags.recheck_nonIlock_ips_f,
*                             osdp_app.free_to_normal_f
*****************************************************************************/
void Group_DR_To_Normal(U8 grpNIdx)
{
	Find_Group_Devices(grpNIdx);  // Find devices in the specified group
	if (groupInfo.length > 0)  // If there are devices in the group
	{
		Sort_Group_devices_For_Free_Access();  // Sort devices for free access
		if (iLock.tx_length > 0)  // If there are devices to process
		{
			fireFlags.f_executing_command = FLAG_SET;  // Set flag for executing command
			fireGroup[grpNIdx].fire_req_detect_f = FLAG_SET;  // Set fire request detect flag for the group
			fireGroup[grpNIdx].grp_in_free_access = FLAG_RST;  // Reset the group's free access flag
			iflags.recheck_nonIlock_ips_f = FLAG_SET;  // Set flag to recheck non-interlock IPs
			fireFlags.chk_fip_after_ipRst_f = FLAG_SET;
			Clear_Whole_Group_Flags();  // Clear all group flags
			Set_Flags_Put_Into_Normal(0);  // Set flags to put the group into normal state
			osdp_app.free_to_normal_f = FLAG_SET;  // Set flag indicating free to normal state transition
		}
	}
}

/*****************************************************************************
* Function name : void Group_AD_To_Normal(U8 grp_adn_Idx)
* Returns       : Nothing
* Arguments     : U8 grp_adn_Idx - Index of the access denied group to be set to normal state
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function sets a group of devices from access denied state to
*                 normal state. It first finds all devices in the specified group, sorts
*                 them for access denied state, and if there are devices to process, sets
*                 various flags and puts the group into normal state.
* Notes         : The function assumes that global structures and flags like
*                 groupInfo, iLock, fireFlags, fireGroup, iflags, osdp_app, and related
*                 functions are defined and properly initialized.
* Global Variables Affected : fireFlags.f_executing_command, fireGroup[grp_adn_Idx].fire_req_detect_f,
*                             fireGroup[grp_adn_Idx].grp_in_acs_denied, iflags.recheck_nonIlock_ips_f,
*                             osdp_app.free_to_normal_f, iflags.ad_executing_command
*****************************************************************************/
void Group_AD_To_Normal(U8 grp_adn_Idx)
{
	Find_Group_Devices(grp_adn_Idx);  // Find devices in the specified group
	if (groupInfo.length > 0)  // If there are devices in the group
	{
		Sort_Group_devices_For_Access_Denied();  // Sort devices for access denied state
		if (iLock.tx_length > 0)  // If there are devices to process
		{
			fireGroup[grp_adn_Idx].fire_req_detect_f = FLAG_SET;  // Set fire request detect flag for the group
			fireGroup[grp_adn_Idx].grp_in_acs_denied = FLAG_RST;  // Reset the group's access denied flag
			iflags.recheck_nonIlock_ips_f = FLAG_SET;  // Set flag to recheck non-interlock IPs
			Clear_Whole_Group_Flags();  // Clear all group flags
			Set_Flags_Put_Into_Normal(0);  // Set flags to put the group into normal state
			osdp_app.free_to_normal_f = FLAG_SET;  // Set flag indicating free to normal state transition
			iflags.ad_executing_command = FLAG_SET;  // Set flag indicating an access denied command is executing
		}
	}
}

/*****************************************************************************
* Function name : void Group_DoorRelease(U8 grp_dr_Idx)
* Returns       : Nothing
* Arguments     : U8 grp_dr_Idx - Index of the group for which the door release action is to be performed
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function performs the door release action for a group of devices.
*                 It first finds all devices in the specified group, sorts them for free
*                 access, and if there are devices to process, sets various flags and puts
*                 the group into free access state.
* Notes         : The function assumes that global structures and flags like
*                 groupInfo, iLock, fireFlags, fireGroup, and related functions are
*                 defined and properly initialized.
* Global Variables Affected : fireFlags.f_executing_command, fireGroup[grp_dr_Idx].fire_req_detect_f,
*                             fireFlags.chk_op_sts_after_cmd, fireGroup[grp_dr_Idx].grp_in_free_access
*****************************************************************************/
void Group_DoorRelease(U8 grp_dr_Idx)
{
	Find_Group_Devices(grp_dr_Idx);  // Find devices in the specified group
	if (groupInfo.length > 0)  // If there are devices in the group
	{
		Sort_Group_devices_For_Free_Access();  // Sort devices for free access
		if (iLock.tx_length > 0)  // If there are devices to process
		{
			fireFlags.f_executing_command = FLAG_SET;  // Set flag for executing command
			fireGroup[grp_dr_Idx].fire_req_detect_f = FLAG_SET;  // Set fire request detect flag for the group
			fireFlags.chk_op_sts_after_cmd = FLAG_SET;  // Set flag to check operation status after command
			fireGroup[grp_dr_Idx].grp_in_free_access = FLAG_SET;  // Set the group's free access flag
			Clear_Whole_Group_Flags();  // Clear all group flags
			Set_Flags_Put_Into_Free_Access();  // Set flags to put the group into free access state
		}
	}
}

/*****************************************************************************
* Function name : void Group_AccessDenied(U8 grp_ad_Idx)
* Returns       : Nothing
* Arguments     : U8 grp_ad_Idx - Index of the group to be set to access denied state
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function sets a group of devices to access denied state. It first
*                 finds all devices in the specified group, sorts them for access denied,
*                 and if there are devices to process, sets various flags and puts the
*                 group into access denied state.
* Notes         : The function assumes that global structures and flags like
*                 groupInfo, iLock, fireFlags, fireGroup, and related functions are
*                 defined and properly initialized.
* Global Variables Affected : fireGroup[grp_ad_Idx].acsD_req_detect_f,
*                             fireGroup[grp_ad_Idx].grp_in_acs_denied,
*                             fireFlags.chk_op_sts_after_cmd,
*                             iflags.ad_executing_command
*****************************************************************************/
void Group_AccessDenied(U8 grp_ad_Idx)
{
	Find_Group_Devices(grp_ad_Idx);  // Find devices in the specified group
	if (groupInfo.length > 0)  // If there are devices in the group
	{
		Sort_Group_devices_For_Access_Denied();  // Sort devices for access denied
		if (iLock.tx_length > 0)  // If there are devices to process
		{
			fireGroup[grp_ad_Idx].acsD_req_detect_f = FLAG_SET;  // Set fire request detect flag for the group
			fireGroup[grp_ad_Idx].grp_in_acs_denied = FLAG_SET;  // Set the group's access denied flag
			fireFlags.chk_op_sts_after_cmd = FLAG_SET;  // Set flag to check operation status after command
			Clear_Whole_Group_Flags();  // Clear all group flags
			Set_Flags_Put_Into_Access_Denied();  // Set flags to put the group into access denied state
			iflags.ad_executing_command = FLAG_SET;  // Set flag indicating access denied command execution
		}
	}
}

/*****************************************************************************
* Function name : void Door_AD_To_Normal(U8 dr_adn_Idx)
* Returns       : Nothing
* Arguments     : U8 dr_adn_Idx - Index of the door to be set to normal state
* Created by    : Ranjitkumar Ainapure.
* Date created  : 13/07/2024.
* Description   : This function sets a door from access denied to normal state. It first
*                 checks if the door is not in an emergency or free access state, then
*                 sets the necessary flags to transition the door to a normal state.
* Notes         : The function assumes that global structures and flags like
*                 slv_data, iLock, iflags, and related functions are
*                 defined and properly initialized.
* Global Variables Affected : iLock.tx_address, iLock.tx_length,
*                             iflags.recheck_nonIlock_ips_f
*****************************************************************************/
void Door_AD_To_Normal(U8 dr_adn_Idx)
{
	// Check if the door is not in emergency or free access state
	if ((slv_data[dr_adn_Idx].oprtState != SLV_OP_STATE_EMERGENCY)
	&& (slv_data[dr_adn_Idx].oprtState != SLV_OP_STATE_FREE_ACCESS))
	{
		iLock.tx_address[0] = inSysDeviceList[dr_adn_Idx].slv_addr;  // Set the address of the door
		iLock.tx_length = 1;  // Set the length to 1

		iflags.recheck_nonIlock_ips_f = FLAG_SET;  // Set flag to recheck non-interlock IPs
		Set_Flags_Put_Into_Normal(0);  // Set flags to put the door into normal state
	}
}
