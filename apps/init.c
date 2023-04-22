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
#include <errno.h>

int main(void)
{

    // the `init` process is launched directly by the kernel,
    // the value of PID should be 1.

    if (getpid() != 1)
    {
        // this process is launched incorrectly.
        return EXIT_FAILURE;
    }

    // the `init` process should not be killed or stoped,
    // so block all signal.
    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);

    int child_pid = fork();
    if (child_pid > 0)
    {
        // parent process start here

        int status;
        while (true)
        {
            // wait child processes exit

            // the exit status of the child process makes no sense for
            // the init process, just discard it here.
            wait(&status);

            // the `wait()` function return the `pid` of the exited child process.
            //
            // note the child processes includes the `/etc/rc` process which
            // is started directly by the `init` process, and all other
            // processes that have lose their parents, i.e. orphan processes.
            //
            // the `wait()` function will be blocked until a child process exits.
            //
            // when there are no child processes, the `wait()` function will return -1
            // and set the `errno` to 10.
        }

        // parent process wouldn't stop until system shut down

    }
    else if (child_pid == 0)
    {
        // child process start here

        sigprocmask(SIG_UNBLOCK, &set, NULL);

        setsid();
        // setpgid(0, 0); // optional

        chdir("/root");

        char *envp[] = {"USER=root",
                        "HOME=/root",
                        "SHELL=/bin/sh",
                        "PWD=/root",
                        "PATH=/bin:/sbin",
                        NULL};

        // execute the shell program
        // char *argv[] = {"sh", NULL};
        // execve("/bin/sh", argv, envp);

        // execute the script
        char *argv[] = {"/etc/rc", NULL};
        execve("/etc/rc", argv, envp);

        // execve nerver return unless error occured.
        perror("execve");
        exit(EXIT_FAILURE);

    }
    else
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}