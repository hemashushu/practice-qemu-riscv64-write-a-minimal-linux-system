/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/utsname.h>

void print_usage(void)
{
    char *text =
        "Available applets:\n"
        "    tee, tr, uname\n"
        "\n"
        "Usage:\n"
        "    applets applet_name args0 args1 ...\n"
        "\n"
        "e.g.\n"
        "    applets tee /path/to/file\n"
        "    applets tee\n"
        "    applets tr [:upper:] [:lower:]\n"
        "    applets tr [:blank:] _\n"
        "    applets uname [OPTION]...\n"
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

void print_uname_usage(void)
{
    char *text =
        "Usage: uname [OPTION]...\n"
        "-a, --all                print all information\n"
        "-s, --kernel-name        print the kernel name\n"
        "-n, --nodename           print the network node hostname\n"
        "-r, --kernel-release     print the kernel release\n"
        "-v, --kernel-version     print the kernel version\n"
        "-m, --machine            print the machine hardware name\n"
        "    --help               display this help and exit\n"
        "    --version            output version information and exit\n";
    fputs(text, stdout);
}

int command_uname(int argc, char **argv)
{
    bool has_all = false;
    bool has_kernel_name = false;
    bool has_nodename = false;
    bool has_kernel_release = false;
    bool has_kernel_version = false;
    bool has_machine = false;
    bool has_help = false;
    bool has_version = false;

    struct option longopts[] = {
        {"all", 0, NULL, 'a'},
        {"kernel-name", 0, NULL, 's'},
        {"nodename", 0, NULL, 'n'},
        {"kernel-release", 0, NULL, 'r'},
        {"kernel-version", 0, NULL, 'v'},
        {"machine", 0, NULL, 'm'},
        {"help", 0, NULL, 'h'},
        {"version", 0, NULL, 'V'},
        {NULL, 0, NULL, 0}};

    if (argc == 1)
    {
        has_kernel_name = true;
    }
    else
    {
        int opt;
        while ((opt = getopt_long(argc, argv, "asnrvm", longopts, NULL)) != -1)
        {
            switch (opt)
            {
            case 'a':
                has_all = true;
                break;
            case 's':
                has_kernel_name = true;
                break;
            case 'n':
                has_nodename = true;
                break;
            case 'r':
                has_kernel_release = true;
                break;
            case 'v':
                has_kernel_version = true;
                break;
            case 'm':
                has_machine = true;
                break;
            case 'h': // long only
                has_help = true;
                break;
            case 'V': // long only
                has_version = true;
                break;
            case ':':
                // unreachable
                // there is no option requre argument currently.
                // note: the argument value is stored in variable `char* optarg`
                exit(EXIT_FAILURE);
                break;
            case '?':
                // unknown option
                // note: the option value is stored in variable `int optopt`
                exit(EXIT_FAILURE);
                break;
            }
        }

        if (optind < argc)
        {
            fprintf(stderr, "unknown argument: %s\n", argv[optind]);
            exit(EXIT_FAILURE);
        }
    }

    if (has_version)
    {
        printf("uname 1.0\n");
        exit(EXIT_SUCCESS);
    }

    if (has_help)
    {
        print_uname_usage();
        exit(EXIT_SUCCESS);
    }

    if (has_all)
    {
        has_kernel_name = true;
        has_nodename = true;
        has_kernel_release = true;
        has_kernel_version = true;
        has_machine = true;
    }

    char buf[1024];
    buf[0] = '\0';

    struct utsname uts;
    if (uname(&uts) == -1)
    {
        perror("uname");
        exit(EXIT_FAILURE);
    }

    if (has_kernel_name)
    {
        strcat(buf, uts.sysname);
        strcat(buf, " ");
    }

    if (has_nodename)
    {
        strcat(buf, uts.nodename);
        strcat(buf, " ");
    }

    if (has_kernel_release)
    {
        strcat(buf, uts.release);
        strcat(buf, " ");
    }

    if (has_kernel_version)
    {
        strcat(buf, uts.version);
        strcat(buf, " ");
    }

    if (has_machine)
    {
        strcat(buf, uts.machine);
        strcat(buf, " ");
    }

    buf[strlen(buf) - 1] = '\0';

    printf("%s\n", buf);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    char *filepath = argv[0];
    char *command = basename(filepath);

    if (strcmp(command, "applets") == 0)
    {
        if (argc == 1)
        {
            print_usage();
            return EXIT_FAILURE;
        }

        // convert:
        //
        // argv[0] argv[1] argv[2] argv[3]
        // applets command arg0    arg1
        //
        //    |
        //    to
        //    V
        //
        // argv[0]  argv[1] argv[2]
        // command  arg0    arg1

        argv++;
        argc--;
        command = argv[0];
    }

    if (strcmp(command, "tee") == 0)
    {
        if (argc > 2)
        {
            fputs("Usage:\n", stderr);
            fputs("    applets tee\n", stderr);
            fputs("    applets tee /path/to/name\n", stderr);
            fputs("or\n", stderr);
            fputs("    tee\n", stderr);
            fputs("    tee /path/to/name\n", stderr);
            return EXIT_FAILURE;
        }

        // usage:
        //
        // tee
        // tee filename
        return command_tee(argv[1]);
    }
    else if (strcmp(command, "tr") == 0)
    {
        if (argc != 3)
        {

            fputs("Usages:\n", stderr);
            fputs("    applets tr [:upper:] [:lower:]\n", stderr);
            fputs("    applets tr [:blank:] _\n", stderr);
            fputs("or\n", stderr);
            fputs("    tr [:upper:] [:lower:]\n", stderr);
            fputs("    tr [:blank:] _\n", stderr);
            return EXIT_FAILURE;
        }

        // usage:
        //
        // tr pattern1 pattern2
        return command_tr(argv[1], argv[2]);
    }
    else if (strcmp(command, "uname") == 0)
    {
        // usage:
        //
        // uname
        // uname -a[-s,-n,-r,-v,-m]
        // uname --all[--kernel-name,--nodename,--kernel-release,--kernel-version,--machine]
        // uname --help[--version]
        return command_uname(argc, argv);
    }
    else
    {
        print_usage();
        return EXIT_FAILURE;
    }
}