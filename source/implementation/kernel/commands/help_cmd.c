#include "print.h"
#include "console.h"
#include "string.h"

void show_help(int color)
{
    print_str("Available commands:");
    print_newline();
    print_str(" - help: Show this help message");
    print_newline();
    print_str(" - clear: Clear the screen");
    print_newline();
    print_str(" - echo <text>: Print text back to the console");
    print_newline();
    print_str(" - about: Get information about this build");
    print_newline();



    print_set_color(PRINT_COLOR_YELLOW, color);
    print_newline();
    print_str("    Direct tools");
    print_set_color(PRINT_COLOR_WHITE, color);
    print_newline();
    // Original partition commands
    print_str(" - create_partition <start_lba> <sector_count>: Create a new FAT32 partition");
    print_set_color(PRINT_COLOR_LIGHT_GRAY, color);
    print_newline();
    print_str("    Note: each sector is 512");
    print_set_color(PRINT_COLOR_WHITE, color);
    print_newline();
    print_str(" - format_partition <partition_number>: Format the partition as FAT32");
    print_newline();
    print_str(" - partitions: Show all partitions");
    print_newline();
    print_str(" - setcolor <background>: Change the text and background colors");
    print_newline();
}