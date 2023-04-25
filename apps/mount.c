/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdlib.h>

int list_mounts(void)
{

//     const int MAX_FILE_PATH_LENGTH = 1024;
//     pid_t pid = getpid();
//     char filepath[MAX_FILE_PATH_LENGTH];
//     sprintf(filepath, "/proc/%d/mounts", pid);

    char* filepath = "/proc/self/mounts";

    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1)
    {
        printf("%s", line);
    }

    free(line);
    fclose(file);
    return EXIT_SUCCESS;
}

int mount_device(const char *type,
                 const char *device,
                 const char *mount_point)
{
    unsigned long mount_flags = 0;
    const char *opts = NULL;

    if (mount(device, mount_point, type, mount_flags, opts) == 0)
    {
        printf("Mount point %s created successfully.\n", mount_point);
        return EXIT_SUCCESS;
    }
    else
    {
        perror("mount");
        return EXIT_FAILURE;
    }
}

void print_usage(void)
{
    char *text =
        "Usage:\n"
        "    mount -t type device mountpoint\n"
        "\n"
        "e.g.\n"
        "    mount -t proc proc /proc\n"
        "    mount -t tmpfs none /var/tmp\n"
        "\n"
        "Run without parameters to list all mount points\n";

    fputs(text, stderr);
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        return list_mounts();
    }
    else if (argc == 5)
    {
        return mount_device(argv[2],
                            argv[3],
                            argv[4]);
    }
    else
    {
        print_usage();
        return EXIT_FAILURE;
    }
}
