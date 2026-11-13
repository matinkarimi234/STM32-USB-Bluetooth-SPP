#ifndef APP_BT_PERIODIC_H
#define APP_BT_PERIODIC_H
#include "bt_library.h"
USBH_StatusTypeDef AppBT_Register(USBH_HandleTypeDef *host);
void AppBT_Process(void);
/* Timer ISR ONLY posts a request; all USB/library calls stay in the main loop. */
void AppBT_RequestTxFromISR(void);
/* Main-loop parser reads copied RX bytes; this API is not an ISR API. */
uint16_t AppBT_Read(uint8_t *out, uint16_t capacity);
extern volatile uint32_t AppBT_RxBytes, AppBT_RxDrops, AppBT_TxPackets;
#endif
