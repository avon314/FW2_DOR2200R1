/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mb_osdp.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 01/12/2023.
* Module
* Description	: Updates the modbus register variables from the actual door-2100's
*				  door status, door operating state and group operating state.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 01/12/2023.
* Changes		: NA.
*****************************************************************************/

/* User Includes */
#include "mb_osdp.h"
#include "app_osdp.h"
#include "mb_tcp_server.h"
#include "user_uart.h"
#include "interlock.h"

/*****************************************************************************
* Function name	: void Send_Slave_Status_Modbus(U8 dvIdx)
* Returns		: Nonthing.
* Arguments    	: U8 dvIdx ---> (Pass device index).
* Created by	: Ranjitkumar Ainapure.
* Date created	: 01/12/2023.
*
* Description	: Function is written to update the modbus registers with the
*				  values of door status, operating status and group state.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Send_Slave_Status_Modbus(U8 dvIdx)
{
	U8 lcl_mbidx = 0;
	bool lcl_door_state_updated = FALSE;
	
	lcl_mbidx = (dvIdx * 2);
	if (slv_data[dvIdx].doorState != slvTemp_data[dvIdx].doorState)
	{
		/* IF door state updates, fill an array.*/
		slvTemp_data[dvIdx].doorState = slv_data[dvIdx].doorState;
		
		lcl_door_state_updated = TRUE;
	}
	
	if (slv_data[dvIdx].lockState != slvTemp_data[dvIdx].lockState)
	{
		/* IF lock state updates, fill an array.*/
		slvTemp_data[dvIdx].lockState = slv_data[dvIdx].lockState;
		
		lcl_door_state_updated = TRUE;
	}
	else
	{
		/*Do nothing*/
	}
	
	if (lcl_door_state_updated == TRUE)
	{
		lcl_door_state_updated = FALSE;
		
		if ((slvTemp_data[dvIdx].doorState == CLOSED) && (slvTemp_data[dvIdx].lockState == UNLOCKED))
		{
			mbTransmit.appIpRegBuf[lcl_mbidx] = 0;	/* Store Higher Byte */
			mbTransmit.appIpRegBuf[lcl_mbidx + 1] = drClosed;	/* Store lower byte */
		}
		else if ((slvTemp_data[dvIdx].doorState == OPEN) && (slvTemp_data[dvIdx].lockState == UNLOCKED))
		{
			mbTransmit.appIpRegBuf[lcl_mbidx] = 0;	/* Store Higher Byte */
			mbTransmit.appIpRegBuf[lcl_mbidx + 1] = drOpen;	/* Store lower byte */
		}
		else if ((slvTemp_data[dvIdx].doorState == CLOSED) && (slvTemp_data[dvIdx].lockState == LOCKED))
		{
			mbTransmit.appIpRegBuf[lcl_mbidx] = 0;	/* Store Higher Byte */
			mbTransmit.appIpRegBuf[lcl_mbidx + 1] = drClose_drLocked;	/* Store lower byte */
		}
		else if ((slvTemp_data[dvIdx].doorState == OPEN) && (slvTemp_data[dvIdx].lockState == LOCKED))
		{
			mbTransmit.appIpRegBuf[lcl_mbidx] = 0;	/* Store Higher Byte */
			mbTransmit.appIpRegBuf[lcl_mbidx + 1] = unDefined;	/* Store lower byte */
		}
		else
		{
			/*Do nothing*/
		}
	}
	
	for (U8 grpIdx = 0; grpIdx < NUM_OF_GROUPS; grpIdx++)
	{
		if (grpData[grpIdx].operationState != tempGrpData[grpIdx].operationState)
		{
			/* IF group operating state updates, fill an array.*/
			tempGrpData[grpIdx].operationState = grpData[grpIdx].operationState;
			
			if ((tempGrpData[grpIdx].operationState >= 0) && (tempGrpData[grpIdx].operationState < 5))
			{
				mbTransmit.appIpRegBuf[(grpIdx * 2) + ipRgGrpIdx] = 0;	/* Store Higher Byte */
				mbTransmit.appIpRegBuf[(grpIdx * 2) + 1 + ipRgGrpIdx] = tempGrpData[grpIdx].operationState;	/* Store lower byte */
			}
			else
			{
				/*Do nothing*/
			}
		}
		else
		{
			/*Do nothing*/
		}
	}
	
	if (slv_data[dvIdx].alarmState != slvTemp_data[dvIdx].alarmState)
	{
		/* IF door alarm state updates, fill an array.*/
		if (slv_data[dvIdx].alarmState < 4)
		{
			/* If there is a valid alarm state, update an array */
			if (slv_data[dvIdx].alarmState == SLV_ALRM_NOT_SET)
			slvTemp_data[dvIdx].alarmState = NoAlarm;
			else if (slv_data[dvIdx].alarmState == SLV_ALRM_FORCE_DR)
			slvTemp_data[dvIdx].alarmState = ForceDrAlarm;
			else if (slv_data[dvIdx].alarmState == SLV_ALRM_NOT_LOCKED)
			slvTemp_data[dvIdx].alarmState = DrNotLockAlarm;
			else if (slv_data[dvIdx].alarmState == SLV_ALRM_PROPPED_DR)
			slvTemp_data[dvIdx].alarmState = ProppedAlarm;
		}
		else
		slvTemp_data[dvIdx].alarmState = NoAlarm;
		
		mbTransmit.appIpRegBuf[lcl_mbidx + ipRgAlrmIdx] = 0;	/* Store Higher Byte */
		mbTransmit.appIpRegBuf[lcl_mbidx + 1 + ipRgAlrmIdx] = slvTemp_data[dvIdx].alarmState;	/* Store Lower Byte */
		
		slvTemp_data[dvIdx].alarmState = slv_data[dvIdx].alarmState;
	}
	else
	{
		/*Do nothing*/
	}
	
	if (slv_data[dvIdx].oprtState != slvTemp_data[dvIdx].oprtState)
	{
		/* If door operating state update than previous state then, update an array. */
		slvTemp_data[dvIdx].oprtState = slv_data[dvIdx].oprtState;
		
		U8 opStateValue = 0;
		opStateValue = Map_Operation_State_To_MBReg(slvTemp_data[dvIdx].oprtState);
		mbTransmit.appHoldingBuf[lcl_mbidx] = 0;	/* Store Higher Byte */
		mbTransmit.appHoldingBuf[lcl_mbidx + 1] = opStateValue;	/* Store Lower Byte */
	}
	else
	{
		/*Do nothing*/
	}
}
