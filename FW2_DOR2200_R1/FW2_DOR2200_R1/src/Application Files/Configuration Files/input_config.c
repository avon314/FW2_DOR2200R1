/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: input_config.c
* Created By	: Harshit Agnihotri.
* Created Date	: 04/12/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 04/12/2023.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "input_config.h"
#include "string.h"
#include "user_uart.h"
#include "definitions.h"
#include "app_utility.h"

// Declare the structure variable of input_configuration to store the input configuration information.
input_configuration gb_input[MAX_INPUTS];

/*****************************************************************************
* Function name: void write_ip_config(U8* received_input_data).
* Returns		: nothing.
* Arguments    : U8* received_input_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function parses received input data into an array of input_configuration structure.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_input.
*****************************************************************************/
void write_ip_config(U8* received_input_data)
{
	updateMnfactInfo.numOfActiveIps = 0;
     for(U8 input_index = 0; input_index < MAX_INPUTS; input_index++)
     {
          gb_input[input_index].ip_en = received_input_data[(input_index * MAX_IP_LEN)];  // Extract input related values from the received input data array.
          gb_input[input_index].door_grp_number = received_input_data[(input_index * MAX_IP_LEN) + 1];
          gb_input[input_index].ip_for_interlock = received_input_data[(input_index * MAX_IP_LEN) + 2];
          gb_input[input_index].ip_activate_state = received_input_data[(input_index * MAX_IP_LEN) + 3];
          gb_input[input_index].ip_type = received_input_data[(input_index * MAX_IP_LEN) + 4];
          gb_input[input_index].ip_function = received_input_data[(input_index * MAX_IP_LEN) + 5];
		  
		  if (gb_input[input_index].ip_en == 1)
		  updateMnfactInfo.numOfActiveIps ++;
     }
}

/*****************************************************************************
* Function name: void read_ip_config(U8* read_input_data).
* Returns		: nothing.
* Arguments    : U8* read_input_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function reads input configuration data from an array of input_configuration structure and stores it in another array.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void read_ip_config(U8* read_input_data)
{
     for (U8 input_index = 0; input_index < MAX_INPUTS; input_index++)
     {
          read_input_data[(input_index * MAX_IP_LEN)] = gb_input[input_index].ip_en;       // Copy input configuration data to the read_ip_data array.
          read_input_data[(input_index * MAX_IP_LEN) + 1] = gb_input[input_index].door_grp_number;
          read_input_data[(input_index * MAX_IP_LEN) + 2] = gb_input[input_index].ip_for_interlock;
          read_input_data[(input_index * MAX_IP_LEN) + 3] = gb_input[input_index].ip_activate_state;
          read_input_data[(input_index * MAX_IP_LEN) + 4] = gb_input[input_index].ip_type;
          read_input_data[(input_index * MAX_IP_LEN) + 5] = gb_input[input_index].ip_function;
     }
}

/*****************************************************************************
* Function name: void test_ip_config().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function tests the write_ip_config and read_ip_config functions.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_ip_config(void)
{
     #if (TEST_IP_CONFIG == uDISABLE)
     U8 received_input_data[MAX_INPUTS * MAX_IP_LEN] = {0x01, 0x02, IP_NOT_FOR_INTERLOCK, LOW_TO_HIGH, MOMENTARY, ACCESS_DENIED,
                                                        0x01, 0x04, IP_NOT_FOR_INTERLOCK, HIGH_TO_LOW, MOMENTARY, RELEASE_DOOR,
                                                        0x01, 0x12, IP_NOT_FOR_INTERLOCK, LOW_TO_HIGH, MOMENTARY, RELEASE_DOOR_DRT,
                                                        0x01, 0x16, IP_NOT_FOR_INTERLOCK, HIGH_TO_LOW, TOGGLE, ACCESS_DENIED,
                                                        0x01, 0x14, IP_NOT_FOR_INTERLOCK, LOW_TO_HIGH, TOGGLE, RELEASE_DOOR,
                                                        0x01, 0x19, IP_NOT_FOR_INTERLOCK, HIGH_TO_LOW, TOGGLE, RELEASE_DOOR_DRT};
     U8 read_input_data[MAX_INPUTS * MAX_IP_LEN];

     write_ip_config(received_input_data);      // Test write_ip_config.

     read_ip_config(read_input_data);       // Test read_ip_config.

     Send_Frame_On_UART(read_input_data, MAX_INPUTS*MAX_IP_LEN);
     #endif
}
