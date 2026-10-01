/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: general_application.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 07/11/2023.
* Module
* Description	: Header file for general_application.c
				  Defines constants and macros for general_application.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 07/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef GENERAL_APPLICATION_H_
#define GENERAL_APPLICATION_H_

/* System Includes */
#include "asf.h"

#include "group_config.h"
#include "app_osdp.h"
/*****
Structure declarations for General purpose memory handling.
*****/
typedef struct
{
	U8 fire_flag : 1;
	U8 emg_f[MAX_GROUPS];
	U8 freeacs_set_f[TOTAL_SLAVES];
}GEN_MEM;

extern GEN_MEM handle_mem;

extern U8 gb_pwrfree_acs_to_normal_f[TOTAL_SLAVES];
extern U8 gb_oprt_at_powerOn_f;
extern U8 gb_power_on_flag_rst;
extern U8 gb_read_flag_val_pwrOn;

/***** Function declaration / prototypes *****/
void General_Task(void *pvParameters);
void Master_General_Purpose_Mem_Handling(void);
void Read_Memory_For_PowerON_State(void);
void Handle_Master_PowerOn_State(void);
void Handle_PowerOn_FireControl_Flags(void);
void Handle_PowerOn_Input_ILock_Flags(void);
void Handle_PowerOn_Input_NonILock_Flags(void);
#endif /* GENERAL_APPLICATION_H_ */