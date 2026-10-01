/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: comn_config.c
* Created By	: Harshit Agnihotri.
* Created Date	: 13/12/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*				1024 KB		Flash.
*				128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 13/12/2023.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "comn_config.h"
#include "string.h"
#include "user_uart.h"
#include "definitions.h"

Ip_Addr IpAddr;     // Declare the structure variable of Ip_Addr to store the ip address.
char Port_Num[6];   // Declare the variable to store the port number.
Sbnt_Msk SbntMsk;   // Declare the structure variable of Sbnt_Msk to store the subnet mask.
Dflt_Gtwy DfltGtwy; // Declare the structure variable of Dflt_Gtwy to store the default gateway.

/*****************************************************************************
* Function name: void write_comn_settings(U8* received_comn_data).
* Returns		: nothing.
* Arguments    : U8* received_comn_data.
* Created by	: Harshit Agnihotri.
* Date created	: 13/12/2023.
*
* Description	: This function parses received communication settings data and write it into various communication settings structures variables.
*              :
* Notes	     : NA.
* Global Variables Affected : IpAddr, Port_Num, SbntMsk, DfltGtwy.
*****************************************************************************/
void write_comn_settings(char* received_comn_data)
{
     U8 received_comn_data_idx = 0;

     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)                    // Parse and write the ip address.
     IpAddr.ip_addr_1[ip_addr_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     IpAddr.ip_addr_2[ip_addr_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     IpAddr.ip_addr_3[ip_addr_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     IpAddr.ip_addr_4[ip_addr_idx] = received_comn_data[received_comn_data_idx++];
     IpAddr.ip_addr_1[3] = '\0';
     IpAddr.ip_addr_2[3] = '\0';
     IpAddr.ip_addr_3[3] = '\0';
     IpAddr.ip_addr_4[3] = '\0';

     for(U8 port_num_idx = 0; port_num_idx < 5; port_num_idx++)                 // Parse and write the port number.
     Port_Num[port_num_idx] = received_comn_data[received_comn_data_idx++];

     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)                 // Parse and write the subnet mask.
     SbntMsk.sbnt_msk_1[sbnt_msk_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     SbntMsk.sbnt_msk_2[sbnt_msk_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     SbntMsk.sbnt_msk_3[sbnt_msk_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     SbntMsk.sbnt_msk_4[sbnt_msk_idx] = received_comn_data[received_comn_data_idx++];
     SbntMsk.sbnt_msk_1[3] = '\0';
     SbntMsk.sbnt_msk_2[3] = '\0';
     SbntMsk.sbnt_msk_3[3] = '\0';
     SbntMsk.sbnt_msk_4[3] = '\0';

     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)              // Parse and write the default gateway.
     DfltGtwy.dflt_gtwy_1[dflt_gtwy_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     DfltGtwy.dflt_gtwy_2[dflt_gtwy_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     DfltGtwy.dflt_gtwy_3[dflt_gtwy_idx] = received_comn_data[received_comn_data_idx++];
     received_comn_data_idx++;
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     DfltGtwy.dflt_gtwy_4[dflt_gtwy_idx] = received_comn_data[received_comn_data_idx++];
     DfltGtwy.dflt_gtwy_1[3] = '\0';
     DfltGtwy.dflt_gtwy_2[3] = '\0';
     DfltGtwy.dflt_gtwy_3[3] = '\0';
     DfltGtwy.dflt_gtwy_4[3] = '\0';
}

/*****************************************************************************
* Function name: void write_comn_settings(U8* received_comn_data).
* Returns		: nothing.
* Arguments    : U8* received_comn_data.
* Created by	: Harshit Agnihotri.
* Date created	: 13/12/2023.
*
* Description	: This function reads and passes the communication setting data.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void read_comn_settings(char* read_comn_data)
{
     U8 read_comn_data_idx = 0;

     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)                     // Read and pass the ip address.
     read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_1[ip_addr_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_2[ip_addr_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_3[ip_addr_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
     read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_4[ip_addr_idx];

     for(U8 port_num_idx = 0; port_num_idx < 5; port_num_idx++)                  // Read and pass the port number.
     read_comn_data[read_comn_data_idx++] = Port_Num[port_num_idx];

     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)                  // Read and pass the subnet mask.
     read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_1[sbnt_msk_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_2[sbnt_msk_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_3[sbnt_msk_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
     read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_4[sbnt_msk_idx];

     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)               // Read and pass the default gateway.
     read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_1[dflt_gtwy_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_2[dflt_gtwy_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_3[dflt_gtwy_idx];
     read_comn_data[read_comn_data_idx++] = '.';
     for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
     read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_4[dflt_gtwy_idx];
}

/*****************************************************************************
* Function name: void test_comn_settings(void).
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 13/12/2023.
*
* Description	: This function tests write_comn_settings and read_comn_settings functions.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_comn_settings(void)
{
     #if (TEST_COMN_SETTINGS == uENABLE)
     char received_comn_data[50]={"192.168.001.00199999255.255.255.000192.168.100.100"};
     char read_comn_data[50] = {"\0"};

     write_comn_settings(received_comn_data);     // Test the function write comn_settings.
     read_comn_settings(read_comn_data);          // Test the function read_comn_settings.

     Print_Message(read_comn_data);
     #endif
}
