/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: general_application.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 07/11/2023.
* Module
* Description	:
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
#include "general_application.h"
#include "user_uart.h"
#include "rs485_driver.h"
#include "mb_tcp_server.h"
#include "string.h"

#include "app_osdp.h"
#include "group_config.h"
#include "interlock.h"
#include "emergency.h"
#include "fire_functionality.h"
#include "digital_ip_app.h"
#include "app_aux_input.h"
#include "input_config.h"
#include "privacy_config.h"
#include "op_func.h"
#include "output_config.h"
#include "app_utility.h"
#include "door_config.h"
#include "onboard_key.h"
#include "config_mode.h"
#include "rfid_database.h"
#include "ext_eeprom.h"


/* Global Variables */
U8 pwr_oprt_state[TOTAL_SLAVES] = {0};


U8 gb_pwrfree_acs_to_normal_f[TOTAL_SLAVES] = {0};
U8 gb_oprt_at_powerOn_f = 1;
U8 gb_power_on_flag_rst = 0;
U8 gb_read_flag_val_pwrOn = 0;
U8 ip_sts_bfr_pwr_off[MAX_INPUTS] = {0};
U8 firePin_bfr_pwr_off = 0;
/*****
Structure definitions.
*****/
GEN_MEM handle_mem;

extern U8 gb_device_power_on;

/*****************************************************************************
* Function name	: void General_Task(void)
* Returns		: Nothing.
* Arguments    	: void *pvParameters.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 07/11/2023.
* Description	: Task created to execute the application related functions.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void General_Task(void *pvParameters)
{
	/* Just to avoid compiler warnings. */
	UNUSED(pvParameters);
	
	while (1)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		if (gb_config_mode_f == FALSE)
		{
			if (gb_device_power_on == FLAG_SET)
			{
				gb_device_power_on = FLAG_RST; // Reset the device power-on flag
				
				osdp_app.gb_poll_time = POLL_DELAY; // Set the poll delay when the device powers on
				Find_Polling_Devices(); // Find devices to include in polling
				
				// Populate the iLock structure with device addresses from the system
				iLock.tx_length = 0;
				for (U8 idx = 0; idx < polling_dev_len; idx++)
				{
					iLock.tx_address[idx] = PollDevice[idx].slv_addr;
					iLock.tx_length++;
				}
				
				// Find the iLock sequence of each group
				for (U8 grpIdx = 0; grpIdx < MAX_GROUPS; grpIdx++)
				{
					Find_iLockSequence_Of_Group(grpIdx);
				}
				
				// Check if fire input is enabled in any group
				Is_Fire_Input_Enabled_InGroup();
				
				// Check if emergency input is enabled in any group
				Is_EMG_Input_Enabled_InGroup();
				
				// Read the input status and operation state at power on.
				Set_Flags_To_Check_IP_Status();
			}
			
			Process_Interlocking();					/*Handles the process of interlocking between devices.*/
			Keep_Monotoring_IPStatus();				/*Continuously monitors the IP status of devices.*/
			Operation_State_AUX_Input_Action();
			Force_Interlock_By_DoorState();			/*Checks for any force interlock conditions, if error in door state.*/
			Force_Interlock_By_NoFrame();			/*Checks for any force interlock conditions, if frame nor received.*/
			Check_Emergency_Signal();				/*Checks for any emergency signals received.*/
			Check_Fire_Input_detection();			/*Monitors and checks for fire input detection.*/
			Execute_Fire_Functionality();			/*Executes the functionality related to fire detection.*/
			Check_EMG_Input_Detection();			/*Checks for emergency input detection.*/
			Check_AuxInput_Signal();				/*Monitors and checks for signals from auxiliary inputs.*/
			Input_1_Functionality();				/*Perform specific functionalities related to input signal 1*/
			Input_2_Functionality();				/*Perform specific functionalities related to input signal 2*/
			Input_3_Functionality();				/*Perform specific functionalities related to input signal 3*/
			Input_4_Functionality();				/*Perform specific functionalities related to input signal 4*/
			Input_5_Functionality();				/*Perform specific functionalities related to input signal 5*/
			Input_6_Functionality();				/*Perform specific functionalities related to input signal 6*/
			Keep_Monitoring_IP_And_TakeAction();	/*Continuously monitors IP status and takes corresponding actions.*/
			Monitor_Flags_After_Normal();			/*Monitors flags after the system returns to normal state.*/
			Output_1_Functionality();				/*Perform specific functionalities related to output signal 1*/
			Output_2_Functionality();				/*Perform specific functionalities related to output signal 2*/
			Output_3_Functionality();				/*Perform specific functionalities related to output signal 3*/
			Output_4_Functionality();				/*Perform specific functionalities related to output signal 4*/
			operate_relay_1_mom();					/*Operates relay in momentary mode of output 1*/
			operate_relay_2_mom();					/*Operates relay in momentary mode of output 2*/
			operate_relay_3_mom();					/*Operates relay in momentary mode of output 3*/
			operate_relay_4_mom();					/*Operates relay in momentary mode of output 4*/
			Set_Default_State_NO_NC();				/*Sets default states for (NO) and (NC) configurations.*/
			Match_Operation_Status_For_Output();	/*Matches the operation status for outputs.*/
			Get_mbHolding_Reg_Data();				/*Retrieves data from Modbus Holding Registers.*/
			Monitor_Key_For_Default_IPAddress();	/*Monitors keys for setting the default IP address.*/
			Validate_Cards();						
			Master_General_Purpose_Mem_Handling();	/*Store operation state at the eeprom.*/
			Handle_Master_PowerOn_State();			/*Manages device's operation state if matser power resets*/
			Reset_devNoResponse_flags();
		}
		vTaskDelay(1);
	}
}

/*****************************************************************************
* Function name	: void Master_General_Purpose_Mem_Handling(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/07/2023.
* Description	: Store slave devices operation state, free access activate by
*				  the device & group emergency state.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Master_General_Purpose_Mem_Handling(void)
{
	for (U8 grpIdx = 0; grpIdx < MAX_GROUPS; grpIdx ++)
	{
		if ((emgGroupArray[grpIdx].group_in_emergency == FLAG_SET) && (handle_mem.emg_f[grpIdx] == FLAG_RST))
		{
			handle_mem.emg_f[grpIdx] = FLAG_SET;
			
			/*****
			Write to eeprom that group emergency state is set.
			*****/
			U8 GRP_EMG_SET = 1;
			eeprom_write_frame(GEN_MEM_BASE_ADD + grpIdx, &GRP_EMG_SET, 1);
		}
		else if ((emgGroupArray[grpIdx].group_in_emergency == FLAG_RST) && (handle_mem.emg_f[grpIdx] == FLAG_SET))
		{
			handle_mem.emg_f[grpIdx] = FLAG_RST;
			
			/*****
			Write to eeprom that group emergency state is reset.
			*****/
			U8 GRP_EMG_RESET = 0;
			eeprom_write_frame(GEN_MEM_BASE_ADD + grpIdx, &GRP_EMG_RESET, 1);
		}
	}
	
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx ++)
	{
		if ((fireFlags.free_acs_active_by[dvIdx] == FLAG_SET) && (handle_mem.freeacs_set_f[dvIdx] == FLAG_RST))
		{
			handle_mem.freeacs_set_f[dvIdx] = FLAG_SET;
			
			/*****
			Write to eeprom that free access is set by the device index.
			*****/
			U8 DEV_FA_SET = 1;
			eeprom_write_frame(GEN_MEM_FA_ACT_BY + dvIdx, &DEV_FA_SET, 1);
		}
		else if ((fireFlags.free_acs_active_by[dvIdx] == FLAG_RST) && (handle_mem.freeacs_set_f[dvIdx] == FLAG_SET))
		{
			handle_mem.freeacs_set_f[dvIdx] = FLAG_RST;
			
			/*****
			Write to eeprom that free access is reset by the device index.
			*****/
			U8 DEV_FA_RESET = 0;
			eeprom_write_frame(GEN_MEM_FA_ACT_BY + dvIdx, &DEV_FA_RESET, 1);
		}
	}
	
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx ++)
	{		
		if (pwr_oprt_state[dvIdx] != slv_data[dvIdx].oprtState)
		{
			pwr_oprt_state[dvIdx] = slv_data[dvIdx].oprtState;
			
			/*****
			Write to eeprom that operation state is updated by the device index.
			*****/
			eeprom_write_frame(GEN_MEM_OP_STATE + dvIdx, &pwr_oprt_state[dvIdx], 1);
		}
	}
	
	static U8 ip1_flag_control;
	static U8 ip2_flag_control;
	static U8 ip3_flag_control;
	static U8 ip4_flag_control;
	static U8 ip5_flag_control;
	static U8 ip6_flag_control;
	static U8 pwron_fire_control;
	
	U8 ipIdxMem = 0;
	if ((ip_control.ip1_control_f == FLAG_SET) && (ip1_flag_control == FLAG_RST))
	{
		ip1_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip1 state is set.
		*****/
		U8 IP1_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP1_SET, 1);
	}
	else if ((ip_control.ip1_control_f == FLAG_RST) && (ip1_flag_control == FLAG_SET))
	{
		ip1_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip1 state is reset.
		*****/
		U8 IP1_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP1_RESET, 1);
	}
	ipIdxMem ++;
	
	if ((ip_control.ip2_control_f == FLAG_SET) && (ip2_flag_control == FLAG_RST))
	{
		ip2_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip2 state is set.
		*****/
		U8 IP2_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP2_SET, 1);
	}
	else if ((ip_control.ip2_control_f == FLAG_RST) && (ip2_flag_control == FLAG_SET))
	{
		ip2_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip2 state is reset.
		*****/
		U8 IP2_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP2_RESET, 1);
	}
	ipIdxMem ++;
	
	if ((ip_control.ip3_control_f == FLAG_SET) && (ip3_flag_control == FLAG_RST))
	{
		ip3_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip3 state is set.
		*****/
		U8 IP3_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP3_SET, 1);
	}
	else if ((ip_control.ip3_control_f == FLAG_RST) && (ip3_flag_control == FLAG_SET))
	{
		ip3_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip3 state is reset.
		*****/
		U8 IP3_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP3_RESET, 1);
	}
	ipIdxMem ++;
	
	if ((ip_control.ip4_control_f == FLAG_SET) && (ip4_flag_control == FLAG_RST))
	{
		ip4_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip4 state is set.
		*****/
		U8 IP4_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP4_SET, 1);
	}
	else if ((ip_control.ip4_control_f == FLAG_RST) && (ip4_flag_control == FLAG_SET))
	{
		ip4_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip4 state is reset.
		*****/
		U8 IP4_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP4_RESET, 1);
	}
	ipIdxMem ++;
	
	if ((ip_control.ip5_control_f == FLAG_SET) && (ip5_flag_control == FLAG_RST))
	{
		ip5_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip5 state is set.
		*****/
		U8 IP5_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP5_SET, 1);
	}
	else if ((ip_control.ip5_control_f == FLAG_RST) && (ip5_flag_control == FLAG_SET))
	{
		ip5_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip5 state is reset.
		*****/
		U8 IP5_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP5_RESET, 1);
	}
	ipIdxMem ++;
	
	if ((ip_control.ip6_control_f == FLAG_SET) && (ip6_flag_control == FLAG_RST))
	{
		ip6_flag_control = FLAG_SET;
		/*****
		Write to eeprom that ip6 state is set.
		*****/
		U8 IP6_SET = 1;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP6_SET, 1);
	}
	else if ((ip_control.ip6_control_f == FLAG_RST) && (ip6_flag_control == FLAG_SET))
	{
		ip6_flag_control = FLAG_RST;
		/*****
		Write to eeprom that ip6 state is reset.
		*****/
		U8 IP6_RESET = 0;
		eeprom_write_frame(GEN_MEM_IP_STATE + ipIdxMem, &IP6_RESET, 1);
	}
	
	/************** FOR FIRE CONTROL FLAG  ************/
	if ((fireFlags.fire_control_f == FLAG_SET) && (pwron_fire_control == FLAG_RST))
	{
		pwron_fire_control = FLAG_SET;
		/*****
		Write to eeprom that fire ip state is set.
		*****/
		U8 FIRE_SET = 1;
		eeprom_write_frame(GEN_MEM_FIRE_STATE, &FIRE_SET, 1);
	}
	else if ((fireFlags.fire_control_f == FLAG_RST) && (pwron_fire_control == FLAG_SET))
	{
		pwron_fire_control = FLAG_RST;
		/*****
		Write to eeprom that fire ip state is reset.
		*****/
		U8 FIRE_RESET = 0;
		eeprom_write_frame(GEN_MEM_FIRE_STATE, &FIRE_RESET, 1);
	}
}

/*****************************************************************************
* Function name	: void Read_Memory_For_PowerON_State(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/07/2023.
* Description	: Read slave devices operation state, free access activate by
*				  the device & group emergency state at the power on.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Read_Memory_For_PowerON_State(void)
{
	U8 grpEmg[MAX_GROUPS] = {0};
	eeprom_read_frame(GEN_MEM_BASE_ADD, grpEmg, MAX_GROUPS);
	
	for (U8 grpIdx = 0; grpIdx < MAX_GROUPS; grpIdx ++)
	{
		//grpEmg[grpIdx];
	}
	
	
	U8 freeAcs_was_act_by[TOTAL_SLAVES] = {0};
	eeprom_read_frame(GEN_MEM_FA_ACT_BY, freeAcs_was_act_by, TOTAL_SLAVES);
	
	for (U8 dvIdx = 0; dvIdx < TOTAL_SLAVES; dvIdx ++)
	{
		fireFlags.free_acs_active_by[dvIdx] = freeAcs_was_act_by[dvIdx];
	}
	
	U8 devOpState[TOTAL_SLAVES] = {0};
	eeprom_read_frame(GEN_MEM_OP_STATE, devOpState, TOTAL_SLAVES);
	
	for (U8 devIdx = 0; devIdx < TOTAL_SLAVES; devIdx ++)
	{
		slv_data[devIdx].oprtState = devOpState[devIdx];
		pwr_oprt_state[devIdx] = devOpState[devIdx];
	}
	
	//U8 ip_sts_bfr_pwr_off[MAX_INPUTS] = {0};
	eeprom_read_frame(GEN_MEM_IP_STATE, ip_sts_bfr_pwr_off, MAX_INPUTS);
	
	//firePin_bfr_pwr_off;
	eeprom_read_frame(GEN_MEM_FIRE_STATE, &firePin_bfr_pwr_off, 1);
}

/*****************************************************************************
* Function name	: void Handle_Master_PowerOn_State(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/07/2023.
* Description	: Takes an action for slave devices to set operation state at
*				  Masters power reset.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Handle_Master_PowerOn_State(void)
{
	U8 lcl_dev_len = 0;
	U8 lcl_dev_list[TOTAL_SLAVES] = {0};
	U8 lcl_high_prio_f = 0;
	
	for (U8 devIdx = 0; devIdx < TOTAL_SLAVES; devIdx ++)
	{
		if ((gb_oprt_at_powerOn_f == FLAG_SET)
		&& (iflags.command_in_process == FLAG_RST)
		&& (emgGroup.e_executing_command == FLAG_RST)
		&& (fireFlags.f_executing_command == FLAG_RST)
		&& (iflags.ad_executing_command == FLAG_RST))
		{
			if (slv_data[devIdx].oprtState == SLV_OP_STATE_EMERGENCY)
			{
				/* If device is already in an emergency, do not put into a normal state. */
				if (lcl_high_prio_f != 2)
				lcl_high_prio_f = 1;
			}
			else if (slv_data[devIdx].oprtState == SLV_OP_STATE_FREE_ACCESS)
			{
				/* If device is already in a free access, do not put into a normal state. */
				if (lcl_high_prio_f != 2)
				lcl_high_prio_f = 1;
			}
			else if ((slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL)
			&& (fireFlags.free_acs_active_by[devIdx] == FLAG_SET)
			&& (slv_data[devIdx].ipTypeState == FLAG_RST))
			{
				/*****
					Master become power off, slave power resets and enter to normal state, and free
					access was activated by this particular device, and now aux input become reset.
					Master power ON.
				*****/
				gb_pwrfree_acs_to_normal_f[devIdx] = FLAG_SET;
				gb_power_on_flag_rst = FLAG_RST;
			
				fireFlags.free_acs_active_by[devIdx] = 0;
			
				// Set recheck flag and clear all group flags
				iflags.recheck_nonIlock_ips_f = FLAG_SET;
				Clear_Whole_Group_Flags();
			
				// Find the group index and devices in the group
				U8 lcl_gpIdx = 0;
				lcl_gpIdx = Find_Device_Group(inSysDeviceList[devIdx].slv_addr);
				Find_Group_Devices(lcl_gpIdx);
			
				// If devices are found in the group, sort them for normal state
				if (groupInfo.length > 0)
				{
					Sort_Group_devices_For_Normal_state();
				
					// If there are devices to be put into normal state
					if (iLock.tx_length > 0)
					{
						// Set fire request flag and reset group flags
						fireGroup[lcl_gpIdx].fire_req_detect_f = FLAG_SET;
						fireGroup[lcl_gpIdx].grp_in_free_access = FLAG_RST;
						fireGroup[lcl_gpIdx].grp_in_acs_denied = FLAG_RST;
						fireFlags.reset_from_aux_ip = FLAG_SET; // Set reset from aux input flag

						// Find the device index and reset the free access flag
						fireFlags.free_acs_active_by[devIdx] = FLAG_RST;

						// Debug messages if enabled
						#if DEBUG_ALL || DEBUG_AUX_INPUT
						Print_Message("\nGroup devices to normal are : ");
						for (U8 dx = 0; dx < iLock.tx_length; dx++)
						{
							Print_Number(iLock.tx_address[dx]);
							Print_Message(",");
						}
						#endif

						// Get auxiliary audio value and set flags to put devices into normal state
						Get_Slave_Aux_Audio_Value(inSysDeviceList[devIdx].slv_addr);
						Set_Flags_Put_Into_Normal(inSysDeviceList[devIdx].slv_addr);
						osdp_app.free_to_normal_f = FLAG_SET; // Set flag indicating free to normal state
					}
					else
					{
						ReAssign_Whole_Group_Flags(); // Reassign whole group flags
					}
				}
			
				break;
			}
			else if ((slv_data[devIdx].oprtState == SLV_OP_STATE_NORMAL)
			&& (fireFlags.free_acs_active_by[devIdx] == FLAG_SET)
			&& (slv_data[devIdx].ipTypeState == FLAG_SET))
			{
				/*****
					Master become power off, slave power resets and enter to normal state, and free
					access was activated by this particular device, and now aux input become reset.
					Master power ON.
				*****/
				//gb_pwrfree_acs_to_normal_f[devIdx] = FLAG_SET;
			
				//fireFlags.free_acs_active_by[devIdx] = 0;
			
				break;
			}
			else
			{
 				if (inSysDeviceList[devIdx].slv_addr > 0)
				{
					gb_power_on_flag_rst = FLAG_SET;
					lcl_high_prio_f = 2;
					lcl_dev_list[lcl_dev_len ++] = inSysDeviceList[devIdx].slv_addr;
				}
			}
		}
	}
	
	if ((lcl_dev_len > 0)
	&& (gb_power_on_flag_rst == FLAG_SET)
	&& (iflags.command_in_process == FLAG_RST))
	{
		gb_power_on_flag_rst = FLAG_RST;
		gb_oprt_at_powerOn_f = FLAG_RST;
		gb_read_flag_val_pwrOn = FLAG_SET;
		
		memcpy(iLock.tx_address, lcl_dev_list, lcl_dev_len);
		iLock.tx_length = lcl_dev_len;
		
		osdp_app.gb_enable_poll = 0; // Disable polling for now
		iflags.put_into_normal_state = FLAG_SET; // Set the system into normal state
		iflags.chk_normal_state_ack = FLAG_SET; // Set flag to check for acknowledgment of normal state
		iflags.command_in_process = FLAG_SET; // Set flag indicating that a command is in process
	}
	else if (lcl_high_prio_f == 1)
	{
		lcl_high_prio_f = 0;
		
		Handle_PowerOn_FireControl_Flags();
		Handle_PowerOn_Input_ILock_Flags();
		Handle_PowerOn_Input_NonILock_Flags();
	}
}

void Handle_PowerOn_FireControl_Flags(void)
{
	if ((digInput.ip1_lth_f == TRUE) && (firePin_bfr_pwr_off == FLAG_SET))
	{
		/* If fire toggle pin reset at power on, set below flag. */
		fireFlags.fire_control_f = FLAG_SET;
	}
}

void Handle_PowerOn_Input_ILock_Flags(void)
{
	if ((gb_input[IP_1_IDX].ip_en == FLAG_SET) && (gb_input[IP_1_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip1_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_1_IDX] == FLAG_SET))
		ip_control.ip1_control_f = FLAG_SET;
	}
	
	if ((gb_input[IP_2_IDX].ip_en == FLAG_SET) && (gb_input[IP_2_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip2_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_2_IDX] == FLAG_SET))
		ip_control.ip2_control_f = FLAG_SET;
	}
	
	if ((gb_input[IP_3_IDX].ip_en == FLAG_SET) && (gb_input[IP_3_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip3_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_3_IDX] == FLAG_SET))
		ip_control.ip3_control_f = FLAG_SET;
	}
	
	if ((gb_input[IP_4_IDX].ip_en == FLAG_SET) && (gb_input[IP_4_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip4_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_4_IDX] == FLAG_SET))
		ip_control.ip4_control_f = FLAG_SET;
	}
	
	if ((gb_input[IP_5_IDX].ip_en == FLAG_SET) && (gb_input[IP_5_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip5_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_5_IDX] == FLAG_SET))
		ip_control.ip5_control_f = FLAG_SET;
	}
	
	if ((gb_input[IP_6_IDX].ip_en == FLAG_SET) && (gb_input[IP_6_IDX].ip_for_interlock == IP_FOR_INTERLOCK))
	{
		if ((gb_ip6_func == IP_ILOCK_N) && (ip_sts_bfr_pwr_off[IP_6_IDX] == FLAG_SET))
		ip_control.ip6_control_f = FLAG_SET;
	}
}

void Handle_PowerOn_Input_NonILock_Flags(void)
{
	if ((gb_input[IP_1_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_1_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip3_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_1_IDX] == FLAG_SET))
		{
			ip_control.ip1_control_f = FLAG_SET;
		}
	}
	
	if ((gb_input[IP_2_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_2_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip4_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_2_IDX] == FLAG_SET))
		{
			ip_control.ip2_control_f = FLAG_SET;
		}
	}
	
	if ((gb_input[IP_3_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_3_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip5_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_3_IDX] == FLAG_SET))
		{
			ip_control.ip3_control_f = FLAG_SET;
		}
	}
	
	if ((gb_input[IP_4_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_4_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip6_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_4_IDX] == FLAG_SET))
		{
			ip_control.ip4_control_f = FLAG_SET;
		}
	}
	
	if ((gb_input[IP_5_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_5_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip7_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_5_IDX] == FLAG_SET))
		{
			ip_control.ip5_control_f = FLAG_SET;
		}
	}
	
	if ((gb_input[IP_6_IDX].ip_en == FLAG_SET)
	&& (gb_input[IP_6_IDX].ip_for_interlock != IP_FOR_INTERLOCK))
	{
		if ((digInput.ip8_lth_f == TRUE) && (ip_sts_bfr_pwr_off[IP_6_IDX] == FLAG_SET))
		{
			ip_control.ip6_control_f = FLAG_SET;
		}
	}
}