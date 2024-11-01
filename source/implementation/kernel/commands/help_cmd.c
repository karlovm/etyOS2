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
    print_str(" - disktool: Manage and view storage");
    print_newline();
    print_str(" - echo <text>: Print text back to the console");
    print_newline();
    print_str(" - about: Get information about this build");
    print_newline();
    print_str(" - partitions: Show all partitions");
    print_newline();
    print_str(" - setcolor <background>: Change the text and background colors");
    print_newline();
}