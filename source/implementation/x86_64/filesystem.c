// filesystem.c
#include "filesystem.h"
#include "print.h"
#include "port.h" // Assume this contains `outb` and `inb` functions

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