[#ftl]
/* Generated from STM32Bluetooth CubeMX settings. MIT licensed. */
#ifndef BT_USER_CONFIG_H
#define BT_USER_CONFIG_H
[#list SWIPdatas as SWIP]
[#if SWIP.defines??]
[#list SWIP.defines as definition]
[#if definition.name == "BT_DEVICE_NAME"]
#define BT_DEVICE_NAME "${definition.value?js_string}"
[#else]
#define ${definition.name} ${definition.value}
[/#if]
[/#list]
[/#if]
[/#list]
#define BT_AUTO_ACCEPT_CONNECTIONS 1U
#define BT_AUTO_AUTH_TEST 0U
#include "bt_f746_flash_layout.h"
#define BT_BONDS_STORAGE_BYTES BT_F746_BOND_SECTOR_BYTES
#define BT_BONDS_FLASH_BYTES BT_BONDS_STORAGE_BYTES
#endif
