/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: emergency.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: File is written to activate the emergency signal received from
*				  the slave devices and also if emergency is activated through an
*				  on board input pins. And take accordingly for reset signals.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"
#include "string.h"

/***** User Includes *****/
#include "emergency.h"

/*************************/
#include "definitions.h"
#include "osdp_protocol_master.h"
#include "user_uart.h"
#include "interlock.h"
#include "fire_functionality.h"
#include "digital_ip_app.h"
#include "group_config.h"
#include "app_aux_input.h"

/* Structure variable */
EMGFLAG emgGroup;
EMGARRAY emgGroupArray[NUM_OF_GROUPS];

/* Global variable */
U8 emergency_flag_check = 0;

void Check_Emergency_Signal(void)
{
	// Local variable to track emergency signal reception
	U8 lcl_emg_sig_receive_f = 0;
	
	// Check if an emergency signal is detected over IP and no command is currently in process
	if ((gb_ip_emg_detected_f == FLAG_SET) && (iflags.command_in_process == FLAG_RST))
	{
		// Reset the emergency signal flag
		gb_ip_emg_detected_f = FLAG_RST;
		
		// Print debug message if enabled
		#if DEBUG_ALL || DEBUG_EMERGENCY
		Print_Message("\nEmergency signal Activated At Master.");
		#endif
		
		// Set flag to indicate emergency signal detection
		emergency_flag_check = FLAG_SET;
		
		// Check if the detected emergency device is valid
		U8 isValidFireDevice = Find_EMG_Devices_With_IP_PIN();
		
		// If the emergency device is valid, clear group flags and set local signal receive flag
		if (isValidFireDevice)
		{
			Clear_Whole_Group_Flags();
			lcl_emg_sig_receive_f = 3;
		}
	}
	// Check if an emergency reset signal is detected over IP
	else if (gb_ip_emg_reset_f == FLAG_SET)
	{
		// Reset the emergency reset signal flag
		gb_ip_emg_reset_f = FLAG_RST;
		
		// Print debug message if enabled
		#if DEBUG_ALL || DEBUG_EMERGENCY
		Print_Message("\nEmergency reset signal detected At Master.");
		#endif
		
		// Reset emergency flag check and find valid emergency device
		emergency_flag_check = FLAG_RST;
		U8 isValidFireDevice = Find_EMG_Devices_With_IP_PIN();
		
		// If the emergency device is valid, clear group flags and set local signal receive flag
		if (isValidFireDevice)
		{
			iflags.recheck_nonIlock_ips_f = FLAG_SET;
			Clear_Whole_Group_Flags();
			lcl_emg_sig_receive_f = 4;
		}
	}
	// Check if an emergency signal is detected over OSDP
	else if (gb_osdp_emg_det_f == FLAG_DETECT)
	{
		// Reset the emergency signal flag after detection
		gb_osdp_emg_det_f = FLAG_RST;
		
		// Print debug message if enabled
		#if DEBUG_ALL || DEBUG_EMERGENCY
		Print_Message("\nEmergency signal, received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address);
		#endif
		
		// Set slave address for emergency group and clear group flags
		emgGroup.slvAddress = gb_osdp.rec_dev_address;
		U8 devIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
		emgGroup.emg_act_by[devIdx] = FLAG_SET;
		Clear_Whole_Group_Flags();
		lcl_emg_sig_receive_f = 1;
		emergency_flag_check = FLAG_SET;
	}
	// Check if an emergency reset signal is detected over OSDP
	else if (gb_osdp_emg_rst_f == FLAG_DETECT)
	{
		// Reset the emergency reset signal flag after detection
		gb_osdp_emg_rst_f = FLAG_RST;
		
		// Print debug message if enabled
		#if DEBUG_ALL || DEBUG_EMERGENCY
		Print_Message("\nReset emergency signal, received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address);
		#endif
		
		// Set slave address for emergency group and clear group flags
		emgGroup.slvAddress = gb_osdp.rec_dev_address;
		U8 devIdx = Find_Device_Index_InSystem(gb_osdp.rec_dev_address);
		emgGroup.emg_act_by[devIdx] = FLAG_RST;
		iflags.recheck_nonIlock_ips_f = FLAG_SET;
		Clear_Whole_Group_Flags();
		lcl_emg_sig_receive_f = 2;
		emergency_flag_check = FLAG_RST;
	}
	
	// Handle emergency signal reception actions
	if (lcl_emg_sig_receive_f > 0)
	{
		// Set flags for emergency command execution
		emgGroup.e_executing_command = FLAG_SET;
		fireFlags.chk_op_sts_after_cmd = FLAG_SET;
		
		// If the signal is related to emergency or emergency reset, find emergency group devices
		if ((lcl_emg_sig_receive_f == 1) || (lcl_emg_sig_receive_f == 2))
		{
			Find_Emergency_Group_Devices(emgGroup.slvAddress);
			
			// Print debug message if enabled
			#if DEBUG_ALL || DEBUG_EMERGENCY
			Print_Message("\nEmergency group devices are : ");
			for (int k = 0; k<emgGroup.tx_length; k++)
			{
				Print_Number(emgGroup.tx_address[k]);
				Print_Message(",");
			}
			#endif
		}
		
		// Set flags for sending or resetting emergency command
		if ((lcl_emg_sig_receive_f == 1) || (lcl_emg_sig_receive_f == 3))
		{
			emgGroup.send_emg_command_f = FLAG_SET;
		}
		else if ((lcl_emg_sig_receive_f == 2) || (lcl_emg_sig_receive_f == 4))
		{
			emgGroup.reset_emg_command_f = FLAG_SET;
			emgGroup.chk_fip_after_emgRst_f = FLAG_SET;
			Reset_Aux_Audio();
			
			for (U8 rstFlag = 0; rstFlag < emgGroup.tx_length; rstFlag ++)
			{
				U8 rstIdx = Find_Device_Index_InSystem(emgGroup.tx_address[rstFlag]);
				emgGroup.emg_act_by[rstIdx] = FLAG_RST;
			}
		}
		
		// Set additional flags and reset local signal receive flag
		osdp_app.gb_enable_poll = 0;
		emgGroup.chkEmgAckFlag = FLAG_SET;
		iflags.command_in_process = FLAG_SET;
		osdp_app.emgTransIdx = 0;
		
		lcl_emg_sig_receive_f = FLAG_RST;
		osdp_app.gb_poll_time = POLL_DELAY; // Refill POLL Timer value.
	}
}

/*****************************************************************************
* Function name	: void Find_Emergency_Group_Devices(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get an emergency device addresses from
*				  the group.
*               :
* Notes			: NA.
* Global Variables Affected : emgGroupArray[lcl_gpIdx].group_in_emergency.
*							  emgGroup.tx_address, emgGroup.tx_length.
*****************************************************************************/
void Find_Emergency_Group_Devices(U8 slvAddress)
{
	// Local variable to store the group index
	U8 lcl_gpIdx = 0;
	
	// Find the group index of the specified slave address
	lcl_gpIdx = Find_Device_Group(slvAddress);
	
	// Set flags in the emergency group array based on the emergency flag check
	emgGroupArray[lcl_gpIdx].emg_detect_f = FLAG_SET;
	if (emergency_flag_check == FLAG_SET)
	emgGroupArray[lcl_gpIdx].group_in_emergency = FLAG_SET;
	else
	emgGroupArray[lcl_gpIdx].group_in_emergency = FLAG_RST;
	
	// Initialize the transmission length for emergency group devices
	emgGroup.tx_length = 0;
	
	// Iterate through devices in the group and populate the transmission address array
	for (U8 idx = 0; idx < grpData[lcl_gpIdx].numOfDevs; idx++)
	{
		emgGroup.tx_address[idx] = grpData[lcl_gpIdx].devices[idx];
		emgGroup.tx_length++;
	}
}

/*****************************************************************************
* Function name	: void Clear_Whole_Group_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to clear whole group flags while executing
*				  the higher priority command.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_poll_time.
*****************************************************************************/
void Clear_Whole_Group_Flags(void)
{
	// Backup the current flags and variables
	memcpy(&copyiflags, &iflags, sizeof(ILCKFLAG));
	memcpy(&copyiDeviceFlag, &iDeviceFlag, sizeof(ILCKFLAG));
	memcpy(&copyosdp_app, &osdp_app, sizeof(OSDP_APP));
	
	// Clear the current flags and variables
	memset(&iflags, 0, sizeof(ILCKFLAG));
	memset(&iDeviceFlag, 0, sizeof(ILCKFLAG));
	memset(&osdp_app, 0, sizeof(OSDP_APP));
	
	// Refill the poll timer value
	osdp_app.gb_poll_time = POLL_DELAY;
}

/*****************************************************************************
* Function name	: void ReAssign_Whole_Group_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to re assign the whole group flags.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void ReAssign_Whole_Group_Flags(void)
{
	// Restore the flags and variables from the backup
	memcpy(&iflags, &copyiflags, sizeof(ILCKFLAG));
	memcpy(&iDeviceFlag, &copyiDeviceFlag, sizeof(ILCKFLAG));
	memcpy(&osdp_app, &copyosdp_app, sizeof(OSDP_APP));
	
	// Iterate through all groups
	for (U8 grpIdx = 0; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Check if a command is executing and either fire or emergency is detected in the group
		if ((grpData[grpIdx].command_executing_f == FLAG_SET) 
		&& ((fireGroup[grpIdx].fire_req_detect_f == FLAG_SET)
		|| (emgGroupArray[grpIdx].emg_detect_f == FLAG_SET)
		|| (fireGroup[grpIdx].acsD_req_detect_f == FLAG_SET)))
		{
			/* If the request comes in the same group */
			if (emgGroupArray[grpIdx].emg_detect_f == FLAG_SET)
			{
				/* If an emergency signal detected reset below flags */
				//fireGroup[grpIdx].grp_in_free_access = FLAG_RST;
				fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
			}
			
			if (fireGroup[grpIdx].fire_req_detect_f == FLAG_SET)
			{
				/* If a free access signal detected reset below flags */
				fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
			}
			
			fireGroup[grpIdx].fire_req_detect_f = FLAG_RST;
			fireGroup[grpIdx].acsD_req_detect_f = FLAG_RST;
			emgGroupArray[grpIdx].emg_detect_f = FLAG_RST;
			
			iflags.put_into_normal_state = FLAG_RST;
			osdp_app.norTransIdx = 0;
			osdp_app.norAckIdx = 0;
			iflags.chk_normal_state_ack = FLAG_RST;
			
			iflags.chk_ip_status = FLAG_RST;
			osdp_app.istTransIdx = 0;
			osdp_app.istRecIdx = 0;
			iflags.chk_ip_sts_ack = FLAG_RST;
			iflags.ip_sts_read_success = FLAG_RST;
			
			iflags.chk_oprt_status = FLAG_RST;
			osdp_app.oprTransIdx = 0;
			osdp_app.oprRecIdx = 0;
			iflags.chk_opt_sts_ack = FLAG_RST;
			iflags.opt_sts_read_success = FLAG_RST;
			
			iflags.send_osdp_out_cmd = FLAG_RST;
			
			iflags.put_into_interlock = FLAG_RST;
			osdp_app.ilockTransIdx = 0;
			osdp_app.ilockRecIdx = 0;
			iflags.chk_osdp_ilck_ack = FLAG_RST;
			
// 			emgGroup.send_emg_command_f = FLAG_RST;
// 			osdp_app.emgTransIdx = 0;
// 			osdp_app.emgAckIdx = 0;
// 			emgGroup.reset_emg_command_f = FLAG_RST;
			
			osdp_app.put_into_free_access = FLAG_RST;
			osdp_app.freeTransIdx = 0;
			osdp_app.freeAckIdx = 0;
			osdp_app.chk_free_acs_ack_f = FLAG_RST;
			osdp_app.chk_singleDr_fa_ack_f = FLAG_RST;
			
			osdp_app.put_into_acs_denied = FLAG_RST;
			osdp_app.acsTransIdx = 0;
			osdp_app.acsAckIdx = 0;
			osdp_app.chk_acs_dnd_ack_f = FLAG_RST;
			osdp_app.chk_singleDr_ad_ack_f = FLAG_RST;
			
			iflags.chk_itd_time_value = FLAG_RST;
			
			iflags.chk_osdp_out_ack = FLAG_RST;
			
			osdp_app.put_into_privacy_state = FLAG_RST;
			osdp_app.prvTransIdx = 0;
			osdp_app.prvAckIdx = 0;
			osdp_app.chk_prv_ack_f = FLAG_RST;
			
			Reset_Device_Specific_Flags(grpIdx);
		}
		// Check if fire or emergency is detected in the group
		else if ((fireGroup[grpIdx].fire_req_detect_f == FLAG_SET)
		|| (fireGroup[grpIdx].acsD_req_detect_f == FLAG_SET)
		|| (emgGroupArray[grpIdx].emg_detect_f == FLAG_SET))
		{
			/* If the request comes in the same group */
			if (emgGroupArray[grpIdx].emg_detect_f == FLAG_SET)
			{
				/* If an emergency signal detected reset below flags */
				//fireGroup[grpIdx].grp_in_free_access = FLAG_RST;
				fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
			}
			
			if (fireGroup[grpIdx].fire_req_detect_f == FLAG_SET)
			{
				/* If a free access signal detected reset below flags */
				fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
			}
			
			// Reset flags and variables related to fire or emergency detection
			fireGroup[grpIdx].fire_req_detect_f = FLAG_RST;
			fireGroup[grpIdx].acsD_req_detect_f = FLAG_RST;
			emgGroupArray[grpIdx].emg_detect_f = FLAG_RST;
			
			Reset_Device_Specific_Flags(grpIdx);
		}
	}
	
	// Check if operation status needs to be checked after a command
	if (fireFlags.chk_op_sts_after_cmd == FLAG_SET)
	{
		fireFlags.chk_op_sts_after_cmd = FLAG_RST;
		
		iflags.chk_flags_after_normal = FLAG_SET;
	}
}

/*****************************************************************************
* Function name	: U8 Find_EMG_Devices_With_IP_PIN(void)
* Returns		: U8 (returns 1 if there is valid device otherwise returns 0).
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get an emergency device addresses from
*				  the group, and inputs assigned.
*               :
* Notes			: NA.
* Global Variables Affected : emgGroupArray[lcl_gpIdx].group_in_emergency.
*							  emgGroup.tx_address, emgGroup.tx_length.
*****************************************************************************/
U8 Find_EMG_Devices_With_IP_PIN(void)
{
	// Initialize local variables and arrays
	U8 lcl_dev_Array[TOTAL_SLAVES] = {0};
	U8 lcl_idx = 0;
	
	// Iterate through all groups
	for (U8 grpIdx = GROUP1; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Check if emergency state is set for the group
		if (IS_BIT_SET(grp_emergency_state, grpIdx) == FLAG_SET)
		{
			// Find devices in the group and copy them to a local array
			Find_Group_Devices(grpIdx);
			memcpy(&lcl_dev_Array[lcl_idx], groupInfo.slaves, groupInfo.length);
			lcl_idx += groupInfo.length;
			
			// Set emergency flags for the group and update group emergency status
			emgGroupArray[grpIdx].emg_detect_f = FLAG_SET;
			if (emergency_flag_check == FLAG_SET)
			emgGroupArray[grpIdx].group_in_emergency = FLAG_SET;
			else
			emgGroupArray[grpIdx].group_in_emergency = FLAG_RST;
			
			// Print debug messages if enabled
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
		}
	}
	
	// If devices with emergency input enabled are found, copy them to the emgGroup structure
	if (lcl_idx > 0)
	{
		memcpy(emgGroup.tx_address, lcl_dev_Array, lcl_idx);
		emgGroup.tx_length = lcl_idx;
	}
	
	// Return 1 if devices are found, else return 0
	return ((lcl_idx > 0)? 1 : 0);
}

/*****************************************************************************
* Function name : void Reset_Device_Specific_Flags(U8 grupIdx)
* Returns       : Nothing.
* Arguments     : U8 grupIdx - The index of the group whose device-specific
*                 flags need to be reset.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : Function is written to reset various device-specific flags
*                 for all devices within a specified group.
* Notes         : This function assumes the existence of the global structures
*                 grpData, iDeviceFlag, and iLockDevice.
* Global Variables Affected : iDeviceFlag[], iLockDevice[].
*****************************************************************************/
void Reset_Device_Specific_Flags(U8 grupIdx)
{
	// Iterate through devices in the group to reset device-specific flags
	for (U8 cdvIdx = 0; cdvIdx < grpData[grupIdx].numOfDevs; cdvIdx++)
	{
		U8 dvIdx = 0;
		dvIdx = Find_Device_Index_InSystem(grpData[grupIdx].devices[cdvIdx]);
		iDeviceFlag[dvIdx].chk_slvdrActive_state = FLAG_RST;
		iDeviceFlag[dvIdx].doorActiveState = FLAG_RST;
		iDeviceFlag[dvIdx].executing_dr_request_f = 0;
		iDeviceFlag[dvIdx].start_itd_time = FLAG_RST;
		iLockDevice[dvIdx].itdCounts = 0;
		iDeviceFlag[dvIdx].itd_timer_running = FLAG_RST;
		iDeviceFlag[dvIdx].itd_time_completed = FLAG_RST;
		iDeviceFlag[dvIdx].door_req_flag = FLAG_RST;
		iDeviceFlag[dvIdx].dps_lfb_error_f = FLAG_RST;
		iDeviceFlag[dvIdx].drLock_control_f = FLAG_RST;
	}
}
