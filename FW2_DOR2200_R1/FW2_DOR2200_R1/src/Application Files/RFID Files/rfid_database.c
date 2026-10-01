#include "asf.h"

#include "rfid_database.h"
#include "osdp_protocol_master.h"


const U8 stored_rfid[NUM_OF_USERS][RFID_UID_LEN] = {0};
	
U8 rfArray1[4] = {0xBB, 0x45, 0xE9, 0x5B};
U8 rfArray2[7] = {0x80, 0x63, 0x28, 0xAA, 0xC5, 0x30, 0x04};
	
void Validate_Cards(void)
{
	if (gb_osdp_rfid_raw_f == TRUE)
	{
		gb_osdp_rfid_raw_f = FALSE;
		
		if (gb_osdp_rfid_len == 4)
		{
			for (U8 chk = 0; chk < gb_osdp_rfid_len; chk ++)
			{
				if (gb_osdp_rfid_data[chk] == rfArray1[chk])
				{
					gb_osdp_rfid_rec_f = 1;
				}
			}
		
		}
		else if (gb_osdp_rfid_len == 7)
		{
			for (U8 chk = 0; chk < gb_osdp_rfid_len; chk ++)
			{
				if (gb_osdp_rfid_data[chk] == rfArray2[chk])
				{
					gb_osdp_rfid_rec_f = 1;
				}
			}
		}
		else
		{
			// NOP
		}
	}
}