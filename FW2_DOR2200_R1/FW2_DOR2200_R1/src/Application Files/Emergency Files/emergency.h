/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: emergency.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: Header file for emergency.c
				  Defines constants and macros for emergency.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NO.
*****************************************************************************/
#ifndef EMERGENCY_H_
#define EMERGENCY_H_

#include "app_osdp.h"
#include "interlock.h"

/* General macros */
#ifndef FLAG_DETECT
#define FLAG_DETECT	(1)
#endif
#ifndef FLAG_RST
#define FLAG_RST	(0)
#endif

typedef struct
{
	U8 send_emg_command_f : 1;
	U8 reset_emg_command_f : 1;
	U8 chkEmgAckFlag : 1;
	U8 chk_sd_emgAck_flag : 1;
	U8 chk_fip_after_emgRst_f : 1;
	U8 e_executing_command : 1;
	U8 tx_length;
	U8 slvAddress;
	U8 tx_address[TOTAL_SLAVES];
	U8 emg_act_by[TOTAL_SLAVES];
}EMGFLAG;

typedef struct  
{
	U8 emg_detect_f : 1;
	U8 group_in_emergency : 1;
}EMGARRAY;

extern EMGFLAG emgGroup;
extern EMGARRAY emgGroupArray[NUM_OF_GROUPS];
extern U8 emergency_flag_check;

/***** Function Prototypes *****/
void Check_Emergency_Signal(void);
void Find_Emergency_Group_Devices(U8 slvAddress);
void Clear_Whole_Group_Flags(void);
void ReAssign_Whole_Group_Flags(void);
void Reset_Device_Specific_Flags(U8 grupIdx);
U8 Find_EMG_Devices_With_IP_PIN(void);
#endif /* EMERGENCY_H_ */