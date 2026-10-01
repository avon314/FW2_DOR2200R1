/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: tcp_server.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/11/2023.
* Module
* Description	: Header file for tcp_server.c
				  Defines constants and macros for tcp_server.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

/***** Macro definitions *****/
#define RCV_NUM_OF_CHR		(255)
#define TX_NUM_OF_CHR		(255)
/** The priorities at which various tasks will get created. */
#define mainTCP_TASK_PRIORITY         (tskIDLE_PRIORITY)
/** The stack sizes allocated to the DSP stack: (256 * 4) = 1024 bytes. */
#define mainTCP_TASK_STACK_SIZE       (256)

/***** Function Prototypes / Declarations *****/
void Create_TCP_Server_Task(void);

/***** Global Function Prototypes *****/
extern U8 gb_mbTcp_rcv_buf[RCV_NUM_OF_CHR];
extern U16 gb_mbTCP_rcvbuf_len;
extern U8 gb_mbTCP_rcvd_f;

extern U8 gb_mbTcp_tx_buf[TX_NUM_OF_CHR];
extern U8 gb_mbTCP_txbuf_len;
extern U8 gb_mbTCP_tx_f;

#endif /* TCP_SERVER_H_ */