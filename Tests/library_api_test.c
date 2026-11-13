#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bt_library.h"
#include "bt_spp.h"
#include "usbh_bluetooth.h"
volatile uint8_t BT_Connected, BT_SPP_SerialOpen;
volatile BT_EndpointInfo_t BT_Endpoints;
USBH_ClassTypeDef USBH_Bluetooth_Class;
volatile uint8_t BT_Bonds_Dirty;
static unsigned registrations, saves, received_count, transitions, writes;
static BT_SPP_ReceiveCallback receiver;
static void *receiver_context;
static USBH_StatusTypeDef register_result = USBH_FAIL;
USBH_StatusTypeDef USBH_RegisterClass(USBH_HandleTypeDef *h, USBH_ClassTypeDef *c)
{ assert(h && c == &USBH_Bluetooth_Class); registrations++; return register_result; }
void BT_SPP_SetReceiver(BT_SPP_ReceiveCallback cb, void *ctx) { receiver=cb; receiver_context=ctx; }
uint8_t BT_SPP_Send(const uint8_t *data, uint16_t length) { assert(length==3 && data[2]==255); writes++; return 1; }
void BT_Bonds_Process(void) { saves++; if(!BT_Connected) BT_Bonds_Dirty=0; }
#include "../Src/bt_library.c"
static void rx(const uint8_t *data, uint16_t length, void *ctx)
{ assert(ctx==&received_count && data[0]==0 && length==3);received_count++;assert(BT_Library_Send(data,length)); }
static void changed(uint8_t open, void *ctx) { assert(ctx==&received_count);assert(open==(transitions==0));transitions++; }
int main(void)
{
 USBH_HandleTypeDef host={0},other={0};BT_LibraryCallbacks cb={rx,changed,&received_count};uint8_t bytes[]={0,128,255};
 BT_Library_Process();assert(!saves && !BT_Library_IsSerialOpen());
 assert(!BT_Library_SaveBonds() && !saves);
 assert(BT_Library_Register(NULL,&cb)==USBH_FAIL && !registrations);
 assert(BT_Library_Register(&host,&cb)==USBH_FAIL && !receiver);
 register_result=USBH_OK;assert(BT_Library_Register(&host,&cb)==USBH_OK);
 memset(&cb,0,sizeof(cb)); /* Library owns a copy, not the caller's struct. */
 assert(BT_Library_Register(&other,NULL)==USBH_BUSY && registrations==2);
 assert(!BT_Library_Send(bytes,3));BT_Connected=BT_SPP_SerialOpen=BT_Endpoints.PipesReady=1;
 BT_Library_Process();BT_Library_Process();assert(transitions==1 && saves==0);
 BT_Bonds_Dirty=1;assert(!BT_Library_SaveBonds() && saves==1);
 receiver(bytes,3,receiver_context);assert(received_count==1 && writes==1);
 BT_Connected=0;BT_Library_Process();assert(transitions==2 && !BT_Library_Send(bytes,3) && saves==1);
 assert(BT_Library_SaveBonds() && saves==2 && !BT_Bonds_Dirty);
 puts("Library API: registration failure/retry, singleton guard, callback ownership, byte dispatch, echo routing, serial transitions, explicit-only persistence passed");return 0;
}
