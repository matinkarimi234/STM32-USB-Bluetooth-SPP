#include "bt_config.h"
#include "bt_port.h"
#include "stm32f7xx_hal.h"
#include <string.h>
#ifndef BT_BONDS_FLASH_START
#error Set BT_BONDS_FLASH_START in the project configuration and reserve its erase sector
#endif
#include "bt_f746_flash_layout.h"
#if BT_BONDS_STORAGE_BYTES != BT_F746_BOND_SECTOR_BYTES
#error Reserve the entire selected erase sector; storage size does not match it
#endif
static uint8_t storage_valid(void)
{
    uint32_t size_kb = *(const volatile uint16_t *)FLASHSIZE_BASE;
    uint32_t end;
    if (size_kb != 512U && size_kb != 1024U) { return 0U; }
    end = 0x08000000UL + size_kb * 1024UL;
    return (uint8_t)(BT_BONDS_FLASH_START < end && BT_BONDS_STORAGE_BYTES <= end - BT_BONDS_FLASH_START);
}
uint32_t BT_Port_GetTick(void) { return HAL_GetTick(); }
uint8_t BT_Port_BondRead(uint32_t offset, void *data, uint32_t length)
{
    if (!storage_valid() || data == NULL || offset > BT_BONDS_STORAGE_BYTES || length > BT_BONDS_STORAGE_BYTES - offset) { return 0U; }
    memcpy(data, (const void *)(BT_BONDS_FLASH_START + offset), length);
    return 1U;
}
uint8_t BT_Port_BondWriteRecord(uint32_t offset, const void *data, uint32_t length)
{
    uint32_t i, word, address;
    const uint8_t *bytes = (const uint8_t *)data;
    uint8_t ok = 1U;
    if (!storage_valid() || data == NULL || length != 36U || offset % 36U != 0U ||
        offset > BT_BONDS_STORAGE_BYTES - length) { return 0U; }
    address = BT_BONDS_FLASH_START + offset;
    /* Refuse all writes to a non-erased slot, including a torn record. */
    for (i = 0U; i < length; i += 4U)
    { if (*(const volatile uint32_t *)(address + i) != 0xFFFFFFFFUL) { return 0U; } }
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0U; }
    for (i = 4U; i < length; i += 4U)
    {
        memcpy(&word, bytes + i, 4U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word) != HAL_OK) { ok = 0U; break; }
    }
    memcpy(&word, bytes, 4U);
    if (ok && HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word) != HAL_OK) { ok = 0U; }
    (void)HAL_FLASH_Lock();
    return ok;
}
uint8_t BT_Port_BondErase(void)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0UL;
    HAL_StatusTypeDef result;
    if (!storage_valid()) { return 0U; }
    memset(&erase, 0, sizeof(erase));
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = BT_F746_BOND_SECTOR;
    erase.NbSectors = 1U;
    if (HAL_FLASH_Unlock() != HAL_OK) { return 0U; }
    result = HAL_FLASHEx_Erase(&erase, &sector_error);
    (void)HAL_FLASH_Lock();
    return (result == HAL_OK) ? 1U : 0U;
}
