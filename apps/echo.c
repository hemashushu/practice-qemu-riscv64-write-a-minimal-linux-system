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
    // usage:
    //
    // echo str1 str2 ...

    // argv[0] is the command line
    for (int i = 1; i < argc; i++)
    {
        fputs(argv[i], stdout);
        if (i < argc - 1)
        {
            fputc(' ', stdout);
        }
    }

    fputc('\n', stdout);

    return EXIT_SUCCESS;
}