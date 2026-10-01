/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: fire_functionality.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: This function finds devices configured within the group,
*				  executes fire functionality, and manages flags related to fire.
*				  It ensures that devices within a specified group are properly
*				  accounted for and implements the necessary actions in response
*				  to fire-related events. Additionally, it oversees the manipulation
*				  of flags associated with fire events, maintaining proper
*				  synchronization and control within the system.
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
#include "fire_functionality.h"
#include "group_config.h"
#include "digital_ip_app.h"
#include "app_osdp.h"
#include "user_uart.h"
#include "definitions.h"
#include "emergency.h"
#include "app_aux_input.h"

/* Structure Variables */
FIRE fireFlags;
FIREGROUP fireGroup[NUM_OF_GROUPS];

/*****************************************************************************
* Function name	: void Execute_Fire_Functionality(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to set flags to put device to free access
*				  if fire signal detected, and also set flags to put to normal
*				  state if fire input resets.
*               :
* Notes			: NA.
* Global Variables Affected : fireFlags.fire_control_f, fireFlags.f_executing_command.
*****************************************************************************/
void Execute_Fire_Functionality(void)
{
	// Check if the fire input is detected and conditions for executing fire functionality are met
	if ((ip_fire_detect_f == FLAG_SET) && (fireFlags.fire_control_f == FLAG_RST)
	&& (iflags.command_in_process == FLAG_RST) && (emgGroup.e_executing_command == FLAG_RST))
	{
		U8 isValidFireDevice = 0;
		isValidFireDevice = Find_Fire_Devices(); // Check for valid fire devices in the system
		
		// If valid fire devices are found, execute fire functionality
		if (isValidFireDevice)
		{
			#if DEBUG_ALL || DEBUG_FIRE_FUN
			Print_Message("\nFire input detected.");
			#endif
			
			/* If the fire input is set */
			fireFlags.fire_control_f = FLAG_SET;

			
			fireFlags.f_executing_command = FLAG_SET; // Set flag to indicate fire command execution
			Clear_Whole_Group_Flags(); // Clear flags related to group functionality
			Reset_Aux_Audio();
			Set_Flags_Put_Into_Free_Access(); // Set flags to put devices into free access mode
		}
	}
	// Check if the fire input is reset and conditions for reverting fire functionality are met
	else if ((ip_fire_detect_f == FLAG_RST) && (fireFlags.fire_control_f == FLAG_SET)
	&& (iflags.command_in_process == FLAG_RST) && (emgGroup.e_executing_command == FLAG_RST))
	{
		U8 isValidFireDevice = 0;
		isValidFireDevice = Find_Fire_Devices(); // Check for valid fire devices in the system
		
		// If valid fire devices are found, revert fire functionality
		if (isValidFireDevice)
		{
			#if DEBUG_ALL || DEBUG_FIRE_FUN
			Print_Message("\nFire input Reset.");
			#endif
			
			/* If the fire input is reset */
			fireFlags.fire_control_f = FLAG_RST;
			
			fireFlags.f_executing_command = FLAG_SET; // Set flag to indicate fire command execution
			iflags.recheck_nonIlock_ips_f = FLAG_SET;
			Clear_Whole_Group_Flags(); // Clear flags related to group functionality
			Reset_Aux_Audio();
			Set_Flags_Put_Into_Normal(0); // Set flags to put devices into normal mode
			osdp_app.free_to_normal_f = FLAG_SET; // Set flag indicating transition to normal mode
		}
	}
}

/*****************************************************************************
* Function name	: U8 Find_Fire_Devices(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to get group devices and also sort the devices
*				  to an array (the emergency devices can not be put into an array).
*               :
* Notes			: NA.
* Global Variables Affected : fireGroup[grpIdx].fire_req_detect_f.
							  fireFlags.chk_op_sts_after_cmd, fireGroup[grpIdx].grp_in_free_access.
							  iLock.tx_address, iLock.tx_length.
*****************************************************************************/
U8 Find_Fire_Devices(void)
{
	U8 lcl_dev_Array[TOTAL_SLAVES] = {0}; // Array to store device addresses
	U8 lcl_idx = 0; // Index for the device array
	
	// Iterate through all groups to find those configured for fire functionality
	for (U8 grpIdx = GROUP1; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Check if the fire functionality is enabled for the current group
		if (IS_BIT_SET(grp_fire_state, grpIdx) == FLAG_SET)
		{
			// Find devices configured in the current group
			Find_Group_Devices(grpIdx);
			
			// If devices are found in the group
			if (groupInfo.length > 0)
			{
				// Sort group devices for free access mode
				Sort_Group_devices_For_Free_Access();
				
				// Check if there are devices to include for fire functionality
				if (iLock.tx_length > 0)
				{
					// Copy device addresses to the local device array
					memcpy(&lcl_dev_Array[lcl_idx], iLock.tx_address, iLock.tx_length);
					lcl_idx += iLock.tx_length;
					
					// Set fire request detect flag for the current group
					fireGroup[grpIdx].fire_req_detect_f = FLAG_SET;
					
					// Check if fire input is detected, set related flags accordingly
					if (ip_fire_detect_f == FLAG_SET)
					{
						fireFlags.chk_op_sts_after_cmd = FLAG_SET;
						fireGroup[grpIdx].grp_in_free_access = FLAG_SET;
					}
					else
					{
						fireGroup[grpIdx].grp_in_free_access = FLAG_RST;
					}
					
					#if DEBUG_ALL || DEBUG_FIRE_FUN
					Print_Message("\nFire input enabled in group : ");
					Print_Number(grpIdx + 1);
					Print_Message("\nGroup devices are : ");
					for (U8 prt = 0; prt < iLock.tx_length; prt++)
					{
						Print_Number(iLock.tx_address[prt]);
						Print_Message(",");
					}
					#endif
				}
			}
		}
	}
	
	// If there are devices for fire functionality, update the iLock structure
	if (lcl_idx > 0)
	{
		memcpy(iLock.tx_address, lcl_dev_Array, lcl_idx);
		iLock.tx_length = lcl_idx;
	}
	
	// Return 1 if there are fire devices, otherwise return 0
	return ((lcl_idx > 0)? 1 : 0);
}

/*****************************************************************************
* Function name	: void Reset_Fire_Control_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to reset the fire control flags.
*               :
* Notes			: NA.
* Global Variables Affected : fireFlags.fire_control_f.
*****************************************************************************/
void Reset_Fire_Control_Flags(void)
{
	if (ip_fire_detect_f == FLAG_SET)
	fireFlags.fire_control_f = FLAG_RST;
}

/*****************************************************************************
* Function name	: U8 Find_All_FA_Group_Devices(void)
* Returns		: U8 ---> returns 1 if there is valid device else 0.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to find all the device addresses from the each
*				  group and sort them to an array (Array updates if the device is not
*				  in emergency).
*               :
* Notes			: NA.
* Global Variables Affected : fireGroup[grpIdx].fire_req_detect_f.
*							  fireFlags.chk_op_sts_after_cmd, fireGroup[grpIdx].grp_in_free_access
							  iLock.tx_address, iLock.tx_length.
*****************************************************************************/
U8 Find_All_FA_Group_Devices(void)
{
	U8 lcl_dev_Array[TOTAL_SLAVES] = {0}; // Array to store device addresses
	U8 lcl_idx = 0; // Index for the device array
	
	// Iterate through all groups to find devices configured for free access
	for (U8 grpIdx = GROUP1; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Find devices configured in the current group
		Find_Group_Devices(grpIdx);
		
		// If devices are found in the group
		if (groupInfo.length > 0)
		{
			// Sort group devices for free access mode
			Sort_Group_devices_For_Free_Access();
			
			// Check if there are devices to include for free access
			if (iLock.tx_length > 0)
			{
				// Copy device addresses to the local device array
				memcpy(&lcl_dev_Array[lcl_idx], iLock.tx_address, iLock.tx_length);
				lcl_idx += iLock.tx_length;
				
				// Set fire request detect flag for the current group
				fireGroup[grpIdx].fire_req_detect_f = FLAG_SET;
				
				// Check if individual input detect flag is set, set related flags accordingly
				if (fireFlags.is_indInput_detect_f == FLAG_SET)
				{
					fireFlags.chk_op_sts_after_cmd = FLAG_SET;
					fireGroup[grpIdx].grp_in_free_access = FLAG_SET;
				}
				else
				{
					fireGroup[grpIdx].grp_in_free_access = FLAG_RST;
				}
			}
		}
	}
	
	// If there are devices for free access, update the iLock structure
	if (lcl_idx > 0)
	{
		memcpy(iLock.tx_address, lcl_dev_Array, lcl_idx);
		iLock.tx_length = lcl_idx;
	}
	
	// Return 1 if there are free access devices found, otherwise return 0
	return ((lcl_idx > 0)? 1 : 0);
}

/*****************************************************************************
* Function name	: U8 Find_All_AD_Group_Devices(void)
* Returns		: U8 ---> returns 1 if there is valid device else 0.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to find all the device addresses from the each
*				  group and sort them to an array (Array updates if the device is not
*				  in emergency and free access).
*               :
* Notes			: NA.
* Global Variables Affected : fireGroup[grpIdx].acsD_req_detect_f.
*							  fireFlags.chk_op_sts_after_cmd, fireGroup[grpIdx].grp_in_acs_denied
							  iLock.tx_address, iLock.tx_length.
*****************************************************************************/
U8 Find_All_AD_Group_Devices(void)
{
	U8 lcl_dev_Array[TOTAL_SLAVES] = {0}; // Array to store device addresses
	U8 lcl_idx = 0; // Index for the device array
	
	// Iterate through all groups to find devices configured for access denied
	for (U8 grpIdx = GROUP1; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Find devices configured in the current group
		Find_Group_Devices(grpIdx);
		
		// If devices are found in the group
		if (groupInfo.length > 0)
		{
			// Sort group devices for access denied mode
			Sort_Group_devices_For_Access_Denied();
			
			// Check if there are devices to include for access denied
			if (iLock.tx_length > 0)
			{
				// Copy device addresses to the local device array
				memcpy(&lcl_dev_Array[lcl_idx], iLock.tx_address, iLock.tx_length);
				lcl_idx += iLock.tx_length;
				
				// Set fire request detect flag for the current group
				fireGroup[grpIdx].acsD_req_detect_f = FLAG_SET;
				
				// Check if individual input detect flag is set, set related flags accordingly
				if (fireFlags.is_indInput_detect_f == FLAG_SET)
				{
					fireFlags.chk_op_sts_after_cmd = FLAG_SET;
					fireGroup[grpIdx].grp_in_acs_denied = FLAG_SET;
				}
				else
				{
					fireGroup[grpIdx].grp_in_acs_denied = FLAG_RST;
				}
			}
		}
	}
	
	// If there are devices for access denied, update the iLock structure
	if (lcl_idx > 0)
	{
		memcpy(iLock.tx_address, lcl_dev_Array, lcl_idx);
		iLock.tx_length = lcl_idx;
	}
	
	// Return 1 if there are access denied devices found, otherwise return 0
	return ((lcl_idx > 0)? 1 : 0);
}

/*****************************************************************************
* Function name	: U8 Find_All_DRT_Group_Devices(void)
* Returns		: U8 ---> returns 1 if there is valid device else 0.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to find all the device addresses from the each
*				  group and sort them to an array (Array updates if the device is not
*				  in emergency and free access).
*               :
* Notes			: NA.
* Global Variables Affected : fireGroup[grpIdx].grp_in_free_access
							  iLock.tx_address, iLock.tx_length.
*****************************************************************************/
U8 Find_All_DRT_Group_Devices(void)
{
	U8 lcl_dev_Array[TOTAL_SLAVES] = {0}; // Array to store device addresses
	U8 lcl_idx = 0; // Index for the device array
	
	// Iterate through all groups to find devices configured for door release for drt.
	for (U8 grpIdx = GROUP1; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		// Find devices configured in the current group
		Find_Group_Devices(grpIdx);
		
		// If devices are found in the group
		if (groupInfo.length > 0)
		{
			// Sort group devices for door release for drt mode
			Sort_Group_devices_For_DRT();
			
			// Check if there are devices to include for door release for drt
			if (iLock.tx_length > 0)
			{
				// Copy device addresses to the local device array
				memcpy(&lcl_dev_Array[lcl_idx], iLock.tx_address, iLock.tx_length);
				lcl_idx += iLock.tx_length;
			}
		}
	}
	
	// If there are devices for door release for drt, update the iLock structure.
	if (lcl_idx > 0)
	{
		memcpy(iLock.tx_address, lcl_dev_Array, lcl_idx);
		iLock.tx_length = lcl_idx;
	}
	
	// Return 1 if there are door release for drt devices found, otherwise return 0
	return ((lcl_idx > 0)? 1 : 0);
}

