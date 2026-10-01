/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: definitions.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 18/08/2023.
* Module
* Description	: Defines constants and macros. This file is common file
*				  for the entire project.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

/***** User defined Definitions *****/
#define uTRUE		(1)
#define uFALSE		(0)
#define uSET		(1)
#define uRESET		(0)
#define uENABLE		(1)
#define uDISABLE	(0)
#define IP_ENABLED	(1)
#define IP_DISABLED	(0)

/***** Make debug messages ON-OFF *****/
#define DEBUG_ALL			uDISABLE
#define DEBUG_ETHERNET		uDISABLE
#define DEBUG_ARP_PKT		uDISABLE
#define DEBUG_IP_PKT		uDISABLE
#define DEBUG_TCP_IP		uDISABLE
#define DEBUG_INTERLOCK		uDISABLE
#define DEBUG_APP_OSDP_TX	uDISABLE
#define DEBUG_APP_OSDP_RX	uDISABLE
#define DEBUG_EMERGENCY		uDISABLE
#define DEBUG_FIRE_FUN		uDISABLE
#define DEBUG_AUX_INPUT		uDISABLE
#define DEBUG_ONBOARD_KEY	uDISABLE
#define DEBUG_EXT_EEPROM	uDISABLE
#define DEBUG_OP			uDISABLE
#define DEBUG_CONFIG_MODE	uDISABLE
#define DEBUG_UTILITY		uDISABLE
#define DEBUG_APP_EEPROM	uDISABLE
#define DEBUG_DIG_IP		uDISABLE

#define SET_BIT(num, bitposition) ((*num) |= (1u << (bitposition)))
#define CLEAR_BIT(num, bitposition)  ((*num) &= ~(1u << (bitposition)))
#define IS_BIT_SET(num, bitposition)  (((num) & (1u << (bitposition))) != 0)

#define COMBINE_BYTES(hb, lb) (((U16)(hb) << 8) | (U16)(lb))     // Macro to combine two bytes into a 16-bit value.

/*****
	Master's general purpose memory related definitions.
*****/
#define GEN_MEM_BASE_ADD	(4000)	/* Byte location. */
#define GEN_MEM_FA_ACT_BY	(4008)
#define GEN_MEM_OP_STATE	(4024)
#define GEN_MEM_IP_STATE	(4040)
#define GEN_MEM_FIRE_STATE	(4046)
#endif /* DEFINITIONS_H_ */