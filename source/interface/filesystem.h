// filesystem.h
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

#define ATA_PRIMARY_IO_BASE    0x1F0
#define ATA_PRIMARY_CONTROL    0x3F6
#define ATA_CMD_READ_SECTORS   0x20
#define SECTOR_SIZE            512

// Function declarations
void init_filesystem();
void check_filesystem();
int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, const uint8_t *buffer);

int ata_read_sector_disk(int controller, int drive, uint32_t lba, uint8_t *buffer) ;
int ata_write_sector_disk(int controller, int drive, uint32_t lba, const uint8_t *buffer);

#endif // FILESYSTEM_H
