// HelpCommand.h
#pragma once
#include "print.h"

class HelpCommand {
public:
    static void show_help() {
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

        // Disk management commands
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
        print_str(" - partitions: Show all partitions");
        print_newline();
    }
};
