/**
 * Copyright (c) 2023 Hemashushu <hippospark@gmail.com>, All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

void loop(void);
int run_script(char *);
char *get_current_working_directory(void);
char *get_command_line(void);
char **convert_to_args(char *);
int command_cd(char *dest);
int command_export(char **args);
int command_help(void);
int command_pwd(void);
int execute(char **);
int execute_external(char **args);

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        return run_script(argv[1]);
    }
    else if (argc == 1)
    {
        loop();
        return EXIT_SUCCESS;
    }
    else
    {
        fputs("Does not support parameters.\n", stderr);
        return EXIT_FAILURE;
    }
}

void loop(void)
{
    const int MAX_PROMPT_LENGTH = 1024;

    char *line;
    char **args;
    char *cwd;
    char prompt[MAX_PROMPT_LENGTH];

    int should_continue_next;

    do
    {
        cwd = get_current_working_directory();
        snprintf(prompt, sizeof(prompt), "%s > ", cwd);
        fputs(prompt, stdout);

        line = get_command_line();
        args = convert_to_args(line);
        should_continue_next = execute(args);

        free(cwd);
        free(line);
        free(args);
    } while (should_continue_next);
}

int run_script(char *filepath)
{
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fd) != -1)
    {
        // skip line comment
        if (line[0] == '#')
        {
            continue;
        }

        char **args = convert_to_args(line);
        execute(args);
        free(args);
    }

    free(line);
    fclose(fd);

    return EXIT_SUCCESS;
}

char *get_current_working_directory(void)
{
    const int MAX_PATH_LENGTH = 1024;
    char *path = malloc(MAX_PATH_LENGTH * sizeof(char));
    if (getcwd(path, MAX_PATH_LENGTH) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    return path;
}

char *get_command_line(void)
{
    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, stdin) == -1)
    {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    return line;
}

char **convert_to_args(char *line)
{
    const int MAX_ARGS = 256;
    const char *DELIMITERS = " \t\n";

    char **tokens = malloc(MAX_ARGS * sizeof(char *));

    int i = 0;
    char *token = strtok(line, DELIMITERS);
    while (token != NULL)
    {
        tokens[i] = token;
        i++;
        token = strtok(NULL, DELIMITERS);
    }

    tokens[i] = NULL;
    return tokens;
}

int execute(char **args)
{

    char *cmd = args[0];
    if (cmd == NULL)
    {
        // empty command
        return 1;
    }
    else
    {
        // to check builtin commands
        // https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html
        if (strcmp(cmd, "cd") == 0)
        {
            // change the current working directory
            return command_cd(args[1]);
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            // exit the current shell
            return 0;
        }
        else if (strcmp(cmd, "export") == 0)
        {
            // set environment variable
            return command_export(args);
        }
        else if (strcmp(cmd, "help") == 0)
        {
            // print help information
            return command_help();
        }
        else if (strcmp(cmd, "pwd") == 0)
        {
            // print the current working directory
            return command_pwd();
        }
        else
        {
            // execute external program
            return execute_external(args);
        }
    }
}

int command_cd(char *dest)
{
    if (dest == NULL)
    {
        // change to $HOME directory
        dest = getenv("HOME");
    }
    else if (strcmp(dest, "-") == 0)
    {
        // change to $OLDPWD directory
        dest = getenv("OLDPWD");
    }

    if (dest != NULL)
    {
        const int MAX_PATH_LENGTH = 1024;
        char last_cwd[MAX_PATH_LENGTH];
        getcwd(last_cwd, MAX_PATH_LENGTH);

        // change to `dest` directory
        if (chdir(dest) != 0)
        {
            perror("chdir");
        }
        else
        {
            // update $PWD and $OLDPWD
            char cwd[MAX_PATH_LENGTH];
            getcwd(cwd, MAX_PATH_LENGTH);
            setenv("PWD", cwd, 1);
            setenv("OLDPWD", last_cwd, 1);
        }
    }

    return 1;
}

int command_help(void)
{
    puts("Shell 1.0");
    return 1;
}

int command_export(char **args)
{
    if (args[1] == NULL)
    {
        // list all environment varables
        for (char **idx = environ; *idx != NULL; idx++)
        {
            puts(*idx);
        }
    }
    else if (args[2] != NULL)
    {
        fputs("Usage:\n", stderr);
        fputs("    export\n", stderr);
        fputs("    export NAME=VALUE\n", stderr);
    }
    else
    {
        char *kvp = args[1];
        char *copied = malloc(strlen(kvp) + 1);
        strcpy(copied, kvp);
        if (putenv(copied) != 0)
        {
            perror("putenv");
        }
    }

    return 1;
}

int command_pwd(void)
{
    char *cwd = get_current_working_directory();
    printf("%s\n", cwd);
    free(cwd);
    return 1;
}

int execute_external(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // child process start ---------------------
        execvp(args[0], args);
        // execve nerver return unless error occured.
        perror("execvp");
        exit(EXIT_FAILURE);
        // child process end -----------------------
    }
    else if (pid > 0) // parent process
    {
        int status;
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // when child process is exited or killed
    }

    return 1;
}