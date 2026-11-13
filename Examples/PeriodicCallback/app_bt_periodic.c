#include "app_bt_periodic.h"
#include "bt_port.h"
#include <string.h>
#define RX_CAPACITY 256U
static uint8_t rx[RX_CAPACITY], pending_bytes[10];
static uint16_t rx_head, rx_tail, rx_count;
/* One aligned 32-bit writer in the timer ISR; main only reads it.
 * Timer requests may coalesce. No unbounded event queue builds while disconnected. */
static volatile uint32_t timer_sequence;
static uint32_t served_sequence, pending_sequence;
static uint8_t pending;
volatile uint32_t AppBT_RxBytes, AppBT_RxDrops, AppBT_TxPackets;
static void put32(uint8_t *out, uint32_t value)
{
    out[0]=(uint8_t)value; out[1]=(uint8_t)(value>>8);
    out[2]=(uint8_t)(value>>16); out[3]=(uint8_t)(value>>24);
}
static void received(const uint8_t *data, uint16_t length, void *context)
{
    uint16_t i;
    (void)context;
    AppBT_RxBytes += length;
    for (i=0U; i<length; ++i)
    {
        if (rx_count==RX_CAPACITY) { AppBT_RxDrops++; continue; }
        rx[rx_tail]=data[i]; rx_tail=(uint16_t)((rx_tail+1U)%RX_CAPACITY); rx_count++;
    }
}
static void changed(uint8_t open, void *context)
{
    (void)context;
    /* RX can arrive inside USBH_Process before its first open notification.
     * Preserve those bytes on open; discard the old stream on disconnect. */
    if (!open) { rx_head=rx_tail=rx_count=0U; }
    pending=0U; served_sequence=timer_sequence;
}
USBH_StatusTypeDef AppBT_Register(USBH_HandleTypeDef *host)
{
    BT_LibraryCallbacks callbacks;
    callbacks.received=received; callbacks.serial_changed=changed; callbacks.context=NULL;
    return BT_Library_Register(host,&callbacks);
}
void AppBT_RequestTxFromISR(void) { timer_sequence++; }
void AppBT_Process(void)
{
    uint32_t requested;
    BT_Library_Process();
    if (!BT_Library_IsSerialOpen()) { pending=0U; served_sequence=timer_sequence; return; }
    requested=timer_sequence;
    if (!pending && requested!=served_sequence)
    {
        pending_sequence=requested; pending_bytes[0]='B'; pending_bytes[1]='T';
        put32(pending_bytes+2U, AppBT_TxPackets); put32(pending_bytes+6U,BT_Port_GetTick());
        pending=1U;
    }
    /* One attempt per poll. On backpressure retain the exact payload for retry. */
    if (pending && BT_Library_Send(pending_bytes,sizeof(pending_bytes)))
    { pending=0U; served_sequence=pending_sequence; AppBT_TxPackets++; }
}
uint16_t AppBT_Read(uint8_t *out, uint16_t capacity)
{
    uint16_t n=0U;
    if (out==NULL) { return 0U; }
    while (n<capacity && rx_count!=0U)
    { out[n++]=rx[rx_head]; rx_head=(uint16_t)((rx_head+1U)%RX_CAPACITY); rx_count--; }
    return n;
}
