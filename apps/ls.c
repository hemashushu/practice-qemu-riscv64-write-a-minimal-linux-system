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
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH_LENGTH 1024

void print_item(char *name, struct stat *s)
{
    // about `stat->st_mode`
    // MSB                             LSB
    // 15 14 13 12  11 10 9  8 7 6  5 4 3  2 1 0
    // 0  0  0  0,  u  g  t, r w x, r w x, r w x
    // ----------   --------------  -----  -----
    // type         user            group  other
    //
    // check _The Linux Programming Interface_ session 15.1

    char type;
    switch (s->st_mode & S_IFMT)
    {
    case S_IFBLK:
        type = 'b';
        break;
    case S_IFCHR:
        type = 'c';
        break;
    case S_IFDIR:
        type = 'd';
        break;
    case S_IFIFO:
        type = 'f';
        break;
    case S_IFLNK:
        type = 'l';
        break;
    case S_IFREG:
        type = ' ';
        break;
    case S_IFSOCK:
        type = 's';
        break;
    default:
        type = '?';
        break;
    }

    int permissions = s->st_mode & 07777;
    long int size = s->st_size;
    char *last_modified = ctime(&s->st_mtim.tv_sec);
    last_modified[strlen(last_modified) - 1] = '\0'; // remove the last '\n' char.

    printf("%c %04o %10ld %s %s\n", type, permissions, size, last_modified, name);
}

void list_file(char *filepath)
{
    struct stat s;
    if (lstat(filepath, &s) == -1)
    {
        perror("list");
        exit(EXIT_FAILURE);
    }

    print_item(basename(filepath), &s);
}

void list_directory(char *filepath)
{
    char fullpath[MAX_PATH_LENGTH];

    DIR *dir = opendir(filepath);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
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

        // join file path and name
        strcpy(fullpath, filepath);
        if (filepath[strlen(filepath) - 1] != '/')
        {
            strcat(fullpath, "/");
        }
        strcat(fullpath, name);

        list_file(fullpath);
    }

    closedir(dir);
}

void list(char *filepath)
{
    struct stat s;
    if (lstat(filepath, &s) == -1)
    {
        if (errno == ENOENT)
        {
            printf("File %s does not exist\n", filepath);
        }
        else
        {
            perror("list");
        }

        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(s.st_mode))
    {
        printf("%s:\n", filepath);
        list_directory(filepath);
        putchar('\n');
    }
    else
    {
        print_item(filepath, &s);
    }
}

int main(int argc, char **argv)
{
    // usage:
    // ls
    // ls file1 file2 ...

    if (argc == 1)
    {
        list_directory(".");
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            list(argv[i]);
        }
    }

    return EXIT_SUCCESS;
}