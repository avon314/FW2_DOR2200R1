#ifndef RFID_DATABASE_H_
#define RFID_DATABASE_H_

#define NUM_OF_USERS	2
#define RFID_UID_LEN	7

extern const U8 stored_rfid[NUM_OF_USERS][RFID_UID_LEN];

void Validate_Cards(void);

#endif /* RFID_DATABASE_H_ */