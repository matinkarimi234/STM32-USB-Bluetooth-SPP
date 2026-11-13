#ifndef BT_USER_CONFIG_H
#define BT_USER_CONFIG_H
#define BT_DEVICE_NAME "STM32 Bluetooth SPP"
#define BT_PAIRING_TEST_MODE 0U /* Explicitly enable only for controlled development. */
#define BT_AUTO_ACCEPT_CONNECTIONS 1U
#define BT_AUTO_AUTH_TEST 0U
#define BT_SPP_RFCOMM_CHANNEL 1U
#define BT_BONDS_MAX_DEVICES 16U
/* Must match linker/programmer reservation; changing this does NOT migrate bonds. */
#define BT_BONDS_FLASH_START 0x080C0000UL
#include "bt_f746_flash_layout.h"
#define BT_BONDS_STORAGE_BYTES BT_F746_BOND_SECTOR_BYTES
#define BT_BONDS_FLASH_BYTES BT_BONDS_STORAGE_BYTES
#endif
