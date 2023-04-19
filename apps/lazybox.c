/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// a program contains multiple applets like BusyBox.
//
// cat, ps, poweroff

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <sys/reboot.h>

void print_usage(void)
{
    char *text =
        "Available applets:\n"
        "    cat, tee, ps, poweroff\n"
        "\n"
        "Usage:\n"
        "    lazybox applet_name args0 args1 ...\n"
        "\n"
        "e.g.\n"
        "    lazybox cat /path/to/file\n"
        "    lazybox tee /path/to/file\n"
        "    lazybox ps\n"
        "    lazybox poweroff\n"
        "\n"
        "You can call the applet by link name by creating a link. e.g.\n"
        "\n"
        "    $ ln -s lazybox tee\n"
        "    $ tee hello.txt\n"
        "      Hello World!\n"
        "      ^D\n";
    fputs(text, stderr);
}

int command_cat(char *filepath)
{
    FILE *fd = fopen(filepath, "r");

    if (fd == NULL)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }

    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    size_t read_size;

    while ((read_size = fread(buf, 1, BUF_SIZE, fd)) > 0)
    {
        fputs(buf, stdout);
    }

    fclose(fd);
    return EXIT_SUCCESS;
}

int command_tee(char *filepath)
{
    //
    fputs("tee TODO::\n", stderr);
    return EXIT_FAILURE;
}

int command_ps(void)
{
    //
    fputs("ps TODO::\n", stderr);
    return EXIT_FAILURE;
}

int command_poweroff(void)
{
    sync();
    reboot(RB_POWER_OFF);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    char *filepath = argv[0];
    char *base_name = basename(filepath);

    char *command;
    int arg_offset;
    if (strcmp(base_name, "lazybox") == 0)
    {
        if (argc == 1)
        {
            print_usage();
            return EXIT_FAILURE;
        }

        // argv[1] argv[2] argv[3]
        // command arg0    arg1
        command = argv[1];
        arg_offset = 2;
    }
    else
    {
        // argv[0]  argv[1] argv[2]
        // command  arg0    arg1
        command = base_name;
        arg_offset = 1;
    }

    if (strcmp(command, "cat") == 0)
    {
        if (argv[arg_offset] == NULL)
        {
            fputs("Usage:\n", stderr);
            fputs("    lazybox cat /path/to/name\n", stderr);
            fputs("or\n", stderr);
            fputs("    cat /path/to/name\n", stderr);
            return EXIT_FAILURE;
        }

        return command_cat(argv[arg_offset]);
    }
    else if (strcmp(command, "tee") == 0)
    {
        if (argv[arg_offset] == NULL)
        {
            fputs("Usage:\n", stderr);
            fputs("    lazybox tee /path/to/name\n", stderr);
            fputs("or\n", stderr);
            fputs("    tee /path/to/name\n", stderr);
            return EXIT_FAILURE;
        }

        return command_tee(argv[arg_offset]);
    }
    else if (strcmp(command, "ps") == 0)
    {
        return command_ps();
    }
    else if (strcmp(command, "poweroff") == 0)
    {
        return command_poweroff();
    }
    else
    {
        print_usage();
        return EXIT_FAILURE;
    }
}