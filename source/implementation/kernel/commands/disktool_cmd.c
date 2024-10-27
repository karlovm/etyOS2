#include "print.h"
#include "console.h"
#include "string.h"

void start_disktool(int color)
{
    print_str("Welcome to disktool. "); // Prompt
    print_newline();
    print_str("Type help to list of commands");
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
            print_str(" - newpart <size_mb>: Create a new partition with size in MB");
            print_newline();
            print_str(" - delpart <number>: Delete a partition");
            print_newline();
            print_str(" - partinfo: Show partition information and filesystem types");
            print_newline();
            print_str(" - exit: Exit from disktool");
            print_newline();
        }
        else if (strcmp(command, "listdisks") == 0)
        {
            list_disks();
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
            uint32_t size_mb = strtoul(command + 8, NULL, 10);
            if (create_partition_mb(size_mb) == 0)
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
        else if (strcmp(command, "partinfo") == 0)
        {
            display_partition_info();
        }

        else if (strcmp(command, "exit") == 0)
        {
            break;
        }
    }
}