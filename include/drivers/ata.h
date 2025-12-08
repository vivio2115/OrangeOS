
#ifndef ATA_H
#define ATA_H

#include <kernel/kernel.h>


#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DEVICE      0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7
#define ATA_ALT_STATUS  0x3F6


#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY      0xEC


#define ATA_STATUS_BSY  0x80  
#define ATA_STATUS_DRDY 0x40  
#define ATA_STATUS_DF   0x20  
#define ATA_STATUS_DRQ  0x08  
#define ATA_STATUS_ERR  0x01  


#define ATA_SECTOR_SIZE 512


void ata_init();
bool ata_read_sector(uint32_t lba, void* buffer);
bool ata_write_sector(uint32_t lba, const void* buffer);
bool ata_read_sectors(uint32_t lba, uint8_t count, void* buffer);
bool ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer);

#endif 

