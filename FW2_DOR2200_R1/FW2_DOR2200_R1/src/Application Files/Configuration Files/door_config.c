/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: door_config.c
* Created By	: Harshit Agnihotri.
* Created Date	: 04/03/2024.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*				1024 KB		Flash.
*				128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 04/03/2024.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "door_config.h"
#include "user_uart.h"
#include "definitions.h"

// Array variable that holds the osdp id for 16 doors.
volatile U8 door_osdp_id[MAX_DOORS];

/*****************************************************************************
* Function name: void write_door_config(U8* received_door_data).
* Returns		: nothing.
* Arguments    : U8* received_door_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/03/2024.
*
* Description	: This function parses received door osdp id data and write it into an array.
*              :
* Notes	     : NA.
* Global Variables Affected : door_osdp_id.
*****************************************************************************/
void write_door_config(U8* received_door_data)
{
     for(U8 door_idx = 0; door_idx < MAX_DOORS; door_idx++)       // Extract door related values from the received door data array.
     {
          door_osdp_id[door_idx] = received_door_data[door_idx];
     }
}

/*****************************************************************************
* Function name: void read_door_config(U8* read_door_data).
* Returns		: nothing.
* Arguments    : U8* read_door_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/03/2024.
*
* Description	: This function reads door configuration data from an array door_osdp_id and stores it in another array.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void read_door_config(U8* read_door_data)
{
     for(U8 door_idx = 0; door_idx < MAX_DOORS; door_idx++)       // Copy door configuration data to the read_door_data array.
     {
          read_door_data[door_idx] = door_osdp_id[door_idx];
     }
}

/*****************************************************************************
* Function name: void test_door_config().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 04/03/2024.
*
* Description	: This function tests the write_door_config and read_door_config functions.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_door_config(void)
{
     #if (TEST_DOOR_CONFIG == uDISABLE)
     U8 received_door_data[MAX_DOORS] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
     U8 read_door_data[MAX_DOORS];

     write_door_config(received_door_data);      // Test write_door_config.

     read_door_config(read_door_data);       // Test read_door_config.

     Send_Frame_On_UART(read_door_data, MAX_DOORS);
     #endif
}