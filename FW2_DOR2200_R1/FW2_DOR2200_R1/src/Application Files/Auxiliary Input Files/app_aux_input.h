/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_aux_input.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: Header file for app_aux_input.c
				  Defines constants and macros for app_aux_input.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NO.
*****************************************************************************/
#ifndef APP_AUX_INPUT_H_
#define APP_AUX_INPUT_H_

/* User Includes */
#include "app_osdp.h"

/* Structure declaration */
typedef struct
{
	U8 enter_to_privacy_f : 1;
}PRV_DATA;

/* Global structure variable */
extern PRV_DATA priGroup[TOTAL_SLAVES];

/* Function Prototypes */
void Check_AuxInput_Signal(void);
void Set_Reset_Privacy_State(U8 devAddress);
void Set_Flags_Put_Into_Privacy(void);
void Set_Flags_Put_Into_Free_Access(void);
void Set_Flags_Put_SD_Into_Free_Access(void);
void Set_Flags_Put_SD_Into_Access_Denied(void);
void Set_Flags_Put_Into_Access_Denied(void);
void Sort_Group_devices_For_Free_Access(void);
void Sort_Group_devices_For_Access_Denied(void);
void Sort_Group_devices_For_Normal_state(void);
void Sort_Group_devices_For_DRT(void);
void Get_Slave_Aux_Audio_Value(U8 slvAddress);
void Reset_Aux_Audio(void);
void Put_Normal_From_FreeAccess(U8 slvAddress);
void Update_Array_In_Ascending_Order(U8 *arr, U8 len);
U8 Remove_Duplicate_Slaves(U8 *arr, U8 len);
U8 Is_Device_Valid(U8 lclDevIdx);
U8 Find_Privacy_Group_Index(U8 devAddress);
U8 Find_Privacy_Group_Devices(U8 slvAddress);

#endif /* APP_AUX_INPUT_H_ */