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
#include <sys/reboot.h>

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        fputs("Does not support parameters.\n", stderr);
        return EXIT_FAILURE;
    }

    // usage:
    //
    // poweroff

    sync();
    reboot(RB_POWER_OFF);
    return EXIT_SUCCESS;
}