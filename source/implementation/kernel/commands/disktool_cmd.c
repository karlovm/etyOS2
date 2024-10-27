#include "print.h"
#include "console.h"
#include "string.h"
#include "disktool.h"

// Helper function to parse filesystem type from command
FileSystemType parse_fs_type(const char *fs_str)
{
    if (strcmp(fs_str, "ext4") == 0)
    {
        return FS_EXT4;
    }
    return FS_FAT32; // Default to FAT32
}

void start_disktool(int color)
{
    print_str("Welcome to disktool. ");
    print_newline();
    print_str("Type help to get list of all commands");
    print_newline();
    char command[256];
    while (1)
    {
        print_set_color(PRINT_COLOR_YELLOW, color);
        print_str("disktool > "); // Prompt
        print_set_color(PRINT_COLOR_WHITE, color);

        read_input(command, sizeof(command)); // Read user input
        print_newline();

        if (command[0] == '\0')
            continue;

        if (strcmp(command, "help") == 0)
        {
            print_str("etyOS Kernel Disk tool commands:");
            print_newline();
            print_str(" - listdisks: Show all available disks");
            print_newline();
            print_str(" - selectdisk <number>: Select a disk to work with");
            print_newline();
            print_str(" - newpart <size_mb> [fat32|ext4]: Create a new partition with size in MB");
            print_newline();
            print_str(" - delpart <number>: Delete a partition");
            print_newline();
            print_str(" - partinfo: Show partition information and filesystem types");
            print_newline();
            print_str(" - formatpart <number> [fat32|ext4]: Format partition with specified filesystem");
            print_newline();
            print_str(" - format [fat32|ext4]: Format selected disk (defaults to ext4)");
            print_newline();
            print_str(" - exit: Exit from disktool");
            print_newline();
        }
        else if (strcmp(command, "listdisks") == 0)
        {
            list_disks();
        }
        else if (strcmp(command, "format") == 0)
        {
            format_disk(FS_EXT4); // Maintain backward compatibility by defaulting to FAT32
        }
        else if (strncmp(command, "format ", 7) == 0)
        {
            char *fs_str = command + 7;
            FileSystemType fs_type = parse_fs_type(fs_str);
            format_disk(fs_type);
        }
        else if (strncmp(command, "selectdisk ", 11) == 0)
        {
            int disk_num = strtoul(command + 11, NULL, 10);
            if (select_disk(disk_num) == 0)
            {
                print_str("Disk selected successfully.");
                print_newline();
            }
        }
        else if (strncmp(command, "newpart ", 8) == 0)
        {
            char *size_str = command + 8;
            char *fs_str = strchr(size_str, ' ');
            FileSystemType fs_type = FS_FAT32;

            if (fs_str)
            {
                *fs_str = '\0'; // Null terminate size string
                fs_str++;       // Move to filesystem type
                fs_type = parse_fs_type(fs_str);
            }

            uint32_t size_mb = strtoul(size_str, NULL, 10);
            if (create_partition_mb(size_mb, fs_type) == 0)
            {
                print_str("Partition created successfully.");
                print_newline();
            }
        }
        else if (strncmp(command, "delpart ", 8) == 0)
        {
            int part_num = strtoul(command + 8, NULL, 10);
            delete_partition(part_num);
        }
        else if (strncmp(command, "formatpart ", 11) == 0)
        {
            char *part_str = command + 11;
            char *fs_str = strchr(part_str, ' ');
            FileSystemType fs_type = FS_FAT32;

            if (fs_str)
            {
                *fs_str = '\0'; // Null terminate partition number string
                fs_str++;       // Move to filesystem type
                fs_type = parse_fs_type(fs_str);
            }

            int part_num = strtoul(part_str, NULL, 10);
            format_partition(part_num, fs_type);
        }
        else if (strcmp(command, "partinfo") == 0)
        {
            display_partition_info();
        }
        else if (strcmp(command, "exit") == 0)
        {
            break;
        }
        else
        {
            print_str("Unknown command. Type 'help' for available commands.");
            print_newline();
        }
    }
}