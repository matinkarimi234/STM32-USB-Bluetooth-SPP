#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bt_bonds.h"
#include "usbh_bluetooth.h"
#include "bt_port.h"
volatile uint8_t BT_Connected, BT_ConnState, BT_PairCmdState, BT_HCI_CmdState;
static unsigned writes;
static uint8_t read_fails=1;
uint8_t BT_Port_BondRead(uint32_t offset,void *data,uint32_t length)
{ (void)offset;if(read_fails)return 0;memset(data,255,length);return 1; }
uint8_t BT_Port_BondWriteRecord(uint32_t offset,const void *data,uint32_t length)
{(void)offset;(void)data;(void)length;writes++;return 1;}
uint8_t BT_Port_BondErase(void){return 0;}
#include "../Src/bt_bonds.c"
int main(void)
{
 uint8_t address[6]={1},key[16]={2};
 BT_Bonds_Init();assert(storage_fault && BT_Bonds_FlashErrors==1);
 BT_Bonds_Remember(address,key,4);BT_Bonds_Process();assert(!writes && BT_Bonds_Dirty);
 read_fails=0;initialized=0;BT_Bonds_Init();assert(!storage_fault && !BT_Bonds_Count);
 BT_Bonds_Remember(address,key,4);BT_Bonds_Process();assert(writes==1 && !BT_Bonds_Dirty);
 assert(!BT_Bonds_ClearAll());
 puts("Bond port: failed reads suppress writes, reinitialization and erase failure passed");return 0;
}
