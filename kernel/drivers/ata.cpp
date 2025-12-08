
#include <drivers/ata.h>
#include <kernel/idt.h>
#include <lib/memory.h>


static bool ata_wait_ready() {
    uint8_t status;
    int timeout = 10000; 
    do {
        status = inb(ATA_STATUS);
        if (--timeout == 0) {
            return false; 
        }
        for (volatile int i = 0; i < 100; i++); 
    } while (status & ATA_STATUS_BSY);
    return true;
}


static bool ata_wait_data() {
    uint8_t status;
    int timeout = 10000; 
    do {
        status = inb(ATA_STATUS);
        if (status & ATA_STATUS_ERR) {
            return false; 
        }
        if (--timeout == 0) {
            return false; 
        }
        for (volatile int i = 0; i < 100; i++); 
    } while (!(status & ATA_STATUS_DRQ));
    return true;
}


void ata_init() {
    
    
    outb(ATA_DEVICE, 0xE0 | (0 << 4)); 
    
    
    for (volatile int i = 0; i < 1000; i++);
    
    
    uint8_t status = inb(ATA_STATUS);
    if (status == 0xFF || status == 0x00) {
        
        
        return;
    }
    
    
    for (int i = 0; i < 1000; i++) {
        status = inb(ATA_STATUS);
        if (!(status & ATA_STATUS_BSY)) {
            break;
        }
        for (volatile int j = 0; j < 1000; j++); 
    }
}


bool ata_read_sector(uint32_t lba, void* buffer) {
    if (!ata_wait_ready()) {
        return false; 
    }
    
    
    outb(ATA_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    
    
    outb(ATA_SECTOR_COUNT, 1);
    
    
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    
    
    outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);
    
    
    if (!ata_wait_data()) {
        return false; 
    }
    
    
    uint8_t status = inb(ATA_STATUS);
    if (status & ATA_STATUS_ERR) {
        return false;
    }
    
    
    uint16_t* dest = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        dest[i] = inw(ATA_DATA);
    }
    
    return true;
}


bool ata_write_sector(uint32_t lba, const void* buffer) {
    ata_wait_ready();
    
    
    outb(ATA_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    
    
    outb(ATA_SECTOR_COUNT, 1);
    
    
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    
    
    outb(ATA_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    
    ata_wait_data();
    
    
    const uint16_t* src = (const uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, src[i]);
    }
    
    
    outb(ATA_COMMAND, 0xE7); 
    
    
    ata_wait_ready();
    
    return true;
}


bool ata_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    uint8_t* buf = (uint8_t*)buffer;
    
    for (uint8_t i = 0; i < count; i++) {
        if (!ata_read_sector(lba + i, buf + (i * ATA_SECTOR_SIZE))) {
            return false;
        }
    }
    
    return true;
}


bool ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    const uint8_t* buf = (const uint8_t*)buffer;
    
    for (uint8_t i = 0; i < count; i++) {
        if (!ata_write_sector(lba + i, buf + (i * ATA_SECTOR_SIZE))) {
            return false;
        }
    }
    
    return true;
}

