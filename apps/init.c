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
    // check APUE chapter 8.2
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
            // note that the _child processes_ includes the `/etc/rc` process which
            // is started directly by the `init` process, and all other
            // processes that have lose their parents, i.e. orphan processes.
            // check APUE chapter 9.10
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

        // undo the signal blocking that set by the parent process.
        sigprocmask(SIG_UNBLOCK, &set, NULL);

        // start new session.
        // a session consists of multiple process groups,
        // typical one foreground process group and one or more background process gorup.
        // each tty a session.
        // check _The Linux Programming Interface_ chapter 2.14
        //
        // setpgid(0, 0);
        // create a new process group (with the current pid as group id)
        // a process group consists of one or multiple processes, process group is used
        // for job control (implemented through the shell), there are several job control commands:
        // e.g.
        // bg: puts group in the background
        // fg: brings backgroud group to the foreground
        // note that `setsid()` implies `setpgid(0, 0)`
        // check APUE chapter 9.6
        setsid();

        chdir("/");

        char *envp[] = {"USER=root",
                        "HOME=/root",
                        "SHELL=/bin/sh",
                        "PWD=/",
                        "PATH=/bin:/sbin:/usr/bin:/usr/sbin",
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