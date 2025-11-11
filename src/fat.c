#include "fat.h"
#include "ide.h"
#include "rprintf.h"
#include "terminal.h"
#include <stddef.h>
#include "malloc.h"

#define FAT_BOOT_SECTOR 0

static struct boot_sector boot_sec;
static struct file *open_files = NULL;

/*
void free(void* ptr) {
    // Simple implementation - no actual freeing
    // In a real implementation, you'd track allocated blocks
    (void)ptr;
}
*/

// Helper function to calculate cluster to LBA conversion
static uint32_t cluster_to_lba(uint32_t cluster) {
    return boot_sec.num_reserved_sectors + 
           (boot_sec.num_fat_tables * boot_sec.num_sectors_per_fat) +
           ((boot_sec.num_root_dir_entries * 32) / SECTOR_SIZE) +
           ((cluster - 2) * boot_sec.num_sectors_per_cluster);
}

// Initialize FAT filesystem
int fat_init(void) {
    // Read boot sector
    if (ata_lba_read(FAT_BOOT_SECTOR, (unsigned char*)&boot_sec, 1) != 0) {
        return -1;
    }
    
    // Verify this is a valid FAT16 filesystem
    if (boot_sec.boot_signature != 0xAA55) {
        return -1;
    }
    
    return 0;
}

// Read FAT table entry
static uint16_t read_fat_entry(uint16_t cluster) {
    uint32_t fat_offset = cluster * 2; // FAT16 uses 2 bytes per entry
    uint32_t fat_sector = boot_sec.num_reserved_sectors + (fat_offset / SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;
    
    unsigned char sector_buffer[SECTOR_SIZE];
    if (ata_lba_read(fat_sector, sector_buffer, 1) != 0) {
        return 0xFFFF; // Error reading
    }
    
    return *(uint16_t*)&sector_buffer[entry_offset];
}

// Find file in root directory
struct root_directory_entry* find_file(const char* filename) {
    static struct root_directory_entry entry;
    unsigned char sector_buffer[SECTOR_SIZE];
    
    uint32_t root_dir_sectors = (boot_sec.num_root_dir_entries * 32) / SECTOR_SIZE;
    uint32_t root_dir_start = boot_sec.num_reserved_sectors + 
                              (boot_sec.num_fat_tables * boot_sec.num_sectors_per_fat);
    
    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (ata_lba_read(root_dir_start + sector, sector_buffer, 1) != 0) {
            return NULL;
        }
        
        for (int i = 0; i < SECTOR_SIZE; i += 32) {
            struct root_directory_entry* dir_entry = 
                (struct root_directory_entry*)&sector_buffer[i];
            
            // Check if entry is empty
            if (dir_entry->file_name[0] == 0) {
                return NULL; // End of directory
            }
            
            // Skip deleted files
            if ((unsigned char)dir_entry->file_name[0] == 0xE5) {
                continue;
            }
            
            // Compare filename (8.3 format)
            int match = 1;
            for (int j = 0; j < 8 && filename[j] != '.' && filename[j] != '\0'; j++) {
                if (dir_entry->file_name[j] != filename[j]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) {
                entry = *dir_entry;
                return &entry;
            }
        }
    }
    
    return NULL;
}

// Open a file
struct file* fat_open(const char* filename) {
    struct root_directory_entry* rde = find_file(filename);
    if (!rde) {
        return NULL;
    }
    
    struct file* file = (struct file*)malloc(sizeof(struct file));
    if (!file) {
        return NULL;
    }
    
    file->rde = *rde;
    file->start_cluster = rde->cluster;
    file->next = open_files;
    file->prev = NULL;
    
    if (open_files) {
        open_files->prev = file;
    }
    open_files = file;
    
    return file;
}

// Read data from file
int fat_read(struct file* file, void* buffer, uint32_t bytes_to_read, uint32_t offset) {
    if (!file || offset >= file->rde.file_size) {
        return 0;
    }
    
    // Adjust size if reading beyond end of file
    if (offset + bytes_to_read > file->rde.file_size) {
        bytes_to_read = file->rde.file_size - offset;
    }
    
    uint32_t cluster = file->start_cluster;
    uint32_t cluster_size = boot_sec.num_sectors_per_cluster * SECTOR_SIZE;
    uint32_t bytes_read = 0;
    
    // Skip to the cluster containing our offset
    while (offset >= cluster_size) {
        cluster = read_fat_entry(cluster);
        if (cluster >= 0xFFF8) {
            return bytes_read; // End of chain
        }
        offset -= cluster_size;
    }
    
    unsigned char cluster_buffer[CLUSTER_SIZE];
    
    while (bytes_to_read > 0 && cluster < 0xFFF8) {
        // Read entire cluster
        uint32_t cluster_lba = cluster_to_lba(cluster);
        if (ata_lba_read(cluster_lba, cluster_buffer, boot_sec.num_sectors_per_cluster) != 0) {
            break;
        }
        
        // Copy data from cluster to buffer
        uint32_t bytes_to_copy = cluster_size - offset;
        if (bytes_to_copy > bytes_to_read) {
            bytes_to_copy = bytes_to_read;
        }
        
        for (uint32_t i = 0; i < bytes_to_copy; i++) {
            ((unsigned char*)buffer)[bytes_read + i] = cluster_buffer[offset + i];
        }
        
        bytes_read += bytes_to_copy;
        bytes_to_read -= bytes_to_copy;
        offset = 0; // After first cluster, offset is always 0
        
        // Move to next cluster
        cluster = read_fat_entry(cluster);
    }
    
    return bytes_read;
}

// Close file
void fat_close(struct file* file) {
    if (!file) return;
    
    // Remove from linked list
    if (file->prev) {
        file->prev->next = file->next;
    } else {
        open_files = file->next;
    }

}

// List files in root directory
void fat_list_files(void) {
    unsigned char sector_buffer[SECTOR_SIZE];
    int file_count = 0;
    
    uint32_t root_dir_sectors = (boot_sec.num_root_dir_entries * 32) / SECTOR_SIZE;
    uint32_t root_dir_start = boot_sec.num_reserved_sectors + 
                              (boot_sec.num_fat_tables * boot_sec.num_sectors_per_fat);
    
    esp_printf((func_ptr)putc, "DEBUG: Root dir starts at LBA %d, reading %d sectors\n", 
               root_dir_start, root_dir_sectors);
    
    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (ata_lba_read(root_dir_start + sector, sector_buffer, 1) != 0) {
            esp_printf((func_ptr)putc, "ERROR: Failed to read sector %d\n", sector);
            continue;
        }
        
        for (int i = 0; i < SECTOR_SIZE; i += 32) {
            struct root_directory_entry* dir_entry = 
                (struct root_directory_entry*)&sector_buffer[i];
            
            // Check if entry is empty (end of directory)
            if (dir_entry->file_name[0] == 0) {
                if (file_count == 0) {
                    esp_printf((func_ptr)putc, "No files found - directory is empty\n");
                }
                return;
            }
            
            // Skip deleted files
            if ((unsigned char)dir_entry->file_name[0] == 0xE5) {
                continue;
            }
            
            // Debug output for ALL entries
            esp_printf((func_ptr)putc, "Entry: [");
            for (int j = 0; j < 8; j++) {
                esp_printf((func_ptr)putc, "%c", dir_entry->file_name[j]);
            }
            esp_printf((func_ptr)putc, "] attr=0x%02x cluster=%d size=%d\n", 
                       dir_entry->attribute, dir_entry->cluster, dir_entry->file_size);
            
            // Skip volume label and subdirectories
            if (dir_entry->attribute & 0x08 || dir_entry->attribute & 0x10) {
                continue;
            }
            
            file_count++;
            
            // Print filename
            for (int j = 0; j < 8 && dir_entry->file_name[j] != ' '; j++) {
                putc(dir_entry->file_name[j]);
            }
            
            if (dir_entry->file_extension[0] != ' ') {
                putc('.');
                for (int j = 0; j < 3 && dir_entry->file_extension[j] != ' '; j++) {
                    putc(dir_entry->file_extension[j]);
                }
            }
            
            // Print file size and newline
            esp_printf((func_ptr)putc, " %d bytes\n", dir_entry->file_size);
        }
    }
}
