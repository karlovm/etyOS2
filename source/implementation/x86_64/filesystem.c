// filesystem.c
#include "filesystem.h"
#include "print.h"
#include "port.h" // Assume this contains `outb` and `inb` functions
#define ATA_PRIMARY_IO_BASE  0x1F0
#define ATA_SECONDARY_IO_BASE 0x170
#define ATA_CMD_IDENTIFY 0xEC
#define SECTOR_SIZE 512
void read_string_from_identify(uint16_t *identify_buffer, int start, int end, char *output)
{
    int j = 0;
    for (int i = start; i <= end; ++i) {
        output[j++] = (identify_buffer[i] >> 8) & 0xFF;  // High byte
        output[j++] = identify_buffer[i] & 0xFF;         // Low byte
    }
    output[j] = '\0';  // Null-terminate the string
}

int is_blank_or_whitespace(const char *str)
{
    while (*str) {
        if (*str != ' ' && *str != '\t') return 0;
        str++;
    }
    return 1;
}


void display_available_disks()
{
    print_str("Model / Size / Type");
    print_newline();

    uint8_t status;
    uint16_t identify_buffer[256];
    int drive_count = 0;

    for (int controller = 0; controller < 2; ++controller)
    {
        uint16_t io_base = (controller == 0) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;

        for (int drive = 0; drive < 2; ++drive)
        {
            // Select drive (0xA0 = Master, 0xB0 = Slave)
            outb(io_base + 6, 0xA0 | (drive << 4));

            // Send IDENTIFY command
            outb(io_base + 7, ATA_CMD_IDENTIFY);

            // Check if the drive exists
            status = inb(io_base + 7);
            if (status == 0) continue;

            // Wait for the drive to be ready
            while ((inb(io_base + 7) & 0x80) != 0); // Wait until BSY is clear

            // Read IDENTIFY data
            for (int i = 0; i < 256; ++i)
            {
                identify_buffer[i] = inw(io_base);
            }

            // Retrieve model number
            char model[41];
            read_string_from_identify(identify_buffer, 27, 46, model);

            // Check if model is empty, if so set to "(Unknown)"
            if (model[0] == '\0') {
                print_str("(Unknown)");
            } else {
                print_str(model);
            }

            // Total sectors and size calculation
            uint32_t total_sectors = identify_buffer[60] | (identify_buffer[61] << 16);
            uint64_t total_size_mb = (total_sectors * SECTOR_SIZE) / (1024 * 1024);

            // Determine type abbreviation
            const char *type;
            if (controller == 0 && drive == 0) type = "PM";
            else if (controller == 0 && drive == 1) type = "PS";
            else if (controller == 1 && drive == 0) type = "SM";
            else if (controller == 1 && drive == 1) type = "SS";

            // Display drive information in requested format
            print_str("  ");
            print_int(total_size_mb);
            print_str(" MB ");
            print_str(type);
            print_newline();

            drive_count++;
        }
    }

    if (drive_count == 0)
    {
        print_str("No drives found.");
        print_newline();
    }
}

void init_filesystem()
{
    print_str("Initializing filesystem...");
    print_newline();
    // We could check if the drive is present or ready here.
}

int ata_read_sector(uint32_t lba, uint8_t *buffer)
{
    // Select drive and head
    outb(ATA_PRIMARY_IO_BASE + 6, 0xE0 | ((lba >> 24) & 0x0F)); // Master, LBA mode
    outb(ATA_PRIMARY_IO_BASE + 2, 1);                           // Sector count: 1
    outb(ATA_PRIMARY_IO_BASE + 3, (uint8_t)lba);                // LBA low byte
    outb(ATA_PRIMARY_IO_BASE + 4, (uint8_t)(lba >> 8));         // LBA mid byte
    outb(ATA_PRIMARY_IO_BASE + 5, (uint8_t)(lba >> 16));        // LBA high byte
    outb(ATA_PRIMARY_IO_BASE + 7, ATA_CMD_READ_SECTORS);        // Send read command

    // Wait for the drive to be ready
    while ((inb(ATA_PRIMARY_IO_BASE + 7) & 0x80) != 0)
        ; // Wait until BSY is clear

    // Check for errors
    if (inb(ATA_PRIMARY_IO_BASE + 7) & 0x01)
    { // ERR bit
        print_str("Error reading from disk.");
        return -1;
    }

    // Read sector data
    for (int i = 0; i < SECTOR_SIZE / 2; ++i)
    {
        uint16_t data = inw(ATA_PRIMARY_IO_BASE); // Read 16 bits at a time
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return 0;
}

// Reads the boot sector to check filesystem type
void read_boot_sector(uint8_t *buffer)
{
    // Attempt to read the first sector (usually the boot sector)
    if (ata_read_sector(0, buffer) != 0)
    {
        print_str("Error: Unable to read boot sector.");
    }
}

void check_filesystem()
{
    print_str("Checking filesystem...");
    print_newline();

    uint8_t buffer[SECTOR_SIZE];
    read_boot_sector(buffer);

    // Check for FAT32 (bytes 0x52-0x55 contain "FAT32")
    if (buffer[0x52] == 'F' && buffer[0x53] == 'A' && buffer[0x54] == 'T' && buffer[0x55] == '3' && buffer[0x56] == '2')
    {
        print_str("Filesystem detected: FAT32");
    }
    // Check for FAT16 (bytes 0x36-0x3A contain "FAT16")
    else if (buffer[0x36] == 'F' && buffer[0x37] == 'A' && buffer[0x38] == 'T' && buffer[0x39] == '1' && buffer[0x3A] == '6')
    {
        print_str("Filesystem detected: FAT16");
    }
    // Check for CD-ROM ISO9660 (bytes 0x8001-0x8005 contain "CD001")
    else if (buffer[0x8001] == 'C' && buffer[0x8002] == 'D' && buffer[0x8003] == '0' && buffer[0x8004] == '0' && buffer[0x8005] == '1')
    {
        print_str("Filesystem detected: ISO9660 (CD-ROM)");
    }
    else
    {
        print_str("Filesystem type: Unknown or unsupported.");
        print_str("Boot Sector Data: ");
        for (int i = 0; i < 64; ++i)
        { // Print first 64 bytes
            char hex[3];
            hex[0] = "0123456789ABCDEF"[buffer[i] >> 4];
            hex[1] = "0123456789ABCDEF"[buffer[i] & 0x0F];
            hex[2] = '\0';
            print_str(hex);
            print_str(" ");
        }
        print_newline();
    }

    print_newline();
}