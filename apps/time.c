/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        return EXIT_SUCCESS;
    }

    // argv
    //     0    1       2    3        N
    //     time program arg1 arg2 ... NULL
    //
    // execvp argv
    //     0       1    2        N
    //     program arg1 arg2 ... NULL

    char *file = argv[1];
    char **child_argv = argv + 1;

    struct timeval start;
    gettimeofday(&start, NULL);

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process
        execvp(file, child_argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process

        // Wait for child process to exit
        int status;
        waitpid(pid, &status, WUNTRACED);

        // the child process usage were appended to the parent process.
        // check _The Linux Programming Interface_ sesstion 26.1.1

        struct rusage usage;
        if (getrusage(RUSAGE_CHILDREN, &usage) == -1)
        {
            perror("getrusage");
            exit(EXIT_FAILURE);
        }

        // - real time: from `program start` to `program exit`
        // - user_time: program in userspace comsuming
        // - system_time: program in kernel comsuming
        //
        // real time = user time + system time + I/O waiting + other etc.

        struct timeval end;
        gettimeofday(&end, NULL);

        long int real_time_us = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

        printf("\n");
        printf("real    %ld.%06ld seconds\n", real_time_us / 1000000, real_time_us % 1000000);
        printf("user    %ld.%06ld seconds\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("sys     %ld.%06ld seconds\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    }

    exit(EXIT_SUCCESS);
}