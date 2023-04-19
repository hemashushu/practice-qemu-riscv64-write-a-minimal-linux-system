/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

int list_directory(void)
{
    const int MAX_PATH_LENGTH = 1024;
    char filepath[MAX_PATH_LENGTH];

    if (getcwd(filepath, MAX_PATH_LENGTH) == NULL)
    {
        perror("getcwd");
        return EXIT_FAILURE;
    }

    DIR *dir = opendir(filepath);
    if (dir == NULL)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char *name = entry->d_name;
        if (strcmp(name, ".") == 0 ||
            strcmp(name, "..") == 0)
        {
            continue;
        }

        char type = ' ';
        switch (entry->d_type)
        {
        case DT_DIR:
            type = 'd';
            break;
        case DT_LNK:
            type = 'l';
            break;
        }
        printf("%c %s\n", type, name);
    }

    closedir(dir);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        fputs("Does not support any parameters.\n", stderr);
        return EXIT_FAILURE;
    }
    else
    {
        return list_directory();
    }
}