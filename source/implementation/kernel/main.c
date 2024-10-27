#include "print.h"
#include "console.h"
#include "string.h"
#include "partition.h"
#include "filesystem.h"
#include "disktool.h"

void kernel_main()
{
    print_clear();
    // Fancy border
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_CYAN);
    print_str("================================================");
    print_newline();
    print_str("|                                              |");
    print_newline();

    // Centered welcome message
    print_str("|             Welcome to etyOS2                |");
    print_newline();
    print_str("|            64-bit Kernel Started             |");
    print_newline();

    print_str("|                                              |");
    print_newline();
    print_str("================================================");
    print_newline();

    // Default command message
    print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    print_str("Type 'help' for a list of commands.");
    print_newline();

    // Command processing loop
    char command[256];

    init_disktool();
    int color = PRINT_COLOR_BLACK;

    print_set_color(PRINT_COLOR_WHITE, color);
    while (1)
    {
        print_set_color(PRINT_COLOR_LIGHT_GRAY, color);
        print_str("etyOS Kernel > "); // Prompt
        print_set_color(PRINT_COLOR_WHITE, color);

        read_input(command, sizeof(command)); // Read user input
        print_newline();

        if (command[0] == '\0')
            continue;

        // Process command
        if (strcmp(command, "help") == 0)
        {
            show_help(color);
        }
        else if (strcmp(command, "disktool") == 0)
        {
            start_disktool(color);
        }
        else if (strcmp(command, "clear") == 0)
        {
            print_clear();
        }
        else if (strcmp(command, "dsks") == 0)
        {
            display_available_disks();
        }
        else if (strcmp(command, "about") == 0)
        {
            print_str("etyOS 2.0");
            print_newline();
            print_str("Version created: 27.10.2024");
            print_newline();
            print_str("Author: Mikhail Karlov");
            print_newline();
            print_str("Configuration: x86_64 multiboot2");
            print_newline();
        }
        else if (strcmp(command, "checkfs") == 0)
        {
            check_filesystem();
        }
        else if (strncmp(command, "echo ", 5) == 0)
        {
            print_str(command + 5); // Print the rest of the input after "echo "
            print_newline();
        }
        else if (strncmp(command, "create_partition ", 17) == 0)
        {
            // Parse parameters for create_partition
            uint32_t start_lba = strtoul(command + 17, NULL, 10);
            uint32_t sector_count = strtoul(strchr(command + 17, ' ') + 1, NULL, 10);

            create_partition(start_lba, sector_count);
        }
        else if (strcmp(command, "partitions") == 0)
        {
            display_partitions();
        }
        else if (strncmp(command, "format_partition ", 17) == 0)
        {
            // Parse parameter for format_partition
            int partition_number = strtoul(command + 17, NULL, 10);

            uint8_t mbr[MBR_SIZE];
            if (ata_read_sector(0, mbr) == 0)
            {
                PartitionEntry *partition_table = (PartitionEntry *)(mbr + PARTITION_TABLE_OFFSET);
                if (partition_number >= 0 && partition_number < 4 && partition_table[partition_number].type != 0x00)
                {
                    format_fat32(partition_table[partition_number].lba_first, partition_table[partition_number].sector_count);
                }
                else
                {
                    print_str("Invalid partition number.");
                }
            }
            else
            {
                print_str("Error: Unable to read MBR.");
            }
        }
        else if (strncmp(command, "setcolor ", 9) == 0)
        {
            // background foreground and background colors
            int background = strtoul(command + 9, NULL, 10);

            color = background;
            // Validate color inputs
            if (background >= 0 && background <= 14)
            {

                print_set_color(PRINT_COLOR_WHITE, color);
                print_clear();
                print_set_color(PRINT_COLOR_WHITE, color);
                print_clear();
                print_str("Colors updated.");
            }
            else
            {
                print_str("Invalid color value. Use numbers between 0 and 14.");
            }
            print_newline();
        }
        else
        {
            print_str("Unknown command: ");
            print_str(command);
            print_newline();
        }
    }
}
