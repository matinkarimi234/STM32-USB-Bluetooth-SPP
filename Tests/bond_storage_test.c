#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "bt_bonds.h"
#include "usbh_bluetooth.h"
#include "../Src/bt_bonds.c"
#include "../Ports/STM32F746/bt_port_stm32f746.c"
volatile uint8_t BT_Connected, BT_ConnState, BT_PairCmdState, BT_HCI_CmdState;
uint16_t BT_Test_FlashSizeKB=1024U;
static unsigned erases, programs;
static int fail_write;
HAL_StatusTypeDef HAL_FLASH_Unlock(void){return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Lock(void){return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type,uint32_t addr,uint64_t value)
{
 uint32_t *p=(uint32_t *)(uintptr_t)addr; (void)type;
 assert(addr>=BT_BONDS_FLASH_START && addr+4<=BT_BONDS_FLASH_START+BT_BONDS_FLASH_BYTES);
 assert(*p==0xFFFFFFFFU);programs++;
 if(fail_write){fail_write=0;*p=0;return HAL_ERROR;}
 *p=(uint32_t)value;return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *e,uint32_t *error)
{(void)error;assert(e->Sector==BT_F746_BOND_SECTOR && e->NbSectors==1);erases++;memset((void *)BT_BONDS_FLASH_START,0xFF,BT_BONDS_FLASH_BYTES);return HAL_OK;}
uint32_t HAL_GetTick(void) { return 0U; }
static void reload(void){initialized=0;BT_Bonds_Init();}
int main(void)
{
 uint8_t address[6]={1,2,3,4,5,6}, key[16]={0xAB}, found[16]; BT_BondRecord *records;
 assert(VirtualAlloc((void *)0x08000000UL,0x00100000UL,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)==(void *)0x08000000UL);
 memset((void *)BT_BONDS_FLASH_START,0xFF,BT_BONDS_FLASH_BYTES);reload();assert(!BT_Bonds_Count);
 BT_Bonds_Remember(address,key,4);BT_Connected=1;BT_Bonds_Process();assert(!programs);
 BT_Connected=0;BT_ConnState=BT_CONN_ACCEPT_SEND;BT_Bonds_Process();assert(!programs);
 BT_ConnState=BT_CONN_IDLE;BT_Bonds_Process();assert(BT_Bonds_Writes==1 && !BT_Bonds_Dirty);
 reload();assert(BT_Bonds_Find(address,found) && !memcmp(key,found,16));
 records=(BT_BondRecord *)BT_BONDS_FLASH_START;
 /* Leave an erased hole at slot 1 and a valid record at slot 2. */
 records[2]=records[0];records[2].address[0]=7;records[2].seq=2;records[2].crc=BT_Bonds_Checksum(&records[2]);
 records[3].seq=0; /* torn record: erased magic with programmed body */
 reload();assert(free_slot==1 && BT_Bonds_Count==2);
 address[0]=8;BT_Bonds_Remember(address,key,4);BT_Bonds_Process();assert(BT_Bonds_Validate(&records[1]));
 address[0]=9;BT_Bonds_Remember(address,key,4);BT_Bonds_Process();assert(BT_Bonds_Validate(&records[4]));
 assert(records[2].address[0]==7 && records[3].magic==0xFFFFFFFFU);
 /* Failed programming retains dirty data and retries another slot. */
 address[0]=10;BT_Bonds_Remember(address,key,4);fail_write=1;BT_Bonds_Process();assert(BT_Bonds_Dirty);
 BT_Bonds_Process();assert(!BT_Bonds_Dirty);reload();assert(BT_Bonds_Find(address,found));
 records[4].crc^=1;reload();address[0]=9;assert(!BT_Bonds_Find(address,found));
 assert(erases==0);free_slot=BT_BOND_MAX_JOURNAL_SLOTS;address[0]=11;BT_Bonds_Remember(address,key,4);BT_Bonds_Process();assert(BT_Bonds_Full && BT_Bonds_Dirty);
 assert(!BT_Port_BondRead(BT_BONDS_STORAGE_BYTES,found,1));
 assert(!BT_Port_BondWriteRecord(0,found,16));
 BT_Connected=1;assert(!BT_Bonds_ClearAll() && !erases);
 BT_Connected=0;assert(BT_Bonds_ClearAll() && erases==1 && !BT_Bonds_Count);
 BT_Test_FlashSizeKB=0;assert(!BT_Port_BondRead(0,found,1) && !BT_Port_BondErase());
 BT_Test_FlashSizeKB=512;
 if (BT_BONDS_FLASH_START>=0x08080000UL) { assert(!BT_Port_BondRead(0,found,1) && !BT_Port_BondErase()); }
 else { assert(BT_Port_BondRead(0,found,1) && BT_Port_BondErase()); }
 BT_Test_FlashSizeKB=1024;
 puts("Bonds: persistence, deferred writes, erased holes, torn/corrupt records, failed-write retry, full journal without erase passed");return 0;
}
