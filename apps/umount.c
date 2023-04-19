/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>

int unmount_directory(char *directory)
{
    if (umount(directory) == 0)
    {
        printf("Unmount %s successfully.\n", directory);
        return EXIT_SUCCESS;
    }
    else
    {
        perror("umount");
        return EXIT_FAILURE;
    }
}

void print_usage(void)
{
    fputs("Usage:\n", stderr);
    fputs("    umount directory\n", stderr);
    fputs("e.g.\n", stderr);
    fputs("    umount /var/tmp\n", stderr);
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        return unmount_directory(argv[1]);
    }
    else
    {
        print_usage();
        return EXIT_FAILURE;
    }
}