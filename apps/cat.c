/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    // Read from file(s) and print to stdout, if no file was specified, then read from stdin.
    // usage:
    //
    // cat
    // cat filename
    // cat file1 file2 ...

    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    size_t bytes_read;

    argv++; // argv[0] is the command line

    if (*argv == NULL)
    {
        // read from stdin
        while ((bytes_read = fread(buf, 1, BUF_SIZE, stdin)) > 0)
        {
            fwrite(buf, 1, bytes_read, stdout);
        }
    }
    else
    {
        while (*argv != NULL)
        {
            FILE *file = fopen(*argv, "r");
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
            argv++;
        }
    }

    return EXIT_SUCCESS;
}