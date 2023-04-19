/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// enable "XPG7"
// /usr/include/features.h
#define _XOPEN_SOURCE 700

// REF::
// https://gist.github.com/rofl0r/6168719
// https://git.suckless.org/sinit/file/sinit.c.html

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wait.h>

int main(void)
{
    sigset_t set;
    int status;

    if (getpid() != 1)
    {
        return EXIT_FAILURE;
    }

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);

    int child_pid = fork();
    if (child_pid > 0)
    {
        // parent process
        while (true)
        {
            // wait all child processes exit
            wait(&status);
        }
    }
    else if (child_pid == 0)
    {
        // child process start -------------------
        sigprocmask(SIG_UNBLOCK, &set, NULL);
        setsid();
        // setpgid(0, 0); // optional
        chdir("/root");

        char *envp[] = {"USER=root",
                        "HOME=/root",
                        "SHELL=/bin/sh",
                        "PWD=/root",
                        "OLDPWD=/root",
                        "PATH=/bin:/sbin",
                        NULL};

        char *argv[] = {"sh", NULL}; // arg0 is the first part of command text
        execve("/bin/sh", argv, envp);

        // execute the script
        // char *argv[] = {"/etc/rc", NULL};
        // execve("/etc/rc", argv, envp);

        // execve nerver return unless error occured.
        perror("execve");
        exit(EXIT_FAILURE);
        // child process end ---------------------
    }
    else
    {
        perror("fork");
    }
}