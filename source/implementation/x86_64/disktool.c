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

// Initialize the disk tool and scan for available disks
void init_disktool() {
    num_disks = 0;
    print_str("Analyzing disks...");
    print_newline();
    uint16_t identify_buffer[256];

    for (int controller = 0; controller < 2; ++controller) {
        uint16_t io_base = (controller == 0) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;

        for (int drive = 0; drive < 2; ++drive) {
            // Select drive
            outb(io_base + 6, 0xA0 | (drive << 4));
            outb(io_base + 7, ATA_CMD_IDENTIFY);

            // Check if drive exists
            uint8_t status = inb(io_base + 7);
            if (status == 0) continue;

            // Wait for drive to be ready
            while ((inb(io_base + 7) & 0x80) != 0);

            // Read IDENTIFY data
            for (int i = 0; i < 256; ++i) {
                identify_buffer[i] = inw(io_base);
            }

            // Store disk information
            DiskInfo* disk = &available_disks[num_disks];
            disk->controller = controller;
            disk->drive = drive;

            // Get model name
            read_string_from_identify(identify_buffer, 27, 46, disk->model);

            // Calculate size
            uint32_t total_sectors = identify_buffer[60] | (identify_buffer[61] << 16);
            disk->size_mb = (total_sectors * SECTOR_SIZE) / (1024 * 1024);

            num_disks++;
        }
    }
    print_str("Found " );
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

// Create a new partition on the selected disk
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
    
    // Find the first available sector after existing partitions
    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector(0, mbr) != 0) {
        print_str("Error reading MBR");
        print_newline();
        return -1;
    }

    PartitionEntry* partition_table = (PartitionEntry*)(mbr + PARTITION_TABLE_OFFSET);
    uint32_t start_lba = 2048; // Start at sector 2048 for alignment

    // Find the first free space
    for (int i = 0; i < 4; i++) {
        if (partition_table[i].type != 0x00) {
            uint32_t end_sector = partition_table[i].lba_first + partition_table[i].sector_count;
            if (end_sector > start_lba) {
                start_lba = end_sector;
            }
        }
    }

    create_partition(start_lba, sector_count);
    return 0;
}

// Display filesystem type for each partition
void display_partition_info() {
    if (current_disk == -1) {
        print_str("No disk selected");
        print_newline();
        return;
    }

    uint8_t mbr[SECTOR_SIZE];
    if (ata_read_sector(0, mbr) != 0) {
        print_str("Error reading MBR");
        print_newline();
        return;
    }

    PartitionEntry* partition_table = (PartitionEntry*)(mbr + PARTITION_TABLE_OFFSET);
    
    for (int i = 0; i < 4; i++) {
        if (partition_table[i].type != 0x00) {
            print_str("Partition ");
            print_int(i);
            print_str(": ");

            // Read the first sector of the partition
            uint8_t boot_sector[SECTOR_SIZE];
            if (ata_read_sector(partition_table[i].lba_first, boot_sector) == 0) {
                // Check filesystem signatures
                if (boot_sector[0x52] == 'F' && boot_sector[0x53] == 'A' && 
                    boot_sector[0x54] == 'T' && boot_sector[0x55] == '3' && 
                    boot_sector[0x56] == '2') {
                    print_str("FAT32");
                }
                else if (boot_sector[0x36] == 'F' && boot_sector[0x37] == 'A' && 
                         boot_sector[0x38] == 'T' && boot_sector[0x39] == '1' && 
                         boot_sector[0x3A] == '6') {
                    print_str("FAT16");
                }
                else {
                    print_str("Unknown");
                }
            }

            print_str(", Size: ");
            print_int((partition_table[i].sector_count * SECTOR_SIZE) / (1024 * 1024));
            print_str(" MB");
            print_newline();
        }
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
    if (ata_read_sector(0, mbr) != 0) {
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

    // Clear the partition entry
    for (int i = 0; i < sizeof(PartitionEntry); i++) {
        ((uint8_t*)&partition_table[partition_index])[i] = 0;
    }

    // Write the updated MBR back to disk
    if (ata_write_sector(0, mbr) != 0) {
        print_str("Error writing MBR");
        print_newline();
        return -1;
    }

    print_str("Partition deleted successfully");
    print_newline();
    return 0;
}