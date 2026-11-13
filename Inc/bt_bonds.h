#ifndef BT_BONDS_H
#define BT_BONDS_H
#include "bt_config.h"
#include <stdint.h>

/* Storage size/policy is configured by the application and port. */

extern volatile uint8_t BT_Bonds_Count;
extern volatile uint8_t BT_Bonds_Dirty;
extern volatile uint32_t BT_Bonds_Writes;
extern volatile uint32_t BT_Bonds_FlashErrors;
extern volatile uint32_t BT_Bonds_Reused;
extern volatile uint32_t BT_Bonds_Evictions;
extern volatile uint8_t BT_Bonds_Full;

void BT_Bonds_Init(void);
uint8_t BT_Bonds_Find(const uint8_t bd_addr_hci[6], uint8_t key_out[16]);
void BT_Bonds_Remember(const uint8_t bd_addr_hci[6], const uint8_t key[16], uint8_t key_type);
/* BLOCKING explicit maintenance only. Prefer BT_Library_SaveBonds.
 * Writes at most one pending entry when ACL/pairing/HCI state allows it.
 * Do not call from normal timing-critical polling or any interrupt/callback. */
void BT_Bonds_Process(void);
/* BLOCKING explicit factory reset: erases ALL remembered bonds.
 * Schedule a maintenance window; never invoke automatically on reconnect. */
uint8_t BT_Bonds_ClearAll(void);

#endif
