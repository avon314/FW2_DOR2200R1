/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: interlock.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 02/12/2023.
* Module
* Description	: Header file for interlock.c
				  Defines constants and macros for interlock.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 02/12/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef INTERLOCK_H_
#define INTERLOCK_H_

/* User Includes */
#include "app_osdp.h"

/***** NUMBER OF GROUP VALUE *****/
#define NUM_OF_GROUPS			(8)
/***** END OF NUMBER OF GROUP VALUE *****/

/***** DOOR STATUS VALUES *****/
#define CLOSED		(0)
#define OPEN		(1)
#define LOCKED		(0)
#define UNLOCKED	(1)
/***** END OF DOOR STATUS VALUES *****/

/***** Macros for Group operational state *****/
#define GRP_IN_NORMAL			0
#define GRP_IN_AN_INTERLOCK		1
#define GRP_IN_AN_EMERGENCY		2
#define GRP_IN_FREE_ACCESS		3
#define	GRP_IN_ACCESS_DENIED	4
#define GRP_STATE_CANT_DEFINE	0xFF
/***** End of macros for Group operational state *****/

/***** STRUCTURE VARIABLES *****/
typedef struct
{
	U8 ilockSeq[TOTAL_SLAVES];
	U8 numOfiLckDevs;
	U16 defItdValue;
}INFO_ILOCK;

typedef struct
{
	U8 devices[TOTAL_SLAVES];
	U8 numOfDevs;
	U8 operationState;
	U8 command_executing_f;
	INFO_ILOCK iLockInfo[TOTAL_SLAVES];
}INFO_GROUP;

typedef struct
{
	U8 length;
	U8 slaves[TOTAL_SLAVES];
}GROUP_DEV_INFO;

/***** Structure variable to handle the flag variables *****/
typedef struct
{
	U8 chk_ip_status				: 1;
	U8 chk_ip_sts_ack				: 1;
	U8 chk_oprt_status				: 1;
	U8 chk_opt_sts_ack				: 1;
	U8 ip_sts_read_success			: 1;
	U8 opt_sts_read_success			: 1;
	U8 send_osdp_out_cmd			: 1;
	U8 chk_osdp_out_ack				: 1;
	U8 chk_drt_osdp_out_ack			: 1;
	U8 put_into_interlock			: 1;
	U8 chk_osdp_ilck_ack			: 1;
	U8 ilock_by_input_f				: 1;
	U8 chk_slvdrActive_state		: 1;
	U8 doorActiveState				: 1;
	U8 chk_itd_time_value			: 1;
	U8 itd_timer_running			: 1;
	U8 chk_for_normal_state			: 1;
	U8 put_into_normal_state		: 1;
	U8 chk_normal_state_ack			: 1;
	U8 command_in_process			: 1;
	U8 rst_noRsp_control_f			: 1;
	U8 chk_noRsp_f_aft_freeRst		: 1;
	U8 ad_executing_command			: 1;
	U8 dps_lfb_error_f				: 1;
	U8 gb_noResponse_f				: 1;	/* Flag used to set/reset, even after defined number of retrials. */
	//U8 is_dev_online				: 1;	/* Flag used check device is become on line or not? */
	U8 drLock_control_f				: 1;
	U8 noFrame_ctrl_f				: 1;
	U8 ilock_by_force_dev_f			: 1;
	U8 door_req_flag				: 1;
	U8 executing_dr_request_f		: 1;
	U8 chk_ip_sts_after_itd			: 1;
	U8 is_action_dev_normal			: 1;
	U8 chk_flags_after_normal		: 1;
	U8 chk_ip_after_normal			: 1;
	U8 chk_oprt_sts_after_normal	: 1;
	U8 reset_from_iLock_f			: 1;
	U8 rd_ip_sts_for_one_dr			: 1;
	U8 chk_for_next_door_close		: 1;
	U8 send_drt_osdp_out_cmd		: 1;
	U8 recheck_nonIlock_ips_f		: 1;
	U8 oprt_state_ax_ip_control_f	: 1;
	U8 slv_in_prvc_N_pwr_UP			: 1;
	U8 chk_rel_cycle_f				: 1;	/* Op-state read of a released door is in progress (iflags only). */
	U8 ilock_by_device;
	
	/*	Written from the 1 ms timer ISR (Match_ITD_Counts). Kept out of the bit
		fields so an ISR update is not lost in a task's read-modify-write. */
	volatile U8 start_itd_time;
	volatile U8 itd_time_completed;
	
	U8 temp_f;
}ILCKFLAG;

typedef struct
{
	U8 actionSlave;
	U8 tx_address[TOTAL_SLAVES];
	U8 utx_address[TOTAL_SLAVES];
	U8 rst_address[TOTAL_SLAVES];
	U8 tx_length;
	U16 itdValue;
	U16 itdCounts;
}ILCK;

typedef struct
{
	U8 ip1_control_f : 1;
	U8 ip2_control_f : 1;
	U8 ip3_control_f : 1;
	U8 ip4_control_f : 1;
	U8 ip5_control_f : 1;
	U8 ip6_control_f : 1;
}IP_CNTRL;

typedef struct
{
	U8 normal_f		: 1;
	U8 free_acs_f	: 1;
	U8 acs_dnd_f	: 1;
	U8 emg_f		: 1;
	U8 prvc_f		: 1;
}MBFLAGS;

/***** Extern / Global variables *****/
extern INFO_GROUP grpData[NUM_OF_GROUPS];
extern INFO_GROUP tempGrpData[NUM_OF_GROUPS];
extern GROUP_DEV_INFO groupInfo;
extern ILCKFLAG iflags;
extern ILCKFLAG iDeviceFlag[TOTAL_SLAVES];
extern ILCKFLAG copyiflags;
extern ILCKFLAG copyiDeviceFlag[TOTAL_SLAVES];
extern ILCK iLock;
extern ILCK iLockDevice[TOTAL_SLAVES];
extern IP_CNTRL ip_control;
extern MBFLAGS mbGetFlags[TOTAL_SLAVES];

/***** Function Prototypes *****/
void Find_iLockSequence_Of_Group(U8 grpNmIdx);
void Process_Interlocking(void);
void Find_Main_iLock_Sequence_With(U8 slvAddress);
void Find_iLock_Sequence();
void Find_Reset_Devices_With(U8 slvAddress);
void Interlock_Time_Delay(void);
void Match_ITD_Counts(U8 idvIdx);
void Keep_Monotoring_IPStatus(void);
void Force_Interlock_By_DoorState(void);
void Force_Interlock_By_NoFrame(void);
void Reset_From_Interlock(U8 slvAddress);
void Find_Group_Devices(U8 lcl_gpIdx);
void Find_iLock_Sequence_And_Self(U8 slvAddress);
void Set_Flags_Put_Into_Interlock(U8 slvAddress);
void Set_Flags_Put_Into_Normal(U8 slvAddress);
void Keep_Monitoring_IP_And_TakeAction(void);
void Interlock_By_The_Input_Number(U8 inpIdx);
void Normal_By_The_Input_Number(U8 inpIdx);
void Reset_Input_ILock_Flags(void);
void Reset_Input_NonILock_Flags(void);
void Reset_DoorIP_Control_Flags(void);
void Get_Group_Operation_State(void);
void Set_Flags_To_Check_IP_Status(void);
void Set_Flags_To_Check_OPRT_Status(void);
void Set_Flags_To_Send_OSDP_OUT(void);
void Monitor_Flags_After_Normal(void);
void Get_mbHolding_Reg_Data(void);
void Get_Door_Access(U8 slvAddress);
void Handle_Slave_Dev_Response_PowerUp(U8);
void Operation_State_AUX_Input_Action(void);
void Handle_Oprtion_State_On_AuxInput(U8 devIdx);
void Take_An_Action_On_Group_AccessD_State(U8 dvIdx, U8 grupIdx);
void Group_DR_To_Normal(U8 grpNIdx);
void Group_AD_To_Normal(U8 grp_adn_Idx);
void Group_DoorRelease(U8 grp_dr_Idx);
void Group_AccessDenied(U8 grp_ad_Idx);
void Door_AD_To_Normal(U8 dr_adn_Idx);
bool Find_iLockSequence_With_Input(U8 IpIdx);
U8 Check_Any_DR_Command_Executing(U8 devIdx);
U8 No_Priority_Flags_Set(void);
U8 Is_IP1_Control_Flag_Reset(void);
U8 Is_IP1_Control_Flag_Set(void);
U8 Is_IP2_Control_Flag_Reset(void);
U8 Is_IP2_Control_Flag_Set(void);
U8 Is_IP3_Control_Flag_Reset(void);
U8 Is_IP3_Control_Flag_Set(void);
U8 Is_IP4_Control_Flag_Reset(void);
U8 Is_IP4_Control_Flag_Set(void);
U8 Is_IP5_Control_Flag_Reset(void);
U8 Is_IP5_Control_Flag_Set(void);
U8 Is_IP6_Control_Flag_Reset(void);
U8 Is_IP6_Control_Flag_Set(void);
U8 Is_Device_Normal_And_Other_Flags_Reset(U8 devIdx, U8 grpIndx);
U8 Find_Device_Group(U8 devAddress);
U8 Find_Device_Index_InGroup(U8 gpIdx, U8 devAddress);
U8 Number_Of_Doors_InGroup(U8 gpNumber, U16 drdata);
U8 Count_Set_Bits(U16 data);
U8 Fill_Array_With_iLock_Devices(U8* UpdArray, U8 lenUpArr, U8* oldArray, U8 lenOldArr);
U8 Determine_Group_Operation_state(void);
U8 Validate_Flags_For_OPRT_STS_AUX_IP_Control(U8 rcdvIdx, U8 rcgrpIdx);
U8 Is_Door_In_Release_Cycle(U8 devIdx);
U8 Get_Effective_Oprt_State(U8 devIdx);
void Start_Release_Cycle_Check(U8 slvAddress);
void End_Release_Cycle(U8 devIdx);
#endif /* INTERLOCK_H_ */
