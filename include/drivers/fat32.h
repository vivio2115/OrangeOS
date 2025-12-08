
#ifndef FAT32_H
#define FAT32_H

#include <kernel/kernel.h>


struct fat32_boot_sector {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;  
    uint16_t total_sectors_16;  
    uint8_t media_type;
    uint16_t sectors_per_fat_16; 
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;  
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;        
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[420];
    uint16_t boot_signature_end;
} __attribute__((packed));


struct fat32_dir_entry {
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t first_cluster_high;  
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t first_cluster_low;   
    uint32_t file_size;
} __attribute__((packed));


#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LONG_NAME  0x0F


#define FAT32_CLUSTER_FREE    0x00000000
#define FAT32_CLUSTER_RESERVED_MIN 0x00000001
#define FAT32_CLUSTER_RESERVED_MAX 0x0FFFFFF6
#define FAT32_CLUSTER_BAD     0x0FFFFFF7
#define FAT32_CLUSTER_EOF_MIN 0x0FFFFFF8
#define FAT32_CLUSTER_EOF_MAX 0x0FFFFFFF


extern uint32_t fat32_partition_offset;


void fat32_init();
bool fat32_is_initialized();  
void fat32_debug();            
bool fat32_read_file(const char* filename, void* buffer, size_t max_size);
bool fat32_write_file(const char* filename, const void* buffer, size_t size);
bool fat32_list_directory(const char* path);
bool fat32_file_exists(const char* filename);
uint32_t fat32_get_file_size(const char* filename);


bool fat32_change_directory(const char* path);
bool fat32_create_directory(const char* path);
bool fat32_get_current_directory(char* path, size_t max_len);
uint32_t fat32_get_current_cluster();  
bool fat32_delete_file(const char* filename);

#endif 

