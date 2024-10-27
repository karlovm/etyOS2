#include "print.h"
#include "console.h"
#include "string.h"

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

    int color = PRINT_COLOR_BLACK;

    //  init_filesystem();
    while (1)
    {
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        print_str("etyOS Kernel > "); // Prompt

        read_input(command, sizeof(command)); // Read user input
        print_newline();

        if (command[0] == '\0')
            continue;

        // Process command
        if (strcmp(command, "help") == 0)
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
        else
        {
            print_str("Unknown command: ");
            print_str(command);
            print_newline();
        }
    }
}
