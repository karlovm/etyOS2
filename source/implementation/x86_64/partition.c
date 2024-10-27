#include "partition.h"
#include "filesystem.h"
#include "print.h"
#include "port.h"


#define MBR_SIZE 512
#define PARTITION_TABLE_OFFSET 446
#define FAT32_PARTITION_TYPE 0x0C  // FAT32 with LBA support


#define SECTORS_PER_CLUSTER    8
#define RESERVED_SECTORS       32
#define NUM_FATS              2
#define ROOT_ENTRIES          0       // Not used in FAT32
#define SECTORS_PER_FAT(total_sectors) ((total_sectors - RESERVED_SECTORS) / (SECTORS_PER_CLUSTER * 128 + 2) * 2)

// Key EXT4 constants
#define EXT4_PARTITION_TYPE 0x83  // Linux native partition type
#define EXT4_SUPERBLOCK_OFFSET 1024
#define EXT4_MAGIC 0xEF53
#define EXT4_DEFAULT_BLOCK_SIZE 4096
#define EXT4_FIRST_INO 11
#define EXT4_FEATURE_COMPAT_DIR_INDEX 0x0020
#define EXT4_FEATURE_INCOMPAT_EXTENTS 0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT 0x0080
// Group descriptor structure
typedef struct {
    uint32_t bg_block_bitmap_lo;
    uint32_t bg_inode_bitmap_lo;
    uint32_t bg_inode_table_lo;
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lo;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;
    uint32_t bg_block_bitmap_hi;
    uint32_t bg_inode_bitmap_hi;
    uint32_t bg_inode_table_hi;
    uint16_t bg_free_blocks_count_hi;
    uint16_t bg_free_inodes_count_hi;
    uint16_t bg_used_dirs_count_hi;
    uint16_t bg_itable_unused_hi;
    uint32_t bg_exclude_bitmap_hi;
    uint16_t bg_block_bitmap_csum_hi;
    uint16_t bg_inode_bitmap_csum_hi;
    uint32_t bg_reserved;
} __attribute__((packed)) Ext4GroupDesc;


static void* memset2(void* dest, int val, size_t len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}
// Buffer for disk operations - declare as static to avoid stack overflow
static uint8_t disk_buffer[SECTOR_SIZE];

static void init_superblock(Ext4Superblock* sb, uint32_t total_sectors) {
    // Clear the superblock first
    memset2(sb, 0, sizeof(Ext4Superblock));
    
    // Calculate fundamental filesystem parameters
    uint64_t total_size = (uint64_t)total_sectors * SECTOR_SIZE;
    uint32_t block_size = EXT4_DEFAULT_BLOCK_SIZE;
    uint64_t blocks_count = total_size / block_size;
    
    // Ensure we don't exceed 32-bit block count if not using 64-bit feature
    if (blocks_count > 0xFFFFFFFF) {
        blocks_count = 0xFFFFFFFF;
    }
    
    // Calculate group descriptor parameters
    uint32_t inodes_per_group = 8192;
    uint32_t blocks_per_group = 32768;
    uint32_t num_block_groups = (blocks_count + blocks_per_group - 1) / blocks_per_group;
    
    // Initialize superblock fields
    sb->s_inodes_count = inodes_per_group * num_block_groups;
    sb->s_blocks_count_lo = blocks_count & 0xFFFFFFFF;
    sb->s_blocks_count_hi = (blocks_count >> 32) & 0xFFFFFFFF;
    sb->s_r_blocks_count_lo = (blocks_count / 20) & 0xFFFFFFFF; // 5% reserved
    sb->s_free_blocks_count_lo = blocks_count - 1; // Subtract superblock
    sb->s_free_inodes_count = sb->s_inodes_count - EXT4_FIRST_INO + 1;
    sb->s_first_data_block = (block_size == 1024) ? 1 : 0;
    sb->s_log_block_size = (uint32_t)(__builtin_ctz(block_size) - 10);
    sb->s_blocks_per_group = blocks_per_group;
    sb->s_inodes_per_group = inodes_per_group;
    sb->s_magic = EXT4_MAGIC;
    sb->s_state = 1; // Cleanly unmounted
    sb->s_errors = 1; // Continue on errors
    sb->s_rev_level = 1; // Dynamic inode sizes
    sb->s_first_ino = EXT4_FIRST_INO;
    sb->s_inode_size = 256;
    sb->s_feature_compat = EXT4_FEATURE_COMPAT_DIR_INDEX;
    sb->s_feature_incompat = EXT4_FEATURE_INCOMPAT_EXTENTS | EXT4_FEATURE_INCOMPAT_64BIT;
    
    // Set filesystem name
    const char* volume_name = "etyOS";
    memset2(sb->s_volume_name, 0, sizeof(sb->s_volume_name));
    copy_bytes(sb->s_volume_name, (const uint8_t*)volume_name, strlen(volume_name));
}

int format_ext4(int controller, int drive, uint32_t start_lba, uint32_t total_sectors) {
    if (total_sectors < 8192) { // Minimum size check (32MB)
        return -1;
    }
    
    // Clear our static buffer
    memset2(disk_buffer, 0, SECTOR_SIZE);
    
    // Calculate sector containing superblock
    uint32_t sb_sector = start_lba + (EXT4_SUPERBLOCK_OFFSET / SECTOR_SIZE);
    
    // Read the sector that will contain the superblock
    if (ata_read_sector_disk(controller, drive, sb_sector, disk_buffer) != 0) {
        return -1;
    }
    
    // Initialize superblock in our buffer
    Ext4Superblock* sb = (Ext4Superblock*)(disk_buffer + (EXT4_SUPERBLOCK_OFFSET % SECTOR_SIZE));
    init_superblock(sb, total_sectors);
    
    // Calculate number of block groups
    uint32_t num_block_groups = (sb->s_blocks_count_lo + sb->s_blocks_per_group - 1) / 
                               sb->s_blocks_per_group;
    
    // Write the superblock sector back
    if (ata_write_sector_disk(controller, drive, sb_sector, disk_buffer) != 0) {
        return -1;
    }
    
    // Process each block group
    uint32_t first_block = sb->s_first_data_block;
    for (uint32_t group = 0; group < num_block_groups; group++) {
        // Clear buffer for block group descriptor
        memset2(disk_buffer, 0, SECTOR_SIZE);
        
        // Initialize block group descriptor
        Ext4GroupDesc* gd = (Ext4GroupDesc*)disk_buffer;
        
        // Calculate blocks for this group
        uint32_t group_first_block = first_block + (group * sb->s_blocks_per_group);
        uint32_t blocks_in_group = sb->s_blocks_per_group;
        if (group == num_block_groups - 1) {
            // Handle last group which might be partial
            blocks_in_group = sb->s_blocks_count_lo - 
                            (group * sb->s_blocks_per_group);
        }
        
        // Set up group descriptor
        gd->bg_block_bitmap_lo = group_first_block;
        gd->bg_inode_bitmap_lo = group_first_block + 1;
        gd->bg_inode_table_lo = group_first_block + 2;
        gd->bg_free_blocks_count_lo = blocks_in_group - 4; // Subtract metadata blocks
        gd->bg_free_inodes_count_lo = sb->s_inodes_per_group;
        gd->bg_used_dirs_count_lo = 0;
        gd->bg_flags = 0;
        
        // Write block group descriptor
        uint32_t gdt_sector = sb_sector + 1 + group;
        if (ata_write_sector_disk(controller, drive, gdt_sector, disk_buffer) != 0) {
            return -1;
        }
        
        // Initialize and write block bitmap
        memset2(disk_buffer, 0, SECTOR_SIZE);
        disk_buffer[0] = 0x0F; // First 4 blocks used
        uint32_t bitmap_sector = start_lba + 
                               ((group_first_block * EXT4_DEFAULT_BLOCK_SIZE) / SECTOR_SIZE);
        if (ata_write_sector_disk(controller, drive, bitmap_sector, disk_buffer) != 0) {
            return -1;
        }
        
        // Write empty inode bitmap
        memset2(disk_buffer, 0, SECTOR_SIZE);
        if (ata_write_sector_disk(controller, drive, bitmap_sector + 1, disk_buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

void create_ext4_partition(int controller, int drive, uint32_t start_lba, uint32_t sector_count) {
    uint8_t mbr[MBR_SIZE];

    // Read the MBR
    if (ata_read_sector_disk(controller, drive, 0, mbr) != 0) {
        print_str("Error: Unable to read MBR.");
        print_newline();
        return;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    // Find first empty partition slot
    for (int i = 0; i < 4; ++i) {
        if (partition_table[i].type == 0x00) {
            partition_table[i].status = 0x00;
            partition_table[i].type = EXT4_PARTITION_TYPE;
            partition_table[i].lba_first = start_lba;
            partition_table[i].sector_count = sector_count;

            // Write updated MBR
            if (ata_write_sector_disk(controller, drive, 0, mbr) != 0) {
                print_str("Error: Unable to write MBR.");
                print_newline();
                return;
            }

            // Format the partition with ext4
            if (format_ext4(controller, drive, start_lba, sector_count) != 0) {
                print_str("Error: Unable to format ext4 partition.");
                print_newline();
                return;
            }

            print_str("EXT4 Partition created and formatted successfully.");
            print_newline();
            return;
        }
    }

    print_str("Error: No empty partition slots available.");
    print_newline();
}
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

 void copy_bytes(uint8_t* dest, const uint8_t* src, int len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}

// Initialize a FAT32 filesystem on the partition
 int format_fat32(int controller, int drive, uint32_t start_lba, uint32_t total_sectors) {
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
void create_fat32_partition(int controller, int drive, uint32_t start_lba, uint32_t sector_count) {
    uint8_t mbr[MBR_SIZE];

    // Read the MBR
    if (ata_read_sector_disk(controller, drive, 0, mbr) != 0) {
        print_str("Error: Unable to read MBR.");
        print_newline();
        return;
    }

    // Locate the first available slot in the MBR partition table
    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    // Find first empty partition slot
    for (int i = 0; i < 4; ++i) {
        if (partition_table[i].type == 0x00) {
            // Set up the new FAT32 partition
            partition_table[i].status = 0x00;               // Mark as inactive; can be set to bootable later
            partition_table[i].type = FAT32_PARTITION_TYPE; // FAT32 type
            partition_table[i].lba_first = start_lba;
            partition_table[i].sector_count = sector_count;

            // Write updated MBR
            if (ata_write_sector_disk(controller, drive, 0, mbr) != 0) {
                print_str("Error: Unable to write MBR.");
                print_newline();
                return;
            }

            // Format the partition with FAT32
            if (format_fat32(controller, drive, start_lba, sector_count) != 0) {
                print_str("Error: Unable to format FAT32 partition.");
                print_newline();
                return;
            }

            print_str("FAT32 Partition created and formatted successfully.");
            print_newline();
            return;
        }
    }

    print_str("Error: No empty partition slots available.");
    print_newline();
}

#define EXT4_SUPERBLOCK_OFFSET 1024
#define EXT4_MAGIC 0xEF53
#define EXT4_VOLUME_NAME_LEN 16

void display_partitions()
{
    uint8_t mbr[MBR_SIZE];
    uint8_t superblock_buffer[sizeof(Ext4Superblock)];

    // Read the MBR (sector 0)
    if (ata_read_sector(0, mbr) != 0) {
        print_str("Error: Unable to read MBR.");
        return;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    print_str("Partition Table:");
    print_newline();

    for (int i = 0; i < 4; ++i) {
        if (partition_table[i].type != 0x00) { // Check if partition entry is valid
            print_str("Partition ");
            print_int(i);
            print_str(":");

            // Calculate partition size in MB
            uint32_t size_in_mb = (partition_table[i].sector_count * SECTOR_SIZE) / (1024 * 1024);

            print_str(" Start LBA: ");
            print_int(partition_table[i].lba_first);
            print_str(", Size: ");
            print_int(size_in_mb);
            print_str(" MB");
            
            // Display filesystem type based on partition type code
            print_str(", Type: ");
            switch(partition_table[i].type) {
                case 0x83: {
                    print_str("EXT4");
                    
                    // For EXT4 partitions, try to read the superblock
                    uint32_t superblock_lba = partition_table[i].lba_first + (EXT4_SUPERBLOCK_OFFSET / SECTOR_SIZE);
                    if (ata_read_sector(superblock_lba, superblock_buffer) == 0) {
                        Ext4Superblock* sb = (Ext4Superblock*)superblock_buffer;
                        
                        // Verify EXT4 magic number
                        if (sb->s_magic == EXT4_MAGIC) {
                            // Check if volume name is not empty
                            int has_name = 0;
                            for (int j = 0; j < EXT4_VOLUME_NAME_LEN; j++) {
                                if (sb->s_volume_name[j] != 0) {
                                    has_name = 1;
                                    break;
                                }
                            }
                            
                            if (has_name == 1) {
                                print_str(" [Volume: ");
                                // Print only up to the first null character or max length
                                for (int j = 0; j < EXT4_VOLUME_NAME_LEN && sb->s_volume_name[j]; j++) {
                                    char c = sb->s_volume_name[j];
                                    // Only print printable ASCII characters
                                    if (c >= 32 && c <= 126) {
                                        print_char(c);
                                    }
                                }
                                print_str("]");
                            }
                        }
                    }
                    break;
                }
                case 0x82:
                    print_str("Linux swap");
                    break;
                case 0x07:
                    print_str("NTFS");
                    break;
                case 0x0C:
                    print_str("FAT32 LBA");
                    break;
                case 0x0B:
                    print_str("FAT32");
                    break;
                case 0x0F:
                    print_str("Extended");
                    break;
                case 0x05:
                    print_str("Extended");
                    break;
                case 0x04:
                    print_str("FAT16");
                    break;
                case 0x06:
                    print_str("FAT16B");
                    break;
                case 0x0E:
                    print_str("FAT16B LBA");
                    break;
                default:
                    print_str("Unknown (0x");
                    uint8_t high_nibble = (partition_table[i].type >> 4) & 0x0F;
                    uint8_t low_nibble = partition_table[i].type & 0x0F;
                    
                    char hex_chars[] = "0123456789ABCDEF";
                    char hex_str[3] = {
                        hex_chars[high_nibble],
                        hex_chars[low_nibble],
                        ')'
                    };
                    print_str(hex_str);
                    break;
            }

            // Display bootable flag if set
            if (partition_table[i].status & 0x80) {
                print_str(" [Bootable]");
            }
            
            print_newline();
        }
    }
}

void select_partition(int partition_number)
{
    uint8_t mbr[MBR_SIZE];

    // Read the MBR (sector 0)
    if (ata_read_sector(0, mbr) != 0)
    {
        print_str("Error: Unable to read MBR.");
        return;
    }

    PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);

    if (partition_number < 0 || partition_number >= 4 || partition_table[partition_number].type == 0x00)
    {
        print_str("Invalid or empty partition.");
        return;
    }

    print_str("Selected partition: ");
    print_int(partition_number);
    print_newline();
}

