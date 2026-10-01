/*****************************************************************************
*							   Copyright (c)
*                    Avon Building Solutions Pvt Ltd
*	                        All RIGTHS RESERVED
*
* Module Name	: osdp_sc.c
* Created By	: Ranjitkumar Ainapure
* Created Date	: 07-09-2022
* Module
* Description	: OSDP Secure channel establishment functions are written here.
*
* Device Used	:
* Controller	: STM32F207VET6
*                 512 KB -----> Flash Memory.
*                 128 KB -----> RAM Memory.
*
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 07-09-2022
* Changes		: NA
*****************************************************************************/
#include "osdp_sc.h"
#include "tinyaes_src.h"
#include "osdp_protocol_master.h"

#define OSDP_SC_EOM_MARKER             0x80  /* End of Message Marker */

/* Default key as specified in OSDP specification */
static const uint8_t osdp_scbk_default[16] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

uint8_t lcl_s_enc[16] = {0};
uint8_t lcl_s_mac1[16] = {0};
uint8_t lcl_s_mac2[16] = {0};

/*****************************************************************************
* Function name	: void osdp_compute_scbk(uint8_t *pd_client_uid,
* 				  uint8_t *master_key, uint8_t *scbk)
* Returns		: None.
* Arguments		: uint8_t *pd_client_uid	---> Holds the address of the client UID.
* 				  uint8_t *master_key		---> Holds the Master key value.
* 				  uint8_t *scbk				---> Holds the address of the Calculated SCBK by Master Key.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to Calculate SCBK using Master Key and Client UID need to pass Master key
* 				  value and Received client UID."
*               :
* Notes			: NA.
* Global Variables Affected	: NA
*****************************************************************************/
void osdp_compute_scbk(uint8_t *pd_client_uid, uint8_t *master_key, uint8_t *scbk)
{
	int i;

	memcpy(scbk, pd_client_uid, 8);
	for (i = 8; i < 16; i++)
	{
		scbk[i] = ~scbk[i - 8];
	}
	osdp_encrypt(master_key, NULL, scbk, 16);
}

/*****************************************************************************
* Function name	: void osdp_compute_session_keys(void)
* Returns		: None.
* Arguments		: void.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to Calculate session keys based on SCBK-D or SCBK"
*               :
* Notes			: SCBK_D (default key) that all the OSDP devices must support.
* 				  SCBK can also be derived using Master key.
* Global Variables Affected	: NA
*****************************************************************************/
void osdp_compute_session_keys(void)
{
	int i;
	uint8_t scbk[16];
	if (gb_osdp.scbk_default_f == 1)	// If Default key using
	{
		memcpy(scbk, osdp_scbk_default, 16);
	}
	else
	{
//		if (is_cp_mode(pd) && !ISSET_FLAG(pd, PD_FLAG_HAS_SCBK))
//		{
//			Print_Message("\nInside is_cp_mode.");
//			osdp_compute_scbk(pd, ctx->sc_master_key, scbk);
//		}
//		else
		{
			memcpy(scbk, gb_osdp.scbk_value, 16);
		}
	}

	memset(lcl_s_enc, 0, 16);
	memset(lcl_s_mac1, 0, 16);
	memset(lcl_s_mac2, 0, 16);

	lcl_s_enc[0] = 0x01;
	lcl_s_enc[1] = 0x82;
	lcl_s_mac1[0] = 0x01;
	lcl_s_mac1[1] = 0x01;
	lcl_s_mac2[0] = 0x01;
	lcl_s_mac2[1] = 0x02;

	for (i = 2; i < 8; i++)
	{
		lcl_s_enc[i] = gb_osdp.cp_random_num[i - 2];
		lcl_s_mac1[i] = gb_osdp.cp_random_num[i - 2];
		lcl_s_mac2[i] = gb_osdp.cp_random_num[i - 2];
	}

	osdp_encrypt(scbk, NULL, lcl_s_enc, 16);
	osdp_encrypt(scbk, NULL, lcl_s_mac1, 16);
	osdp_encrypt(scbk, NULL, lcl_s_mac2, 16);
}

/*****************************************************************************
* Function name	: void osdp_compute_cp_cryptogram(void)
* Returns		: None.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to calculate cryptogram of the control panel(Server device)"
*               :
* Notes			: NA
* Global Variables Affected	: gb_osdp.cp_cryptogram	---> calculated cryptogram.
*****************************************************************************/
void osdp_compute_cp_cryptogram(void)
{
	/* cp_cryptogram = AES-ECB( pd_random[8] || cp_random[8], s_enc ) */
	memcpy(gb_osdp.cp_cryptogram + 0, gb_osdp.pd_random_num, 8);
	memcpy(gb_osdp.cp_cryptogram + 8, gb_osdp.cp_random_num, 8);
	osdp_encrypt(lcl_s_enc, NULL, gb_osdp.cp_cryptogram, 16);
}

/*****************************************************************************
* Function name	: void osdp_compute_pd_cryptogram(void)
* Returns		: None.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to calculate cryptogram of the Peripheral device (client device)"
*               :
* Notes			: NA
* Global Variables Affected	: gb_osdp.pd_cryptogram	---> calculated cryptogram.
*****************************************************************************/
void osdp_compute_pd_cryptogram(void)
{
	/* pd_cryptogram = AES-ECB( cp_random[8] || pd_random[8], s_enc ) */
	memcpy(gb_osdp.pd_cryptogram + 0, gb_osdp.cp_random_num, 8);
	memcpy(gb_osdp.pd_cryptogram + 8, gb_osdp.pd_random_num, 8);
	osdp_encrypt(lcl_s_enc, NULL, gb_osdp.pd_cryptogram, 16);
}

/*****************************************************************************
* Function name	: static int osdp_ct_compare(const void *s1, const void *s2, size_t len)
* Returns		: static int ---> Returns 0 if memory pointed to by s1 and and s2
* 				  are identical; non-zero otherwise.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: Like memcmp; but operates at constant time.
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
static int osdp_ct_compare(const void *s1, const void *s2, size_t len)
{
	size_t i;
	size_t ret = 0;
	const uint8_t *_s1 = s1;
	const uint8_t *_s2 = s2;

	for (i = 0; i < len; i++)
	{
		ret |= _s1[i] ^ _s2[i];
	}
	return (int)ret;
}

/*****************************************************************************
* Function name	: int osdp_verify_cp_cryptogram(void)
* Returns		: int value ---> returns zero if verified. else -1.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	:  "Function to Verify the received cryptogram by calculating
* 				  using random numbers and session key."
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
int osdp_verify_cp_cryptogram(void)
{
	uint8_t cp_crypto[16];

	/* cp_cryptogram = AES-ECB( pd_random[8] || cp_random[8], s_enc ) */
	memcpy(cp_crypto + 0, gb_osdp.pd_random_num, 8);
	memcpy(cp_crypto + 8, gb_osdp.cp_random_num, 8);
	osdp_encrypt(lcl_s_enc, NULL, cp_crypto, 16);

	if (osdp_ct_compare(gb_osdp.cp_cryptogram, cp_crypto, 16) != 0)
	{
		return -1;
	}
	return 0;
}

/*****************************************************************************
* Function name	: void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
* Returns		: int value ---> returns zero if verified. else -1.
* Arguments		: void.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to Verify the received cryptogram by calculating
* 				  using random numbers and session key."
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
int osdp_verify_pd_cryptogram(void)
{
	uint8_t pd_crypto[16];

	/* pd_cryptogram = AES-ECB( cp_random[8] || pd_random[8], s_enc ) */
	memcpy(pd_crypto + 0, gb_osdp.cp_random_num, 8);
	memcpy(pd_crypto + 8, gb_osdp.pd_random_num, 8);

	osdp_encrypt(lcl_s_enc, NULL, pd_crypto, 16);

	if (osdp_ct_compare(gb_osdp.pd_cryptogram, pd_crypto, 16) != 0)
	{
		return -1;
	}
	return 0;
}

/*****************************************************************************
* Function name	: void osdp_compute_rmac_i(void)
* Returns		: Nothing
* Arguments		: None
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to Calculate initial r_mac_i value using Clients cryptogram
* 				  s_mac1 and s_mac2 values (Session keys)."
*               :
* Notes			: NA
* Global Variables Affected	: gb_osdp.rmac_value ---> Calculated Initial r_mac_i value.
*****************************************************************************/
void osdp_compute_rmac_i(void)
{
	/* rmac_i = AES-ECB( AES-ECB( cp_cryptogram, s_mac1 ), s_mac2 ) */
	memcpy(gb_osdp.rmac_value, gb_osdp.cp_cryptogram, 16);
	osdp_encrypt(lcl_s_mac1, NULL, gb_osdp.rmac_value, 16);
	osdp_encrypt(lcl_s_mac2, NULL, gb_osdp.rmac_value, 16);
}

/*****************************************************************************
* Function name	: int osdp_decrypt_data(uint8_t *data, int length)
* Returns		: int value ---> if received length doesn't match returns negative value.
* Arguments		: uint8_t *data ---> Data pointer holds the address of the data array
* 				  to be decrypted.
* Created by	: Ranjitkumar Ainapure
* Date created	: 09-09-2022
* Description	: "Function to decrypt the encrypted data"
*               :
* Notes			: NA
* Global Variables Affected	: NA.
*****************************************************************************/
int osdp_decrypt_data(uint8_t *data, int length)
{
	int i;
	uint8_t iv[16];

	if (length % 16 != 0)
	{
		return -1;
	}

	memcpy(iv, gb_osdp.rmac_value, 16);
	for (i = 0; i < 16; i++)
	{
		iv[i] = ~iv[i];
	}

	osdp_decrypt(lcl_s_enc, iv, data, length);

	length--;
	while (length && data[length] == 0x00)
	{
		length--;
	}
	if (data[length] != OSDP_SC_EOM_MARKER)
	{
		return -1;
	}
	data[length] = 0;

	return length;
}

/*****************************************************************************
* Function name	: int osdp_encrypt_data(uint8_t *data, int length)
* Returns		: int value ---> returns pad_len value.
* Arguments		: uint8_t *data ---> Data pointer holds the address of the data array
* 				  to be encrypted.
* Created by	: Ranjitkumar Ainapure
* Date created	: 09-09-2022
* Description	: "Function to encrypt the data"
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
int osdp_encrypt_data(uint8_t *data, int length)
{
	int i;
	int pad_len;
	uint8_t iv[16];

	data[length] = OSDP_SC_EOM_MARKER;  /* append EOM marker */
	pad_len = AES_PAD_LEN(length + 1);
	if ((pad_len - length - 1) > 0)
	{
		memset(data + length + 1, 0, pad_len - length - 1);
	}
	memcpy(iv, gb_osdp.rmac_value, 16);
	for (i = 0; i < 16; i++)
	{
		iv[i] = ~iv[i];
	}

	osdp_encrypt(lcl_s_enc, iv, data, pad_len);

	return pad_len;
}

/*****************************************************************************
* Function name	: int osdp_compute_mac(const uint8_t *data, int len)
* Returns		: integer value.
* Arguments		: uint8_t *data	---> Data pointer to hold the data to to compute mac
* 				  int len		---> length of the data buffer.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to compute the MAC"
*               :
* Notes			: NA
* Global Variables Affected	: gb_osdp.rmac_value	---> received rmac value.
* 							  gb_osdp.cmac_value	---> calculated cmac value.
*****************************************************************************/
int osdp_compute_mac(const uint8_t *data, int len)
{
	int pad_len;
	uint8_t buf[OSDP_PACKET_BUF_SIZE] = { 0 };
	uint8_t iv[16];

	memcpy(buf, data, len);
	pad_len = (len % 16 == 0) ? len : AES_PAD_LEN(len);
	if (len % 16 != 0)
	{
		buf[len] = 0x80; /* end marker */
	}
	/**
	 * MAC for data blocks B[1] .. B[N] (post padding) is computed as:
	 * IV1 = R_MAC (or) C_MAC  -- depending on is_cmd
	 * IV2 = B[N-1] after -- AES-CBC ( IV1, B[1] to B[N-1], SMAC-1 )
	 * MAC = AES-ECB ( IV2, B[N], SMAC-2 )
	 */

	memcpy(iv, gb_osdp.rmac_value, 16);
	if (pad_len > 16)
	{
		/* N-1 blocks -- encrypted with SMAC-1 */
		osdp_encrypt(lcl_s_mac1, iv, buf, pad_len - 16);
		/* N-1 th block is the IV for N th block */
		memcpy(iv, buf + pad_len - 32, 16);
	}

	/* N-th Block encrypted with SMAC-2 == MAC */
	osdp_encrypt(lcl_s_mac2, iv, buf + pad_len - 16, 16);
	memcpy(gb_osdp.cmac_value, buf + pad_len - 16, 16);	// Calculated MAC.
	memcpy(gb_osdp.rmac_value, gb_osdp.cmac_value, 16);				// Copy calculated MAC (gb_osdp.cmac_value) to gb_osdp.rmac_value.

	return 0;
}

/*****************************************************************************
* Function name	: void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
* Returns		: None.
* Arguments		: uint8_t *key	---> Pointer to hold the address of the key variable.
* 				  uint8_t *iv	---> Pointer to hold the address of rmac variable.
* 				  uint8_t *data	---> Data pointer to hold the data to encrypt.
* 				  int len		---> length of the data buffer.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to encrypt the data/Frame"
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	struct AES_ctx aes_ctx;

	if (iv != NULL)
	{
		/* encrypt multiple block with AES in CBC mode */
		AES_init_ctx_iv(&aes_ctx, key, iv);
		if (len < OSDP_TX_FRAME_SIZE)
		AES_CBC_encrypt_buffer(&aes_ctx, data, len);
	}
	else
	{
		/* encrypt one block with AES in ECB mode */
		//assert(len <= 16);
		AES_init_ctx(&aes_ctx, key);
		AES_ECB_encrypt(&aes_ctx, data);
	}
}

/*****************************************************************************
* Function name	: void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
* Returns		: None.
* Arguments		: uint8_t *key	---> Pointer to hold the address of the key variable.
* 				  uint8_t *iv	---> Pointer to hold the address of rmac variable.
* 				  uint8_t *data	---> Data pointer to hold the data to decrypt.
* 				  int len		---> length of the data buffer.
* Created by	: Ranjitkumar Ainapure
* Date created	: 07-09-2022
* Description	: "Function to decrypt the data/Frame"
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	struct AES_ctx aes_ctx;

	if (iv != NULL)
	{
		/* decrypt multiple block with AES in CBC mode */
		AES_init_ctx_iv(&aes_ctx, key, iv);
		if (len < OSDP_RX_FRAME_SIZE)
		AES_CBC_decrypt_buffer(&aes_ctx, data, len);
	}
	else
	{
		/* decrypt one block with AES in ECB mode */
		//assert(len <= 16);
		AES_init_ctx(&aes_ctx, key);
		AES_ECB_decrypt(&aes_ctx, data);
	}
}
