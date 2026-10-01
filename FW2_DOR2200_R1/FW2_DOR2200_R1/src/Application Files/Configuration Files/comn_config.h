/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: comn_config.h
* Created By	: Harshit Agnihotri.
* Created Date	: 13/12/2023.
* Module
* Description	: Header file for comn_config.c.
		       Defines constants and macros for group_config.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 13/12/2023.
* Changes		: NA.
*****************************************************************************/

#ifndef COMN_CONFIG_H_
#define COMN_CONFIG_H_

/***** User Includes *****/

/***** Definitions / Macros *****/
#define COMN_CONFIG_BYTES (50)
#define TEST_COMN_SETTINGS (1)

/***** Structure variable that holds the ip adress *****/
typedef struct
{
     char ip_addr_1[4];
     char ip_addr_2[4];
     char ip_addr_3[4];
     char ip_addr_4[4];
}Ip_Addr;
// Declare the structure variable of Ip_Addr to store the ip address.
extern Ip_Addr IpAddr;

// Declare the variable to store the port number.
extern char Port_Num[6];

/***** Structure variable that holds the subnet mask *****/
typedef struct
{
     char sbnt_msk_1[4];
     char sbnt_msk_2[4];
     char sbnt_msk_3[4];
     char sbnt_msk_4[4];
}Sbnt_Msk;
// Declare the structure variable of Sbnt_Msk to store the subnet mask.
extern Sbnt_Msk SbntMsk;

/***** Structure variable that holds the default gateway *****/
typedef struct
{
     char dflt_gtwy_1[4];
     char dflt_gtwy_2[4];
     char dflt_gtwy_3[4];
     char dflt_gtwy_4[4];
}Dflt_Gtwy;
// Declare the structure variable of Dflt_Gtwy to store the default gateway.
extern Dflt_Gtwy DfltGtwy;

/***** Function Declarations / Prototypes *****/
void write_comn_settings(char*);
void read_comn_settings(char*);
void test_comn_settings(void);

#endif /* COMN_CONFIG_H_ */