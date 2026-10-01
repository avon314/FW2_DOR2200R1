/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: app_eeprom.c
* Created By	: Harshit Agnihotri
* Created Date	: 01/04/2024
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash
*					128 KB		RAM
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 02/04/2024
* Changes		: NA
/*****************************************************************************/

/* System Includes */
#include "asf.h"
#include "string.h"

/* User Includes */
#include "app_eeprom.h"
#include "user_uart.h"
#include "CRC.h"
#include "ext_eeprom.h"
#include "dflt_config.h"
#include "app_utility.h"
#include "definitions.h"

/*****************************************************************************
* Function name: void write_to_eeprom(U16 addr, U8* data_to_write, U16 len)
* Returns		: None
* Arguments	: U16 addr, U8* data_to_write, U16 len
* Created by	: Harshit Agnihotri
* Date created	: 02-04-2024
* Description	: Add CRC to the data at the end and write it to eeprom at the specified address.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void write_to_eeprom(U16 addr, U8* data_to_write, U16 len)
{
     U8 frame_to_write[MAX_BYTES];
     memcpy(frame_to_write, data_to_write, len);
     for (uint16_t lcl_idx = len; lcl_idx < MAX_BYTES; lcl_idx++)
     {
          frame_to_write[lcl_idx] = 0xFF;
     }
	 
	 if (len <= UTLTY_FRM_MX_SZ)
	 {
		 if(CRC_Calculate_Check(data_to_write, len, CRC_CALC))                      // Calculate and set CRC at the end.
		 {
			  frame_to_write[len] = gb_Temp_CRC[0];
			  frame_to_write[len+1] = gb_Temp_CRC[1];

			  U8 bytes_remaining;
			  if(len > (PAGE_SIZE - CRC_BYTES))
			  {
				   bytes_remaining = (PAGE_SIZE - ((len + CRC_BYTES) % PAGE_SIZE));
			  }
			  else
			  {
				   bytes_remaining = (PAGE_SIZE - (len + CRC_BYTES));
			  }
			  len = len + CRC_BYTES + bytes_remaining;

			  eeprom_write_frame(addr, frame_to_write, len);                        // Write the data to eeprom.
		 }
	 }
	 else
	 {
		 __NOP();
	 }
}

/*****************************************************************************
* Function name: void read_from_eeprom(U16 addr, U8* read_data, U16 len)
* Returns		: None
* Arguments	: U16 addr, U8* read_data, U16 len
* Created by	: Harshit Agnihotri
* Date created	: 02-04-2024
* Description	: Read the data from specified address, verify the CRC and store it in the passed buffer.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void read_from_eeprom(U16 addr, U8* read_data, U16 len, U8 err_byte)
{
     eeprom_read_frame(addr, read_data, len + CRC_BYTES);                       // Read the data from eeprom.
     if (len <= UTLTY_FRM_MX_SZ)
	 {
		 if(CRC_Calculate_Check(read_data, len, CRC_CHECK))                         // Calculate and verify CRC at the end.
		 {
			  switch(err_byte)                                                      // If CRC verified write the respective data in RAM.
			  {
				   case READ_GRP_1 :
				   write_group_configuration(read_data, GRP_1_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 1, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_2 :
				   write_group_configuration(read_data, GRP_2_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 2, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_3 :
				   write_group_configuration(read_data, GRP_3_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 3, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_4 :
				   write_group_configuration(read_data, GRP_4_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 4, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_5 :
				   write_group_configuration(read_data, GRP_5_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 5, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_6 :
				   write_group_configuration(read_data, GRP_6_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 6, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_7 :
				   write_group_configuration(read_data, GRP_7_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 7, set EEPROM values");
				   #endif

				   break;

				   case READ_GRP_8 :
				   write_group_configuration(read_data, GRP_8_IDX + 1);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for group 8, set EEPROM values");
				   #endif

				   break;

				   case READ_IP :
				   write_ip_config(read_data);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for input, set EEPROM values");
				   #endif

				   break;

				   case READ_OP :
				   write_op_config(read_data);
				   gb_op_config_cplt = true;
				
				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for output, set EEPROM values");
				   #endif

				   break;

				   case READ_DOOR :
				   write_door_config(read_data);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for door, set EEPROM values");
				   #endif

				   break;

				   case READ_PVC :
				   write_pvc_grp_config(read_data);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for privacy, set EEPROM values");
				   #endif

				   break;

				   case READ_COMN :
				   write_comn_settings(read_data);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification passed for communication, set EEPROM values");
				   #endif

				   break;

				   default :
				   break;
			  }
		 }
		 else
		 {
			  switch(err_byte)                                                      // If CRC not verified write the default data in RAM.
			  {
				   case READ_GRP_1 :
				   write_dflt_grp_config(GRP_1_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 1, set default values.");
				   #endif

				   break;

				   case READ_GRP_2 :
				   write_dflt_grp_config(GRP_2_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 2, set default values.");
				   #endif

				   break;

				   case READ_GRP_3 :
				   write_dflt_grp_config(GRP_3_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 3, set default values.");
				   #endif

				   break;

				   case READ_GRP_4 :
				   write_dflt_grp_config(GRP_4_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 4, set default values.");
				   #endif

				   break;

				   case READ_GRP_5 :
				   write_dflt_grp_config(GRP_5_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 5, set default values.");
				   #endif

				   break;

				   case READ_GRP_6 :
				   write_dflt_grp_config(GRP_6_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 6, set default values.");
				   #endif

				   break;

				   case READ_GRP_7 :
				   write_dflt_grp_config(GRP_7_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 7, set default values.");
				   #endif

				   break;

				   case READ_GRP_8 :
				   write_dflt_grp_config(GRP_8_IDX);

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for group 8, set default values.");
				   #endif

				   break;

				   case READ_IP :
				   write_dflt_ip_config();

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for input, set default values.");
				   #endif

				   break;

				   case READ_OP :
				   write_dflt_op_config();
				   gb_op_config_cplt = true;
				
				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for output, set default values.");
				   #endif

				   break;

				   case READ_DOOR :
				   write_dflt_door_config();

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for door, set default values.");
				   #endif

				   break;

				   case READ_PVC :
				   write_dflt_pvc_config();

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for privacy, set default values.");
				   #endif

				   break;

				   case READ_COMN :
				   write_dflt_comn_config();

				   #if DEBUG_ALL || DEBUG_APP_EEPROM
				   Print_Message("\nCRC verification failed for communication, set default values.");
				   #endif

				   break;

				   default :
				   break;
			  }
		 }
	 }
	 else
	 {
		 __NOP();
	 }
}

/*****************************************************************************
* Function name: void write_config_eeprom(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 03-04-2024
* Description	: Write all configuration data to eeprom.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void write_config_eeprom(void)
{

     #if DEBUG_ALL || DEBUG_APP_EEPROM
     Print_Message("\nWriting data to EEPROM");
     #endif

     U8 read_config_data[MAX_BYTES];

     read_group_configuration(read_config_data, GRP_1_IDX + 1);                                                                        // Read group 1 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_1_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_2_IDX + 1);                                                                        // Read group 2 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_2_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_3_IDX + 1);                                                                        // Read group 3 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_3_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_4_IDX + 1);                                                                        // Read group 4 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_4_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_5_IDX + 1);                                                                        // Read group 5 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_5_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_6_IDX + 1);                                                                        // Read group 6 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_6_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_7_IDX + 1);                                                                        // Read group 7 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_7_IDX)), read_config_data, MAX_GRP_BYTES);

     read_group_configuration(read_config_data, GRP_8_IDX + 1);                                                                        // Read group 8 configuration and write to EEPROM.
     write_to_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_8_IDX)), read_config_data, MAX_GRP_BYTES);

     read_ip_config(read_config_data);                                                                                                 // Read input configuration and write to EEPROM.
     write_to_eeprom(EEPROM_IP_CONFIG_ADDR, read_config_data, IP_CONFIG_BYTES);

     read_op_config(read_config_data);                                                                                                 // Read output configuration and write to EEPROM.
     write_to_eeprom(EEPROM_OP_CONFIG_ADDR, read_config_data, OP_CONFIG_BYTES);

     read_door_config(read_config_data);                                                                                               // Read door configuration and write to EEPROM.
     write_to_eeprom(EEPROM_DOOR_CONFIG_ADDR, read_config_data, MAX_DOORS);

     read_pvc_grp_config(read_config_data);                                                                                            // Read privacy configuration and write to EEPROM.
     write_to_eeprom(EEPROM_PRIVACY_CONFIG_ADDR, read_config_data, PVC_CONFIG_BYTES);

     read_comn_settings(read_config_data);                                                                                             // Read communication configuration and write to EEPROM.
     write_to_eeprom(EEPROM_COMN_CONFIG_ADDR, read_config_data, COMN_CONFIG_BYTES);

     #if DEBUG_ALL || DEBUG_APP_EEPROM
     Print_Message("\nData written to EEPROM");
     #endif

}

/*****************************************************************************
* Function name: void read_config_eeprom(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 04-04-2024
* Description	: Read all configuration data from eeprom.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void read_config_eeprom(void)
{

     #if DEBUG_ALL || DEBUG_APP_EEPROM
     Print_Message("\nReading data from EEPROM");
     #endif

     U8 write_config_data[MAX_BYTES];

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_1_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_1);              // Read group 1 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_2_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_2);              // Read group 2 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_3_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_3);              // Read group 3 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_4_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_4);              // Read group 4 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_5_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_5);              // Read group 5 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_6_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_6);              // Read group 6 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_7_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_7);              // Read group 7 configuration from EEPROM and write to RAM.

     read_from_eeprom(((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP)*(GRP_8_IDX)), write_config_data, MAX_GRP_BYTES, READ_GRP_8);              // Read group 8 configuration from EEPROM and write to RAM.

     read_from_eeprom(EEPROM_IP_CONFIG_ADDR, write_config_data, IP_CONFIG_BYTES, READ_IP);                                                        // Read input configuration from EEPROM and write to RAM.

     read_from_eeprom(EEPROM_OP_CONFIG_ADDR, write_config_data, OP_CONFIG_BYTES, READ_OP);                                                        // Read output configuration from EEPROM and write to RAM.

     read_from_eeprom(EEPROM_DOOR_CONFIG_ADDR, write_config_data, MAX_DOORS, READ_DOOR);                                                          // Read door configuration from EEPROM and write to RAM.

     read_from_eeprom(EEPROM_PRIVACY_CONFIG_ADDR, write_config_data, PVC_CONFIG_BYTES, READ_PVC);                                                 // Read privacy configuration from EEPROM and write to RAM.

     read_from_eeprom(EEPROM_COMN_CONFIG_ADDR, write_config_data, COMN_CONFIG_BYTES, READ_COMN);                                                  // Read communication configuration from EEPROM and write to RAM.

     #if DEBUG_ALL || DEBUG_APP_EEPROM
     Print_Message("\nData read from EEPROM");
     #endif

	Update_Manufacturing_Info();
}