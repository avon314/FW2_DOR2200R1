/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: output_config.c
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
#include "output_config.h"
#include "user_uart.h"
#include "definitions.h"
#include "app_utility.h"

// Declare the structure variable of output_configuration to store the output configuration information.
output_configuration gb_output[MAX_OUTPUTS];

/*****************************************************************************
* Function name: void write_op_config(U8* received_output_data).
* Returns		: nothing.
* Arguments    : U8* received_output_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function parses received output data into an array of output_configuration structure.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_output.
*****************************************************************************/
void write_op_config(U8* received_output_data)
{
	updateMnfactInfo.numOfActiveOps = 0;
     for(U8 op_index = 0; op_index < MAX_OUTPUTS; op_index++)
     {
          gb_output[op_index].op_en = received_output_data[(op_index * MAX_OP_LEN)];       // Extract output related values from the received output array.
          gb_output[op_index].door_number = received_output_data[(op_index * MAX_OP_LEN) + 1];
          gb_output[op_index].op_function = received_output_data[(op_index * MAX_OP_LEN) + 2];
          gb_output[op_index].op_type = received_output_data[(op_index * MAX_OP_LEN) + 3];
          gb_output[op_index].op_dflt_state = received_output_data[(op_index * MAX_OP_LEN) + 4];
		  
		  if (gb_output[op_index].op_en == 1)
		  updateMnfactInfo.numOfActiveOps ++;
     }
}

/*****************************************************************************
* Function name: void read_op_config(U8* read_output_data).
* Returns		: nothing.
* Arguments    : U8* read_output_data.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function reads output configuration data from an array of output_configuration structure and stores it in another array.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void read_op_config(U8* read_output_data)
{
     for (U8 op_index=0; op_index<MAX_OUTPUTS; op_index++)
     {
          read_output_data[(op_index * MAX_OP_LEN)] = gb_output[op_index].op_en;       // Copy output configuration data to the read_op_data array.
          read_output_data[(op_index * MAX_OP_LEN) + 1] = gb_output[op_index].door_number;
          read_output_data[(op_index * MAX_OP_LEN) + 2] = gb_output[op_index].op_function;
          read_output_data[(op_index * MAX_OP_LEN) + 3] = gb_output[op_index].op_type;
          read_output_data[(op_index * MAX_OP_LEN) + 4] = gb_output[op_index].op_dflt_state;
     }
}

/*****************************************************************************
* Function name: void test_op_config().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 04/12/2023.
*
* Description	: This function tests the write_op_config and read_op_config functions.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_op_config(void)
{
     #if (TEST_OP_CONFIG == uDISABLE)
     U8 received_output[MAX_OUTPUTS * MAX_OP_LEN] = {0x01, 0x01, DOOR_OPEN, OP_TYPE_MOMENTARY, STATE_NO,
                                                     0x01, 0x03, DOOR_INTERLOCKED, OP_TYPE_TOGGLE, STATE_NC,
                                                     0x01, 0x05, PRIVACY, OP_TYPE_MOMENTARY, STATE_NO,
                                                     0x01, 0x10, NORMAL_STATE, OP_TYPE_TOGGLE, STATE_NC};
     U8 read_output_data[MAX_OUTPUTS * MAX_OP_LEN];

     write_op_config(received_output);           // Test write_op_config.

     read_op_config(read_output_data);           // Test read_op_config.

     Send_Frame_On_UART(read_output_data, MAX_OUTPUTS*MAX_OP_LEN);
     #endif
}