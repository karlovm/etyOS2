#include "print.h"
#include "console.h"

void kernel_main()
{
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_CYAN);
    print_str("etyOS2 64-bit kernel started");
    print_newline();
    print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    print_str("Type 'help' for a list of commands.");
    print_newline();

    // Command processing loop
    char command[256];
    while (1) {
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        print_str("kernel> ");  // Prompt

        read_input(command, sizeof(command));  // Read user input
        print_newline();
        
        if (command[0] == '\0') continue;

        // Process command
        if (strcmp(command, "help") == 0) {
            print_str("Available commands:");
            print_newline();
            print_str(" - help: Show this help message");
            print_newline();
            print_str(" - clear: Clear the screen");
            print_newline();
            print_str(" - echo <text>: Print text back to the console");
            print_newline();
        } else if (strcmp(command, "clear") == 0) {
            print_clear();
        } else if (strncmp(command, "echo ", 5) == 0) {
            print_str(command + 5);  // Print the rest of the input after "echo "
            print_newline();
        } else {
            print_str("Unknown command: ");
            print_str(command);
            print_newline();
        }
    }
}
