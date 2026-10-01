/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mb_osdp.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 01/12/2023.
* Module
* Description	: Header file for mb_osdp.c
				  Defines constants and macros for mb_osdp.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 01/12/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef MB_OSDP_H_
#define MB_OSDP_H_

/* System Includes */
#include "asf.h"

/* Constant Variables */
enum
{
	drClosed,
	drOpen,
	drClose_drLocked,
	unDefined/*drReleased*/
};

/* Constant Variables */
enum
{
	NoAlarm,
	ProppedAlarm,
	DrNotLockAlarm,
	ForceDrAlarm
};

/* Function Prototypes */
void Send_Slave_Status_Modbus(U8 dvIdx);


#endif /* MB_OSDP_H_ */