/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const int MAX_PATH_LENGTH = 1024;
    char path[MAX_PATH_LENGTH];

    if (getcwd(path, MAX_PATH_LENGTH) == NULL)
    {
        perror("getcwd");
        return EXIT_FAILURE;
    }
    else
    {
        printf("%s\n", path);
        return EXIT_SUCCESS;
    }
}