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
#include <string.h>

int main(int argc, char **argv)
{
    if (argc <= 2)
    {
        fputs("Usage:\n", stderr);
        fputs("    fork-test message-indent-level seconds seconds seconds ...\n", stderr);
        fputs("    \n", stderr);
        fputs("e.g.\n", stderr);
        fputs("    # sleep 3 seconds and then exit.\n", stderr);
        fputs("    fork-test 0 3\n", stderr);
        fputs("    # parent process sleep 3 seconds and child process sleep 1 second.\n", stderr);
        fputs("    fork-test 0 3 1\n", stderr);
        fputs("    # parent process sleep 5 seconds and child process sleep 3 seconds, and child-child process 1 second.\n", stderr);
        fputs("    fork-test 0 5 3 1\n", stderr);
        fputs("    # parent process sleep 1 seconds and child process sleep 3 seconds, parent exit first.\n", stderr);
        fputs("    fork-test 0 1 3\n", stderr);
        exit(EXIT_FAILURE);
    }

    const char *tab = "    ";
    const int tab_len = strlen(tab);

    int level = atoi(argv[1]);
    char *leading = malloc(tab_len * level + 1);

    for (int i = 0; i < level; i++)
    {
        memcpy(leading + tab_len * i, tab, tab_len);
    }

    int seconds = atoi(argv[2]);
    pid_t pid = getpid();
    pid_t sppid = getppid();
    printf("%s[%d] my parent pid after startup : %d\n", leading, pid, sppid);
    printf("%s[%d] sleep %d seconds...\n", leading, pid, seconds);

    if (argc > 3)
    {
        // e.g.
        // 0         1 2 3 4    << argv index
        // fork-test 0 5 3 1    << parent process
        // fork-test 1 3 1      << child process

        char **child_argv = malloc(argc * sizeof(char *));
        child_argv[0] = argv[0];

        char child_level_buf[20];
        snprintf(child_level_buf, sizeof(child_level_buf), "%d", level + 1);

        child_argv[1] = child_level_buf;

        for (int i = 3; i < argc; i++)
        {
            child_argv[i - 1] = argv[i];
        }

        child_argv[argc - 1] = NULL; // set NULL terminate

        pid_t child_pid = fork();
        if (child_pid == 0)
        {
            // child process
            execvp(argv[0], child_argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            // parent process
            printf("%s[%d] create a child process: %d\n", leading, pid, child_pid);
        }
        else
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        free(child_argv);
    }

    sleep(seconds);

    pid_t eppid = getppid();
    if (eppid != sppid)
    {
        puts("");
        printf("%s[%d] !!! my parent process has been changed.\n", leading, pid);
    }

    printf("%s[%d] my parent pid before exiting: %d\n", leading, pid, eppid);
    printf("%s[%d] exit\n", leading, pid);

    free(leading);
    exit(EXIT_SUCCESS);
}