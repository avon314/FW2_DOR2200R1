/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: privacy_config.c
* Created By	: Harshit Agnihotri.
* Created Date	: 05/03/2024.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*				1024 KB		Flash.
*				128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 05/03/2024.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "privacy_config.h"
#include "user_uart.h"
#include "definitions.h"

/***** Variable that holds the information about each privacy group (group of doors that are included in privacy) *****/
volatile U16 pvc_grp[MAX_PVC_GRPS];

/*****************************************************************************
* Function name: void write_pvc_grp_config(U8* received_pvc_data)
* Returns		: nothing.
* Arguments    : U8* received_pvc_data.
* Created by	: Harshit Agnihotri.
* Date created	: 05/03/2024.
*
* Description	: This function parses received privacy group configuration data and write it into an array variable.
*              :
* Notes	     : NA.
* Global Variables Affected : pvc_grp.
*****************************************************************************/
void write_pvc_grp_config(U8* received_pvc_data)
{
     U8 received_pvc_data_idx = 0;
     for(U8 pvc_grp_idx = 0; pvc_grp_idx < MAX_PVC_GRPS; pvc_grp_idx++)         // Extract privacy related values from the received privacy data array.
     {
          for(U8 door_idx = 0; door_idx < MAX_DOORS; door_idx++)
          {
               if(received_pvc_data[received_pvc_data_idx++])
               {
                    SET_BIT(&pvc_grp[pvc_grp_idx], door_idx);
               }
               else
               {
                    CLEAR_BIT(&pvc_grp[pvc_grp_idx], door_idx);
               }
          }
     }
}

/*****************************************************************************
* Function name: void read_pvc_grp_config(U8* read_pvc_data).
* Returns		: nothing.
* Arguments    : U8* read_pvc_data.
* Created by	: Harshit Agnihotri.
* Date created	: 05/03/2024.
*
* Description	: This function reads and passes the group configuration data.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void read_pvc_grp_config(U8* read_pvc_data)
{
     U8 read_pvc_data_idx = 0;
     for(U8 pvc_grp_idx = 0; pvc_grp_idx < MAX_PVC_GRPS; pvc_grp_idx++)         // Copy privacy configuration data to the read_pvc_data array.
     {
          for(U8 door_idx = 0; door_idx < MAX_DOORS; door_idx++)
          {
               if(IS_BIT_SET(pvc_grp[pvc_grp_idx], door_idx))
               {
                    read_pvc_data[read_pvc_data_idx++] = 0x01;
               }
               else
               {
                    read_pvc_data[read_pvc_data_idx++] = 0x00;
               }
          }
     }
}

/*****************************************************************************
* Function name: void test_pvc_grp_config(void).
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 05/03/2024.
*
* Description	: This function tests the write_pvc_grp_config and read_pvc_grp_config functions.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_pvc_grp_config(void)
{
     #if (TEST_PVC_GRP_CONFIG == uDISABLE)
     U8 received_pvc_grp_data[MAX_PVC_GRPS * MAX_DOORS] = {00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01, 00, 01,
                                                           01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 01};
     U8 read_pvc_grp_data[MAX_PVC_GRPS * MAX_DOORS];

     write_pvc_grp_config(received_pvc_grp_data);                               // Test write_pvc_grp_config.

     read_pvc_grp_config(read_pvc_grp_data);                                    // Test read_pvc_grp_config.

     Send_Frame_On_UART(read_pvc_grp_data, MAX_PVC_GRPS * MAX_DOORS);
     #endif
}