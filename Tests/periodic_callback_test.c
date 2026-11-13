#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bt_library.h"
static BT_LibraryCallbacks cb;
static uint8_t open,can_send,bytes[10];
static unsigned calls,polls;
USBH_StatusTypeDef BT_Library_Register(USBH_HandleTypeDef *h,const BT_LibraryCallbacks *c) { (void)h;cb=*c;return USBH_OK; }
void BT_Library_Process(void) { polls++; }
uint8_t BT_Library_IsSerialOpen(void) { return open; }
uint8_t BT_Library_Send(const uint8_t *data,uint16_t length) { assert(length==10);calls++;memcpy(bytes,data,length);return can_send; }
uint32_t BT_Port_GetTick(void) { return polls*100; }
#include "../Examples/PeriodicCallback/app_bt_periodic.c"
int main(void)
{
 USBH_HandleTypeDef host={0};uint8_t first[10],data[300],out[256];unsigned i;
 assert(AppBT_Register(&host)==USBH_OK);
 AppBT_RequestTxFromISR();AppBT_RequestTxFromISR();assert(!calls && !polls);
 AppBT_Process();assert(!calls);open=1;
 data[0]=0x42;cb.received(data,1,cb.context); /* RX before first Process/open notice. */
 cb.serial_changed(1,cb.context);assert(AppBT_Read(out,256)==1 && out[0]==0x42);
 AppBT_RequestTxFromISR();AppBT_Process();assert(calls==1 && pending && !AppBT_TxPackets);memcpy(first,bytes,10);
 AppBT_RequestTxFromISR();can_send=1;AppBT_Process();assert(calls==2 && !memcmp(first,bytes,10) && AppBT_TxPackets==1);
 AppBT_Process();assert(AppBT_TxPackets==2);AppBT_Process();assert(calls==3);
 for(i=0;i<300;i++)data[i]=(uint8_t)i;
 cb.received(data,300,cb.context);memset(data,0,sizeof(data));
 assert(AppBT_RxBytes==301 && AppBT_RxDrops==44 && AppBT_Read(out,256)==256);
 for(i=0;i<256;i++)assert(out[i]==(uint8_t)i);
 open=0;cb.serial_changed(0,cb.context);AppBT_RequestTxFromISR();AppBT_Process();assert(calls==3 && !pending && !rx_count);
 puts("Periodic callback: ISR posts only, coalescing, exact retry, early-open RX retention, transient RX copy, overflow and disconnect passed");return 0;
}
