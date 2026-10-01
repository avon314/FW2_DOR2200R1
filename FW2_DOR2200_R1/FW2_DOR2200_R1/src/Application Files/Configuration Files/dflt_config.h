/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: dflt_config.h
* Created By	: Harshit Agnihotri
* Created Date	: 05/04/2024
* Module
* Description	: Header file for dflt_config.c
				  Defines constants and macros for dflt_config.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 05/04/2024
* Changes		: NA
*****************************************************************************/

#ifndef DFLT_CONFIG_H_
#define DFLT_CONFIG_H_

/***** Function Declarations / Prototypes *****/
void write_dflt_grp_config(U8 grp_no);
void write_dflt_ip_config(void);
void write_dflt_op_config(void);
void write_dflt_door_config(void);
void write_dflt_pvc_config(void);
void write_dflt_comn_config(void);

#endif /* DFLT_CONFIG_H_ */