#include "filesystem.h"
#include "partition.h"
#include "print.h"
#include "port.h"

#define MAX_DISKS 4  // Maximum number of disks (2 controllers × 2 drives)
#define MB_TO_SECTORS(mb) ((mb * 1024 * 1024) / SECTOR_SIZE)
// ATA I/O Port Base Addresses
#define ATA_PRIMARY_IO_BASE    0x1F0
#define ATA_SECONDARY_IO_BASE  0x170
// ATA Commands
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30

// Structure to keep track of available disks
typedef struct {
    int controller;
    int drive;
    uint64_t size_mb;
    char model[41];
} DiskInfo;

static DiskInfo available_disks[MAX_DISKS];
static int current_disk = -1;
static int num_disks = 0;

#define SECTORS_PER_CLUSTER    8
#define RESERVED_SECTORS       32
#define NUM_FATS              2
#define ROOT_ENTRIES          0       // Not used in FAT32
#define SECTORS_PER_FAT(total_sectors) ((total_sectors - RESERVED_SECTORS) / (SECTORS_PER_CLUSTER * 128 + 2) * 2)


// FAT32 Boot Sector structure
typedef struct {
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t extended_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
    uint8_t  boot_code[420];
    uint16_t boot_signature_2;
} __attribute__((packed)) FAT32BootSector;

static void copy_bytes(uint8_t* dest, const uint8_t* src, int len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}

// Initialize a FAT32 filesystem on the partition
static int dt_format_fat32(int controller, int drive, uint32_t start_lba, uint32_t total_sectors) {
    FAT32BootSector boot_sector = {0};
    
    // Basic boot sector fields
    boot_sector.jump_boot[0] = 0xEB;
    boot_sector.jump_boot[1] = 0x58;
    boot_sector.jump_boot[2] = 0x90;
    copy_bytes(boot_sector.oem_name, "MSWIN4.1", 8);
    boot_sector.bytes_per_sector = SECTOR_SIZE;
    boot_sector.sectors_per_cluster = SECTORS_PER_CLUSTER;
    boot_sector.reserved_sectors = RESERVED_SECTORS;
    boot_sector.num_fats = NUM_FATS;
    boot_sector.root_entries = ROOT_ENTRIES;
    boot_sector.total_sectors_16 = 0;
    boot_sector.media_type = 0xF8;
    boot_sector.sectors_per_fat_16 = 0;
    boot_sector.sectors_per_track = 63;
    boot_sector.num_heads = 255;
    boot_sector.hidden_sectors = start_lba;
    boot_sector.total_sectors_32 = total_sectors;
    
    // FAT32 specific fields
    boot_sector.sectors_per_fat_32 = SECTORS_PER_FAT(total_sectors);
    boot_sector.extended_flags = 0;
    boot_sector.fs_version = 0;
    boot_sector.root_cluster = 2;
    boot_sector.fs_info = 1;
    boot_sector.backup_boot = 6;
    boot_sector.drive_number = 0x80;
    boot_sector.boot_signature = 0x29;
    boot_sector.volume_id = 0x12345678;
    copy_bytes(boot_sector.volume_label, "NO NAME    ", 11);
    copy_bytes(boot_sector.fs_type, "FAT32   ", 8);
    boot_sector.boot_signature_2 = 0xAA55;

    // Write boot sector
    if (ata_write_sector_disk(controller, drive, start_lba, (uint8_t*)&boot_sector) != 0) {
        return -1;
    }

    // Initialize FAT tables
    uint8_t fat_sector[SECTOR_SIZE] = {0};
    // First FAT entries
    uint32_t* fat = (uint32_t*)fat_sector;
    fat[0] = 0x0FFFFFF8;  // Media type
    fat[1] = 0x0FFFFFFF;  // End of cluster chain
    fat[2] = 0x0FFFFFFF;  // End of root directory

    // Write first sector of each FAT
    uint32_t fat1_start = start_lba + RESERVED_SECTORS;
    uint32_t fat2_start = fat1_start + boot_sector.sectors_per_fat_32;
    
    if (ata_write_sector_disk(controller, drive, fat1_start, fat_sector) != 0 ||
        ata_write_sector_disk(controller, drive, fat2_start, fat_sector) != 0) {
        return -1;
    }

    return 0;
}
// Initialize the disk tool and scan for available disks
void init_disktool() {
    num_disks = 0;
    print_str("Analyzing disks...");
    print_newline();
    uint16_t identify_buffer[256];

    for (int controller = 0; controller < 2; ++controller) {
        uint16_t io_base = (controller == 0) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;

        for (int drive = 0; drive < 2; ++drive) {
            outb(io_base + 6, 0xA0 | (drive << 4));
            outb(io_base + 7, ATA_CMD_IDENTIFY);

            uint8_t status = inb(io_base + 7);
            if (status == 0) continue;

            while ((inb(io_base + 7) & 0x80) != 0);

            for (int i = 0; i < 256; ++i) {
                identify_buffer[i] = inw(io_base);
            }

            DiskInfo* disk = &available_disks[num_disks];
            disk->controller = controller;
            disk->drive = drive;

            read_string_from_identify(identify_buffer, 27, 46, disk->model);

            uint32_t total_sectors = identify_buffer[60] | (identify_buffer[61] << 16);
            disk->size_mb = (total_sectors * SECTOR_SIZE) / (1024 * 1024);

            num_disks++;
        }
    }
    print_str("Found ");
    print_dec(num_disks);
    print_str(" disks");
    print_newline();
    print_newline();
}

// List all available disks
void list_disks() {
    print_str("Available Disks:\n");
    print_newline();

    for (int i = 0; i < num_disks; i++) {
        print_str("[");
        print_int(i);
        print_str("] ");
        print_str(available_disks[i].model);
        print_str(" (");
        print_int(available_disks[i].size_mb);
        print_str(" MB)");
        print_newline();
    }
}

// Select a disk to work with
int select_disk(int disk_index) {
    if (disk_index < 0 || disk_index >= num_disks) {
        print_str("Invalid disk index");
        print_newline();
        return -1;
    }

    current_disk = disk_index;
    print_str("Selected disk: ");
    print_str(available_disks[disk_index].model);
    print_newline();
    return 0;
}

// Create a new FAT32 partition on the selected disk
int create_partition_mb(uint32_t size_mb) {
    if (current_disk == -1) {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    if (size_mb > available_disks[current_disk].size_mb) {
        print_str("Requested size exceeds disk capacity");
        print_newline();
        return -1;
    }

    uint32_t sector_count = MB_TO_SECTORS(size_mb);

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, 0, mbr) != 0) {
        print_str("Error reading MBR");
        print_newline();
        return -1;
    }

    PartitionEntry* partition_table = (PartitionEntry*)(mbr + PARTITION_TABLE_OFFSET);
    uint32_t start_lba = 2048;

    // Find first available partition slot and suitable start sector
    int partition_slot = -1;
    for (int i = 0; i < 4; i++) {
        if (partition_table[i].type == 0x00) {
            if (partition_slot == -1) partition_slot = i;
        } else {
            uint32_t end_sector = partition_table[i].lba_first + partition_table[i].sector_count;
            if (end_sector > start_lba) {
                start_lba = end_sector;
                // Align to 2048 sectors
                start_lba = (start_lba + 2047) & ~2047;
            }
        }
    }

    if (partition_slot == -1) {
        print_str("No free partition slots");
        print_newline();
        return -1;
    }

    // Create partition entry using your PartitionEntry structure
    partition_table[partition_slot].status = 0x00;        // Not bootable
    partition_table[partition_slot].start_head = 0xFE;    // Using LBA, these values are dummy
    partition_table[partition_slot].start_sector = 0xFFFF; // Maximum value
    partition_table[partition_slot].type = 0x0C;          // FAT32 LBA
    partition_table[partition_slot].end_head = 0xFE;      // Using LBA, these values are dummy
    partition_table[partition_slot].end_sector = 0xFFFF;  // Maximum value
    partition_table[partition_slot].lba_first = start_lba;
    partition_table[partition_slot].sector_count = sector_count;

    // Write updated MBR
    if (ata_write_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, 0, mbr) != 0) {
        print_str("Error writing MBR");
        print_newline();
        return -1;
    }

    // Format the partition as FAT32
    if (dt_format_fat32(available_disks[current_disk].controller, 
                     available_disks[current_disk].drive,
                     start_lba, 
                     sector_count) != 0) {
        print_str("Error formatting partition");
        print_newline();
        return -1;
    }

    print_str("Partition created and formatted successfully");
    print_newline();
    return 0;
}

// Display filesystem type for each partition on the selected disk
void display_partition_info() {
    if (current_disk == -1) {
        print_str("No disk selected");
        print_newline();
        return;
    }

    // Print selected disk's name
    print_str("Disk: ");
    print_str(available_disks[current_disk].model);
    print_str(" (");
    print_int(available_disks[current_disk].size_mb);
    print_str(" MB)");
    print_newline();

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, 0, mbr) != 0) {
        print_str("Error reading MBR");
        print_newline();
        return;
    }

    PartitionEntry* partition_table = (PartitionEntry*)(mbr + PARTITION_TABLE_OFFSET);
    int partition_found = 0;  // Flag to check if any partition is present

    for (int i = 0; i < 4; i++) {
        if (partition_table[i].type != 0x00) {
            partition_found = 1;  // At least one partition exists

            print_str("Partition ");
            print_int(i);
            print_str(": ");

            uint8_t boot_sector[SECTOR_SIZE];
            if (ata_read_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, partition_table[i].lba_first, boot_sector) == 0) {
                if (boot_sector[0x52] == 'F' && boot_sector[0x53] == 'A' && 
                    boot_sector[0x54] == 'T' && boot_sector[0x55] == '3' && 
                    boot_sector[0x56] == '2') {
                    print_str("FAT32");
                } else if (boot_sector[0x36] == 'F' && boot_sector[0x37] == 'A' && 
                           boot_sector[0x38] == 'T' && boot_sector[0x39] == '1' && 
                           boot_sector[0x3A] == '6') {
                    print_str("FAT16");
                } else {
                    print_str("Unknown FS");
                }
            }

            print_str(", Size: ");
            print_int((partition_table[i].sector_count * SECTOR_SIZE) / (1024 * 1024));
            print_str(" MB");
            print_newline();
        }
    }

    // If no partitions were found, display a message
    if (!partition_found) {
        print_str("No partitions found on this disk.");
        print_newline();
    }
}

// Delete a partition
int delete_partition(int partition_index) {
    if (current_disk == -1) {
        print_str("No disk selected");
        print_newline();
        return -1;
    }

    if (partition_index < 0 || partition_index >= 4) {
        print_str("Invalid partition index");
        print_newline();
        return -1;
    }

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, 0, mbr) != 0) {
        print_str("Error reading MBR");
        print_newline();
        return -1;
    }

    PartitionEntry* partition_table = (PartitionEntry*)(mbr + PARTITION_TABLE_OFFSET);
    
    if (partition_table[partition_index].type == 0x00) {
        print_str("Partition does not exist");
        print_newline();
        return -1;
    }

    for (int i = 0; i < sizeof(PartitionEntry); i++) {
        ((uint8_t*)&partition_table[partition_index])[i] = 0;
    }

    if (ata_write_sector_disk(available_disks[current_disk].controller, available_disks[current_disk].drive, 0, mbr) != 0) {
        print_str("Error writing MBR");
        print_newline();
        return -1;
    }

    print_str("Partition deleted successfully");
    print_newline();
    return 0;
}
