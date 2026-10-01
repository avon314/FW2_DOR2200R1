/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_aux_input.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: The auxiliary input data received from the slave devices
				  monitored here, take an appropriate action with the received
				  data. (For ex - free access, access denied and privacy input).
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
/* System Includes */
#include "asf.h"
#include "string.h"

/* User Includes */
#include "app_aux_input.h"
#include "definitions.h"
#include "user_uart.h"
#include "interlock.h"
#include "app_osdp.h"
#include "emergency.h"
#include "fire_functionality.h"
#include "group_config.h"
#include "privacy_config.h"

/* Global structure variable */
PRV_DATA priGroup[TOTAL_SLAVES];

/*****************************************************************************
* Function name	: void Check_AuxInput_Signal(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to check auxiliary data is receiving from
				  the slaves or not, if received executes appropriate action.
*               :
* Notes			: NA.
* Global Variables Affected : gb_osdp_free_acs_f, fireGroup[lcl_gpIdx].fire_req_detect_f
							  fireGroup[lcl_gpIdx].acsD_req_detect_f,
							  fireGroup[lcl_gpIdx].grp_in_free_access,	fireFlags.chk_op_sts_after_cmd						  
							  fireFlags.f_executing_command, fireGroup[lcl_gpIdx].grp_in_acs_denied
							  fireFlags.chk_op_sts_after_cmd, iflags.ad_executing_command
							  osdp_app.free_to_normal_f
*****************************************************************************/
void Check_AuxInput_Signal(void)
{
	// Check if Free Access signal is detected
	if ((gb_osdp_free_acs_f == FLAG_DETECT) && // Check if the Free Access flag is set
	(iflags.command_in_process == FLAG_RST) && // Check if no command is in process
	(emgGroup.e_executing_command == FLAG_RST)) // Check if no emergency command is executing
	{
		gb_osdp_free_acs_f = FLAG_RST; // Reset the Free Access flag
		
		#if DEBUG_ALL || DEBUG_AUX_INPUT
		Print_Message("\nFree Access signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address); // Print the slave address
		#endif
		
		U8 lcl_Slave_Dev = gb_osdp.rec_dev_address; // Store the slave address locally
		Clear_Whole_Group_Flags(); // Clear all group flags
		U8 lcl_gpIdx = 0;
		lcl_gpIdx = Find_Device_Group(lcl_Slave_Dev); // Find the group index of the device
		Find_Group_Devices(lcl_gpIdx); // Find devices in the group
		if (groupInfo.length > 0) // If devices are found in the group
		{
			Sort_Group_devices_For_Free_Access(); // Sort devices for free access
			if (iLock.tx_length > 0) // If there are devices with free access
			{
				fireGroup[lcl_gpIdx].fire_req_detect_f = FLAG_SET; // Set fire request flag for the group
				fireGroup[lcl_gpIdx].grp_in_free_access = FLAG_SET; // Set group in free access flag
				fireFlags.chk_op_sts_after_cmd = FLAG_SET; // Set flag to check operation status after command
				
				U8 devIdx = Find_Device_Index_InSystem(lcl_Slave_Dev);
				fireFlags.free_acs_active_by[devIdx] = FLAG_SET;
				
				#if DEBUG_ALL || DEBUG_AUX_INPUT
				Print_Message("\nFree Access Group devices are : ");
				for (U8 dx = 0; dx < iLock.tx_length; dx++) // Print devices in the group
				{
					Print_Number(iLock.tx_address[dx]);
					Print_Message(",");
				}
				#endif
				
				fireFlags.f_executing_command = FLAG_SET; // Set flag indicating a command is being executed
				Get_Slave_Aux_Audio_Value(lcl_Slave_Dev); // Get auxiliary audio value of the slave
				Set_Flags_Put_Into_Free_Access(); // Set flags to put devices into free access
			}
			else
			{
				ReAssign_Whole_Group_Flags(); // Reassign whole group flags
			}
		}
		else
		{
			// Do nothing.
		}
	}
	// Check if Access Denied signal is detected
	else if ((gb_osdp_acs_denied_f == FLAG_DETECT) && // Check if the Access Denied flag is set
	(iflags.command_in_process == FLAG_RST) && // Check if no command is in process
	(emgGroup.e_executing_command == FLAG_RST) && // Check if no emergency command is executing
	(fireFlags.f_executing_command == FLAG_RST)) // Check if no fire command is executing
	{
		gb_osdp_acs_denied_f = FLAG_RST; // Reset the Access Denied flag
		
		#if DEBUG_ALL || DEBUG_AUX_INPUT
		Print_Message("\nAccess Denied signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address); // Print the slave address
		#endif
		
		U8 lcl_Slave_Dev = gb_osdp.rec_dev_address; // Store the slave address locally
		Clear_Whole_Group_Flags(); // Clear all group flags
		U8 lcl_gpIdx = 0;
		lcl_gpIdx = Find_Device_Group(lcl_Slave_Dev); // Find the group index of the device
		Find_Group_Devices(lcl_gpIdx); // Find devices in the group
		if (groupInfo.length > 0) // If devices are found in the group
		{
			Sort_Group_devices_For_Access_Denied(); // Sort devices for access denied
			if (iLock.tx_length > 0) // If there are devices with access denied
			{
				fireGroup[lcl_gpIdx].acsD_req_detect_f = FLAG_SET; // Set fire request flag for the group
				fireGroup[lcl_gpIdx].grp_in_acs_denied = FLAG_SET; // Set group in access denied flag
				fireFlags.chk_op_sts_after_cmd = FLAG_SET; // Set flag to check operation status after command
				
				#if DEBUG_ALL || DEBUG_AUX_INPUT
				Print_Message("\nAccess Denied Group devices are : ");
				for (U8 dx = 0; dx < iLock.tx_length; dx++) // Print devices in the group
				{
					Print_Number(iLock.tx_address[dx]);
					Print_Message(",");
				}
				#endif
				
				iflags.ad_executing_command = FLAG_SET; // Set flag indicating access denied command is executing
				Get_Slave_Aux_Audio_Value(lcl_Slave_Dev); // Get auxiliary audio value of the slave
				
				Set_Flags_Put_Into_Access_Denied(); // Set flags to put devices into access denied state
			}
			else
			{
				ReAssign_Whole_Group_Flags(); // Reassign whole group flags
			}
		}
	}
	else if (gb_osdp_privacy_f == TRUE) // Check if Privacy state signal is received
	{
		gb_osdp_privacy_f = FALSE; // Reset the Privacy flag once the reply is received
		
		#if DEBUG_ALL || DEBUG_AUX_INPUT
		Print_Message("\nPrivacy state reply received from the sAddress : ");
		Print_Number(gb_osdp.rec_dev_address); // Print the slave address
		#endif
		
		Set_Reset_Privacy_State(gb_osdp.rec_dev_address); // Set/reset privacy state for the slave
	}
	else if ((gb_osdp_normal_f == FLAG_DETECT) && // Check if Normal signal is received
	(iflags.command_in_process == FLAG_RST) && // Check if no command is in process
	(emgGroup.e_executing_command == FLAG_RST) && // Check if no emergency command is executing
	(fireFlags.f_executing_command == FLAG_RST) && // Check if no fire command is executing
	(iflags.ad_executing_command == FLAG_RST)) // Check if no access denied command is executing
	{
		gb_osdp_normal_f = FLAG_RST; // Reset the Normal flag
		
		#if DEBUG_ALL || DEBUG_AUX_INPUT
		Print_Message("\nNormal signal received from sAddress : ");
		Print_Number(gb_osdp.rec_dev_address); // Print the slave address
		#endif
		
		Put_Normal_From_FreeAccess(gb_osdp.rec_dev_address);
	}
}

/*****************************************************************************
* Function name	: void Set_Reset_Privacy_State(U8 devAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to call the functions to set privacy state
				  and normal state.
*               :
* Notes			: NA.
* Global Variables Affected : .
*****************************************************************************/
void Set_Reset_Privacy_State(U8 devAddress)
{
	// Find Privacy devices from the group.
	U8 slv_valid = Find_Privacy_Group_Devices(devAddress);
	
	// If devices are found in the group.
	if (slv_valid > 0)
	{
		// Find device index of the particular device.
		U8 lclDevIdx = Find_Device_Index_InSystem(devAddress);
		if (priGroup[lclDevIdx].enter_to_privacy_f == FLAG_SET)
		{
			// If privacy flag set? Set flags to put into privacy state.
			Set_Flags_Put_Into_Privacy();
		}
		else
		{
			if (iflags.slv_in_prvc_N_pwr_UP == FLAG_SET)
			{
				iflags.slv_in_prvc_N_pwr_UP = FLAG_RST;
				
				U8 lclSlvArr[32] = {0};
				U8 lclSlvLen = 0;
				
				if (iLock.tx_length > 0)
				{
					/* Length received from the above function "Find_Privacy_Group_Devices()" */
					memcpy(lclSlvArr, iLock.tx_address, iLock.tx_length);
					lclSlvLen = iLock.tx_length;
					iLock.tx_length = 0;
				}
				
				Reset_From_Interlock(devAddress);
				if (iLock.tx_length > 0)
				{
					memcpy(&lclSlvArr[lclSlvLen], iLock.tx_address, iLock.tx_length);
					lclSlvLen += iLock.tx_length;
					
					if (lclSlvLen < 32)
					{
						Update_Array_In_Ascending_Order(lclSlvArr, lclSlvLen);
					}
					else
					{
						__NOP();
					}
					
					if (lclSlvLen < 32)
					{
						lclSlvLen = Remove_Duplicate_Slaves(lclSlvArr, lclSlvLen);
					}
					else
					{
						__NOP();
					}
					
					memcpy(iLock.tx_address, lclSlvArr, lclSlvLen);
					iLock.tx_length = lclSlvLen;
				}
			}
			// Else put into normal state.
			Set_Flags_Put_Into_Normal(0);
		}
	}
}

/*****************************************************************************
* Function name	: U8 Find_Privacy_Group_Index(U8 devAddress)
* Returns		: U8 (Returns privacy group number (form of index))
* Arguments		: U8 devAddress ---> Pass device Address.
* Created by	: Ranjitkumar Ainapure.
* Date created	:
* Description	: Function to find Privacy device Group.
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Find_Privacy_Group_Index(U8 devIndex)
{
	for (U8 grpIdx = 0; grpIdx < MAX_PVC_GRPS; grpIdx++)
	{
		if (IS_BIT_SET(pvc_grp[grpIdx], devIndex))
		{
			return grpIdx;	// Returns privacy group number (form of index)
		}
	}
	return -1;
}

/*****************************************************************************
* Function name	: U8 Find_Privacy_Group_Devices(U8 slvAddress)
* Returns		: U8 ---> returns index length.
* Arguments    	: U8 slvAddress ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get privacy devices from the group.
*               :
* Notes			: NA.
* Global Variables Affected : priGroup[dvIdx].enter_to_privacy_f.
*****************************************************************************/
U8 Find_Privacy_Group_Devices(U8 slvAddress)
{
	bool lclValid_f = FALSE; // Flag indicating if the device is valid
	U8 lclIdx_len = 0; // Local index length
	U8 lclPrvArray[TOTAL_SLAVES] = {0}; // Array to store privacy group devices
	U8 lclDevIdx = Find_Device_Index_InSystem(slvAddress); // Find index of the device in the system
	U8 lclpGrpIdx = Find_Privacy_Group_Index(lclDevIdx); // Find index of privacy group for the device

	if (Is_Device_Valid(lclDevIdx)) // Check if the device index is valid
	{
		lclValid_f = TRUE; // Set the valid flag to true if the device is valid
	}
	
	if (lclValid_f == TRUE) // If the device is valid
	{
		for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx++) // Loop through all devices
		{
			if (IS_BIT_SET(pvc_grp[lclpGrpIdx], dvIdx)) // Check if the device is in the privacy group
			{
				if (Is_Device_Valid(dvIdx)) // Check if the device is valid
				{
					lclPrvArray[lclIdx_len++] = inSysDeviceList[dvIdx].slv_addr; // Store the slave address in the array
					
					if (priGroup[dvIdx].enter_to_privacy_f == FLAG_RST) // Check if the device is entering privacy mode
					priGroup[dvIdx].enter_to_privacy_f = FLAG_SET; // Set flag indicating entering privacy mode
					else
					priGroup[dvIdx].enter_to_privacy_f = FLAG_RST; // Reset flag indicating exiting privacy mode
				}
			}
		}
		if (lclIdx_len > 0) // If there are devices in the privacy group
		{
			memcpy(iLock.tx_address, lclPrvArray, lclIdx_len); // Copy the array to the iLock address array
			iLock.tx_length = lclIdx_len; // Set the length of the iLock address array
			
			#if DEBUG_ALL || DEBUG_AUX_INPUT
			Print_Message("\nPrivacy group devices are : ");
			for (U8 pIdx = 0; pIdx < iLock.tx_length; pIdx++) // Print the devices in the privacy group
			{
				Print_Number(iLock.tx_address[pIdx]);
				Print_Message(",");
			}
			#endif
		}
	}
	
	return lclIdx_len; // Return the length of the privacy group devices array
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_Into_Privacy(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags related to privacy state.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_enable_poll, osdp_app.chk_prv_ack_f
*							  osdp_app.put_into_privacy_state, osdp_app.prvTransIdx
							  iflags.command_in_process
*****************************************************************************/
void Set_Flags_Put_Into_Privacy(void)
{
	// Disable OSDP polling and set flags for entering privacy state
	osdp_app.gb_enable_poll = 0; // Disable OSDP polling
	osdp_app.put_into_privacy_state = FLAG_SET; // Set flag to put into privacy state
	osdp_app.chk_prv_ack_f = FLAG_SET; // Set flag to check privacy acknowledgment
	osdp_app.prvTransIdx = 0; // Reset privacy transition index
	iflags.command_in_process = FLAG_SET; // Set flag indicating command processing
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_Into_Free_Access(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags related to free access.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_enable_poll, osdp_app.put_into_free_access
							  iflags.command_in_process, osdp_app.freeTransIdx
							  osdp_app.chk_free_acs_ack_f
*****************************************************************************/
void Set_Flags_Put_Into_Free_Access(void)
{
	// Disable OSDP polling and set flags for entering free access state
	osdp_app.gb_enable_poll = 0; // Disable OSDP polling
	osdp_app.put_into_free_access = FLAG_SET; // Set flag to put into free access state
	iflags.command_in_process = FLAG_SET; // Set flag indicating command processing
	osdp_app.freeTransIdx = 0; // Reset free access transition index
	osdp_app.chk_free_acs_ack_f = FLAG_SET; // Set flag to check free access acknowledgment
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_SD_Into_Free_Access(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags related to free access single door.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_enable_poll, osdp_app.put_into_free_access
							  iflags.command_in_process, osdp_app.freeTransIdx
							  osdp_app.chk_singleDr_fa_ack_f
*****************************************************************************/
void Set_Flags_Put_SD_Into_Free_Access(void)
{
	// Disable OSDP polling and set flags for putting single door into free access state
	osdp_app.gb_enable_poll = 0; // Disable OSDP polling
	osdp_app.put_into_free_access = FLAG_SET; // Set flag to put single door into free access state
	iflags.command_in_process = FLAG_SET; // Set flag indicating command processing
	osdp_app.freeTransIdx = 0; // Reset free access transition index
	osdp_app.chk_singleDr_fa_ack_f = FLAG_SET; // Set flag to check single door free access acknowledgment
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_SD_Into_Access_Denied(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags related to access denied single door.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_enable_poll, osdp_app.put_into_acs_denied
							  iflags.command_in_process, osdp_app.acsTransIdx
							  osdp_app.chk_singleDr_ad_ack_f
*****************************************************************************/
void Set_Flags_Put_SD_Into_Access_Denied(void)
{
	// Disable OSDP polling and set flags for putting single door into access denied state
	osdp_app.gb_enable_poll = 0; // Disable OSDP polling
	osdp_app.put_into_acs_denied = FLAG_SET; // Set flag to put single door into access denied state
	iflags.command_in_process = FLAG_SET; // Set flag indicating command processing
	osdp_app.acsTransIdx = 0; // Reset access denied transition index
	osdp_app.chk_singleDr_ad_ack_f = FLAG_SET; // Set flag to check single door access denied acknowledgment
}

/*****************************************************************************
* Function name	: void Set_Flags_Put_Into_Access_Denied(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags related to access denied.
*               :
* Notes			: NA.
* Global Variables Affected : osdp_app.gb_enable_poll, osdp_app.put_into_acs_denied
							  iflags.command_in_process, osdp_app.acsTransIdx
							  osdp_app.chk_acs_dnd_ack_f
*****************************************************************************/
void Set_Flags_Put_Into_Access_Denied(void)
{
	// Disable OSDP polling and set flags for putting devices into access denied state
	osdp_app.gb_enable_poll = 0; // Disable OSDP polling
	osdp_app.put_into_acs_denied = FLAG_SET; // Set flag to put devices into access denied state
	iflags.command_in_process = FLAG_SET; // Set flag indicating command processing
	osdp_app.acsTransIdx = 0; // Reset access denied transition index
	osdp_app.chk_acs_dnd_ack_f = FLAG_SET; // Set flag to check access denied acknowledgment
}

/*****************************************************************************
* Function name	: U8 Is_Device_Valid(U8 lclDevIdx)
* Returns		: U8 ---> returns 1 for valid, 0 for not valid.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to check device is valid or not.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
U8 Is_Device_Valid(U8 lclDevIdx)
{
	// Check if the device is valid based on certain conditions
	if (((slv_data[lclDevIdx].oprtState == SLV_OP_STATE_NORMAL) || (slv_data[lclDevIdx].oprtState == SLV_OP_STATE_PRIVACY))
	&& ((iDeviceFlag[lclDevIdx].executing_dr_request_f == FLAG_RST)
	&& (iDeviceFlag[lclDevIdx].doorActiveState == FLAG_RST)
	&& (iDeviceFlag[lclDevIdx].itd_timer_running == FLAG_RST)
	&& (iDeviceFlag[lclDevIdx].chk_slvdrActive_state == FLAG_RST)
	&& (slv_data[lclDevIdx].doorState == CLOSED)
	/*&& (slv_data[lclDevIdx].lockState == LOCKED)*/))
	{
		return 1; // Return 1 if the device is valid
	}
	else
	{
		return 0; // Return 0 if the device is not valid
	}
}

/*****************************************************************************
* Function name	: void Sort_Group_devices_For_Free_Access(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to sort the group devices to an transmit
*				  array if devices are not in emergency state.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_length, iLock.tx_address[]
*****************************************************************************/
void Sort_Group_devices_For_Free_Access(void)
{
	iLock.tx_length = 0; // Reset the length of the transmission array
	
	// Iterate through the devices in the group
	for (U8 srtIdx = 0; srtIdx < groupInfo.length; srtIdx++)
	{
		// Find the index of the device in the system
		U8 devIdx = Find_Device_Index_InSystem(groupInfo.slaves[srtIdx]);
		
		// Check if the device is not in emergency state
		if (slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
		{
			// Add the device to the transmission array and increment the length
			iLock.tx_address[iLock.tx_length++] = groupInfo.slaves[srtIdx];
		}
	}
}

/*****************************************************************************
* Function name	: void Sort_Group_devices_For_Access_Denied(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to sort devices to transmit array, if devices
*				  are not in emergency & free access.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
void Sort_Group_devices_For_Access_Denied(void)
{
	iLock.tx_length = 0; // Reset the length of the transmission array
	
	// Iterate through the devices in the group
	for (U8 srtIdx = 0; srtIdx < groupInfo.length; srtIdx++)
	{
		// Find the index of the device in the system
		U8 devIdx = Find_Device_Index_InSystem(groupInfo.slaves[srtIdx]);
		
		// Check if the device is not in emergency state and not in free access state
		if ((slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
		&& (slv_data[devIdx].oprtState != SLV_OP_STATE_FREE_ACCESS)
		&& (slv_data[devIdx].doorState == CLOSED)
		&& (slv_data[devIdx].lockState == LOCKED)
		)
		{
			// Add the device to the transmission array and increment the length
			iLock.tx_address[iLock.tx_length++] = groupInfo.slaves[srtIdx];
		}
	}
}

/*****************************************************************************
* Function name	: void Sort_Group_devices_For_Normal_state(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to sort devices to transmit array, if devices
*				  are not in emergency.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
void Sort_Group_devices_For_Normal_state(void)
{
	iLock.tx_length = 0; // Reset the length of the transmission array
	
	// Iterate through the devices in the group
	for (U8 srtIdx = 0; srtIdx < groupInfo.length; srtIdx++)
	{
		// Find the index of the device in the system
		U8 devIdx = Find_Device_Index_InSystem(groupInfo.slaves[srtIdx]);
		
		// Check if the device is not in emergency state
		if (slv_data[devIdx].oprtState != SLV_OP_STATE_EMERGENCY)
		{
			// Add the device to the transmission array and increment the length
			iLock.tx_address[iLock.tx_length++] = groupInfo.slaves[srtIdx];
		}
	}
}

/*****************************************************************************
* Function name	: void Sort_Group_devices_For_DRT(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to sort devices to transmit array, if devices
*				  are in normal.
*               :
* Notes			: NA.
* Global Variables Affected : iLock.tx_address, iLock.tx_length.
*****************************************************************************/
void Sort_Group_devices_For_DRT(void)
{
	iLock.tx_length = 0; // Reset the length of the transmission array
	
	// Iterate through the devices in the group
	for (U8 srtIdx = 0; srtIdx < groupInfo.length; srtIdx++)
	{
		// Find the index of the device in the system
		U8 devIdx = Find_Device_Index_InSystem(groupInfo.slaves[srtIdx]);
		
		// Check if the device is in normal state
		if (slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL)
		{
			// Add the device to the transmission array and increment the length
			iLock.tx_address[iLock.tx_length++] = groupInfo.slaves[srtIdx];
		}
	}
}

/*****************************************************************************
* Function name	: void Get_Slave_Aux_Audio_Value(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: U8 slvAddress ---> Pass the slave address.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get slave auxiliary value.
*               :
* Notes			: NA.
* Global Variables Affected : gb_slave_aux_audio.
*****************************************************************************/
void Get_Slave_Aux_Audio_Value(U8 slvAddress)
{
	// Find the index of the device in the system
	U8 devIdx = Find_Device_Index_InSystem(slvAddress);
	
	// Retrieve the auxiliary audio value from the slave data and store it in a global variable
	gb_slave_aux_audio = slv_data[devIdx].auxAudioSelect;
}

/*****************************************************************************
* Function name	: void Reset_Aux_Audio(U8 slvAddress)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset slave auxiliary value.
*               :
* Notes			: NA.
* Global Variables Affected : gb_slave_aux_audio.
*****************************************************************************/
void Reset_Aux_Audio(void)
{
	gb_slave_aux_audio = 0;
}

/*****************************************************************************
* Function name : void Put_Normal_From_FreeAccess(U8 slvAddress)
* Returns       : Nothing.
* Arguments     : U8 slvAddress - The slave address to be handled.
* Created by    : Ranjitkumar Ainapure.
* Date created  : 12/07/2024.
* Description   : Function is written to reset the group of devices from free access
*                 to normal state based on the auxiliary input.
* Notes         : NA.
* Global Variables Affected : iflags.recheck_nonIlock_ips_f, fireGroup[],
*                             fireFlags.free_acs_active_by[], osdp_app.free_to_normal_f.
*****************************************************************************/
void Put_Normal_From_FreeAccess(U8 slvAddress)
{
	U8 lcl_Slave_Dev = slvAddress; // Store the slave address locally
	
	// Set recheck flag and clear all group flags
	iflags.recheck_nonIlock_ips_f = FLAG_SET;
	Clear_Whole_Group_Flags(); // Clear all group flags
	
	// Find the group index and devices in the group
	U8 lcl_gpIdx = 0;
	lcl_gpIdx = Find_Device_Group(lcl_Slave_Dev); // Find the group index of the device
	Find_Group_Devices(lcl_gpIdx); // Find devices in the group
	if (groupInfo.length > 0) // If devices are found in the group
	{
		Sort_Group_devices_For_Normal_state(); // Sort devices for normal state
		if (iLock.tx_length > 0) // If there are devices to be put into normal state
		{
			// Set fire request flag and reset group flags
			fireGroup[lcl_gpIdx].fire_req_detect_f = FLAG_SET; // Set fire request flag for the group
			fireGroup[lcl_gpIdx].grp_in_free_access = FLAG_RST; // Reset group in free access flag
			fireGroup[lcl_gpIdx].grp_in_acs_denied = FLAG_RST; // Reset group in access denied flag
			fireFlags.reset_from_aux_ip = FLAG_SET;	// Set flag to say that, resetting from aux input.
			fireFlags.chk_fip_after_ipRst_f = FLAG_SET;	/* Set flag to recheck the fire input is detected or not? */
			
			// Find the device index and reset the free access flag
			U8 devIdx = Find_Device_Index_InSystem(lcl_Slave_Dev);
			fireFlags.free_acs_active_by[devIdx] = FLAG_RST;
			
			// Debug messages if enabled
			#if DEBUG_ALL || DEBUG_AUX_INPUT
			Print_Message("\nGroup devices to normal are : ");
			for (U8 dx = 0; dx < iLock.tx_length; dx++) // Print devices in the group
			{
				Print_Number(iLock.tx_address[dx]);
				Print_Message(",");
			}
			#endif
			
			// Get auxiliary audio value and set flags to put devices into normal state
			Get_Slave_Aux_Audio_Value(lcl_Slave_Dev); // Get auxiliary audio value of the slave
			Set_Flags_Put_Into_Normal(lcl_Slave_Dev); // Set flags to put devices into normal state
			osdp_app.free_to_normal_f = FLAG_SET; // Set flag indicating free to go to normal state
		}
		else
		{
			ReAssign_Whole_Group_Flags(); // Reassign whole group flags
		}
	}
}

/*****************************************************************************
* Function name : void Update_Array_In_Ascending_Order(U8 *arr, U8 len)
* Returns       : Nothing
* Arguments     : U8 *arr - Pointer to the array to be sorted
*                 U8 len - Length of the array
* Created by    : Ranjitkumar Ainapure
* Date created  : 18/07/2024
* Description   : This function sorts an array of unsigned 8-bit integers in
*                 ascending order using the bubble sort algorithm.
* Notes         : The function assumes the array length is provided correctly
*                 and the array is of type U8 (unsigned 8-bit integer).
* Global Variables Affected : None
*****************************************************************************/
void Update_Array_In_Ascending_Order(U8 *arr, U8 len)
{
	U8 temp = 0;  // Temporary variable for swapping

	// Loop through each element of the array
	for (U8 i = 0; i < len - 1; i++)
	{
		// Loop through the elements that are after the current element
		for (U8 j = (i + 1); j < len; j++)
		{
			// If the current element is greater than the next element, swap them
			if (arr[i] > arr[j])
			{
				temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
		}
	}
}


/*****************************************************************************
* Function name : U8 Remove_Duplicate_Slaves(U8 *arr, U8 len)
* Returns       : U8 - New length of the array after removing duplicates
* Arguments     : U8 *arr - Pointer to the array from which duplicates are to be removed
*                 U8 len - Length of the array
* Created by    : Ranjitkumar Ainapure
* Date created  : 18/07/2024
* Description   : This function removes duplicate elements from a sorted array of
*                 unsigned 8-bit integers. It returns the new length of the array
*                 after duplicates have been removed.
* Notes         : The function assumes that the array is already sorted in ascending
*                 order. It creates a temporary array to store unique elements and
*                 then copies them back to the original array.
* Global Variables Affected : None
*****************************************************************************/
U8 Remove_Duplicate_Slaves(U8 *arr, U8 len)
{
	if (len == 0 || len == 1)  // If the array is empty or has only one element, return the length as is
	{
		return len;
	}

	U8 temp[len];  // Temporary array to store unique elements
	U8 j = 0;      // Index for the temporary array

	// Loop through the array to find unique elements
	for (U8 i = 0; i < len - 1; i++)
	{
		// If the current element is not equal to the next element, add it to the temporary array
		if (arr[i] != arr[i + 1])
		{
			temp[j++] = arr[i];
		}
	}
	// Add the last element of the array to the temporary array
	temp[j++] = arr[len - 1];

	// Copy the unique elements back to the original array
	for (U8 i = 0; i < j; i++)
	{
		arr[i] = temp[i];
	}

	return j;  // Return the new length of the array
}
