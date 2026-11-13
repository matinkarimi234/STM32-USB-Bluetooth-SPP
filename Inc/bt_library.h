#ifndef BT_LIBRARY_H
#define BT_LIBRARY_H
#include "usbh_core.h"
#define BT_LIBRARY_VERSION "0.2.0"
/* One active USB host / ACL / SPP service. Cooperative, single-thread use only.
 * RX spans are valid only during the callback. Copy or consume immediately.
 * Callbacks must not block USB polling or change registration. */
typedef struct {
    void (*received)(const uint8_t *data, uint16_t length, void *context);
    void (*serial_changed)(uint8_t open, void *context);
    void *context;
} BT_LibraryCallbacks;
/* Call after USBH_Init and before USBH_Start, exactly once. Callbacks are copied.
 * No USBH_Process, USBH_Start, flash erase, or HCI reset is performed here. */
USBH_StatusTypeDef BT_Library_Register(USBH_HandleTypeDef *host, const BT_LibraryCallbacks *callbacks);
/* Call after USBH_Process every main-loop iteration. No flash program/erase.
 * Reports serial-open/closed transitions. Callbacks run here or in USBH_Process,
 * never in a timer ISR. All library API calls require the same cooperative context. */
void BT_Library_Process(void);
/* Explicit BLOCKING maintenance, main-loop context only, when disconnected and
 * no timing-critical work is due. Commits at most one dirty bond per call.
 * 1 = all pending bonds saved; 0 = still pending, unsafe/busy, full or I/O error.
 * Never erases automatically. Inspect BT_Bonds_Dirty/Full/FlashErrors for details.
 * RAM-only new bonds will be lost on reset unless maintenance succeeds. */
uint8_t BT_Library_SaveBonds(void);
uint8_t BT_Library_IsSerialOpen(void);
/* Atomic enqueue: 1 accepted, 0 disconnected/invalid/full. Not delivery proof. */
uint8_t BT_Library_Send(const uint8_t *data, uint16_t length);
#endif
