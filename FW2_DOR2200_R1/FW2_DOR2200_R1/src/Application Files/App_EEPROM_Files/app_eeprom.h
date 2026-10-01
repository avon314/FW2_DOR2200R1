/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: app_eeprom.h
* Created By	: Harshit Agnihotri
* Created Date	: 30/03/2024
* Module
* Description	: Header file for app_eeprom.c
				  Defines constants and macros for app_eeprom.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 30/03/2024
* Changes		: NA
*****************************************************************************/

/* User Includes */
#include "group_config.h"
#include "input_config.h"
#include "output_config.h"
#include "door_config.h"
#include "privacy_config.h"
#include "comn_config.h"

#ifndef APP_EEPROM_H_
#define APP_EEPROM_H_

/***** Macro definitions *****/
#define MAX_BYTES                   (512)
#define CRC_BYTES                   (2)

#define REMAINING_BYTES_GRP         (PAGE_SIZE - ((MAX_GRP_BYTES + CRC_BYTES) % PAGE_SIZE))
#define GRP_1_IDX                   (0)
#define GRP_2_IDX                   (1)
#define GRP_3_IDX                   (2)
#define GRP_4_IDX                   (3)
#define GRP_5_IDX                   (4)
#define GRP_6_IDX                   (5)
#define GRP_7_IDX                   (6)
#define GRP_8_IDX                   (7)

#define EEPROM_IP_CONFIG_ADDR       ((MAX_GRP_BYTES + CRC_BYTES + REMAINING_BYTES_GRP) * MAX_GROUPS)
#define REMAINING_BYTES_IP          ((IP_CONFIG_BYTES > (PAGE_SIZE - CRC_BYTES)) ? (PAGE_SIZE - ((IP_CONFIG_BYTES + CRC_BYTES) % PAGE_SIZE)) : (PAGE_SIZE - (IP_CONFIG_BYTES + CRC_BYTES)))

#define EEPROM_OP_CONFIG_ADDR       (EEPROM_IP_CONFIG_ADDR + IP_CONFIG_BYTES + CRC_BYTES + REMAINING_BYTES_IP)
#define REMAINING_BYTES_OP          ((OP_CONFIG_BYTES > (PAGE_SIZE - CRC_BYTES)) ? (PAGE_SIZE - ((OP_CONFIG_BYTES + CRC_BYTES) % PAGE_SIZE)) : (PAGE_SIZE - (OP_CONFIG_BYTES + CRC_BYTES)))

#define EEPROM_DOOR_CONFIG_ADDR     (EEPROM_OP_CONFIG_ADDR + OP_CONFIG_BYTES + CRC_BYTES + REMAINING_BYTES_OP)
#define REMAINING_BYTES_DOOR        ((MAX_DOORS > (PAGE_SIZE - CRC_BYTES)) ? (PAGE_SIZE - ((MAX_DOORS + CRC_BYTES) % PAGE_SIZE)) : (PAGE_SIZE - (MAX_DOORS + CRC_BYTES)))

#define EEPROM_PRIVACY_CONFIG_ADDR  (EEPROM_DOOR_CONFIG_ADDR + MAX_DOORS + CRC_BYTES + REMAINING_BYTES_DOOR)
#define REMAINING_BYTES_PRIVACY     ((PVC_CONFIG_BYTES > (PAGE_SIZE - CRC_BYTES)) ? (PAGE_SIZE - ((PVC_CONFIG_BYTES + CRC_BYTES) % PAGE_SIZE)) : (PAGE_SIZE - (PVC_CONFIG_BYTES + CRC_BYTES)))

#define EEPROM_COMN_CONFIG_ADDR     (EEPROM_PRIVACY_CONFIG_ADDR + PVC_CONFIG_BYTES + CRC_BYTES + REMAINING_BYTES_PRIVACY)

#define READ_GRP_1                   (1)
#define READ_GRP_2                   (2)
#define READ_GRP_3                   (3)
#define READ_GRP_4                   (4)
#define READ_GRP_5                   (5)
#define READ_GRP_6                   (6)
#define READ_GRP_7                   (7)
#define READ_GRP_8                   (8)
#define READ_IP                      (9)
#define READ_OP                      (10)
#define READ_DOOR                    (11)
#define READ_PVC                     (12)
#define READ_COMN                    (13)


// Function prototypes
void write_to_eeprom(U16 addr, U8* data_to_write, U16 len);
void read_from_eeprom(U16 addr, U8* read_data, U16 len, U8 err_byte);
void write_config_eeprom(void);
void read_config_eeprom(void);

#endif /* APP_EEPROM_H_ */