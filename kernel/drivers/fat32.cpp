
#include <drivers/fat32.h>
#include <drivers/ata.h>
#include <drivers/vga.h>
#include <kernel/heap.h>
#include <lib/memory.h>


static struct fat32_boot_sector boot_sector_data;
static struct fat32_boot_sector* boot_sector = &boot_sector_data;
static bool fat32_initialized = false;
static uint32_t fat_start_sector = 0;
static uint32_t data_start_sector = 0;
static uint32_t sectors_per_fat = 0;
static uint32_t bytes_per_cluster = 0;
static uint32_t root_cluster = 0;
static uint32_t current_directory_cluster = 0;  


uint32_t fat32_partition_offset = 0;


static void print_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    vga_write("0x");
    bool started = false;
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        if (nibble != 0 || started || i == 0) {
            vga_putchar(hex_chars[nibble]);
            started = true;
        }
    }
}


void fat32_debug() {
    vga_writeln("=== FAT32 Debug Information ===");
    vga_write("Initialized: ");
    vga_writeln(fat32_initialized ? "YES" : "NO");
    vga_write("Partition offset: ");
    print_hex(fat32_partition_offset);
    vga_writeln("");
    
    if (fat32_initialized) {
        vga_write("FAT start sector: ");
        print_hex(fat_start_sector);
        vga_writeln("");
        vga_write("Data start sector: ");
        print_hex(data_start_sector);
        vga_writeln("");
        vga_write("Root cluster: ");
        print_hex(root_cluster);
        vga_writeln("");
    } else {
        vga_writeln("File system not initialized - scanning for boot sectors...");
        
        
        uint8_t test_buffer[512];
        bool found = false;
        uint32_t test_offsets[] = {0, 2048, fat32_partition_offset};
        int num_offsets = 3;
        
        for (int idx = 0; idx < num_offsets; idx++) {
            uint32_t test_lba = test_offsets[idx];
            if (ata_read_sector(test_lba, test_buffer)) {
                struct fat32_boot_sector* test_bs = (struct fat32_boot_sector*)test_buffer;
                vga_write("  LBA ");
                print_hex(test_lba);
                vga_write(": bytes_per_sector=");
                print_hex(test_bs->bytes_per_sector);
                vga_write(", sectors_per_fat_16=");
                print_hex(test_bs->sectors_per_fat_16);
                vga_write(", sectors_per_fat_32=");
                print_hex(test_bs->sectors_per_fat_32);
                vga_write(", fs_type=");
                
                for (int k = 0; k < 8; k++) {
                    char c = test_bs->fs_type[k];
                    if (c >= 32 && c < 127) {
                        vga_putchar(c);
                    } else {
                        vga_write(".");
                    }
                }
                vga_writeln("");
                
                
                if (test_bs->bytes_per_sector == 512 && 
                    test_bs->sectors_per_fat_16 == 0 && 
                    test_bs->sectors_per_fat_32 > 0 &&
                    memcmp(test_bs->fs_type, "FAT32", 5) == 0) {
                    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
                    vga_writeln("    -> VALID FAT32 BOOT SECTOR!");
                    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                    found = true;
                } else {
                    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    vga_writeln("    -> Not a valid FAT32 boot sector");
                    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                }
            } else {
                vga_write("  LBA ");
                print_hex(test_lba);
                vga_writeln(": READ FAILED");
            }
        }
        
        if (!found) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_writeln("No valid FAT32 boot sector found at any tested offset!");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
    }
    vga_writeln("===============================");
}


void fat32_init() {
    
    fat32_initialized = false;
    fat_start_sector = 0;
    data_start_sector = 0;
    sectors_per_fat = 0;
    bytes_per_cluster = 0;
    root_cluster = 0;
    
    
    memset(boot_sector, 0, sizeof(struct fat32_boot_sector));
    
    
    
    uint32_t test_offsets[] = {fat32_partition_offset, 0, 2048};
    int num_offsets = 3;
    
    
    if (fat32_partition_offset == 0 || fat32_partition_offset == 2048) {
        num_offsets = 2;
        if (fat32_partition_offset == 0) {
            test_offsets[0] = 0;
            test_offsets[1] = 2048;
        } else {
            test_offsets[0] = 2048;
            test_offsets[1] = 0;
        }
    }
    
    uint32_t boot_sector_lba = 0xFFFFFFFF;
    uint8_t test_buffer[512];
    
    
    for (int idx = 0; idx < num_offsets; idx++) {
        uint32_t test_lba = test_offsets[idx];
        if (ata_read_sector(test_lba, test_buffer)) {
            struct fat32_boot_sector* test_bs = (struct fat32_boot_sector*)test_buffer;
            
            
            if (test_bs->bytes_per_sector == 512 && 
                test_bs->sectors_per_fat_16 == 0 && 
                test_bs->sectors_per_fat_32 > 0 &&
                memcmp(test_bs->fs_type, "FAT32", 5) == 0) {
                
                boot_sector_lba = test_lba;
                memcpy(boot_sector, test_bs, sizeof(struct fat32_boot_sector));
                fat32_partition_offset = test_lba;
                break;
            }
        }
    }
    
    
    if (boot_sector_lba == 0xFFFFFFFF) {
        fat32_initialized = false;
        return;
    }
    
    
    if (boot_sector->bytes_per_sector != 512) {
        fat32_initialized = false;
        return;
    }
    
    
    if (boot_sector->sectors_per_fat_16 != 0 || boot_sector->sectors_per_fat_32 == 0) {
        fat32_initialized = false;
        return;
    }
    
    
    
    if (memcmp(boot_sector->fs_type, "FAT32", 5) != 0) {
        fat32_initialized = false;
        return;
    }
    
    
    fat_start_sector = boot_sector->reserved_sectors;
    sectors_per_fat = boot_sector->sectors_per_fat_32;
    bytes_per_cluster = boot_sector->sectors_per_cluster * boot_sector->bytes_per_sector;
    
    
    root_cluster = boot_sector->root_cluster;
    
    
    uint32_t fat_size = sectors_per_fat * boot_sector->fat_count;
    data_start_sector = fat_start_sector + fat_size;
    
    
    fat_start_sector += fat32_partition_offset;
    data_start_sector += fat32_partition_offset;
    
    fat32_initialized = true;
    
    
    current_directory_cluster = root_cluster;
}


bool fat32_is_initialized() {
    return fat32_initialized;
}


static void filename_to_fat32(const char* filename, char* fat32_name) {
    memset(fat32_name, ' ', 11);
    
    int i = 0;
    int j = 0;
    
    
    while (filename[i] != '\0' && filename[i] != '.' && j < 8) {
        if (filename[i] >= 'a' && filename[i] <= 'z') {
            fat32_name[j] = filename[i] - 32; 
        } else {
            fat32_name[j] = filename[i];
        }
        i++;
        j++;
    }
    
    
    if (filename[i] == '.') {
        i++;
    }
    
    
    j = 8;
    int ext_count = 0;
    while (filename[i] != '\0' && ext_count < 3) {
        if (filename[i] >= 'a' && filename[i] <= 'z') {
            fat32_name[j] = filename[i] - 32; 
        } else {
            fat32_name[j] = filename[i];
        }
        i++;
        j++;
        ext_count++;
    }
}


static uint32_t read_fat32_entry(uint32_t cluster) {
    
    uint32_t fat_offset = cluster * 4; 
    
    
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t fat_offset_in_sector = fat_offset % 512;
    
    uint8_t fat_buffer[512];
    if (!ata_read_sector(fat_sector, fat_buffer)) {
        return 0xFFFFFFFF; 
    }
    
    
    uint32_t value = *(uint32_t*)&fat_buffer[fat_offset_in_sector];
    return value & 0x0FFFFFFF; 
}


static bool write_fat32_entry(uint32_t cluster, uint32_t value) {
    
    value &= 0x0FFFFFFF;
    
    
    uint32_t fat_offset = cluster * 4;
    
    
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t fat_offset_in_sector = fat_offset % 512;
    
    uint8_t fat_buffer[512];
    if (!ata_read_sector(fat_sector, fat_buffer)) {
        return false;
    }
    
    
    uint32_t* fat_dword = (uint32_t*)&fat_buffer[fat_offset_in_sector];
    *fat_dword = (*fat_dword & 0xF0000000) | value; 
    
    
    if (!ata_write_sector(fat_sector, fat_buffer)) {
        return false;
    }
    
    
    if (boot_sector->fat_count > 1) {
        uint32_t backup_fat_sector = fat_sector + sectors_per_fat;
        if (!ata_write_sector(backup_fat_sector, fat_buffer)) {
            return false;
        }
    }
    
    return true;
}


static uint32_t cluster_to_lba(uint32_t cluster) {
    
    
    return data_start_sector + ((cluster - 2) * boot_sector->sectors_per_cluster);
}


static uint32_t find_free_cluster() {
    
    
    uint32_t max_clusters = 10000;
    if (sectors_per_fat * 512 / 4 < max_clusters) {
        max_clusters = sectors_per_fat * 512 / 4;
    }
    
    for (uint32_t cluster = 2; cluster < max_clusters; cluster++) {
        uint32_t fat_value = read_fat32_entry(cluster);
        if (fat_value == FAT32_CLUSTER_FREE) {
            return cluster;
        }
    }
    
    return 0; 
}


static void free_cluster_chain(uint32_t first_cluster) {
    uint32_t cluster = first_cluster;
    
    while (cluster != 0 && cluster < FAT32_CLUSTER_EOF_MIN) {
        uint32_t next = read_fat32_entry(cluster);
        write_fat32_entry(cluster, FAT32_CLUSTER_FREE);
        cluster = next;
    }
}


static bool read_directory_cluster(uint32_t cluster, struct fat32_dir_entry* entries, uint32_t max_entries) {
    uint32_t lba = cluster_to_lba(cluster);
    uint32_t sectors = boot_sector->sectors_per_cluster;
    
    
    uint8_t* cluster_buffer = (uint8_t*)kmalloc(sectors * 512);
    if (cluster_buffer == NULL) {
        return false;
    }
    
    
    if (!ata_read_sectors(lba, sectors, cluster_buffer)) {
        kfree(cluster_buffer);
        return false;
    }
    
    
    uint32_t entries_per_sector = 512 / sizeof(struct fat32_dir_entry);
    uint32_t total_entries = sectors * entries_per_sector;
    if (total_entries > max_entries) {
        total_entries = max_entries;
    }
    
    memcpy(entries, cluster_buffer, total_entries * sizeof(struct fat32_dir_entry));
    
    kfree(cluster_buffer);
    return true;
}


static struct fat32_dir_entry* find_file_in_directory(uint32_t dir_cluster, const char* filename);



static uint32_t path_to_cluster(const char* path) {
    if (!fat32_initialized) {
        return 0;
    }
    
    
    if (path == NULL || strlen(path) == 0 || (strlen(path) == 1 && path[0] == '/')) {
        return root_cluster;
    }
    
    
    uint32_t current_cluster = (path[0] == '/') ? root_cluster : current_directory_cluster;
    
    
    const char* path_ptr = (path[0] == '/') ? path + 1 : path;
    
    
    char component[256];
    int comp_idx = 0;
    
    while (*path_ptr != '\0') {
        if (*path_ptr == '/') {
            
            if (comp_idx > 0) {
                component[comp_idx] = '\0';
                
                
                struct fat32_dir_entry* entry = find_file_in_directory(current_cluster, component);
                if (entry == NULL) {
                    return 0; 
                }
                
                
                if (!(entry->attributes & FAT32_ATTR_DIRECTORY)) {
                    kfree(entry);
                    return 0; 
                }
                
                
                current_cluster = entry->first_cluster_low | (entry->first_cluster_high << 16);
                kfree(entry);
                
                comp_idx = 0;
            }
        } else {
            if (comp_idx < 255) {
                component[comp_idx++] = *path_ptr;
            }
        }
        path_ptr++;
    }
    
    
    if (comp_idx > 0) {
        component[comp_idx] = '\0';
        
        
        struct fat32_dir_entry* entry = find_file_in_directory(current_cluster, component);
        if (entry == NULL) {
            return 0; 
        }
        
        
        if (!(entry->attributes & FAT32_ATTR_DIRECTORY)) {
            kfree(entry);
            return 0; 
        }
        
        
        current_cluster = entry->first_cluster_low | (entry->first_cluster_high << 16);
        kfree(entry);
    }
    
    return current_cluster;
}


static struct fat32_dir_entry* find_file_in_directory(uint32_t dir_cluster, const char* filename) {
    char fat32_name[11];
    filename_to_fat32(filename, fat32_name);
    
    
    uint32_t entries_per_cluster = (bytes_per_cluster) / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* dir_entries = (struct fat32_dir_entry*)kmalloc(entries_per_cluster * sizeof(struct fat32_dir_entry));
    
    if (dir_entries == NULL) {
        return NULL;
    }
    
    uint32_t current_cluster = dir_cluster;
    
    
    while (current_cluster != 0 && current_cluster < FAT32_CLUSTER_EOF_MIN) {
        
        if (!read_directory_cluster(current_cluster, dir_entries, entries_per_cluster)) {
            kfree(dir_entries);
            return NULL;
        }
        
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            
            if (dir_entries[i].filename[0] == 0x00) {
                
                kfree(dir_entries);
                return NULL;
            }
            if (dir_entries[i].filename[0] == 0xE5) {
                continue; 
            }
            
            
            if (dir_entries[i].attributes == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            
            if (memcmp(dir_entries[i].filename, fat32_name, 11) == 0) {
                
                struct fat32_dir_entry* entry = (struct fat32_dir_entry*)kmalloc(sizeof(struct fat32_dir_entry));
                memcpy(entry, &dir_entries[i], sizeof(struct fat32_dir_entry));
                kfree(dir_entries);
                return entry;
            }
        }
        
        
        current_cluster = read_fat32_entry(current_cluster);
        if (current_cluster == 0 || current_cluster >= FAT32_CLUSTER_EOF_MIN) {
            break;
        }
    }
    
    kfree(dir_entries);
    return NULL;
}


bool fat32_file_exists(const char* filename) {
    if (!fat32_initialized) {
        return false;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = filename;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t dir_cluster = root_cluster;
    const char* file_part = filename;
    
    if (last_slash != NULL) {
        
        char dir_path[256];
        size_t dir_len = last_slash - filename;
        if (dir_len > 0 && dir_len < 256) {
            strncpy(dir_path, filename, dir_len);
            dir_path[dir_len] = '\0';
            dir_cluster = path_to_cluster(dir_path);
            if (dir_cluster == 0) {
                return false;
            }
        }
        file_part = last_slash + 1;
    } else {
        
        dir_cluster = current_directory_cluster;
    }
    
    struct fat32_dir_entry* entry = find_file_in_directory(dir_cluster, file_part);
    if (entry != NULL) {
        kfree(entry);
        return true;
    }
    
    return false;
}


uint32_t fat32_get_file_size(const char* filename) {
    if (!fat32_initialized) {
        return 0;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = filename;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t dir_cluster = root_cluster;
    const char* file_part = filename;
    
    if (last_slash != NULL) {
        
        char dir_path[256];
        size_t dir_len = last_slash - filename;
        if (dir_len > 0 && dir_len < 256) {
            strncpy(dir_path, filename, dir_len);
            dir_path[dir_len] = '\0';
            dir_cluster = path_to_cluster(dir_path);
            if (dir_cluster == 0) {
                return 0;
            }
        }
        file_part = last_slash + 1;
    } else {
        
        dir_cluster = current_directory_cluster;
    }
    
    struct fat32_dir_entry* entry = find_file_in_directory(dir_cluster, file_part);
    if (entry != NULL) {
        uint32_t size = entry->file_size;
        kfree(entry);
        return size;
    }
    
    return 0;
}


bool fat32_read_file(const char* filename, void* buffer, size_t max_size) {
    if (!fat32_initialized) {
        vga_writeln("FAT32: File system not initialized");
        return false;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = filename;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t dir_cluster = root_cluster;
    const char* file_part = filename;
    
    if (last_slash != NULL) {
        
        char dir_path[256];
        size_t dir_len = last_slash - filename;
        if (dir_len > 0 && dir_len < 256) {
            strncpy(dir_path, filename, dir_len);
            dir_path[dir_len] = '\0';
            dir_cluster = path_to_cluster(dir_path);
            if (dir_cluster == 0) {
                vga_write("FAT32: Directory '");
                vga_write(dir_path);
                vga_writeln("' not found");
                return false;
            }
        }
        file_part = last_slash + 1;
    } else {
        
        dir_cluster = current_directory_cluster;
    }
    
    
    struct fat32_dir_entry* entry = find_file_in_directory(dir_cluster, file_part);
    if (entry == NULL) {
        vga_write("FAT32: File '");
        vga_write(filename);
        vga_writeln("' not found");
        return false;
    }
    
    
    uint32_t file_size = entry->file_size;
    uint32_t cluster = entry->first_cluster_low | (entry->first_cluster_high << 16);
    
    if (file_size > max_size) {
        file_size = max_size;
    }
    
    uint8_t* dest = (uint8_t*)buffer;
    uint32_t bytes_read = 0;
    
    
    while (cluster != 0 && cluster < FAT32_CLUSTER_EOF_MIN && bytes_read < file_size) {
        
        uint32_t cluster_lba = cluster_to_lba(cluster);
        
        
        uint32_t bytes_to_read = file_size - bytes_read;
        if (bytes_to_read > bytes_per_cluster) {
            bytes_to_read = bytes_per_cluster;
        }
        
        uint8_t* cluster_buffer = (uint8_t*)kmalloc(bytes_per_cluster);
        if (cluster_buffer == NULL) {
            kfree(entry);
            return false;
        }
        
        if (!ata_read_sectors(cluster_lba, boot_sector->sectors_per_cluster, cluster_buffer)) {
            kfree(cluster_buffer);
            kfree(entry);
            return false;
        }
        
        
        memcpy(dest + bytes_read, cluster_buffer, bytes_to_read);
        bytes_read += bytes_to_read;
        
        kfree(cluster_buffer);
        
        
        cluster = read_fat32_entry(cluster);
    }
    
    kfree(entry);
    return true;
}


bool fat32_write_file(const char* filename, const void* buffer, size_t size) {
    if (!fat32_initialized) {
        return false;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = filename;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t dir_cluster = root_cluster;
    const char* file_part = filename;
    
    if (last_slash != NULL) {
        
        char dir_path[256];
        size_t dir_len = last_slash - filename;
        if (dir_len > 0 && dir_len < 256) {
            strncpy(dir_path, filename, dir_len);
            dir_path[dir_len] = '\0';
            dir_cluster = path_to_cluster(dir_path);
            if (dir_cluster == 0) {
                vga_write("FAT32: Directory '");
                vga_write(dir_path);
                vga_writeln("' not found");
                return false;
            }
        }
        file_part = last_slash + 1;
    } else {
        
        dir_cluster = current_directory_cluster;
    }
    
    
    char fat32_name[11];
    filename_to_fat32(file_part, fat32_name);
    
    
    struct fat32_dir_entry* existing_entry = find_file_in_directory(dir_cluster, file_part);
    uint32_t old_first_cluster = 0;
    bool found_existing = (existing_entry != NULL);
    
    if (found_existing) {
        old_first_cluster = existing_entry->first_cluster_low | (existing_entry->first_cluster_high << 16);
        kfree(existing_entry);
    }
    
    
    if (found_existing && old_first_cluster != 0) {
        free_cluster_chain(old_first_cluster);
    }
    
    
    uint32_t clusters_needed = 0;
    if (size > 0) {
        clusters_needed = (size + bytes_per_cluster - 1) / bytes_per_cluster;
    }
    
    
    uint32_t first_cluster = 0;
    uint32_t prev_cluster = 0;
    
    for (uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t cluster = find_free_cluster();
        if (cluster == 0) {
            
            if (first_cluster != 0) {
                free_cluster_chain(first_cluster);
            }
            vga_writeln("FAT32: Out of disk space");
            return false;
        }
        
        if (i == 0) {
            first_cluster = cluster;
        } else {
            
            write_fat32_entry(prev_cluster, cluster);
        }
        
        prev_cluster = cluster;
        
        
        if (i == clusters_needed - 1) {
            write_fat32_entry(cluster, FAT32_CLUSTER_EOF_MIN);
        }
    }
    
    
    const uint8_t* src = (const uint8_t*)buffer;
    uint32_t bytes_written = 0;
    uint32_t current_cluster = first_cluster;
    
    for (uint32_t i = 0; i < clusters_needed && bytes_written < size; i++) {
        uint32_t cluster_lba = cluster_to_lba(current_cluster);
        uint32_t bytes_to_write = size - bytes_written;
        if (bytes_to_write > bytes_per_cluster) {
            bytes_to_write = bytes_per_cluster;
        }
        
        
        uint8_t* cluster_buffer = (uint8_t*)kmalloc(bytes_per_cluster);
        if (cluster_buffer == NULL) {
            if (first_cluster != 0) {
                free_cluster_chain(first_cluster);
            }
            vga_writeln("FAT32: Out of memory");
            return false;
        }
        
        memset(cluster_buffer, 0, bytes_per_cluster);
        memcpy(cluster_buffer, src + bytes_written, bytes_to_write);
        
        
        if (!ata_write_sectors(cluster_lba, boot_sector->sectors_per_cluster, cluster_buffer)) {
            kfree(cluster_buffer);
            if (first_cluster != 0) {
                free_cluster_chain(first_cluster);
            }
            vga_writeln("FAT32: Failed to write data");
            return false;
        }
        
        kfree(cluster_buffer);
        bytes_written += bytes_to_write;
        
        
        if (i < clusters_needed - 1) {
            current_cluster = read_fat32_entry(current_cluster);
        }
    }
    
    
    
    uint32_t entries_per_cluster = bytes_per_cluster / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* dir_entries = (struct fat32_dir_entry*)kmalloc(entries_per_cluster * sizeof(struct fat32_dir_entry));
    if (dir_entries == NULL) {
        if (first_cluster != 0) {
            free_cluster_chain(first_cluster);
        }
        return false;
    }
    
    
    uint32_t target_cluster = dir_cluster;
    uint32_t dir_index = 0xFFFFFFFF;
    bool found_slot = false;
    
    while (target_cluster != 0 && target_cluster < FAT32_CLUSTER_EOF_MIN && !found_slot) {
        if (!read_directory_cluster(target_cluster, dir_entries, entries_per_cluster)) {
            kfree(dir_entries);
            if (first_cluster != 0) {
                free_cluster_chain(first_cluster);
            }
            return false;
        }
        
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (dir_entries[i].filename[0] == 0x00) {
                
                dir_index = i;
                found_slot = true;
                break;
            }
            if (dir_entries[i].filename[0] == 0xE5) {
                
                if (dir_index == 0xFFFFFFFF) {
                    dir_index = i;
                }
            } else if (memcmp(dir_entries[i].filename, fat32_name, 11) == 0) {
                
                dir_index = i;
                found_slot = true;
                break;
            }
        }
        
        if (!found_slot) {
            
            uint32_t next_cluster = read_fat32_entry(target_cluster);
            if (next_cluster == 0 || next_cluster >= FAT32_CLUSTER_EOF_MIN) {
                
                uint32_t new_cluster = find_free_cluster();
                if (new_cluster == 0) {
                    kfree(dir_entries);
                    if (first_cluster != 0) {
                        free_cluster_chain(first_cluster);
                    }
                    vga_writeln("FAT32: Out of disk space");
                    return false;
                }
                write_fat32_entry(target_cluster, new_cluster);
                write_fat32_entry(new_cluster, FAT32_CLUSTER_EOF_MIN);
                target_cluster = new_cluster;
                
                uint8_t* zero_buffer = (uint8_t*)kmalloc(bytes_per_cluster);
                if (zero_buffer != NULL) {
                    memset(zero_buffer, 0, bytes_per_cluster);
                    uint32_t new_lba = cluster_to_lba(new_cluster);
                    ata_write_sectors(new_lba, boot_sector->sectors_per_cluster, zero_buffer);
                    kfree(zero_buffer);
                }
                dir_index = 0;
                found_slot = true;
            } else {
                target_cluster = next_cluster;
            }
        }
    }
    
    if (dir_index == 0xFFFFFFFF) {
        kfree(dir_entries);
        if (first_cluster != 0) {
            free_cluster_chain(first_cluster);
        }
        vga_writeln("FAT32: Directory full");
        return false;
    }
    
    
    if (dir_entries[dir_index].filename[0] == 0x00 || dir_entries[dir_index].filename[0] == 0xE5) {
        
        memset(&dir_entries[dir_index], 0, sizeof(struct fat32_dir_entry));
        memcpy(dir_entries[dir_index].filename, fat32_name, 11);
    }
    
    dir_entries[dir_index].first_cluster_low = first_cluster & 0xFFFF;
    dir_entries[dir_index].first_cluster_high = (first_cluster >> 16) & 0xFFFF;
    dir_entries[dir_index].file_size = size;
    dir_entries[dir_index].attributes = 0; 
    
    
    uint32_t dir_lba = cluster_to_lba(target_cluster);
    if (!ata_write_sectors(dir_lba, boot_sector->sectors_per_cluster, dir_entries)) {
        kfree(dir_entries);
        if (first_cluster != 0) {
            free_cluster_chain(first_cluster);
        }
        vga_writeln("FAT32: Failed to update directory");
        return false;
    }
    
    kfree(dir_entries);
    return true;
}


bool fat32_list_directory(const char* path) {
    if (!fat32_initialized) {
        vga_writeln("FAT32: File system not initialized");
        return false;
    }
    
    
    uint32_t list_cluster = root_cluster;
    
    if (path != NULL && strlen(path) > 0) {
        if (strcmp(path, "/") == 0) {
            list_cluster = root_cluster;
        } else {
            list_cluster = path_to_cluster(path);
            if (list_cluster == 0) {
                vga_write("FAT32: Directory '");
                vga_write(path);
                vga_writeln("' not found");
                return false;
            }
        }
    } else {
        
        list_cluster = current_directory_cluster;
    }
    
    
    uint32_t entries_per_cluster = bytes_per_cluster / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* dir_entries = (struct fat32_dir_entry*)kmalloc(entries_per_cluster * sizeof(struct fat32_dir_entry));
    
    if (dir_entries == NULL) {
        vga_writeln("FAT32: Out of memory");
        return false;
    }
    
    
    vga_writeln("Files:");
    bool found_any = false;
    
    uint32_t current_cluster = list_cluster;
    
    
    while (current_cluster != 0 && current_cluster < FAT32_CLUSTER_EOF_MIN) {
        if (!read_directory_cluster(current_cluster, dir_entries, entries_per_cluster)) {
            kfree(dir_entries);
            return false;
        }
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (dir_entries[i].filename[0] == 0x00) {
                
                kfree(dir_entries);
                if (!found_any) {
                    vga_writeln("(empty)");
                }
                return true;
            }
            if (dir_entries[i].filename[0] == 0xE5) {
                continue; 
            }
            
            
            if (dir_entries[i].attributes == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            
            if (dir_entries[i].attributes & FAT32_ATTR_VOLUME_ID) {
                continue;
            }
            
            found_any = true;
            
            
            bool is_dir = (dir_entries[i].attributes & FAT32_ATTR_DIRECTORY) != 0;
            
            
            for (int j = 0; j < 8; j++) {
                char c = dir_entries[i].filename[j];
                if (c != ' ') {
                    vga_putchar(c);
                }
            }
            
            
            if (!is_dir) {
                vga_putchar('.');
                for (int j = 8; j < 11; j++) {
                    char c = dir_entries[i].filename[j];
                    if (c != ' ') {
                        vga_putchar(c);
                    }
                }
            }
            
            
            if (is_dir) {
                vga_writeln(" <DIR>");
            } else {
                vga_write(" (");
                char size_str[32];
                itoa((int)dir_entries[i].file_size, size_str, 10);
                vga_write(size_str);
                vga_writeln(" bytes)");
            }
        }
        
        
        current_cluster = read_fat32_entry(current_cluster);
        if (current_cluster == 0 || current_cluster >= FAT32_CLUSTER_EOF_MIN) {
            break;
        }
    }
    
    if (!found_any) {
        vga_writeln("(empty)");
    }
    
    kfree(dir_entries);
    return true;
}


bool fat32_change_directory(const char* path) {
    if (!fat32_initialized) {
        return false;
    }
    
    
    uint32_t target_cluster = path_to_cluster(path);
    if (target_cluster == 0) {
        return false;
    }
    
    
    
    
    
    current_directory_cluster = target_cluster;
    return true;
}


uint32_t fat32_get_current_cluster() {
    return current_directory_cluster;
}


bool fat32_create_directory(const char* path) {
    if (!fat32_initialized) {
        return false;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = path;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t parent_cluster = root_cluster;
    const char* dir_name = path;
    
    if (last_slash != NULL) {
        
        char parent_path[256];
        size_t parent_len = last_slash - path;
        if (parent_len > 0 && parent_len < 256) {
            strncpy(parent_path, path, parent_len);
            parent_path[parent_len] = '\0';
            parent_cluster = path_to_cluster(parent_path);
            if (parent_cluster == 0) {
                vga_write("FAT32: Parent directory '");
                vga_write(parent_path);
                vga_writeln("' not found");
                return false;
            }
        }
        dir_name = last_slash + 1;
    } else {
        
        parent_cluster = current_directory_cluster;
    }
    
    
    struct fat32_dir_entry* existing = find_file_in_directory(parent_cluster, dir_name);
    if (existing != NULL) {
        kfree(existing);
        vga_write("FAT32: Directory '");
        vga_write(dir_name);
        vga_writeln("' already exists");
        return false;
    }
    
    
    uint32_t new_cluster = find_free_cluster();
    if (new_cluster == 0) {
        vga_writeln("FAT32: Out of disk space");
        return false;
    }
    
    
    write_fat32_entry(new_cluster, FAT32_CLUSTER_EOF_MIN);
    
    
    uint32_t entries_per_cluster = bytes_per_cluster / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* new_dir = (struct fat32_dir_entry*)kmalloc(bytes_per_cluster);
    if (new_dir == NULL) {
        write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
        return false;
    }
    
    memset(new_dir, 0, bytes_per_cluster);
    
    
    memset(new_dir[0].filename, ' ', 11);
    new_dir[0].filename[0] = '.';
    new_dir[0].attributes = FAT32_ATTR_DIRECTORY;
    new_dir[0].first_cluster_low = new_cluster & 0xFFFF;
    new_dir[0].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
    new_dir[0].file_size = 0;
    
    
    memset(new_dir[1].filename, ' ', 11);
    new_dir[1].filename[0] = '.';
    new_dir[1].filename[1] = '.';
    new_dir[1].attributes = FAT32_ATTR_DIRECTORY;
    new_dir[1].first_cluster_low = parent_cluster & 0xFFFF;
    new_dir[1].first_cluster_high = (parent_cluster >> 16) & 0xFFFF;
    new_dir[1].file_size = 0;
    
    
    uint32_t new_lba = cluster_to_lba(new_cluster);
    if (!ata_write_sectors(new_lba, boot_sector->sectors_per_cluster, new_dir)) {
        kfree(new_dir);
        write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
        return false;
    }
    
    kfree(new_dir);
    
    
    char fat32_name[11];
    filename_to_fat32(dir_name, fat32_name);
    
    
    uint32_t entries_per_parent = bytes_per_cluster / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* parent_entries = (struct fat32_dir_entry*)kmalloc(entries_per_parent * sizeof(struct fat32_dir_entry));
    if (parent_entries == NULL) {
        write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
        return false;
    }
    
    uint32_t parent_target = parent_cluster;
    uint32_t dir_index = 0xFFFFFFFF;
    bool found_slot = false;
    
    while (parent_target != 0 && parent_target < FAT32_CLUSTER_EOF_MIN && !found_slot) {
        if (!read_directory_cluster(parent_target, parent_entries, entries_per_parent)) {
            kfree(parent_entries);
            write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
            return false;
        }
        
        for (uint32_t i = 0; i < entries_per_parent; i++) {
            if (parent_entries[i].filename[0] == 0x00) {
                dir_index = i;
                found_slot = true;
                break;
            }
            if (parent_entries[i].filename[0] == 0xE5 && dir_index == 0xFFFFFFFF) {
                dir_index = i;
            }
        }
        
        if (!found_slot) {
            uint32_t next = read_fat32_entry(parent_target);
            if (next == 0 || next >= FAT32_CLUSTER_EOF_MIN) {
                
                uint32_t new_parent_cluster = find_free_cluster();
                if (new_parent_cluster == 0) {
                    kfree(parent_entries);
                    write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
                    vga_writeln("FAT32: Out of disk space");
                    return false;
                }
                write_fat32_entry(parent_target, new_parent_cluster);
                write_fat32_entry(new_parent_cluster, FAT32_CLUSTER_EOF_MIN);
                parent_target = new_parent_cluster;
                
                uint8_t* zero_buffer = (uint8_t*)kmalloc(bytes_per_cluster);
                if (zero_buffer != NULL) {
                    memset(zero_buffer, 0, bytes_per_cluster);
                    uint32_t new_parent_lba = cluster_to_lba(new_parent_cluster);
                    ata_write_sectors(new_parent_lba, boot_sector->sectors_per_cluster, zero_buffer);
                    kfree(zero_buffer);
                }
                dir_index = 0;
                found_slot = true;
            } else {
                parent_target = next;
            }
        }
    }
    
    if (dir_index == 0xFFFFFFFF) {
        kfree(parent_entries);
        write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
        vga_writeln("FAT32: Parent directory full");
        return false;
    }
    
    
    memset(&parent_entries[dir_index], 0, sizeof(struct fat32_dir_entry));
    memcpy(parent_entries[dir_index].filename, fat32_name, 11);
    parent_entries[dir_index].attributes = FAT32_ATTR_DIRECTORY;
    parent_entries[dir_index].first_cluster_low = new_cluster & 0xFFFF;
    parent_entries[dir_index].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
    parent_entries[dir_index].file_size = 0;
    
    
    uint32_t parent_lba = cluster_to_lba(parent_target);
    if (!ata_write_sectors(parent_lba, boot_sector->sectors_per_cluster, parent_entries)) {
        kfree(parent_entries);
        write_fat32_entry(new_cluster, FAT32_CLUSTER_FREE);
        return false;
    }
    
    kfree(parent_entries);
    return true;
}


bool fat32_delete_file(const char* filename) {
    if (!fat32_initialized) {
        return false;
    }
    
    
    const char* last_slash = NULL;
    const char* ptr = filename;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    uint32_t dir_cluster = root_cluster;
    const char* file_part = filename;
    
    if (last_slash != NULL) {
        char dir_path[256];
        size_t dir_len = last_slash - filename;
        if (dir_len > 0 && dir_len < 256) {
            strncpy(dir_path, filename, dir_len);
            dir_path[dir_len] = '\0';
            dir_cluster = path_to_cluster(dir_path);
            if (dir_cluster == 0) {
                return false;
            }
        }
        file_part = last_slash + 1;
    } else {
        dir_cluster = current_directory_cluster;
    }
    
    
    char fat32_name[11];
    filename_to_fat32(file_part, fat32_name);
    
    uint32_t entries_per_cluster = bytes_per_cluster / sizeof(struct fat32_dir_entry);
    struct fat32_dir_entry* dir_entries = (struct fat32_dir_entry*)kmalloc(entries_per_cluster * sizeof(struct fat32_dir_entry));
    if (dir_entries == NULL) {
        return false;
    }
    
    uint32_t current_cluster = dir_cluster;
    bool found = false;
    uint32_t found_index = 0;
    uint32_t found_cluster = 0;
    
    while (current_cluster != 0 && current_cluster < FAT32_CLUSTER_EOF_MIN && !found) {
        if (!read_directory_cluster(current_cluster, dir_entries, entries_per_cluster)) {
            kfree(dir_entries);
            return false;
        }
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            if (dir_entries[i].filename[0] == 0x00) {
                break; 
            }
            if (dir_entries[i].filename[0] == 0xE5) {
                continue; 
            }
            
            if (memcmp(dir_entries[i].filename, fat32_name, 11) == 0) {
                found = true;
                found_index = i;
                found_cluster = current_cluster;
                
                
                uint32_t file_cluster = dir_entries[i].first_cluster_low | (dir_entries[i].first_cluster_high << 16);
                if (file_cluster != 0) {
                    free_cluster_chain(file_cluster);
                }
                break;
            }
        }
        
        if (!found) {
            current_cluster = read_fat32_entry(current_cluster);
            if (current_cluster == 0 || current_cluster >= FAT32_CLUSTER_EOF_MIN) {
                break;
            }
        }
    }
    
    if (!found) {
        kfree(dir_entries);
        return false;
    }
    
    
    dir_entries[found_index].filename[0] = 0xE5;
    
    
    uint32_t dir_lba = cluster_to_lba(found_cluster);
    if (!ata_write_sectors(dir_lba, boot_sector->sectors_per_cluster, dir_entries)) {
        kfree(dir_entries);
        return false;
    }
    
    kfree(dir_entries);
    return true;
}


bool fat32_get_current_directory(char* path, size_t max_len) {
    if (current_directory_cluster == root_cluster) {
        if (max_len > 1) {
            path[0] = '/';
            path[1] = '\0';
            return true;
        }
    } else {
        
        if (max_len > 1) {
            path[0] = '/';
            path[1] = '\0';
            return true;
        }
    }
    return false;
}

