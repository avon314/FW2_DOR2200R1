/*****************************************************************************
*							   Copyright (c)
*                    Avon Building Solutions Pvt Ltd
*	                        All RIGTHS RESERVED
*
* Module Name	: osdp_sc.h
* Created By	: Ranjitkumar Ainapure
* Created Date	: 07-09-2022
* Module
* Description	: Header file for osdp_sc.c
*                 Defines constants and macros for osdp_sc.c
*
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 07-09-2022
* Changes		: NA
*****************************************************************************/
#ifndef OSDP_SC_H_
#define OSDP_SC_H_

#include "asf.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define AES_PAD_LEN(x) ((x + 16 - 1) & (~(16 - 1)))

#define OSDP_PACKET_BUF_SIZE                    (256)
#define OSDP_RX_RB_SIZE                         (512)

/**
 * @brief secure block types
 */
#define SCS_11 0x11 /* CP -> PD -- CMD_CHLNG */
#define SCS_12 0x12 /* PD -> CP -- REPLY_CCRYPT */
#define SCS_13 0x13 /* CP -> PD -- CMD_SCRYPT */
#define SCS_14 0x14 /* PD -> CP -- REPLY_RMAC_I */

#define SCS_15 0x15 /* CP -> PD -- packets w MAC w/o ENC */
#define SCS_16 0x16 /* PD -> CP -- packets w MAC w/o ENC */
#define SCS_17 0x17 /* CP -> PD -- packets w MAC w ENC*/
#define SCS_18 0x18 /* PD -> CP -- packets w MAC w ENC*/

void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len);
void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len);
void osdp_compute_scbk(uint8_t *pd_client_uid, uint8_t *master_key, uint8_t *scbk);
void osdp_compute_session_keys(void);
void osdp_compute_cp_cryptogram(void);
void osdp_compute_pd_cryptogram(void);
int osdp_verify_cp_cryptogram(void);
int osdp_verify_pd_cryptogram(void);
void osdp_compute_rmac_i(void);
int osdp_decrypt_data(uint8_t *data, int len);
int osdp_encrypt_data(uint8_t *data, int len);
int osdp_compute_mac(const uint8_t *data, int len);

#endif /* INC_OSDP_OSDP_SC_H_ */
