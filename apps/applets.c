/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// a program contains multiple applets like BusyBox.

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/reboot.h>

void print_usage(void)
{
    char *text =
        "Available applets:\n"
        "    cat, tee, tr, uname, poweroff\n"
        "\n"
        "Usage:\n"
        "    applets applet_name args0 args1 ...\n"
        "\n"
        "e.g.\n"
        "    applets cat /path/to/file\n"
        "    applets cat file1 file2 ...\n"
        "    applets cat\n"
        "    applets tee /path/to/file\n"
        "    applets tee\n"
        "    applets tr [:upper:] [:lower:]\n"
        "    applets tr [:blank:] _\n"
        "    applets uname\n"
        "    applets poweroff\n"
        "\n"
        "You can call the applet by link name by creating a link. e.g.\n"
        "\n"
        "    $ ln -s applets tee\n"
        "    $ tee hello.txt\n"
        "      Hello World!\n"
        "      ^D\n";
    fputs(text, stderr);
}

/**
 * @brief Read from file(s) and print to stdout, if no file was specified, then read from stdin.
 *
 *  Usage:
 *      applets cat
 *      applets cat /path/to/name
 *      applets cat file1 file2 ...
 *  or
 *      cat
 *      cat /path/to/name
 *      cat file1 file2 ...
 *
 * @param filepath
 * @return int
 */
int command_cat(char **filepath)
{
    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    size_t bytes_read;

    if (*filepath == NULL)
    {
        // read from stdin
        while ((bytes_read = fread(buf, 1, BUF_SIZE, stdin)) > 0)
        {
            fwrite(buf, 1, bytes_read, stdout);
        }
    }
    else
    {
        while (*filepath != NULL)
        {
            FILE *file = fopen(*filepath, "r");
            if (file == NULL)
            {
                perror("fopen");
                return EXIT_FAILURE;
            }

            while ((bytes_read = fread(buf, 1, BUF_SIZE, file)) > 0)
            {
                fwrite(buf, 1, bytes_read, stdout);
            }

            fclose(file);

            // next file
            filepath++;
        }
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Read data from stdin and write to both the specified file and stdout.
 *
 * @param filepath optional
 * @return int
 */
int command_tee(char *filepath)
{
    FILE *file = NULL;

    if (filepath != NULL)
    {
        file = fopen(filepath, "w");
        if (file == NULL)
        {
            perror("fopen");
            return EXIT_FAILURE;
        }
    }

    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE];

    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, BUF_SIZE, stdin)) > 0)
    {
        // write to stdout
        fwrite(buf, 1, bytes_read, stdout);

        // write to the specified file
        if (file != NULL)
        {
            size_t bytes_write = fwrite(buf, 1, bytes_read, file);
            if (bytes_write < bytes_read)
            {
                perror("fwrite");
                fclose(file);
                return EXIT_FAILURE;
            }
        }

        if (feof(stdin) != 0)
        {
            // Ctrl+D is pressed.
            break;
        }
    }

    if (file != NULL)
    {
        fclose(file);
    }
    return EXIT_SUCCESS;
}

bool is_valid_pattern(char *pattern)
{
    return strcmp(pattern, "[:blank:]") == 0 ||
           strcmp(pattern, "[:upper:]") == 0 ||
           strcmp(pattern, "[:lower:]") == 0 ||
           strcmp(pattern, "_") == 0;
}

bool is_match_pattern(char *pattern, char ch)
{
    return (strcmp(pattern, "[:blank:]") == 0 && ch == ' ') ||
           (strcmp(pattern, "[:upper:]") == 0 && (ch >= 'A' && ch <= 'Z')) ||
           (strcmp(pattern, "[:lower:]") == 0 && (ch >= 'a' && ch <= 'z')) ||
           (strcmp(pattern, "_") == 0 && ch == '_');
}

char convert_to(char *pattern, char ch)
{
    if (strcmp(pattern, "[:blank:]") == 0)
    {
        return ' ';
    }
    else if (strcmp(pattern, "[:upper:]") == 0)
    {
        return (ch - 32);
    }
    else if (strcmp(pattern, "[:lower:]") == 0)
    {
        return (ch + 32);
    }
    else if (strcmp(pattern, "_") == 0)
    {
        return '_';
    }

    exit(EXIT_FAILURE);
}

/**
 * @brief Find and replace string
 *
 * @param find
 * @param replace
 * @return int
 */
int command_tr(char *find, char *replace)
{
    if (!is_valid_pattern(find))
    {
        fprintf(stderr,
                "Pattern \"%s\" is not supported, only pattern '_', [:blank:], [:upper:], [:lower:] are allowed.\n",
                find);
        return EXIT_FAILURE;
    }

    if (!is_valid_pattern(replace))
    {
        fprintf(stderr,
                "Pattern \"%s\" is not supported, only pattern '_', [:blank:], [:upper:], [:lower:] are allowed.\n",
                replace);
        return EXIT_FAILURE;
    }

    char ch;
    while ((ch = getchar()) != EOF)
    {
        char result;

        if (is_match_pattern(find, ch))
        {
            result = convert_to(replace, ch);
        }
        else
        {
            result = ch;
        }

        putchar(result);

        // fix QEMU terminate buffer
        if (feof(stdin) != 0)
        {
            break;
        }
    }

    return EXIT_SUCCESS;
}

int command_uname(void)
{
    char *filepath = "/proc/version";
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("fopen");
        fputs("mount /proc first.\n", stderr);
        return EXIT_FAILURE;
    }

    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    fread(buf, 1, BUF_SIZE, file);
    fclose(file);

    fputs(buf, stdout);
    return EXIT_SUCCESS;
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
    if (strcmp(base_name, "applets") == 0)
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
        return command_cat(argv + arg_offset);
    }
    else if (strcmp(command, "tee") == 0)
    {
        if (argc - arg_offset > 1)
        {
            fputs("Usage:\n", stderr);
            fputs("    applets tee\n", stderr);
            fputs("    applets tee /path/to/name\n", stderr);
            fputs("or\n", stderr);
            fputs("    tee\n", stderr);
            fputs("    tee /path/to/name\n", stderr);
            return EXIT_FAILURE;
        }

        return command_tee(argv[arg_offset]);
    }
    else if (strcmp(command, "tr") == 0)
    {
        if (argv[arg_offset] == NULL || argv[arg_offset + 1] == NULL)
        {
            fputs("Usages:\n", stderr);
            fputs("    applets tr [:upper:] [:lower:]\n", stderr);
            fputs("    applets tr [:blank:] _\n", stderr);
            fputs("or\n", stderr);
            fputs("    tr [:upper:] [:lower:]\n", stderr);
            fputs("    tr [:blank:] _\n", stderr);
            return EXIT_FAILURE;
        }

        return command_tr(argv[arg_offset], argv[arg_offset + 1]);
    }
    else if (strcmp(command, "uname") == 0)
    {
        if (argv[arg_offset] != NULL)
        {
            fputs("Does not support parameters.\n", stderr);
            return EXIT_FAILURE;
        }

        return command_uname();
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